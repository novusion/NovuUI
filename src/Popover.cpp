#include "novu/ui/Popover.h"

#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QScreen>

#include <utility>

namespace novu::ui {

namespace {

// `.popover { min-width:230px; border-radius:12px; padding:6px }`.
constexpr int kMinWidth = 230;
constexpr int kPadding = 6;

// `.pitem { padding:7px 10px; gap:9px; border-radius:8px }`.
constexpr int kItemPaddingH = 10;
constexpr int kItemPaddingV = 7;
constexpr int kItemGap = 9;
constexpr int kItemRadius = 8;
constexpr int kItemGlyph = 14;

// `.popover .pgrp { padding:8px 10px 5px }`.
constexpr int kGroupTop = 8;
constexpr int kGroupBottom = 5;

// `.psep { height:1px; margin:5px 4px }`.
constexpr int kSeparatorMargin = 5;
constexpr int kSeparatorInset = 4;

constexpr double kDisabledOpacity = 0.42;

QFont itemFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::Body);
    font.setPixelSize(profile.type.sm);
    return font;
}

QFont groupFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleMonoFont(context, TypeRole::Eyebrow);
    font.setPixelSize(qMax(8, profile.type.xxs - 1));
    font.setLetterSpacing(QFont::PercentageSpacing, 112);
    return font;
}

QFont metaFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleMonoFont(context, TypeRole::Mono);
    font.setPixelSize(qMax(8, profile.type.xxs));
    return font;
}

} // namespace

Popover::Popover(QWidget* parent)
    : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
{
    // Qt::Popup is what closes the menu when the user clicks elsewhere and what
    // gives it the mouse grab. Without it a "menu" is a window that stays open
    // behind whatever the user does next.
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

int Popover::addItem(const QString& key, const QString& label, Glyph glyph)
{
    Entry entry;
    entry.kind = Kind::Item;
    entry.key = key;
    entry.label = label;
    entry.glyph = glyph;
    m_items.append(entry);
    updateGeometry();
    update();
    return m_items.size() - 1;
}

int Popover::addGroup(const QString& title)
{
    Entry entry;
    entry.kind = Kind::Group;
    entry.label = title;
    m_items.append(entry);
    updateGeometry();
    update();
    return m_items.size() - 1;
}

int Popover::addSeparator()
{
    Entry entry;
    entry.kind = Kind::Separator;
    m_items.append(entry);
    updateGeometry();
    update();
    return m_items.size() - 1;
}

void Popover::clear()
{
    m_items.clear();
    m_hovered = -1;
    m_pressed = -1;
    updateGeometry();
    update();
}

int Popover::indexOfKey(const QString& key) const
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).kind == Kind::Item && m_items.at(i).key == key) {
            return i;
        }
    }
    return -1;
}

const Popover::Entry* Popover::find(const QString& key) const
{
    const int index = indexOfKey(key);
    return index < 0 ? nullptr : &m_items.at(index);
}

Popover::Entry* Popover::find(const QString& key)
{
    return const_cast<Entry*>(std::as_const(*this).find(key));
}

void Popover::setMeta(const QString& key, const QString& meta)
{
    Entry* entry = find(key);
    if (!entry || entry->meta == meta) {
        return;
    }
    entry->meta = meta;
    updateGeometry();
    update();
}

void Popover::setChecked(const QString& key, bool checked)
{
    Entry* entry = find(key);
    if (!entry || entry->checked == checked) {
        return;
    }
    entry->checked = checked;
    update();
}

bool Popover::isChecked(const QString& key) const
{
    const Entry* entry = find(key);
    return entry && entry->checked;
}

void Popover::setDangerous(const QString& key, bool dangerous)
{
    Entry* entry = find(key);
    if (!entry || entry->dangerous == dangerous) {
        return;
    }
    entry->dangerous = dangerous;
    update();
}

void Popover::setItemEnabled(const QString& key, bool enabled)
{
    Entry* entry = find(key);
    if (!entry || entry->enabled == enabled) {
        return;
    }
    entry->enabled = enabled;
    update();
}

int Popover::entryHeight(const Entry& entry) const
{
    switch (entry.kind) {
    case Kind::Separator:
        return 1 + 2 * kSeparatorMargin;
    case Kind::Group:
        return QFontMetrics(groupFont(this)).height() + kGroupTop + kGroupBottom;
    case Kind::Item:
        break;
    }
    return qMax(QFontMetrics(itemFont(this)).height(), kItemGlyph) + 2 * kItemPaddingV;
}

QRect Popover::itemRect(int index) const
{
    if (index < 0 || index >= m_items.size()) {
        return QRect();
    }
    int y = kPadding;
    for (int i = 0; i < index; ++i) {
        y += entryHeight(m_items.at(i));
    }
    return QRect(kPadding, y, qMax(0, width() - 2 * kPadding),
                 entryHeight(m_items.at(index)));
}

int Popover::indexAt(const QPoint& position) const
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).kind == Kind::Item && itemRect(i).contains(position)) {
            return i;
        }
    }
    return -1;
}

