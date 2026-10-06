#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/AppMark.h"
#include "novu/ui/Icon.h"
#include "novu/ui/StyleSpec.h"

#include <QPointer>
#include <QString>
#include <QWidget>

class QGraphicsOpacityEffect;
class QHBoxLayout;
class QParallelAnimationGroup;
class QVBoxLayout;

namespace novu::ui {

class IconButton;
class TabBar;

// The detail view that opens over a list instead of navigating away from it.
//
// NovuAdmin's note is the reason it exists: *it opens over the list rather than
// navigating away, so the table keeps its filters and its scroll position. The
// web admin loses both on every drill-down, and that is the difference the
// contract calls out.* The approved NovuSpace design draws the same thing as
// `.drawer` with its `.scrim`.
//
// ── how it differs from the other three ──────────────────────────────────
//
// The library now has four ways to put something on top of something else, and
// telling them apart is most of using them correctly:
//
//   Dialog         a question. Modal, small, centred, and it blocks until
//                  answered. The application stops.
//   Drawer         a detail. Anchored to an edge, as tall as the window, with
//                  a scrim that dims the list behind it. The application waits
//                  but is not asking anything, and Escape closes it.
//   FloatingPanel  a detached view. No scrim, several at once, draggable, and
//                  the application keeps working underneath.
//   Popover        a menu. Small, transient, closed by the next click anywhere.
//
// The scrim is what separates a Drawer from a FloatingPanel: a drawer is about
// *one row of the list behind it*, and dimming that list is how it says so.
//
// ── what it owns ─────────────────────────────────────────────────────────
//
// The scrim, the edge, the width, closing on Escape and on a click outside, and
// the head. A caller fills `body()` and, optionally, a footer.
class NOVUUI_API Drawer : public QWidget
{
    Q_OBJECT

public:
    // Which edge it is anchored to. Trailing is the default and the one the
    // designs use: a detail reads as coming from the direction the list flows
    // towards.
    enum class Edge { Leading, Trailing };

    // `host` is the widget it covers — usually the window, sometimes one panel
    // of it. The drawer parents itself to the host and lives as long as it
    // does.
    explicit Drawer(QWidget* host);

    void setTitle(const QString& title);
    QString title() const { return m_title; }
    // The mono line under the title: an id, a path, a version.
    void setSubtitle(const QString& subtitle);
    QString subtitle() const { return m_subtitle; }
    void setGlyph(Glyph glyph);

    AppMark* mark() const { return m_mark; }

    // The scrolling body. Fill this.
    QVBoxLayout* body() const;
    QWidget* bodyWidget() const { return m_body; }

    // `.drawer-acts` — the strip of actions directly under the head, above the
    // body. Created on first call; a drawer with nothing to do never builds it.
    QHBoxLayout* actions();
    QWidget* actionsWidget();

    // `.drawer-head h2{display:flex;gap:10px;flex-wrap:wrap}` — the badges
    // that qualify the name, on the title's own line. A status and an
    // execution mode belong *beside* the thing they describe: moved down into
    // the body they read as two more facts among thirty rather than as what
    // the reader needs before any of them. Created on first call.
    QHBoxLayout* titleBadges();

    // `.tabs` — the strip between the actions and the body, for a drawer whose
    // detail has several faces. The reference's bot drawer has eight of them
    // (Overview, Risk, Indicators, Filters, Trend, Orders, Close, Logs), and
    // they belong to the drawer's own chrome rather than to its contents: they
    // must stay put while the body under them scrolls. Created on first call,
    // in the underlined variant, because these are faces of one thing and not
    // separate documents.
    TabBar* tabs();
    bool hasTabs() const { return m_tabs != nullptr; }

    Edge edge() const { return m_edge; }
    void setEdge(Edge edge);

    // How wide, as a fraction of the host, capped in pixels — the design's
    // `width:min(760px, 92%)`. Both matter: the cap stops a drawer swallowing
    // a wide window, the fraction stops it overflowing a narrow one.
    int maximumWidth() const { return m_maximumWidth; }
    void setMaximumWidth(int px);
    double widthFraction() const { return m_fraction; }
    void setWidthFraction(double fraction);

    bool isOpen() const { return m_open; }
    void open();
    void close();

    // Whether clicking the dimmed area closes it. True by default; false for a
    // drawer holding an edit in progress, where a stray click would lose it.
    bool closesOnScrimClick() const { return m_scrimCloses; }
    void setClosesOnScrimClick(bool closes);

    QWidget* scrim() const { return m_scrim; }

    // The head's own close box — `<button class="winbtn" data-drawer-close>`.
    // Escape and a click on the scrim already leave, but neither is visible:
    // a panel with no way out drawn on it is one a reader has to guess at.
    IconButton* closeButton() const { return m_close; }

    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

signals:
    void opened();
    void closed();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    bool event(QEvent* event) override;

private:
    int headHeight() const;
    void relayout();
    void placeOnHost();

    QString m_title;
    QString m_subtitle;
    Edge m_edge = Edge::Trailing;
    int m_maximumWidth = 760;
    double m_fraction = 0.92;
    bool m_open = false;
    bool m_scrimCloses = true;

    QPointer<QWidget> m_host;
    QWidget* m_scrim = nullptr;
    // `.drawer{animation:slidein .28s cubic-bezier(.2,.8,.2,1) both}` and
    // `.scrim{animation:fadein .2s ease both}` - an instant show()/hide() said
    // a detail was simply swapped in, where the design says it arrives. Kept
    // as members, not locals, so a close() that lands mid-open (or the
    // reverse) stops the run in progress instead of fighting it for the same
    // geometry.
    QGraphicsOpacityEffect* m_opacityEffect = nullptr;
    QGraphicsOpacityEffect* m_scrimOpacityEffect = nullptr;
    QParallelAnimationGroup* m_anim = nullptr;
    AppMark* m_mark = nullptr;
    QWidget* m_body = nullptr;
    QWidget* m_actions = nullptr;
    IconButton* m_close = nullptr;
    TabBar* m_tabs = nullptr;
    QWidget* m_badges = nullptr;
    QWidget* m_bodyHost = nullptr;
};

} // namespace novu::ui
