#include "novu/playground/ShadowRun.h"

#include "novu/playground/Paths.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

namespace novu::playground::shadow {

namespace {

QString g_origin;

// Enough slots that two or three galleries can be open at once — a second
// instance finds the first one's copy locked and takes the next.
constexpr int kMaxSlots = 8;

QString slotName(int slot)
{
    return slot == 0 ? QStringLiteral("NovuUIPlayground-run.exe")
                     : QStringLiteral("NovuUIPlayground-run%1.exe").arg(slot);
}

// Anything the executable needs beside it. In a plain developer build tree
// this finds nothing at all — Qt is resolved through PATH and the prefix
// compiled into Qt6Core. It matters only if someone has run windeployqt into
// the output directory, in which case the copy would otherwise start without
// a platform plugin.
void copyNeighbours(const QDir& from, const QDir& to)
{
    const QStringList files =
        from.entryList({QStringLiteral("*.dll"), QStringLiteral("qt.conf")}, QDir::Files);
    for (const QString& name : files) {
        const QString target = to.filePath(name);
        if (QFileInfo::exists(target)) {
            continue;   // already there from an earlier run, and possibly in use
        }
        QFile::copy(from.filePath(name), target);
    }

    const QStringList directories =
        from.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& name : directories) {
        QDir source(from.filePath(name));
        QDir destination(to.filePath(name));
        if (!destination.exists() && !to.mkpath(name)) {
            continue;
        }
        copyNeighbours(source, destination);
    }
}

} // namespace

QString shadowDirectory()
{
    return QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                           + QStringLiteral("/NovuUIGallery"));
}

bool pathIsInsideBuildTree(const QString& executablePath)
{
    const QString build = QDir::cleanPath(paths::buildDir());
    if (build.isEmpty()) {
        return false;
    }
    const QString path = QDir::cleanPath(QDir::fromNativeSeparators(executablePath));
    return path.startsWith(build + QLatin1Char('/'), Qt::CaseInsensitive);
}

bool runsFromBuildTree()
{
    return pathIsInsideBuildTree(QCoreApplication::applicationFilePath());
}

QString originExecutable()
{
    return g_origin.isEmpty() ? QCoreApplication::applicationFilePath() : g_origin;
}

void setOriginExecutable(const QString& path)
{
    g_origin = path;
}

bool relaunchFromCopy(const QStringList& arguments, QString* error)
{
    const QString self = QCoreApplication::applicationFilePath();
    if (!pathIsInsideBuildTree(self)) {
        return false;
    }

    QDir shadow(shadowDirectory());
    if (!shadow.exists() && !QDir().mkpath(shadow.absolutePath())) {
        if (error) {
            *error = QStringLiteral("cannot create %1").arg(shadow.absolutePath());
        }
        return false;
    }

    QString target;
    for (int slot = 0; slot < kMaxSlots; ++slot) {
        const QString candidate = shadow.filePath(slotName(slot));
        // Remove before copying: the copy has to be *this* build, not the one
        // a previous session left behind. A remove that fails means another
        // gallery is running from that slot, so the next one is tried.
        if (QFileInfo::exists(candidate) && !QFile::remove(candidate)) {
            continue;
        }
        if (QFile::copy(self, candidate)) {
            target = candidate;
            break;
        }
    }

    if (target.isEmpty()) {
        if (error) {
            *error = QStringLiteral("no free slot in %1").arg(shadow.absolutePath());
        }
        return false;
    }

    copyNeighbours(QFileInfo(self).absoluteDir(), shadow);

    QStringList forwarded = arguments;
    forwarded << QStringLiteral("--origin") << self;

    if (!QProcess::startDetached(target, forwarded, shadow.absolutePath())) {
        if (error) {
            *error = QStringLiteral("cannot start %1").arg(target);
        }
        return false;
    }
    return true;
}

} // namespace novu::playground::shadow
