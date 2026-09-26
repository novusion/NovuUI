#include "novu/ui/Chip.h"

#include "novu/ui/FlowLayout.h"
#include "novu/ui/StyleSheet.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include "novu/ui/ThemeProfile.h"

#include <QFontMetrics>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>

namespace novu::ui {

namespace {

// `.chip{padding:0 6px 0 9px; gap:6px}`, `.chip svg{11x11}`.
constexpr int kTokenPaddingLeft = 9;
constexpr int kTokenPaddingRight = 6;
constexpr int kTokenGap = 6;
constexpr int kTokenMark = 11;

} // namespace

Chip::Chip(const QString& text, QWidget* parent, bool on)
    : QPushButton(text, parent)
{
    const ThemeProfile& profile = themeFor(this);

    setProperty(prop::kind, QStringLiteral("pill"));
    setCheckable(true);
    setChecked(on);
    setCursor(Qt::PointingHandCursor);
    // Caption size, bold. A chip is a compact toggle, not body text: at the
    // body size a strip of nineteen column pills was wider than the card it
    // filters.
    setFont(roleFont(this, TypeRole::Caption, true));

    // Geometry from the component, not the stylesheet: Qt does not fold QSS
    // padding into sizeHint() until a widget is polished, so anything that
    // measures a chip before then reads a width that clips the label.
    applyGeometry();
    Q_UNUSED(profile);
}

// `.chip{height:22px; padding:0 6px 0 9px; gap:6px}`, `.chip svg{11x11}` --
// the token pays for what it carries: 9px in, the glyph, the gap, the label,
// the gap, the x, 6px out.
void Chip::applyGeometry()
{
    const ThemeProfile& profile = themeFor(this);
    const QFontMetrics metrics(tokenFont());

    if(m_tagStyle){setFixedSize(metrics.horizontalAdvance(text())+18,22);return;}
    setFixedHeight(profile.controlHeight(ControlSize::Small) - 2);
    if (!isToken()) {
        setFixedWidth(metrics.horizontalAdvance(text())
                      + profile.space(SpacingRole::ToolbarGap) + 10);
        return;
    }

    int width = kTokenPaddingLeft + metrics.horizontalAdvance(text())
        + kTokenPaddingRight;
    if (m_glyph != Glyph::None) {
        width += kTokenMark + kTokenGap;
    }
    if (m_removable) {
        width += kTokenGap + kTokenMark;
    }
    setFixedWidth(width);
}

void Chip::setGlyph(Glyph glyph)
{
    if (m_glyph == glyph) {
        return;
    }
    m_glyph = glyph;
    if (isToken()) {
        setCheckable(false);
    }
    applyGeometry();
    update();
}

void Chip::setRemovable(bool removable)
{
    if (m_removable == removable) {
        return;
    }
    m_removable = removable;
    if (isToken()) {
        setCheckable(false);
    }
    setMouseTracking(removable);
    applyGeometry();
    update();
}

QFont Chip::tokenFont() const
{
    if(m_tagStyle){auto f=roleFont(this,TypeRole::Body);f.setPointSizeF(themeFor(this).type.xs*72.0/logicalDpiY());f.setWeight(QFont::Normal);return f;}
    return m_monospaced ? roleMonoFont(this, TypeRole::Caption, true) : font();
}

void Chip::setMonospaced(bool monospaced)
{
    if (m_monospaced == monospaced) {
        return;
    }
    m_monospaced = monospaced;
    applyGeometry();
    update();
}

void Chip::setAccented(bool accented)
{
    if (m_accented == accented) {
        return;
    }
    m_accented = accented;
    update();
}

QRect Chip::removeRect() const
{
    if (!m_removable) {
        return QRect();
    }
    return QRect(width() - kTokenPaddingRight - kTokenMark,
                 (height() - kTokenMark) / 2, kTokenMark, kTokenMark);
}

void Chip::paintEvent(QPaintEvent* event)
{
    if(m_tagStyle){
        const auto& profile=themeFor(this);const auto& c=profile.colors;QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(m_accented?withAlpha(c.accent,.45):underMouse()?c.borderSubtle:c.borderHover,1));
        p.setBrush(m_accented?QBrush(withAlpha(c.accent,.10)):underMouse()?QBrush(c.panelBgAlt):QBrush(Qt::NoBrush));
        p.drawRoundedRect(QRectF(rect()).adjusted(.5,.5,-.5,-.5),height()/2.0,height()/2.0);
        p.setFont(tokenFont());p.setPen(m_accented?c.accent:underMouse()?c.textPrimary:c.textTertiary);
        p.drawText(rect(),Qt::AlignCenter,text());return;
    }
    // A plain chip is still the stylesheet's: painting it here would be a
    // second answer to a question that already has one.
    if (!isToken()) {
        QPushButton::paintEvent(event);
        return;
    }

