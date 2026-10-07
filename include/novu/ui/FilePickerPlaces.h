#pragma once

#include <QString>
#include <QVector>

namespace novu::ui {

// What FilePickerDialog remembers between one question and the next: the
// folders a person pinned, and where they last were.
//
// A picker that forgets both sends someone uploading a build back to their
// home folder every time, to click down the same six levels again. The
// platform dialog it replaces remembers where it was, so not remembering is
// a regression rather than a missing feature.
//
// ── favourites ──────────────────────────────────────────────────────────────
//
// Folders the person added themselves, under a name they may have chosen.
// They belong to the person rather than to one product, so every Novu
// application that opens a picker shows the same ones: a folder pinned while
// uploading a build in NovuAdmin is still pinned in NovuSpace.
//
// The system's own places (Desktop, Documents, Downloads, home) are not kept
// here. They are always shown, are not the person's to rename, and storing
// them would mean a stale copy the day the system moves one.
//
// ── the last directory ──────────────────────────────────────────────────────
//
// Kept per application. Where NovuAdmin last opened a build has nothing to
// say about where NovuSpace should open an attachment.
//
// ── storage ─────────────────────────────────────────────────────────────────
//
// An INI file in the user scope, Novusion/NovuUI.ini, rather than the
// registry: the user-scope INI location can be redirected with
// QSettings::setPath(), so a test never touches the real one.
//
// Every path is stored and compared in one form - absolute, cleaned, forward
// slashes, no trailing separator except on a root - so "J:\Builds\" and
// "J:/Builds" are one favourite and not two.
class FilePickerPlaces
{
public:
    struct Favourite
    {
        QString path;
        //! What the tree shows. Never empty: a favourite added without a
        //! name is shown under its folder's own name.
        QString label;
    };

    //! In the order they were added. Includes favourites whose folder does
    //! not currently exist - a drive that is unplugged is still a place the
    //! person wants back when it returns - so a caller that shows them
    //! decides what to do about a missing one.
    static QVector<Favourite> favourites();

    //! False when `path` is not an existing directory or is already a
    //! favourite. An empty `label` means the folder's own name.
    static bool addFavourite(const QString& path, const QString& label = QString());
    //! False when `path` is not a favourite or `label` is blank. Surrounding
    //! whitespace is not part of a name.
    static bool renameFavourite(const QString& path, const QString& label);
    //! False when `path` was not a favourite. Only the pin goes; the folder
    //! is never touched.
    static bool removeFavourite(const QString& path);
    static bool isFavourite(const QString& path);

    //! Where this application last chose a file. Empty when it never has,
    //! or when that folder has since gone away.
    static QString lastDirectory();
    static void setLastDirectory(const QString& directory);

    //! The one form every path above is stored and compared in.
    static QString normalised(const QString& path);
};

} // namespace novu::ui
