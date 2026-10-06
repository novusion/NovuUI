#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/ThemeProfile.h"

#include <QList>
#include <QPointer>
#include <QString>
#include <QWidget>

namespace novu::ui {

// The colour of one help note.
//
// Accent by default. The rest exist so a note that *means* something can look
// like it - "this is destructive", "this is not ready yet" - and because a
// screen with eight notes on it needs adjacent bubbles to be told apart. It is
// the one place in the system where a colour is allowed to be decorative, and
// the enum says so rather than leaving a caller to pass a QColor.
enum class HelpTone { Accent, Info, Success, Warning, Danger, Neutral };

// Where a note sits relative to the thing it explains.
enum class HelpPlacement { Auto, Left, Right, Above, Below };

// An explanatory layer drawn over a live window.
//
// Notes are bubbles - a coloured outline, a title, a sentence - each joined to
// the widget it explains by a dashed leader ending in an arrow. The window
// underneath keeps working: the overlay is transparent to the mouse, so a user
// can read the note about the toolbar and then use the toolbar, without
// dismissing anything.
//
// That is the whole design constraint, and it is why this is not a dialog, not
// a wizard and not a modal tour. Those stop the application to explain it. This
// annotates an application that is still running.
//
//     auto* help = new HelpOverlay(window);
//     help->addNote(sidebar, tr("Workspaces"),
//                   tr("Every workspace and its modules. The number is how many "
//                      "items the category holds."), HelpPlacement::Right);
//     help->addNote(rebuildButton, tr("Full rebuild"),
//                   tr("Rebuilds the index and the knowledge base from scratch."),
//                   HelpPlacement::Below, HelpTone::Warning);
//     help->show();
//
// Anchors are resolved at paint time from the target widget's current geometry,
// so notes follow their targets through a resize, a splitter drag or a scroll
// without the caller re-registering anything. A target that is hidden or
// destroyed takes its note with it rather than leaving a leader pointing at
// nothing.
class NOVUUI_API HelpOverlay : public QWidget
{
    Q_OBJECT

public:
    // Covers `host` entirely and follows its size. The host keeps ownership.
    explicit HelpOverlay(QWidget* host);

    // `target` is the widget being explained - any descendant of the host.
    // Passing the host itself anchors the note to the window as a whole, which
    // is what a "what is this screen" note wants.
    void addNote(QWidget* target, const QString& title, const QString& body,
                 HelpPlacement placement = HelpPlacement::Auto,
                 HelpTone tone = HelpTone::Accent);
    void clear();
    int noteCount() const { return m_notes.size(); }

    // The bubble's width. Notes are all one width on purpose: a column of
    // bubbles that each sized to their own sentence reads as clutter rather
    // than as a layer. Derived from the profile's type scale when unset.
    int noteWidth() const;
    void setNoteWidth(int width);

    // Convenience for the usual case - build it, fill it, show it - so an
    // application's help action is one call rather than a lifetime to manage.
    static HelpOverlay* attach(QWidget* host);

protected:
    void paintEvent(QPaintEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    bool event(QEvent* event) override;

private:
    struct Note
    {
        QPointer<QWidget> target;
        QString title;
        QString body;
        HelpPlacement placement = HelpPlacement::Auto;
        HelpTone tone = HelpTone::Accent;
    };

    // Everything about one note in overlay coordinates, computed for a paint.
    struct Placed
    {
        const Note* note = nullptr;
        QRect bubble;
        QRect target;
        QColor colour;
        QString body;      // elided to fit, if it had to be
    };

    QColor colourFor(HelpTone tone) const;
    int bubbleHeightFor(const QString& title, const QString& body, int width) const;
    QVector<Placed> layoutNotes() const;
    void paintNote(QPainter& painter, const Placed& placed) const;
    void paintLeader(QPainter& painter, const Placed& placed) const;

    QList<Note> m_notes;
    QWidget* m_host = nullptr;
    int m_noteWidth = 0;
};

} // namespace novu::ui
