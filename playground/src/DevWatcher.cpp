#include "novu/playground/DevWatcher.h"

#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QTimer>

namespace novu::playground {

namespace {
// One save in an editor produces several filesystem events — a temporary
// file, a rename, a touch of the directory. Anything shorter than this
// launches two compilers for one Ctrl+S.
constexpr int kDebounceMs = 300;
} // namespace

DevWatcher::DevWatcher(QObject* parent)
    : QObject(parent)
    , m_watcher(new QFileSystemWatcher(this))
    , m_debounce(new QTimer(this))
{
    m_debounce->setSingleShot(true);
    m_debounce->setInterval(kDebounceMs);
    connect(m_debounce, &QTimer::timeout, this, &DevWatcher::flush);
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &DevWatcher::onPathChanged);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &DevWatcher::onPathChanged);
}

void DevWatcher::addTree(const QStringList& directories, const QStringList& suffixes,
                         QStringList& into)
{
    for (const QString& directory : directories) {
        QDir dir(directory);
        if (!dir.exists()) {
            continue;
        }
        if (!m_watcher->directories().contains(dir.absolutePath())) {
            m_watcher->addPath(dir.absolutePath());
        }
        const QStringList files = dir.entryList(suffixes, QDir::Files);
        for (const QString& file : files) {
            const QString path = dir.absoluteFilePath(file);
            if (!into.contains(path)) {
                into.append(path);
            }
            // Re-added unconditionally: an editor that saves by replacing the
            // file drops it from the watcher, and a file that appeared after
            // startup was never in it.
            if (!m_watcher->files().contains(path)) {
                m_watcher->addPath(path);
            }
        }
    }
}

void DevWatcher::rescan()
{
    addTree(m_sourceDirs,
            {QStringLiteral("*.cpp"), QStringLiteral("*.h"), QStringLiteral("*.hpp")},
            m_sourcePaths);
    addTree(m_dataDirs, {QStringLiteral("*.json")}, m_dataPaths);
}

namespace {

// Stored absolute and cleaned, because the watcher reports absolute paths and
// classification is a prefix comparison against these.
void remember(const QStringList& directories, QStringList& into)
{
    for (const QString& directory : directories) {
        const QString absolute = QDir(directory).absolutePath();
        if (!into.contains(absolute)) {
            into.append(absolute);
        }
    }
}

} // namespace

void DevWatcher::watchSources(const QStringList& directories)
{
    remember(directories, m_sourceDirs);
    rescan();
}

void DevWatcher::watchData(const QStringList& directories)
{
    remember(directories, m_dataDirs);
    rescan();
}

QStringList DevWatcher::watchedDirectories() const
{
    return m_watcher->directories();
}

bool DevWatcher::isActive() const
{
    return !m_watcher->directories().isEmpty() || !m_watcher->files().isEmpty();
}

namespace {

bool isUnder(const QString& path, const QStringList& roots)
{
    for (const QString& root : roots) {
        if (path == root || path.startsWith(root + QLatin1Char('/'))) {
            return true;
        }
    }
    return false;
}

} // namespace

void DevWatcher::onPathChanged(const QString& path)
{
    const QString normalised = QDir::cleanPath(QDir::fromNativeSeparators(path));

    // Classified by which tree the path belongs to, never by what it looks
    // like. An earlier version tested for ".json" and for "/data/" in the
    // path, which worked only because the shipped data directory happens to
    // be called "data": point NOVUUI_PLAYGROUND_DATA somewhere else and every
    // theme edit reported "Rebuild needed" instead of reloading. A watcher
    // that guesses is a watcher that lies about which kind of change it saw.
    QStringList* pending = nullptr;
    if (m_dataPaths.contains(normalised) || isUnder(normalised, m_dataDirs)) {
        pending = &m_pendingData;
    } else if (m_sourcePaths.contains(normalised) || isUnder(normalised, m_sourceDirs)) {
        pending = &m_pendingSources;
    } else {
        // Not in a tree we were asked to watch. Ignored rather than assumed
        // to be source: assuming would mark the preview stale for a file
        // nobody here cares about.
        return;
    }

    if (!pending->contains(normalised)) {
        pending->append(normalised);
    }
    m_debounce->start();
}

void DevWatcher::flush()
{
    // Picks up files created since the last event and re-adds the ones an
    // editor replaced. Without it the second save of any file is silent.
    rescan();

    if (!m_pendingData.isEmpty()) {
        const QStringList paths = m_pendingData;
        m_pendingData.clear();
        emit dataChanged(paths);
    }
    if (!m_pendingSources.isEmpty()) {
        const QStringList paths = m_pendingSources;
        m_pendingSources.clear();
        emit sourcesChanged(paths);
    }
}

} // namespace novu::playground