int Popover::neighbour(int from, int step) const
{
    for (int i = from + step; i >= 0 && i < m_items.size(); i += step) {
        if (m_items.at(i).kind == Kind::Item && m_items.at(i).enabled) {
            return i;
        }
    }
    return -1;
}

QSize Popover::sizeHint() const
{
    const QFontMetrics items(itemFont(this));
    const QFontMetrics metas(metaFont(this));
    const QFontMetrics groups(groupFont(this));

    int height = 2 * kPadding;
    int width = 0;
    for (const Entry& entry : m_items) {
        height += entryHeight(entry);
        switch (entry.kind) {
        case Kind::Separator:
            break;
        case Kind::Group:
            width = qMax(width, groups.horizontalAdvance(entry.label.toUpper())
                                    + 2 * kItemPaddingH);
            break;
        case Kind::Item: {
            int own = 2 * kItemPaddingH + items.horizontalAdvance(entry.label);
            if (entry.glyph != Glyph::None) {
                own += kItemGlyph + kItemGap;
            }
            if (!entry.meta.isEmpty()) {
                own += kItemGap * 2 + metas.horizontalAdvance(entry.meta);
            }
            width = qMax(width, own);
            break;
        }
        }
    }
    return QSize(qMax(kMinWidth, width + 2 * kPadding), height);
}

void Popover::popupAt(const QPoint& globalPosition)
{
    const QSize size = sizeHint();
    setFixedSize(size);

    const QScreen* screen = QGuiApplication::screenAt(globalPosition);
    const QRect bounds = (screen ? screen : QGuiApplication::primaryScreen())
                             ->availableGeometry();

    int x = qBound(bounds.left() + 4, globalPosition.x(),
                   qMax(bounds.left() + 4, bounds.right() - size.width() - 4));
    int y = globalPosition.y();
    if (y + size.height() > bounds.bottom() - 4) {
        // No room below: flip above the point rather than pushing the menu up
        // over the thing that opened it.
        y = qMax(bounds.top() + 4, globalPosition.y() - size.height());
    }

    move(x, y);
    m_hovered = -1;
    m_pressed = -1;
    show();
    raise();
    setFocus(Qt::PopupFocusReason);
}

void Popover::popupUnder(QWidget* anchor)
{
    if (!anchor) {
        return;
    }
    m_anchor = anchor;
    const QRect box(anchor->mapToGlobal(QPoint(0, 0)), anchor->size());

    const QSize size = sizeHint();
    setFixedSize(size);

    const QScreen* screen = anchor->screen();
    const QRect bounds = (screen ? screen : QGuiApplication::primaryScreen())
                             ->availableGeometry();

    int x = qBound(bounds.left() + 4, box.left(),
                   qMax(bounds.left() + 4, bounds.right() - size.width() - 4));
    int y = box.bottom() + 4;
    if (y + size.height() > bounds.bottom() - 4) {
        y = qMax(bounds.top() + 4, box.top() - size.height() - 4);
    }

    move(x, y);
    m_hovered = -1;
    m_pressed = -1;
    show();
    raise();
    setFocus(Qt::PopupFocusReason);
}

void Popover::trigger(int index)
{
    if (index < 0 || index >= m_items.size()) {
        return;
    }
    const Entry& entry = m_items.at(index);
    if (entry.kind != Kind::Item || !entry.enabled) {
        return;
    }
    // Closed before the signal, not after: a handler that opens a dialog would
    // otherwise do so with a menu still on screen above it.
    const QString key = entry.key;
    hide();
    emit triggered(key);
}

void Popover::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    const int index = indexAt(event->position().toPoint());
    m_pressed = (index >= 0 && m_items.at(index).enabled) ? index : -1;
    update();
    event->accept();
}

void Popover::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    const int released = indexAt(event->position().toPoint());
    const int pressed = m_pressed;
    m_pressed = -1;
    if (pressed >= 0 && released == pressed) {
        trigger(released);
        return;
    }
    update();
    event->accept();
}

void Popover::mouseMoveEvent(QMouseEvent* event)
{
    const int index = indexAt(event->position().toPoint());
    const int hovered = (index >= 0 && m_items.at(index).enabled) ? index : -1;
    if (hovered != m_hovered) {
        m_hovered = hovered;
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void Popover::leaveEvent(QEvent* event)
{
    if (m_hovered != -1) {
        m_hovered = -1;
        update();
    }
    QWidget::leaveEvent(event);
}

void Popover::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Down: {
        const int next = neighbour(m_hovered, 1);
        if (next >= 0) {
            m_hovered = next;
            update();
        }
        return;
    }
    case Qt::Key_Up: {
        const int previous = neighbour(m_hovered < 0 ? m_items.size() : m_hovered, -1);
        if (previous >= 0) {
            m_hovered = previous;
            update();
        }
        return;
    }
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space:
        trigger(m_hovered);
        return;
    case Qt::Key_Escape:
        hide();
        return;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void Popover::hideEvent(QHideEvent* event)
{
    m_hovered = -1;
    m_pressed = -1;
    QWidget::hideEvent(event);
}

bool Popover::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        updateGeometry();
        update();
    }
    return QWidget::event(event);
}

