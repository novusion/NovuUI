#pragma once

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
//      in proportion to their measured width. If remaining <= 0 every column
//      keeps its measured width and the view scrolls horizontally — content is
//      never compressed and never elided.
class TableView : public QTableWidget
{
    Q_OBJECT

public:
    enum class ColumnRole { Text, Content };

    explicit TableView(QWidget* parent = nullptr);

    // Declares the columns. Also sets the header captions, so a page states
    // each column exactly once.
    void setColumns(const QVector<QPair<QString, ColumnRole>>& columns);

    // Installs a widget in a cell. Use this instead of setCellWidget() so the
    // table can account for the widget when measuring the column.
    void setWidget(int row, int column, QWidget* widget);

    // Convenience: a cell containing one or more widgets laid out
    // horizontally with the standard spacing and margins (badges, row
    // actions). The container is transparent so it never paints a block over
    // the row.
    void setWidgets(int row, int column, const QVector<QWidget*>& widgets);

    // Sets cell text with the full value as tooltip.
    void setText(int row, int column, const QString& text, const QString& tooltip = {});

    // Call once after the rows for a refresh have been populated.
    void relayoutColumns();

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    int measureColumn(int column) const;
    Qt::Alignment alignmentFor(int column) const;

    QVector<ColumnRole> m_roles;
    bool m_inRelayout = false;
};

} // namespace novu::ui
