#pragma once

#include <QString>

namespace novu::playground::paths {

// Where the development tool finds the things it is a tool for.
//
// Every value is baked in at configure time and overridable by an environment
// variable, in that order of precedence:
//
//     NOVUUI_PLAYGROUND_DATA   sample data + theme overlays
//     NOVUUI_REPO_DIR          the NovuUI checkout being previewed
//     NOVUUI_BUILD_DIR         the build tree to rebuild
//     NOVUUI_EDITOR            command used by openInEditor()
//
// The overrides exist so one built gallery can preview a second checkout —
// a worktree, a branch under review — without reconfiguring.

QString repoRoot();
QString dataDir();
QString sampleDataDir();
QString themeDir();
QString buildDir();
QString cmakeCommand();
QString buildConfig();

// Absolute path for a repository-relative one ("src/Button.cpp").
QString absoluteInRepo(const QString& repoRelative);
bool existsInRepo(const QString& repoRelative);

// Opens a source file in whatever the developer already uses: $NOVUUI_EDITOR
// when set, otherwise the system handler for the file — the same mechanism
// NovuSpace uses for every "reveal this file" affordance. This tool does not
// contain an editor and will not grow one.
bool openInEditor(const QString& repoRelative);

// True when buildDir() looks like a configured CMake build tree. The gallery
// disables its rebuild action rather than running a command that cannot work.
bool buildTreeConfigured();

} // namespace novu::playground::paths
