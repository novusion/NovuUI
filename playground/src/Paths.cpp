#include "novu/playground/Paths.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QUrl>

namespace novu::playground::paths {

namespace {

QString fromEnvOr(const char* variable, const char* compiled)
{
    const QString value = qEnvironmentVariable(variable);
    if (!value.isEmpty()) {
        return QDir::fromNativeSeparators(value);
    }
    return QDir::fromNativeSeparators(QString::fromUtf8(compiled));
}

} // namespace

QString repoRoot()
{
    // Cleaned because the configure-time value is "<playground>/..", which is
    // correct but reads badly in a tooltip and in an error message.
    return QDir::cleanPath(fromEnvOr("NOVUUI_REPO_DIR", NOVUUI_PLAYGROUND_REPO_DIR));
}

QString dataDir()
{
    return QDir::cleanPath(fromEnvOr("NOVUUI_PLAYGROUND_DATA", NOVUUI_PLAYGROUND_DATA_DIR));
}

QString sampleDataDir()
{
    return dataDir() + QStringLiteral("/sample_data");
}

QString themeDir()
{
    return dataDir() + QStringLiteral("/themes");
}

QString buildDir()
{
    return QDir::cleanPath(fromEnvOr("NOVUUI_BUILD_DIR", NOVUUI_PLAYGROUND_BUILD_DIR));
}

QString cmakeCommand()
{
    return QString::fromUtf8(NOVUUI_PLAYGROUND_CMAKE);
}

QString buildConfig()
{
    return QString::fromUtf8(NOVUUI_PLAYGROUND_CONFIG);
}

QString absoluteInRepo(const QString& repoRelative)
{
    if (repoRelative.isEmpty()) {
        return {};
    }
    if (QFileInfo(repoRelative).isAbsolute()) {
        return QDir::cleanPath(repoRelative);
    }
    return QDir::cleanPath(repoRoot() + QLatin1Char('/') + repoRelative);
}

bool existsInRepo(const QString& repoRelative)
{
    const QString path = absoluteInRepo(repoRelative);
    return !path.isEmpty() && QFileInfo::exists(path);
}

bool openInEditor(const QString& repoRelative)
{
    const QString path = absoluteInRepo(repoRelative);
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        return false;
    }

    // $NOVUUI_EDITOR is honoured first so a developer keeps the editor they
    // already have open, with the arguments they already use. "code -g" and
    // "devenv /edit" both work; the file is appended.
    const QString editor = qEnvironmentVariable("NOVUUI_EDITOR");
    if (!editor.isEmpty()) {
        QStringList parts = QProcess::splitCommand(editor);
        if (!parts.isEmpty()) {
            const QString program = parts.takeFirst();
            parts.append(QDir::toNativeSeparators(path));
            if (QProcess::startDetached(program, parts)) {
                return true;
            }
        }
    }

    // Otherwise the system handler — the same mechanism every other Novu
    // application uses to reveal a file. This tool does not contain an editor.
    return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

bool buildTreeConfigured()
{
    return QFileInfo::exists(buildDir() + QStringLiteral("/CMakeCache.txt"));
}

} // namespace novu::playground::paths
