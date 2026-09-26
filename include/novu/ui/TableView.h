#pragma once

#include "novu/ui/Badge.h"
#include "novu/ui/StatusDot.h"

#include <QColor>
#include <QFont>
#include <QHash>
#include <QTableWidget>
#include <QVector>

namespace novu::ui {

// The table used across all Novu applications.
//
// It owns column layout entirely. A page declares what each column IS; the
// table decides how wide it becomes. Pages never measure text, never call
// setColumnWidth(), never touch resize modes.
//
// Column roles
// ------------
//   Text     free-form prose (Name, Email, Description, Filename, Resource).
//            Absorbs leftover width.
//   Content  fixed-form values and widgets (Status, Plan, Release, Actions,
//            Online, Date, Price, Version). Always exactly as wide as its
//            widest cell.
//
// A role is a property of the column, declared once. It is never inferred from
// the data — data-driven guessing made the same column change behaviour
// depending on which rows happened to be loaded.
//
// Layout algorithm (runs on data change and on resize)
// ----------------------------------------------------
//   1. Measure every cell: widgets report sizeHint(), text is measured with
//      that cell's own font, plus the header caption.
//   2. columnWidth = max(header, all cells) + padding.
//   3. Content columns are fixed at that width.
//   4. remaining = viewport - sum(content widths); shared among Text columns
//      in proportion to their measured width. If remaining <= 0, Flexible
//      columns give up width (down to a floor, eliding what no longer fits)
//      and only if that is still not enough does the view scroll — a Text or
//      Content column is never compressed and never elided.
class TableView : public QTableWidget
{
    Q_OBJECT

public:
    enum class ColumnRole {
        Text,
        Content,
        // A column the design lets give way. Text and Content are both
        // floors - the view scrolls rather than shrink them, because a
        // clipped price or a cut-off badge is a wrong reading. A sentence
        // is not: the approved design puts one ("Why", a bot's last error)
        // in a card narrower than the sentence and lets it end in an
        // ellipsis, and without this the column shouldered the table past
        // the card and took the P&L, Since and action columns off the right
        // edge with it. Compressed only when the table does not otherwise
        // fit, and never below kFlexibleFloor.
        //
        // What it does with the text it can no longer fit on one line is
        // *wrap* it, because that is what the design does: a `td` carries
        // no `white-space` rule, so only `.num` and `.act` stay on one line
        // and the sentence columns run onto a second. Eliding instead threw
        // away the half of the sentence that says what went wrong.
        Flexible,
        // The three cell classes the sheet actually writes, which this
        // vocabulary was missing. Every page was expressing them by putting a
        // *widget* in the cell - a mono QLabel per price, per timestamp, per
        // id - which is a column's typography reimplemented once per page,
        // and a widget cannot wrap or compress the way a `td` does.
        //
        // `td.num` — right, monospace, tabular, never wrapped. Prices and
        // counts, measured to fit and never compressed: a clipped price is a
        // wrong reading.
        Numeric,
        // `td.mono` — the monospace face a step down, in the muted ink.
        // Timestamps, ids, paths. It wraps and may give width back, because
        // the sheet states no `white-space` on it.
        Mono,
        // `td.act` — `text-align:right; white-space:nowrap; width:1%`. The row
        // actions: as narrow as the buttons in it and never a pixel more.
        Action,
    };

