#include "novu/playground/BuildBridge.h"

#include "novu/playground/Paths.h"
#include "novu/playground/ShadowRun.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QTimer>

namespace novu::playground {

QString buildStateLabel(BuildState state)
{
    switch (state) {
    case BuildState::Ready:     return QStringLiteral("Ready");
    case BuildState::Stale:     return QStringLiteral("Rebuild needed");
    case BuildState::Building:  return QStringLiteral("Building");
    case BuildState::Reloading: return QStringLiteral("Reloading");
    case BuildState::Failed:    return QStringLiteral("Build failed");
    }
    return QStringLiteral("Ready");
}

BuildBridge::BuildBridge(QObject* parent)
    : QObject(parent)
{
}

BuildBridge::~BuildBridge() = default;

bool BuildBridge::canBuild() const
{
    return paths::buildTreeConfigured() && QFileInfo::exists(paths::cmakeCommand());
}

QString BuildBridge::buildCommandDescription() const
{
    return QStringLiteral("%1 --build %2 --config %3 --target NovuUIPlayground")
        .arg(QDir::toNativeSeparators(paths::cmakeCommand()),
             QDir::toNativeSeparators(paths::buildDir()),
             paths::buildConfig());
}

void BuildBridge::setRestoreArguments(const QStringList& arguments)
{
    m_restoreArguments = arguments;
}

void BuildBridge::setState(BuildState state, const QString& detail)
{
    m_state = state;
    emit stateChanged(state, detail);
}

void BuildBridge::markStale(const QString& reason)
{
    // Never downgrades a failure: a source edit while the last build is still
    // broken does not mean the build is fine now.
    if (m_state == BuildState::Building || m_state == BuildState::Reloading) {
        return;
    }
    setState(BuildState::Stale, reason);
}

void BuildBridge::markReady()
{
    setState(BuildState::Ready, QString());
}

void BuildBridge::rebuild()
{
    if (m_state == BuildState::Building || m_state == BuildState::Reloading) {
        return;
    }
    if (!canBuild()) {
        m_lastError = QStringLiteral(
            "No configured build tree at %1. Configure one, or set NOVUUI_BUILD_DIR.")
            .arg(QDir::toNativeSeparators(paths::buildDir()));
        setState(BuildState::Failed, m_lastError);
        return;
    }

    m_log.clear();
    m_lastError.clear();
    setState(BuildState::Building, buildCommandDescription());

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    // The visible gallery normally runs from its private copy under %TEMP% so
    // the linker can replace the build-tree executable. Do not let that
    // implementation detail become the build's working directory: generators
    // and custom commands are allowed to resolve relative paths from the
    // checkout just like run.bat does.
    m_process->setWorkingDirectory(paths::repoRoot());
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        const QString chunk = QString::fromLocal8Bit(m_process->readAllStandardOutput());
        m_log += chunk;
        emit logAppended(chunk);
    });
    connect(m_process, &QProcess::finished, this, &BuildBridge::onFinished);
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        // errorOccurred can fire without finished() — a missing cmake, a
        // denied execution. Without this the gallery would sit on "Building"
        // for the rest of the session.
        if (m_state != BuildState::Building) {
            return;
        }
        m_lastError = m_process->errorString();
        setState(BuildState::Failed, m_lastError);
    });

    m_process->start(paths::cmakeCommand(),
                     {QStringLiteral("--build"), paths::buildDir(),
                      QStringLiteral("--config"), paths::buildConfig(),
                      QStringLiteral("--target"), QStringLiteral("NovuUIPlayground")});
}

void BuildBridge::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    const QString processError = m_process->errorString();
    m_process->deleteLater();
    m_process = nullptr;

    if (exitCode != 0 || exitStatus == QProcess::CrashExit) {
        // The preview on screen is untouched. A failed build must not replace
        // a working reference with a blank surface — the last known good
        // render is the thing the developer is comparing against.
        const QStringList lines = m_log.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        QStringList interesting;
        for (const QString& line : lines) {
            if (line.contains(QLatin1String("error"), Qt::CaseInsensitive)
                || line.contains(QLatin1String("Error"))) {
                interesting.append(line.trimmed());
            }
        }
        if (!interesting.isEmpty()) {
            m_lastError = interesting.mid(0, 6).join(QLatin1Char('\n'));
        } else if (exitStatus == QProcess::CrashExit && !processError.isEmpty()) {
            m_lastError = QStringLiteral("Build process stopped: %1").arg(processError);
        } else {
            m_lastError = QStringLiteral("Build failed with exit code %1.").arg(exitCode);
        }
        setState(BuildState::Failed, m_lastError);
        return;
    }

    reloadNow();
}

void BuildBridge::reloadNow()
{
    setState(BuildState::Reloading, QStringLiteral("Restarting with the new binary"));

    // Restart, not hot reload. The new code is in a static library that was
    // linked into this process at load time; there is no supported way to
    // swap it under live QWidgets. What the restart preserves is the part
    // that matters — the component, scenario and profile being looked at.
    //
    // The *origin* is restarted, not this process's own path: this process is
    // usually a temporary copy, and the freshly linked binary is the one in
    // the build tree. See ShadowRun.h.
    const QString executable = shadow::originExecutable();
    const QStringList arguments = m_restoreArguments;

    QTimer::singleShot(0, this, [executable, arguments]() {
        QProcess::startDetached(executable, arguments);
        QCoreApplication::quit();
    });
}

} // namespace novu::playground
