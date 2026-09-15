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

    // A tip about a *row* rather than about a control: the same card in a
    // smaller hand. A list of full-size tips under the pointer covers the list
    // they are about, which is the defect this flag exists to avoid.
    bool compact = false;

    bool isEmpty() const
    {
        return title.isEmpty() && brief.isEmpty() && status.isEmpty() && detail.isEmpty();
    }
};

// Tooltips, in two modes.
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

    // The short form. Installing an empty string removes the tip.
    static void install(QWidget* target, const QString& brief);
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
    bool m_help = false;
};

} // namespace novu::ui