    // `.tbl-wrap.cap{max-height:440px;overflow-y:auto}` — the one place the
    // design lets a table scroll. Everywhere else a table is as tall as its
    // rows and the page scrolls instead, which is what a table inside a card
    // is: markup with a height, not a viewport. 0 removes the cap.
    void setHeightCap(int pixels);
    int heightCap() const { return m_heightCap; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    explicit TableView(QWidget* parent = nullptr);

    // Declares the columns. Also sets the header captions, so a page states
    // each column exactly once.
    void setColumns(const QVector<QPair<QString, ColumnRole>>& columns);

    // ── painted cells ────────────────────────────────────────────────────
    //
    // The cell vocabulary for repeated rows: a status dot, a primary text and
    // any number of badges after it, described as data and drawn by the
    // table's delegate.
    //
    // This is what setWidget() was being used for and should not have been.
    // A models table of 28 rows built a container, a dot, a title, a
    // quantisation badge and a badge per capability in every row - 336
    // widgets - and because selecting a row rebuilt the page, every one of
    // them was destroyed and built again on a click. Content described this
    // way costs nothing per row: there is no widget to create, the row is
    // painted only while it is on screen, and selection is a repaint.
    //
    // A cell still gets a widget when it holds a control somebody clicks.
    // A badge is not a control.
    struct CellBadge
    {
        QString text;
        Badge::Tone tone = Badge::Tone::Neutral;
    };

    struct CellContent
    {
        bool hasStatus = false;
        StatusDot::Tone status = StatusDot::Tone::Neutral;
        QString text;
        // Drawn after the text, in order, each sized to its own caption.
        QVector<CellBadge> badges;
        // Muted, and only if there is room left once the badges have theirs.
        QString trailing;
        bool strongText = false;
    };

    void setContent(int row, int column, const CellContent& content);

    // Installs a widget in a cell. Use this instead of setCellWidget() so the
    // table can account for the widget when measuring the column. Prefer
    // setContent() for anything that is only drawn.
    void setWidget(int row, int column, QWidget* widget);

    // Convenience: a cell containing one or more widgets laid out
    // horizontally with the standard spacing and margins (badges, row
    // actions). The container is transparent so it never paints a block over
    // the row.
    void setWidgets(int row, int column, const QVector<QWidget*>& widgets);

    // A leading visual followed by a two-line identity. This is the shared
    // shape used by model/file/resource tables: a type badge or glyph at the
    // start, the primary name, and a muted mono identifier below it.
    // `leading` may be null. Ownership transfers to the table.
    void setIdentity(int row, int column, const QString& title,
                     const QString& subtitle = QString(),
                     QWidget* leading = nullptr);

    // Sets cell text with the full value as tooltip.
    void setText(int row, int column, const QString& text, const QString& tooltip = {});

    // Call once after the rows for a refresh have been populated.
    void relayoutColumns();

    // A row's selection reads as the product pointing at it, not as a generic
    // highlight - the design's `tr.sel{background:rgba(var(--rl),.13)}` plus
    // a hairline accent bar on the row's leading edge
    // (`tr.sel td:first-child{box-shadow:inset 2.5px 0 0 var(--acc)}`). This
    // is the master list beside a reading pane's own selection, and it needs
    // to read as different from picking a row to act on (a table's ordinary
    // `:selected`, which stays the neutral surface tint) - a mail list where
    // every row read like a bulk-selected one would not say which message is
    // actually open. Off by default: most tables select a row to act on it,
    // not to keep it open beside them.
    void setSelectionAccented(bool accented);
    bool isSelectionAccented() const { return m_selectionAccented; }

    // On by default: a table with room to spare hands the extra width to its
    // Text/Flexible/Mono columns (`table.tbl{width:100%}` under automatic
    // layout). A wide preset with many pinned Numeric/Content columns and one
    // narrow Mono timestamp is the case that wants this off - handing every
    // spare pixel to a sixteen-character timestamp opened a gap between it
    // and the next column rather than reading as one compact row, where the
    // approved sheet leaves the table at its natural width and lets it
    // scroll. Compression when the table does *not* fit is unaffected either
    // way - a preset that outgrows the card still gives width back and still
    // scrolls; this only stops offering back the width nothing asked for.
    void setStretchToFill(bool stretch);
    bool stretchesToFill() const { return m_stretchToFill; }

signals:
    // A row was opened: the design's `class="selectable"` plus a
    // `data-open-*` / `data-select-*` attribute on the row itself.
    //
    // Not `cellClicked`, for two reasons. A page cares which *row* was opened
    // and not which column the pointer happened to be over; and a click that
    // lands on a control inside the row - a Stop button, a Cancel, an inspect
    // eye - is that control's click and not the row's. The second falls out of
    // how Qt delivers events: a button in a cell consumes the press, so it
    // never reaches the view, which is exactly the reference's own rule
    // ("ignored if the click was inside [data-bot-stop],[data-bot-start],…").
    //
    // Without this every page grew its own `connect(table,
    // &QTableWidget::cellClicked, …)` with its own idea of what counts as a
    // click on a row - or, more often, grew nothing at all and left the row
    // dead while only the eye button worked.
    void rowActivated(int row);

protected:
    bool event(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    // Qt's own grid is one pen in both directions; the design draws a
    // row's own bottom edge and nothing between its columns. Painted here,
    // after the base class, rather than through showGrid() and a
    // stylesheet gridline-color, which cannot be told to skip one axis.
    void paintEvent(QPaintEvent* event) override;
    // `table.tbl tbody tr:hover{background:var(--panel)}` - every row in
    // every table in the product is hoverable, whether or not it opens
    // anything. Qt's own item delegate paints no such thing on its own.
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    // Raises the row height to fit the tallest cell widget actually in the
    // table. Runs from relayoutColumns(), which is the one call a page makes
    // after its rows are populated.
    void applyMetrics();
    // One pass, after the widths are assigned: both the tallest cell widget
    // and a wrapped cell's height depend on the width its column ended up
    // with.
    void fitRows();
    int measureHeader(int column) const;
    int measureColumn(int column) const;
    int measureColumnFloor(int column) const;
    Qt::Alignment alignmentFor(int column) const;
    QFont columnFont(int column) const;
    QColor columnInk(int column) const;
    void applySelectionStyle();

    QVector<ColumnRole> m_roles;
    bool m_inRelayout = false;
    bool m_selectionAccented = false;
    bool m_stretchToFill = true;
    int m_heightCap = 0;
    int m_hoveredRow = -1;
};

// What a painted cell measures and how it draws, so the delegate and the
// column measurement cannot disagree about how wide a row needs to be.
QSize cellContentSize(const QWidget* context, const TableView::CellContent& content);
void paintCellContent(QPainter* painter, const QRect& box, const QWidget* context,
                      const TableView::CellContent& content, const QColor& textInk);

} // namespace novu::ui

Q_DECLARE_METATYPE(novu::ui::TableView::CellContent)