StyleSpec Popover::styleSpec() const
{
    StyleSpec spec;
    spec.background = token(ColorRole::PanelBg);
    spec.border = token(ColorRole::Border);
    spec.foreground = token(ColorRole::TextSecondary);
    spec.radius = SurfaceRole::Overlay;
    spec.borderWidth = BorderRole::Card;
    spec.type = TypeRole::Body;
    return spec;
}

StyleSpec Popover::itemStyleSpec(int index) const
{
    StyleSpec spec = styleSpec();
    spec.radius = SurfaceRole::Button;
    spec.background = token(ColorRole::Transparent);
    spec.border = token(ColorRole::Transparent);
    spec.hoverBackground = token(ColorRole::PanelBgAlt);
    spec.hoverForeground = token(ColorRole::TextPrimary);
    spec.checkedForeground = token(ColorRole::AccentLight);
    spec.disabledForeground = token(ColorRole::TextDisabled, kDisabledOpacity);

    if (index >= 0 && index < m_items.size() && m_items.at(index).dangerous) {
        spec.foreground = statusToken(SemanticRole::Danger, ColorRole::StatusText);
        spec.hoverForeground = statusToken(SemanticRole::Danger, ColorRole::StatusText);
        spec.hoverBackground = statusToken(SemanticRole::Danger, ColorRole::StatusFill);
    }
    return spec;
}

ResolvedStyle Popover::resolvedStyle() const
{
    return resolve(styleSpec(), this, height());
}

void Popover::paintEvent(QPaintEvent*)
{
    // Resolved against the anchor where there is one. A popover is a top-level
    // window and so belongs to no ThemeScope; resolving against itself would
    // give a NovuTrade menu the default product's colours.
    const QWidget* context = m_anchor ? m_anchor.data() : static_cast<const QWidget*>(this);
    const ThemeProfile& profile = themeFor(context);
    const ResolvedStyle surface = resolve(styleSpec(), context, height());

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF card = QRectF(rect()).adjusted(surface.borderWidth / 2.0,
                                                surface.borderWidth / 2.0,
                                                -surface.borderWidth / 2.0,
                                                -surface.borderWidth / 2.0);
    painter.setPen(QPen(surface.border, surface.borderWidth));
    painter.setBrush(surface.background);
    painter.drawRoundedRect(card, surface.radius, surface.radius);

    const QFont items = itemFont(context);
    const QFont groups = groupFont(context);
    const QFont metas = metaFont(context);
    const QFontMetrics itemMetrics(items);
    const QFontMetrics metaMetrics(metas);

    for (int index = 0; index < m_items.size(); ++index) {
        const Entry& entry = m_items.at(index);
        const QRect box = itemRect(index);

        if (entry.kind == Kind::Separator) {
            painter.setPen(QPen(profile.colors.borderSubtle, 1));
            painter.drawLine(box.left() + kSeparatorInset, box.center().y(),
                             box.right() - kSeparatorInset, box.center().y());
            continue;
        }

        if (entry.kind == Kind::Group) {
            painter.setFont(groups);
            painter.setPen(profile.colors.textDisabled);
            painter.drawText(box.adjusted(kItemPaddingH, kGroupTop, -kItemPaddingH,
                                          -kGroupBottom),
                             Qt::AlignLeft | Qt::AlignVCenter, entry.label.toUpper());
            continue;
        }

        const ResolvedStyle style = resolve(itemStyleSpec(index), context, box.height());
        const bool hot = index == m_hovered || index == m_pressed;

        if (hot && entry.enabled) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(style.hoverBackground);
            painter.drawRoundedRect(QRectF(box), kItemRadius, kItemRadius);
        }

        QColor ink = entry.checked ? style.checkedForeground
                                   : (hot ? style.hoverForeground : style.foreground);
        if (!entry.enabled) {
            ink = withAlpha(ink, kDisabledOpacity);
        }

        int left = box.left() + kItemPaddingH;
        if (entry.glyph != Glyph::None) {
            paintGlyph(&painter, entry.glyph,
                       QRectF(left, box.center().y() - kItemGlyph / 2.0,
                              kItemGlyph, kItemGlyph),
                       ink);
            left += kItemGlyph + kItemGap;
        }

        int rightEdge = box.right() - kItemPaddingH;
        if (!entry.meta.isEmpty()) {
            const int metaWidth = metaMetrics.horizontalAdvance(entry.meta);
            painter.setFont(metas);
            painter.setPen(entry.enabled ? profile.colors.textDisabled
                                         : withAlpha(profile.colors.textDisabled,
                                                     kDisabledOpacity));
            painter.drawText(QRect(rightEdge - metaWidth, box.top(), metaWidth, box.height()),
                             Qt::AlignRight | Qt::AlignVCenter, entry.meta);
            rightEdge -= metaWidth + kItemGap;
        }

        painter.setFont(items);
        painter.setPen(ink);
        const QRect textBox(left, box.top(), qMax(0, rightEdge - left), box.height());
        painter.drawText(textBox, Qt::AlignLeft | Qt::AlignVCenter,
                         itemMetrics.elidedText(entry.label, Qt::ElideRight, textBox.width()));
    }
}

} // namespace novu::ui
