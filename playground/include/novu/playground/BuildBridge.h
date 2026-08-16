#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class QProcess;

namespace novu::playground {

// What the gallery is currently doing, so the window never has to guess.
enum class BuildState {
    Ready,        // preview is current
    Stale,        // a C++ file changed; what is on screen is the old binary
    Building,     // incremental build running
    Reloading,    // build succeeded, process restarting
    Failed,       // build failed; the preview on screen is the last good one
};

QString buildStateLabel(BuildState state);

// Runs the incremental build and restarts the gallery.
//
// There is no C++ hot reload here and this class does not pretend otherwise.
// Replacing a static library under a running process with live QWidgets built
// from it is not something Qt or the loader supports; what is achievable is a
// fast incremental build followed by a restart that lands on the same
// component, scenario and profile — which is the workflow, and it costs about
// a second.
//
// Data and theme changes take the other path entirely: those are reloaded in
// place, without either a build or a restart. See DevWatcher.
class BuildBridge : public QObject
{
    Q_OBJECT

public:
    explicit BuildBridge(QObject* parent = nullptr);
    ~BuildBridge() override;

    BuildState state() const { return m_state; }
    QString log() const { return m_log; }
    QString lastError() const { return m_lastError; }

    // False when the build tree is not a configured CMake tree. The gallery
    // disables the action rather than running a command that cannot work —
    // this happens when the executable was copied somewhere else.
    bool canBuild() const;
    QString buildCommandDescription() const;

    // The arguments the gallery is restored with. Set by the window before a
    // reload so the restart lands where the developer was.
    void setRestoreArguments(const QStringList& arguments);
    QStringList restoreArguments() const { return m_restoreArguments; }

public slots:
    // No-op while a build is running: a second click must not spawn a second
    // compiler over the same object files.
    void rebuild();

    // Restarts this executable with the restore arguments and quits. Called
    // automatically after a successful build; exposed so a developer who
    // built in their IDE can reload without building again.
    void reloadNow();

    void markStale(const QString& reason);
    void markReady();

signals:
    void stateChanged(BuildState state, const QString& detail);
    void logAppended(const QString& text);

private:
    void setState(BuildState state, const QString& detail);
    void onFinished(int exitCode);

    QProcess* m_process = nullptr;
    BuildState m_state = BuildState::Ready;
    QString m_log;
    QString m_lastError;
    QStringList m_restoreArguments;
};

} // namespace novu::playground
