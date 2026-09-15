#include "novu/ui/Disclosure.h"

#include "novu/ui/Motion.h"
#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/Tooltip.h"

#include <QEvent>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QVariantAnimation>

namespace novu::ui {

namespace {

// `.disclose { padding:8px 11px; border-radius:9px; gap:8px }`
constexpr int kInlinePaddingH = 11;
constexpr int kInlinePaddingV = 8;
constexpr int kInlineGap = 8;
constexpr int kInlineChevron = 12;

// `.sb-bhead { padding:8px 8px 6px 6px; gap:7px; margin-top:8px }`
constexpr int kBlockPaddingLeft = 6;
constexpr int kBlockPaddingRight = 8;
constexpr int kBlockPaddingTop = 8;
constexpr int kBlockPaddingBottom = 6;
constexpr int kBlockGap = 7;
constexpr int kBlockChevron = 11;

// `.sb-add { width:18px; height:18px; border-radius:5px }`
constexpr int kActionSide = 18;
constexpr int kActionGlyph = 13;

// `.sb-bhead .bn` — the count pill.
constexpr int kCountPaddingH = 6;

// `transform .18s cubic-bezier(.3,.8,.3,1)`.
constexpr int kChevronMs = 180;

// Shut is -90 degrees from open in both variants.
constexpr double kShutAngle = -90.0;

// Which part of the header the pointer is in. Not an enum class because the
// value is stored as the -1 "nothing" sentinel the rest of the library uses.
enum Part { PartNone = -1, PartHeader = 0, PartAction = 1 };

QFont headerFont(const QWidget* context, Disclosure::Variant variant)
{
    const ThemeProfile& profile = themeFor(context);
    if (variant == Disclosure::Variant::Block) {
        // The design's `.sb-bhead`: the mono eyebrow, tracked and uppercase.
        // TypeRole::Eyebrow exists for exactly this line.
        QFont font = roleMonoFont(context, TypeRole::Eyebrow);
        font.setPixelSize(qMax(8, profile.type.xxs - 1));
        font.setLetterSpacing(QFont::PercentageSpacing, 114);
        return font;
    }
    QFont font = roleFont(context, TypeRole::Body);
    font.setPixelSize(profile.type.sm);
    return font;
}

QFont countFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleMonoFont(context, TypeRole::Mono);
    font.setPixelSize(qMax(8, profile.type.xxs - 1));
    return font;
}

QFont hintFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::Caption);
    font.setPixelSize(profile.type.xs);
    return font;
}

QString displayTitle(const QString& title, Disclosure::Variant variant)
{
    return variant == Disclosure::Variant::Block ? title.toUpper() : title;
}

} // namespace

Disclosure::Disclosure(const QString& title, QWidget* parent, Variant variant)
    : QWidget(parent)
    , m_title(title)
    , m_variant(variant)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);

    m_body = new QWidget(this);
    auto* layout = new QVBoxLayout(m_body);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(themeFor(this).spacing.xs);
}

void Disclosure::setTitle(const QString& title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    updateGeometry();
    update();
}

void Disclosure::setVariant(Variant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    updateGeometry();
    relayout();
    update();
}

void Disclosure::setOpen(bool open)
{
    if (m_open == open) {
        return;
    }
    m_open = open;
    m_body->setVisible(open);
    animateChevron();
    updateGeometry();
    relayout();
    update();
}

void Disclosure::toggle()
{
    setOpen(!m_open);
    emit toggled(m_open);
}

void Disclosure::setHint(const QString& hint)
{
    if (m_hint == hint) {
        return;
    }
    m_hint = hint;
    update();
}

void Disclosure::setCount(int count)
{
    if (m_count == count) {
        return;
    }
    m_count = count;
    update();
}

void Disclosure::setAction(Glyph glyph, const QString& tooltip)
{
    m_action = glyph;
    m_actionTip = tooltip;
    update();
}

void Disclosure::clearAction()
{
    m_action = Glyph::None;
    m_actionTip.clear();
    update();
}

QVBoxLayout* Disclosure::body() const
{
    return qobject_cast<QVBoxLayout*>(m_body->layout());
}

