#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class QFileSystemWatcher;
class QTimer;

namespace novu::playground {

// Watches the two kinds of change a gallery has to tell apart.
//
//   sourcesChanged   a .h/.cpp under NovuUI changed. Nothing on screen is
//                    wrong yet — it is simply old. Requires a build.
//   dataChanged      a sample-data or theme-overlay JSON changed. Reloaded in
//                    place, no build, no restart.
//
// Conflating the two is how a development tool starts lying: a theme tweak
// that triggers a compiler, or a C++ edit that appears to have taken effect
// because the page was rebuilt from the same old binary.
//
// Registered by the gallery and by nothing else. No production target links
// this class, so no application ever installs a file watcher.
class DevWatcher : public QObject
{
    Q_OBJECT

public:
    explicit DevWatcher(QObject* parent = nullptr);

    // Directories are watched recursively one level deep — enough for
    // NovuUI's flat src/ and include/novu/ui/, and it avoids walking a build
    // tree.
    void watchSources(const QStringList& directories);
    void watchData(const QStringList& directories);

    QStringList watchedDirectories() const;
    bool isActive() const;

signals:
    void sourcesChanged(const QStringList& paths);
    void dataChanged(const QStringList& paths);

private:
    void addTree(const QStringList& directories, const QStringList& suffixes,
                 QStringList& into);
    // Re-reads both watched trees. Called on every flush, because a file
    // created after startup is otherwise watched only through its directory —
    // which reports that something appeared but never that it changed again.
    // A theme overlay added while the gallery is running would then apply
    // once and never respond to another save.
    void rescan();
    void onPathChanged(const QString& path);
    void flush();

    QFileSystemWatcher* m_watcher = nullptr;
    QStringList m_sourceDirs;
    QStringList m_dataDirs;
    // Editors write by replacing the file, which drops it from the watcher.
    // Both lists are re-added on every flush.
    QStringList m_sourcePaths;
    QStringList m_dataPaths;
    QStringList m_pendingSources;
    QStringList m_pendingData;
    QTimer* m_debounce = nullptr;
};

} // namespace novu::playground