    const ThemeProfile& profile = themeFor(this);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF box = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const double radius = box.height() / 2.0;
    painter.setPen(QPen(m_accented ? withAlpha(profile.colors.accent, 0.42)
                                   : profile.colors.borderHover, 1));
    painter.setBrush(m_accented ? withAlpha(profile.colors.accent, 0.10)
                                : profile.colors.panelBg);
    painter.drawRoundedRect(box, radius, radius);

    const QColor ink = m_accented ? profile.colors.accent
                                  : profile.colors.textSecondary;
    int x = kTokenPaddingLeft;
    if (m_glyph != Glyph::None) {
        paintGlyph(&painter, m_glyph,
                   QRectF(x, (height() - kTokenMark) / 2.0, kTokenMark, kTokenMark),
                   m_accented ? ink : profile.colors.textDisabled);
        x += kTokenMark + kTokenGap;
    }

    const QRect remove = removeRect();
    const int right = m_removable ? remove.left() - kTokenGap
                                  : width() - kTokenPaddingRight;
    painter.setPen(ink);
    painter.setFont(tokenFont());
    painter.drawText(QRect(x, 0, qMax(0, right - x), height()),
                     Qt::AlignVCenter | Qt::AlignLeft,
                     painter.fontMetrics().elidedText(text(), Qt::ElideRight,
                                                      qMax(0, right - x)));

    if (m_removable) {
        // `.chip .x:hover{color:var(--pink)}` — taking something off is the
        // one destructive thing a chip does, and it says so on hover.
        paintGlyph(&painter, Glyph::Close, QRectF(remove).adjusted(1, 1, -1, -1),
                   m_removeHovered ? profile.semantic.danger.text
                                   : profile.colors.textDisabled);
    }
}

void Chip::mousePressEvent(QMouseEvent* event)
{
    if (m_removable && removeRect().contains(event->pos())) {
        m_removePressed = true;
        update();
        return;
    }
    QPushButton::mousePressEvent(event);
}

void Chip::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_removePressed) {
        m_removePressed = false;
        if (removeRect().contains(event->pos())) {
            emit removed();
        }
        update();
        return;
    }
    QPushButton::mouseReleaseEvent(event);
}

void Chip::mouseMoveEvent(QMouseEvent* event)
{
    if (m_removable) {
        const bool hovered = removeRect().contains(event->pos());
        if (hovered != m_removeHovered) {
            m_removeHovered = hovered;
            update();
        }
    }
    QPushButton::mouseMoveEvent(event);
}

void Chip::leaveEvent(QEvent* event)
{
    if (m_removeHovered) {
        m_removeHovered = false;
        update();
    }
    QPushButton::leaveEvent(event);
}

bool Chip::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type()) {
        setFont(roleFont(this, TypeRole::Caption, true));
        applyGeometry();
        update();
    }
    return QPushButton::event(event);
}

ChipBar::ChipBar(const QStringList& items, QWidget* parent,
                 const QStringList& initiallyOn, const QString& captionText)
    : QWidget(parent)
    , m_items(items)
{
    const ThemeProfile& profile = themeFor(this);

    auto* flow = new FlowLayout(this, profile.space(SpacingRole::ToolbarGap) - 2);
    const bool allOn = initiallyOn.isEmpty();

    if (!captionText.isEmpty()) {
        flow->addWidget(caption(captionText, this));
    }

    for (const QString& item : items) {
        auto* chip = new Chip(item, this, allOn || initiallyOn.contains(item));
        connect(chip, &QPushButton::clicked, this, &ChipBar::changed);
        flow->addWidget(chip);
        m_chips.append(chip);
    }
}

QStringList ChipBar::selected() const
{
    QStringList result;
    for (Chip* chip : m_chips) {
        if (chip->isChecked()) {
            result.append(chip->text());
        }
    }
    return result;
}

void ChipBar::setSelected(const QStringList& items)
{
    for (Chip* chip : m_chips) {
        const QSignalBlocker blocker(chip);
        chip->setChecked(items.contains(chip->text()));
    }
    emit changed();
}

} // namespace novu::ui
