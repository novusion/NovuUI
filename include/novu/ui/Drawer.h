#pragma once

#include "novu/ui/AppMark.h"
#include "novu/ui/Icon.h"
#include "novu/ui/StyleSpec.h"

#include <QPointer>
#include <QString>
#include <QWidget>

class QHBoxLayout;
class QVBoxLayout;

namespace novu::ui {

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
class Drawer : public QWidget
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
    AppMark* m_mark = nullptr;
    QWidget* m_body = nullptr;
    QWidget* m_actions = nullptr;
    QWidget* m_bodyHost = nullptr;
};

} // namespace novu::ui
