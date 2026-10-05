#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

class QWidget;

namespace novu::ui {

// Opening and saving a file, in the product's own window.
//
// FilePicker is the presentation: a tree, a listing, a name field. It does no
// filesystem I/O on purpose - a caller populates it - which left every product
// that needed to actually open a file reaching for QFileDialog, and a platform
// dialog in the middle of a Novu window is the one place the product stops
// looking like itself.
//
// This is the missing half: the driver that walks the real filesystem, and the
// modal window around it. Two calls, shaped like the platform's so that
// replacing a QFileDialog line is a one-line change, returning an empty string
// when the person cancelled - which is not a failure and should not be
// reported as one.
//
// ── filters ────────────────────────────────────────────────────────────────
//
// Every pattern of every filter given is applied at once, and the listing
// shows files matching any of them. There is no filter selector, because
// FilePicker's design has no such control and inventing one here would make
// this window differ from the design it is meant to be. A caller that wants
// everything shown passes no filters; a caller that wants one kind passes one
// filter. Passing two kinds means "show both", not "offer a choice between
// them".
class FilePickerDialog
{
public:
    struct Filter
    {
        //! Shown in the window's own note, so the person can see what is being
        //! listed. Never matched against anything.
        QString label;
        //! Wildcards, as QDir takes them: "*.yaml", "*.csv".
        QStringList patterns;
    };

    //! An existing file, chosen for reading. Empty when cancelled, and never a
    //! path that does not exist.
    //!
    //! `startPath` may be a directory to open in or a file to start on; either
    //! is accepted, and a path that has gone away falls back to the home
    //! directory rather than opening on nothing.
    static QString open(QWidget* parent, const QString& title,
                        const QString& startPath,
                        const QVector<Filter>& filters = {});

    //! A path to write to. Empty when cancelled.
    //!
    //! `suggestedPath` carries both where to start and what to call it. The
    //! first filter's first pattern supplies the extension when the typed name
    //! has none, so a person who types "october" gets "october.csv" rather
    //! than an extensionless file their own system cannot open. Overwriting an
    //! existing file is confirmed first.
    static QString save(QWidget* parent, const QString& title,
                        const QString& suggestedPath,
                        const QVector<Filter>& filters = {});
};

} // namespace novu::ui
