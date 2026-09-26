#include "novu/ui/ItemList.h"

#include "novu/ui/Separator.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

namespace novu::ui {

namespace {

// `.hrow{padding:9px 16px;gap:10px}`.
constexpr int kRowPaddingH = 16;
constexpr int kRowPaddingV = 9;
constexpr int kRowGap = 10;
// `.hrow .hn .s{margin-top:2px}`.
constexpr int kSubGap = 2;

// The row's own three type steps, none of which is a role: `.hn` is a step
// under the body, `.s` two steps under it, and `.hv` is the monospace face at
// the count size. They are stated here so six lists cannot disagree.
QFont nameFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::Body);
    font.setPixelSize(profile.type.md);
    return font;
}

QFont subFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::Caption);
    font.setPixelSize(qMax(8, profile.type.xxs));
    return font;
}

QFont valueFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleMonoFont(context, TypeRole::Mono);
    font.setPixelSize(qMax(8, profile.type.xs));
    return font;
}

// A row is a plain QWidget: it has no signals of its own and nothing to
// paint. `.hrow.clickable:hover{background:var(--panel)}` is a palette fill
// toggled by the list, which is one property rather than a subclass - and a
// subclass in an anonymous namespace could not be qobject_cast back out of
// the QWidget* the list stores anyway.
QWidget* buildRow(QWidget* parent)
{
    auto* row = new QWidget(parent);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(kRowPaddingH, kRowPaddingV, kRowPaddingH, kRowPaddingV);
    layout->setSpacing(kRowGap);
    return row;
}

QHBoxLayout* rowLayout(QWidget* row)
{
    return row ? qobject_cast<QHBoxLayout*>(row->layout()) : nullptr;
}

void setRowHot(QWidget* row, bool hot)
{
    if (!row) {
        return;
    }
    if (!hot) {
        row->setAutoFillBackground(false);
        row->update();
        return;
    }
    // `--panel` is the raised surface, which the profile calls panelBgAlt -
    // the step above the card's own ground.
    QPalette palette = row->palette();
    palette.setColor(QPalette::Window, themeFor(row).colors.panelBgAlt);
    row->setPalette(palette);
    row->setAutoFillBackground(true);
    row->update();
}

} // namespace

ItemList::ItemList(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_column = new QVBoxLayout(this);
    m_column->setContentsMargins(0, 0, 0, 0);
    m_column->setSpacing(0);
    setMouseTracking(true);
}

QWidget* ItemList::addRow(const QString& title, const QString& subtitle,
                          const QString& value, Ink valueInk)
{
    QWidget* row = buildRow(this);

    auto* names = new QWidget(row);
    auto* stack = new QVBoxLayout(names);
    stack->setContentsMargins(0, 0, 0, 0);
    stack->setSpacing(kSubGap);

    auto* name = new ElidedLabel(title, names);
    name->setFont(nameFont(this));
    setInk(name, Ink::Secondary);
    stack->addWidget(name);

    if (!subtitle.isEmpty()) {
        auto* sub = new ElidedLabel(subtitle, names);
        sub->setFont(subFont(this));
        setInk(sub, Ink::Muted);
        stack->addWidget(sub);
    }
    // `.hn{flex:1}` — the name takes the width and the value stays at the edge.
    rowLayout(row)->addWidget(names, 1);

    if (!value.isEmpty()) {
        auto* figure = new ElidedLabel(value, row);
        // Prefer the full one-line value at the trailing edge, matching
        // `.hv{white-space:nowrap}`, but retain ElidedLabel's zero minimum so
        // the row can still contract without crossing the card edge.
        figure->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        figure->setFont(valueFont(this));
        setInk(figure, valueInk);
        rowLayout(row)->addWidget(figure, 0, Qt::AlignVCenter);
    }

    addRow(row);
    return row;
}

int ItemList::addRow(QWidget* content)
{
    content->setParent(this);
    content->installEventFilter(this);

    Row entry;
    entry.widget = content;
    entry.rule = new Separator(Qt::Horizontal, this);
    m_column->addWidget(content);
    m_column->addWidget(entry.rule);
    m_rows.append(entry);
    updateRules();
    return m_rows.size() - 1;
}

void ItemList::addTrailing(int index, QWidget* widget)
{
    QHBoxLayout* row = rowLayout(rowAt(index));
    if (!row || !widget) {
        return;
    }
    widget->setParent(rowAt(index));
    row->addWidget(widget, 0, Qt::AlignVCenter);
}

void ItemList::setLeading(int index, QWidget* widget)
{
    QHBoxLayout* row = rowLayout(rowAt(index));
    if (!row || !widget) {
        return;
    }
    widget->setParent(rowAt(index));
    row->insertWidget(0, widget, 0, Qt::AlignTop);
}

void ItemList::setRowClickable(int index, bool clickable)
{
    if (index < 0 || index >= m_rows.size()) {
        return;
    }
    m_rows[index].clickable = clickable;
    m_rows[index].widget->setCursor(clickable ? Qt::PointingHandCursor : Qt::ArrowCursor);
    m_rows[index].widget->setMouseTracking(true);
}

bool ItemList::isRowClickable(int index) const
{
    return index >= 0 && index < m_rows.size() && m_rows.at(index).clickable;
}

QWidget* ItemList::rowAt(int index) const
{
    return index >= 0 && index < m_rows.size() ? m_rows.at(index).widget : nullptr;
}

void ItemList::clear()
{
    for (const Row& row : m_rows) {
        delete row.widget;
        delete row.rule;
    }
    m_rows.clear();
    m_hovered = -1;
    m_pressed = -1;
}

// `.hrow:last-child{border-bottom:none}` — the rule belongs *between* rows,
// and a list that ends in one draws a line across the bottom of its card.
void ItemList::updateRules()
{
    for (int i = 0; i < m_rows.size(); ++i) {
        m_rows[i].rule->setVisible(i != m_rows.size() - 1);
    }
}

bool ItemList::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type()) {
        applyTheme();
    }
    return QWidget::event(event);
}

void ItemList::applyTheme()
{
    for (const Row& row : m_rows) {
        row.widget->update();
    }
    update();
}

bool ItemList::eventFilter(QObject* watched, QEvent* event)
{
    const int index = [this, watched] {
        for (int i = 0; i < m_rows.size(); ++i) {
            if (m_rows.at(i).widget == watched) {
                return i;
            }
        }
        return -1;
    }();
    if (index < 0 || !m_rows.at(index).clickable) {
        return QWidget::eventFilter(watched, event);
    }

    QWidget* row = m_rows.at(index).widget;
    switch (event->type()) {
    case QEvent::Enter:
        setRowHot(row, true);
        m_hovered = index;
        break;
    case QEvent::Leave:
        setRowHot(row, false);
        m_hovered = -1;
        break;
    case QEvent::MouseButtonPress:
        if (static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
            m_pressed = index;
        }
        break;
    case QEvent::MouseButtonRelease:
        // Press and release in the same row, or it is a cancelled gesture -
        // the same rule NavList states for its own rows.
        if (static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
            const bool inside = m_rows.at(index).widget->rect().contains(
                static_cast<QMouseEvent*>(event)->pos());
            if (m_pressed == index && inside) {
                emit activated(index);
            }
            m_pressed = -1;
        }
        break;
    default:
        break;
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace novu::ui