void Disclosure::setBodyWidget(QWidget* widget)
{
    if (!widget || widget == m_body) {
        return;
    }
    // The old body is deleted rather than orphaned: leaving it parented but
    // unplaced is a widget that still receives theme events and still paints
    // nowhere, which is the hardest kind of leak to see.
    m_body->deleteLater();
    m_body = widget;
    m_body->setParent(this);
    m_body->setVisible(m_open);
    updateGeometry();
    relayout();
}

int Disclosure::headerHeight() const
{
    const QFontMetrics fm(headerFont(this, m_variant));
    if (m_variant == Variant::Block) {
        return qMax(fm.height(), kActionSide) + kBlockPaddingTop + kBlockPaddingBottom;
    }
    return qMax(fm.height(), kInlineChevron) + 2 * kInlinePaddingV;
}

QRect Disclosure::headerRect() const
{
    return QRect(0, 0, width(), headerHeight());
}

QRect Disclosure::actionRect() const
{
    if (m_action == Glyph::None) {
        return QRect();
    }
    const int right = m_variant == Variant::Block ? kBlockPaddingRight : kInlinePaddingH;
    const int top = (headerHeight() - kActionSide) / 2;
    return QRect(width() - right - kActionSide, top, kActionSide, kActionSide);
}

void Disclosure::relayout()
{
    const int top = headerHeight();
    if (m_open) {
        m_body->setGeometry(0, top, width(), qMax(0, height() - top));
    }
}

void Disclosure::animateChevron()
{
    const double target = m_open ? 1.0 : 0.0;
    const int duration = themeFor(this).animation.normalMs > 0 && motion::enabled(this)
        ? kChevronMs
        : 0;
    if (duration == 0) {
        m_chevron = target;
        update();
        return;
    }

    auto* turn = new QVariantAnimation(this);
    turn->setDuration(duration);
    turn->setStartValue(m_chevron);
    turn->setEndValue(target);
    // The design's cubic-bezier(.3,.8,.3,1) - fast away, settling in.
    turn->setEasingCurve(QEasingCurve::OutCubic);
    connect(turn, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        m_chevron = value.toDouble();
        update();
    });
    // One-shot, so Qt's own animation timer drives it. The shared Pulse is for
    // loops; see Motion.h.
    turn->start(QAbstractAnimation::DeleteWhenStopped);
}

StyleSpec Disclosure::styleSpec() const
{
    StyleSpec spec;
    spec.radius = SurfaceRole::Button;
    spec.borderWidth = BorderRole::Input;
    spec.type = m_variant == Variant::Block ? TypeRole::Eyebrow : TypeRole::Body;

    if (m_variant == Variant::Block) {
        // No outline at all, and the hover is the sidebar's own wash rather
        // than a surface change - `.sb-bhead:hover { background:rgba(127,140,160,.09) }`.
        spec.background = token(ColorRole::Transparent);
        spec.border = token(ColorRole::Transparent);
        spec.foreground = token(ColorRole::TextDisabled);
        spec.hoverBackground = token(ColorRole::SurfaceHover, 0.55);
        spec.hoverForeground = token(ColorRole::TextSecondary);
        spec.hoverBorder = token(ColorRole::Transparent);
    } else {
        spec.background = token(ColorRole::Transparent);
        spec.border = token(ColorRole::Border);
        spec.foreground = token(ColorRole::TextSecondary);
        spec.hoverBackground = token(ColorRole::Transparent);
        spec.hoverForeground = token(ColorRole::TextPrimary);
        spec.hoverBorder = token(ColorRole::TextDisabled);
    }
    return spec;
}

ResolvedStyle Disclosure::resolvedStyle() const
{
    return resolve(styleSpec(), this, headerHeight());
}

