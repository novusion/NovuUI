#include "novu/ui/TableView.h"

#include "novu/ui/ThemeManager.h"

#include <QApplication>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QResizeEvent>
#include <QTimer>

namespace novu::ui {

namespace {

constexpr int kColumnPadding = 16;

} // namespace

TableView::TableView(QWidget* parent)
    : QTableWidget(parent)
{
    const ThemeProfile& profile = themeFor(this);
    const ThemeColors& t = profile.colors;
    const int rowHeight = profile.table.rowHeight;

    verticalHeader()->setVisible(false);
    verticalHeader()->setDefaultSectionSize(rowHeight);
    verticalHeader()->setMinimumSectionSize(rowHeight);

    horizontalHeader()->setMinimumHeight(profile.table.headerHeight);
    horizontalHeader()->setHighlightSections(false);
    horizontalHeader()->setStretchLastSection(false);
    // Every width is assigned by relayoutColumns(); the automatic modes would
    // ignore those assignments.
    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setWordWrap(false);
    // Truncation is not an accepted outcome: columns fit their content and the
    // view scrolls when the window is too narrow.
    setTextElideMode(Qt::ElideNone);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    // Alternating row colours are off by design: the alternate colour is drawn
    // by the item delegate and does not extend under cell widgets, so badge
    // and button cells showed the base colour as dark blocks. Grid lines are
    // painted across the whole viewport and stay continuous instead.
    setAlternatingRowColors(false);
    setShowGrid(true);

    // Only the header *sections* carry the header colour. Styling QHeaderView
    // itself would paint the empty area past the last column as a grey block.
    setStyleSheet(QStringLiteral(
        "QTableWidget { background: %1; color: %2; border: 1px solid %3;"
        " gridline-color: %3; outline: none; }"
        "QTableWidget::item { padding: 4px 8px; border: none; }"
        "QTableWidget::item:selected { background: %4; color: %2; }"
        "QHeaderView { background: transparent; border: none; }"
        "QHeaderView::section { background: %5; color: %6; border: none;"
        " border-bottom: 1px solid %3; padding: 4px 8px; font-weight: 600; }")
        .arg(t.panelBg.name(), t.textPrimary.name(), t.borderSubtle.name(),
             rgba(t.surfaceActive, 1.0), t.headerBg.name(), t.textSecondary.name()));
}

void TableView::setColumns(const QVector<QPair<QString, ColumnRole>>& columns)
{
    setColumnCount(columns.size());
    m_roles.clear();
    m_roles.reserve(columns.size());

    QStringList captions;
    captions.reserve(columns.size());
    for (const auto& column : columns) {
        captions << column.first.toUpper();
        m_roles.append(column.second);
    }
    setHorizontalHeaderLabels(captions);

    // Content columns hold values - prices, counts, timestamps - and read
    // right-aligned against each other; text columns stay left.
    for (int i = 0; i < m_roles.size(); ++i) {
        if (auto* header = horizontalHeaderItem(i)) {
            header->setTextAlignment(alignmentFor(i) | Qt::AlignVCenter);
        }
    }
}

void TableView::setWidget(int row, int column, QWidget* widget)
{
    // The item carries the widget's size hint. QHeaderView and our own
    // measurement both look at items; without this a column holding only
    // widgets measures as empty and collapses.
    auto* item = new QTableWidgetItem();
    item->setSizeHint(widget->sizeHint());
    setItem(row, column, item);
    setCellWidget(row, column, widget);
}

void TableView::setWidgets(int row, int column, const QVector<QWidget*>& widgets)
{
    auto* container = new QWidget(this);
    // Transparent: a plain container would paint over the row background.
    container->setObjectName(QStringLiteral("NovuTableCell"));
    container->setStyleSheet(
        QStringLiteral("QWidget#NovuTableCell { background: transparent; border: none; }"));

    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(metrics::paddingMedium, metrics::paddingSmall,
                               metrics::paddingMedium, metrics::paddingSmall);
    layout->setSpacing(metrics::spacingSmall);
    layout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    for (QWidget* widget : widgets) {
        layout->addWidget(widget);
    }
    layout->activate();

    setWidget(row, column, container);
}

void TableView::setText(int row, int column, const QString& text, const QString& tooltip)
{
    auto* item = new QTableWidgetItem(text.isEmpty() ? QStringLiteral("—") : text);
    const QString tip = tooltip.isEmpty() ? text : tooltip;
    if (!tip.isEmpty()) {
        item->setToolTip(tip);
    }
    item->setTextAlignment(alignmentFor(column) | Qt::AlignVCenter);
    setItem(row, column, item);
}

// A column's alignment follows from its role, so a page never states it and
// two tables cannot disagree about how a price is aligned.
Qt::Alignment TableView::alignmentFor(int column) const
{
    const bool numeric = column >= 0 && column < m_roles.size()
        && m_roles.at(column) == ColumnRole::Content;
    return numeric ? Qt::AlignRight : Qt::AlignLeft;
}

int TableView::measureColumn(int column) const
{
    // Step 1 + 2: widest of the header caption and every cell, plus padding.
    const QFontMetrics headerMetrics(horizontalHeader()->font());
    const QTableWidgetItem* header = horizontalHeaderItem(column);
    int width = header ? headerMetrics.horizontalAdvance(header->text()) : 0;

    for (int row = 0; row < rowCount(); ++row) {
        if (QWidget* widget = cellWidget(row, column)) {
            width = qMax(width, widget->sizeHint().width());
            continue;
        }
        const QTableWidgetItem* item = this->item(row, column);
        if (!item) {
            continue;
        }
        const QVariant fontData = item->data(Qt::FontRole);
        const QFontMetrics metrics(fontData.isValid() ? fontData.value<QFont>() : font());
        width = qMax(width, metrics.horizontalAdvance(item->text()));
    }
    return width + kColumnPadding;
}

void TableView::relayoutColumns()
{
    const int columns = columnCount();
    if (columns <= 0 || m_roles.size() != columns || m_inRelayout) {
        return;
    }
    // Assigning widths can toggle the scrollbar, which resizes the viewport
    // and would re-enter through resizeEvent().
    m_inRelayout = true;

    QVector<int> widths(columns);
    int contentTotal = 0;
    int textMeasured = 0;
    int lastTextColumn = -1;

    for (int column = 0; column < columns; ++column) {
        widths[column] = measureColumn(column);
        if (m_roles[column] == ColumnRole::Text) {
            textMeasured += widths[column];
            lastTextColumn = column;
        } else {
            contentTotal += widths[column];
        }
    }

    // Step 4: leftover width goes to Text columns only.
    const int remaining = viewport()->width() - contentTotal - textMeasured;
    if (remaining > 0 && textMeasured > 0) {
        int handedOut = 0;
        for (int column = 0; column < columns; ++column) {
            if (m_roles[column] != ColumnRole::Text) {
                continue;
            }
            const int share = (column == lastTextColumn)
                ? remaining - handedOut
                : static_cast<int>(static_cast<qint64>(remaining) * widths[column] / textMeasured);
            handedOut += share;
            widths[column] += share;
        }
    }

    for (int column = 0; column < columns; ++column) {
        setColumnWidth(column, widths[column]);
    }

    m_inRelayout = false;
}

void TableView::resizeEvent(QResizeEvent* event)
{
    QTableWidget::resizeEvent(event);
    relayoutColumns();
}

} // namespace novu::ui
