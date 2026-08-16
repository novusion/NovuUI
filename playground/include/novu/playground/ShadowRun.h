#pragma once

#include <QString>
#include <QStringList>

namespace novu::playground::shadow {

// Why the gallery runs itself from a copy.
//
// Windows holds an exclusive lock on a running executable. The gallery's
// whole point is to rebuild the library it is displaying and come back with
// the new one — but while it is running, the linker cannot write
// NovuUIPlayground.exe, and every Rebuild & reload would end at
//
//     LINK : fatal error LNK1104: cannot open file '...NovuUIPlayground.exe'
//
// So on startup, when the executable is sitting in the build tree, it copies
// itself into a temporary directory and relaunches from there. The build
// output is then free for the linker to overwrite, and the copy is what holds
// the lock.
//
// A reload restarts the *origin* — the build-tree executable, which has just
// been relinked — and that one shadows itself again. Two hops, both instant.
//
// Nothing here runs when the executable is outside the build tree: a copied
// or installed gallery has nothing to unlock.

// True when this executable lives inside the configured build tree, and a
// rebuild would therefore try to overwrite it.
bool runsFromBuildTree();
bool pathIsInsideBuildTree(const QString& executablePath);

// Copies this executable to a private directory and starts it there with
// `arguments` plus the origin. Returns true when the caller should exit
// immediately, having handed over to the copy.
//
// Returns false — and leaves the process to carry on normally — when there is
// nothing to shadow, or when the copy failed for any reason. A gallery that
// cannot shadow itself is still a working gallery; only the rebuild action
// suffers, and it reports its own error.
bool relaunchFromCopy(const QStringList& arguments, QString* error = nullptr);

// The build-tree executable a reload must restart. Defaults to this process.
QString originExecutable();
void setOriginExecutable(const QString& path);

// Where copies are kept. Exposed for diagnostics.
QString shadowDirectory();

} // namespace novu::playground::shadow