QSize Disclosure::sizeHint() const
{
    const QFontMetrics fm(headerFont(this, m_variant));
    const int padding = m_variant == Variant::Block
        ? kBlockPaddingLeft + kBlockPaddingRight
        : 2 * kInlinePaddingH;
    const int chevron = m_variant == Variant::Block ? kBlockChevron : kInlineChevron;

    int width = padding + chevron + kInlineGap
        + fm.horizontalAdvance(displayTitle(m_title, m_variant));
    if (m_count >= 0) {
        width += kBlockGap + QFontMetrics(countFont(this)).horizontalAdvance(
                                 QString::number(m_count)) + 2 * kCountPaddingH;
    }
    if (!m_hint.isEmpty()) {
        width += kInlineGap + QFontMetrics(hintFont(this)).horizontalAdvance(m_hint);
    }
    if (m_action != Glyph::None) {
        width += kBlockGap + kActionSide;
    }

    int height = headerHeight();
    if (m_open && m_body) {
        height += m_body->sizeHint().height();
    }
    return QSize(width, height);
}

QSize Disclosure::minimumSizeHint() const
{
    int height = headerHeight();
    if (m_open && m_body) {
        height += m_body->minimumSizeHint().height();
    }
    return QSize(80, height);
}

void Disclosure::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    relayout();
}

bool Disclosure::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        updateGeometry();
        relayout();
        update();
    }
    return QWidget::event(event);
}

void Disclosure::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || !headerRect().contains(event->pos())) {
        QWidget::mousePressEvent(event);
        return;
    }
    // The action is tested first. The design requires that creating something
    // inside a block must not collapse the block.
    m_pressed = actionRect().contains(event->pos()) ? PartAction : PartHeader;
    update();
}

void Disclosure::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || m_pressed == PartNone) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    const int pressed = m_pressed;
    m_pressed = PartNone;

    // Press and release must land in the same part. A release somewhere else
    // is a cancelled gesture, not a click.
    const int released = !headerRect().contains(event->pos())
        ? PartNone
        : (actionRect().contains(event->pos()) ? PartAction : PartHeader);

    if (released == pressed) {
        if (pressed == PartAction) {
            emit actionTriggered();
        } else {
            toggle();
        }
    }
    update();
}

