#pragma once

#include "novu/ui/ThemeProfile.h"

#include <QHash>
#include <QList>
#include <QPointer>
#include <QString>
#include <QWidget>

class QTimer;

namespace novu::ui {

// What a control is, in the words a person would use.
//
// Five fields rather than one string, because the approved design's tooltip is
// five things — `.tip-t` the name, `.tip-t .sc` the key that reaches it,
// `.tip-s` one line on what it does, `.tip-st` what it is doing right now, and
// `.tip-d` the long form. A tooltip that takes a single string cannot draw any
// of that, which is why every application that wanted the design's tip ended
// up composing it out of newlines.
//
// It is also the description a help window wants. One explanation of a button,
// read by the hover and by the help list, rather than the same sentence written
// twice and drifting.
struct TipContent
{
    QString title;      // the control's name
    QString shortcut;   // the key that reaches it
    QString brief;      // one line: what it does
    QString status;     // what it is doing right now, in the accent
    QString detail;     // the long form, shown only in help mode

    // The three content levels, as named constructors.
    //
    // They are field subsets of this one struct - not variants, not sizes.
    // Nothing about the card changes between them; what changes is how much it
    // has to say. Named because "which level is this" should be readable at
    // the call site, and because a tip assembled field by field is a tip that
    // quietly ends up at the wrong level.
    //
    // See docs/VOCABULARY.md.

    // Identification only: a tab, a path, a row. No promise about what
    // activating it does, because these are not activated.
    static TipContent label(const QString& title, const QString& status = {});

    // The default for anything actionable - buttons, toolbar and title-bar
    // actions, session and console controls.
    //
    // `brief` says *what happens when it is activated*, not what the control
    // is called again: "Create a workspace for organizing chats, tasks and
    // context", never "New workspace". A brief that repeats the title is the
    // one failure this level has.
    static TipContent action(const QString& title, const QString& brief,
                             const QString& shortcut = {}, const QString& status = {});

    // Exceptional. Only where the extra paragraph earns its place - the
    // consequence of a destructive action, the thing a person cannot guess.
    static TipContent extended(const QString& title, const QString& brief,
                               const QString& detail, const QString& shortcut = {},
                               const QString& status = {});

    bool isEmpty() const
    {
        return title.isEmpty() && brief.isEmpty() && status.isEmpty() && detail.isEmpty();
    }
};

// The only tooltip in the system. Two states, and no third.
//
// ── the rule ──────────────────────────────────────────────────────────────
//
// A hover explanation in a Novusion application is this class, in one of two
// states - the approved design's own two, and its words for them:
//
//     compact  `.tip-t` `.tip-s` `.tip-st`, and `.tip-more` saying there is
//              more. What a hover gives straight away.
//     full     the same card with `.tip-d` added under its rule, once the
//              pointer has stayed.
//
// *Compact is a state of this card, not a smaller card.* Both are the one
// `.tip` the design defines, at its one max-width of 274px. There was briefly
// a `TipContent::compact` flag here that drew the tip a step down the type
// scale for tips about rows - a third size the mockup has no trace of. It is
// gone, and a row that needs a smaller explanation needs a shorter `brief`.
//
// Nothing else explains a control on hover. Not QToolTip, whose surface, delay
// and type belong to the OS and cannot be themed. Not an application's own tip
// widget. Three hover treatments across four applications is how the ecosystem
// got here.
//
// ── in two modes ──────────────────────────────────────────────────────────
//
// Off: hovering gives the name and the key, and nothing else. On: the same
// hover also explains the control and says what it is currently doing, and the
// long form arrives a moment later if the pointer stays.
//
// Help is a mode rather than an overlay on purpose — and it is the one thing
// this class does that HelpOverlay does not. HelpOverlay annotates a window all
// at once, with leaders pointing at everything; that is the right answer for
// "explain this screen" and the wrong one for "what is this button". Here
// nothing is highlighted in advance and nothing is covered: the person keeps
// working and the explanation appears where they were already looking.
//
// ── what this replaces ────────────────────────────────────────────────────
//
// Three implementations of the same idea existed. QToolTip, which is an OS
// tooltip — its own delay, its own type, a square surface, and unstyleable.
// NovusionUI's `Tooltip`, which is the branded replacement and got the
// positioning right: centred over the target, 8px clear of it, flipped below
// when there is no room above, clamped to the screen. And NovusionUI's
// `HelpTip`, which got the *content* right: the five fields above and the help
// mode.
//
// This is those two, merged. The geometry is Tooltip's, the content model is
// HelpTip's, and the colours are tokens rather than the `Token::Panel` /
// `Token::Hair2` constants either of them read — so a tip inside a NovuTrade
// scope is a NovuTrade tip, which neither original could be.
//
//     Tooltip::install(button, tr("Reset password"));
//     Tooltip::install(rebuild, {tr("Full rebuild"), tr("Ctrl+Shift+R"),
//                                tr("Rebuilds the index from scratch."),
//                                tr("last run 4 minutes ago"),
//                                tr("Every document is re-read ...")});
//
// One reused popup follows the pointer and the focus ring, and tears the tip
// down when the widget goes. Install and forget.
class Tooltip : public QWidget
{
    Q_OBJECT

public:
    static Tooltip& instance();

    // A LABEL: the string is the control's *name*, not a sentence about it.
    //
    // It used to fill `brief`, which produced a card with no title and matched
    // none of the three levels. Most callers were already passing a name - a
    // tab's label, a path, "Close" - so the string means what those callers
    // meant. A sentence belongs in TipContent::action().
    //
    // Installing an empty string removes the tip.
    static void install(QWidget* target, const QString& title);
    // The full form.
    static void install(QWidget* target, const TipContent& content);
    static void remove(QWidget* target);

    static TipContent contentFor(const QWidget* target);
    // Every description installed anywhere, for a help window to list. Ordered
    // by title so the list is stable between runs rather than following hash
    // order.
    static QList<TipContent> all();

    // Shows immediately, without the hover delay — for a tip triggered by
    // keyboard focus rather than the pointer.
    static void showFor(QWidget* target);
    static void hideTip();

    static void setHelpMode(bool on);
    static bool helpMode();

    // What is on screen, for a test that wants to assert on the tip rather
    // than photograph it. Null when nothing is showing.
    static QWidget* shownFor();
    // The content the visible tip is drawing, including whether the detail has
    // arrived yet.
    TipContent visibleContent() const { return m_content; }
    bool detailVisible() const { return m_detail; }

    QSize sizeHint() const override;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    Tooltip();

    void scheduleFor(QWidget* target);
    void present(QWidget* target);
    void dismiss();
    // The card's height for the content it is currently carrying, which is
    // what makes the detail arriving a resize rather than a clipped paragraph.
    QSize measure(const TipContent& content, bool withDetail) const;
    void relayout();

    QHash<QWidget*, TipContent> m_tips;
    QPointer<QWidget> m_pending;
    QPointer<QWidget> m_shown;
    QTimer* m_showTimer = nullptr;
    // The second delay: the long form is not part of the first impression, so
    // it arrives only if the pointer is still there.
    QTimer* m_detailTimer = nullptr;
    TipContent m_content;
    bool m_detail = false;
    // On, matching the design's own default (`prefs.tooltipHelp: true` in the
    // approved mockup). Off, a tip is the name and the key and nothing else -
    // which is the OS tooltip this class replaced, and is not what anyone
    // expects to see on a first hover.
    bool m_help = true;
};

} // namespace novu::ui
