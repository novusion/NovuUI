#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/StyleSpec.h"

#include <QAbstractScrollArea>
#include <QFont>
#include <QSize>

namespace novu::ui {

// A long monospace list that paints only what is on screen.
//
// Two components need this and a third will: LogView (a running log), DiffView
// (a unified diff) and, when NovuExplorer arrives, a hex or a manifest view.
// All three hold thousands of fixed-height monospace rows, and all three would
// otherwise reach for QPlainTextEdit — which cannot do any of them properly.
//
// What a text edit gets wrong, in the same way each time: a column (a level, a
// gutter, a line number) has to be a *column*, aligned down the page, which
// means either padding every row with spaces — wrong the moment a row differs
// in width — or a rich-text table costing a layout pass per row. And a text
// edit holds its content as a document, so appending to ten thousand lines
// re-lays out ten thousand lines.
//
// ── what this owns ────────────────────────────────────────────────────────
//
// The arithmetic, and nothing else: the row height for the profile's monospace
// face, the scroll range as the viewport and the content change, and which
// rows are actually on screen. A subclass says how many rows it has and paints
// one, and gets virtualisation without writing it a second time.
//
// It owns no content. There is no line struct here, because a log line and a
// diff line share nothing but their height — merging them would be a base
// class holding a level, a timestamp, a hunk header and two line numbers, of
// which any given subclass uses half.
class NOVUUI_API MonoLineView : public QAbstractScrollArea
{
    Q_OBJECT

public:
    // The first row on screen and how many are drawn. Public because a test
    // that wants to prove virtualisation happens should not have to count
    // paint calls.
    int firstVisibleRow() const;
    int visibleRowCount() const;
    int rowHeight() const;

    // Scrolls so `row` is on screen, and does nothing if it already is.
    void scrollToRow(int row);
    void scrollToEnd();
    bool isAtEnd() const;

    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

    // ── sizing ───────────────────────────────────────────────────────────
    //
    // A scroll area has no height of its own: it takes whatever a layout
    // gives it, which is right for a log filling a panel and wrong for the
    // other half of the cases. A diff quoted inside a chat turn, a log inside
    // a card, a manifest under a file row -- each is as tall as what it holds
    // and scrolls only past a cap, and a column cannot lay that out unless
    // the view says how tall it is.
    //
    // Left to do it themselves, callers multiply rowHeight() by rowCount()
    // and add the padding, which is this class's own arithmetic written a
    // second time -- and written without the leading, which is the part that
    // differs between a log and a diff.

    // What the rows need, padding included, whatever the viewport is.
    int contentHeight() const;

    // Report contentHeight() as the size hint, up to `maxRows` (0 = no cap);
    // past the cap the view scrolls as before. Off by default: a view that
    // fills a panel should keep filling it.
    void setFitToContent(bool fit, int maxRows = 0);
    bool fitsContent() const { return m_fitToContent; }
    int maxVisibleRows() const { return m_maxRows; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    explicit MonoLineView(QWidget* parent = nullptr);

    // ── what a subclass must answer ──────────────────────────────────────
    virtual int rowCount() const = 0;
    // `box` spans the full viewport width at this row's position. The painter
    // is already set to the row font; a subclass that changes it should set it
    // back, since the next row will not be told.
    virtual void paintRow(QPainter* painter, int row, const QRect& box) = 0;

    // ── what it may override ─────────────────────────────────────────────
    // The line-height multiplier. The design uses 1.75 for logs and 1.65 for
    // diffs, which is the only geometry the two disagree on.
    virtual double lineLeading() const { return 1.75; }
    // Padding inside the viewport.
    virtual int contentPaddingH() const { return 14; }
    virtual int contentPaddingV() const { return 10; }
    // The ground the rows are drawn on. Both current subclasses use the window
    // colour: output sits on the window, not on a card.
    virtual ColorRole groundRole() const { return ColorRole::WindowBg; }

    // The profile's monospace face at the row size. Subclasses paint with it
    // and measure with it, so a column cannot be measured with one font and
    // drawn with another - which is the bug that made the level column jitter.
    QFont rowFont() const;

    // Call when the number of rows changes. Keeps the scroll bar honest and
    // holds the view at the end if it was already there.
    void rowsChanged(bool keepAtEnd);

    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;
    bool event(QEvent* event) override;

private:
    void updateScrollRange(bool keepAtEnd);

    bool m_fitToContent = false;
    int m_maxRows = 0;
};

} // namespace novu::ui