void Disclosure::mouseMoveEvent(QMouseEvent* event)
{
    const bool inHeader = headerRect().contains(event->pos());
    const bool inAction = actionRect().contains(event->pos());
    if (inHeader != m_hovered || inAction != m_actionHovered) {
        m_hovered = inHeader;
        // The action is a painted region, not a child, so the tip is swapped
        // onto this widget as the pointer crosses into it and taken away
        // again on the way out. Without this the caller's tooltip for "add a
        // workspace" would appear over the whole caption row.
        if (inAction != m_actionHovered && !m_actionTip.isEmpty()) {
            if (inAction) {
                Tooltip::install(this, m_actionTip);
            } else {
                Tooltip::remove(this);
            }
        }
        m_actionHovered = inAction;
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void Disclosure::leaveEvent(QEvent* event)
{
    m_hovered = false;
    m_actionHovered = false;
    update();
    QWidget::leaveEvent(event);
}

void Disclosure::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Space:
    case Qt::Key_Return:
    case Qt::Key_Enter:
        toggle();
        return;
    case Qt::Key_Left:
        if (m_open) {
            toggle();
            return;
        }
        break;
    case Qt::Key_Right:
        if (!m_open) {
            toggle();
            return;
        }
        break;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void Disclosure::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const ResolvedStyle style = resolvedStyle();
    const bool hot = m_hovered && !m_actionHovered;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRect header = headerRect();
    const int chevronSide = m_variant == Variant::Block ? kBlockChevron : kInlineChevron;
    const int left = m_variant == Variant::Block ? kBlockPaddingLeft : kInlinePaddingH;
    const int right = m_variant == Variant::Block ? kBlockPaddingRight : kInlinePaddingH;

    // ── the plate ────────────────────────────────────────────────────────
    const QColor background = hot && style.hoverBackground.alpha() > 0
        ? style.hoverBackground
        : style.background;
    const QColor outline = hot && style.hoverBorder.alpha() > 0
        ? style.hoverBorder
        : style.border;

    if (background.alpha() > 0 || outline.alpha() > 0) {
        const double radius = m_variant == Variant::Block
            ? profile.radii.small + 3
            : style.radius;
        const QRectF plate = QRectF(header).adjusted(0.5, 0.5, -0.5, -0.5);
        painter.setPen(outline.alpha() > 0 ? QPen(outline, style.borderWidth)
                                           : QPen(Qt::NoPen));
        painter.setBrush(background.alpha() > 0 ? QBrush(background) : QBrush(Qt::NoBrush));
        painter.drawRoundedRect(plate, radius, radius);
    }

    const QColor ink = hot ? style.hoverForeground : style.foreground;

    // ── the chevron ──────────────────────────────────────────────────────
    //
    // One drawing at an angle rather than two glyphs. paintGlyph() takes the
    // rotation, so there is nothing to keep in step between an open chevron and
    // a shut one.
    const QRectF chevron(left, (header.height() - chevronSide) / 2.0,
                         chevronSide, chevronSide);
    paintGlyph(&painter, Glyph::ChevronDown, chevron, ink,
               kShutAngle * (1.0 - m_chevron));

    int x = static_cast<int>(chevron.right()) + kInlineGap;
    int rightEdge = width() - right;

    // ── the action ───────────────────────────────────────────────────────
    if (m_action != Glyph::None) {
        const QRect box = actionRect();
        if (m_actionHovered) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(withAlpha(profile.colors.accent, 0.16));
            painter.drawRoundedRect(QRectF(box), profile.radii.small + 1,
                                    profile.radii.small + 1);
        }
        const double inset = (kActionSide - kActionGlyph) / 2.0;
        paintGlyph(&painter, m_action, QRectF(box).adjusted(inset, inset, -inset, -inset),
                   m_actionHovered ? profile.colors.accentLight : profile.colors.textDisabled);
        rightEdge = box.left() - kBlockGap;
    }

    // ── the count ────────────────────────────────────────────────────────
    if (m_count >= 0) {
        const QFont font = countFont(this);
        const QFontMetrics fm(font);
        const QString text = QString::number(m_count);
        const int pillWidth = fm.horizontalAdvance(text) + 2 * kCountPaddingH;
        const int pillHeight = fm.height() + 2;
        const QRectF pill(rightEdge - pillWidth, (header.height() - pillHeight) / 2.0,
                          pillWidth, pillHeight);
        painter.setPen(QPen(profile.colors.border, 1));
        painter.setBrush(profile.colors.windowBg);
        painter.drawRoundedRect(pill, pillHeight / 2.0, pillHeight / 2.0);
        painter.setFont(font);
        painter.setPen(profile.colors.textDisabled);
        painter.drawText(pill, Qt::AlignCenter, text);
        rightEdge = static_cast<int>(pill.left()) - kBlockGap;
    }

    // ── the hint ─────────────────────────────────────────────────────────
    if (!m_hint.isEmpty()) {
        const QFont font = hintFont(this);
        const QFontMetrics fm(font);
        const int hintWidth = fm.horizontalAdvance(m_hint);
        painter.setFont(font);
        painter.setPen(profile.colors.textDisabled);
        painter.drawText(QRect(rightEdge - hintWidth, 0, hintWidth, header.height()),
                         Qt::AlignVCenter | Qt::AlignRight, m_hint);
        rightEdge -= hintWidth + kInlineGap;
    }

    // ── the title ────────────────────────────────────────────────────────
    const QFont font = headerFont(this, m_variant);
    const QFontMetrics fm(font);
    painter.setFont(font);
    painter.setPen(ink);
    const QRect textBox(x, 0, qMax(0, rightEdge - x), header.height());
    painter.drawText(textBox, Qt::AlignVCenter | Qt::AlignLeft,
                     fm.elidedText(displayTitle(m_title, m_variant), Qt::ElideRight,
                                   textBox.width()));

    // The focus ring, drawn last so it is never under the plate.
    if (hasFocus()) {
        painter.setPen(QPen(profile.colors.accent, profile.border(BorderRole::Focus)));
        painter.setBrush(Qt::NoBrush);
        const double radius = m_variant == Variant::Block ? profile.radii.small + 3
                                                          : style.radius;
        painter.drawRoundedRect(QRectF(header).adjusted(1.0, 1.0, -1.0, -1.0), radius, radius);
    }
}

} // namespace novu::ui
