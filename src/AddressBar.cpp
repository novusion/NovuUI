#include "novu/ui/AddressBar.h"

#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/Tooltip.h"

#include <QEnterEvent>
#include <QEvent>
#include <QFontMetrics>
#include <QPainter>
#include <QPen>
#include <QResizeEvent>

namespace novu::ui {

namespace {

// `.omni{padding:4px 10px;gap:9px}`.
constexpr int kPaddingH = 10;
constexpr int kPaddingV = 4;
constexpr int kGap = 9;

// `.omni` stands as tall as a control of the profile's own medium size: the
// design's address field and the buttons beside it are one row, and a field
// that sized itself to its text alone would be a step shorter than everything
// on that row.

QColor securityInk(const ThemeProfile& profile, AddressBar::Security security)
{
    switch (security) {
    case AddressBar::Security::Secure:
        return profile.semantic.success.text;
    case AddressBar::Security::Failed:
        return profile.semantic.danger.text;
    case AddressBar::Security::Insecure:
        return profile.semantic.warning.text;
    case AddressBar::Security::None:
        break;
    }
    return profile.colors.textDisabled;
}

Glyph securityGlyph(AddressBar::Security security)
{
    switch (security) {
    case AddressBar::Security::Failed:
    case AddressBar::Security::Insecure:
        return Glyph::Warning;
    case AddressBar::Security::Secure:
        return Glyph::Lock;
    case AddressBar::Security::None:
        break;
    }
    return Glyph::None;
}

} // namespace

AddressBar::AddressBar(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAttribute(Qt::WA_Hover, true);
}

void AddressBar::setUrl(const QString& url)
{
    if (m_url == url) {
        return;
    }
    m_url = url;
    // The whole address stays reachable even when the field is too narrow for
    // it, which is the case a browser is in most of the time.
    Tooltip::install(this, url);
    update();
}

void AddressBar::setPlaceholder(const QString& placeholder)
{
    if (m_placeholder == placeholder) {
        return;
    }
    m_placeholder = placeholder;
    update();
}

void AddressBar::setSecurity(Security security)
{
    if (m_security == security) {
        return;
    }
    m_security = security;
    update();
}

void AddressBar::setTrailing(QWidget* trailing)
{
    if (m_trailing == trailing) {
        return;
    }
    delete m_trailing;
    m_trailing = trailing;
    if (m_trailing) {
        m_trailing->setParent(this);
        m_trailing->show();
    }
    updateGeometry();
    placeTrailing();
    update();
}

StyleSpec AddressBar::styleSpec() const
{
    StyleSpec spec;
    // `.omni{background:var(--window)}` - the address sits in a well, a step
    // below the chrome around it, rather than on a raised plate.
    spec.background = token(ColorRole::WindowBg);
    spec.foreground = token(ColorRole::TextSecondary);
    spec.border = token(ColorRole::Border);
    spec.hoverBorder = token(ColorRole::BorderHover);
    spec.radius = SurfaceRole::Input;
    spec.borderWidth = BorderRole::Input;
    spec.type = TypeRole::Mono;
    return spec;
}

ResolvedStyle AddressBar::resolvedStyle() const
{
    return resolve(styleSpec(), this, height());
}

QSize AddressBar::sizeHint() const
{
    const ThemeProfile& profile = themeFor(this);
    int inner = QFontMetrics(roleMonoFont(this, TypeRole::Mono)).height();
    if (m_trailing) {
        inner = qMax(inner, m_trailing->sizeHint().height());
    }
    const int borderWidth = 2 * qMax(1, profile.border(BorderRole::Input));
    return QSize(220, qMax(profile.controlHeight(ControlSize::Medium),
                           inner + 2 * kPaddingV + borderWidth));
}

QSize AddressBar::minimumSizeHint() const
{
    // An address never decides how wide its window is - it is handed whatever
    // is left and elides. A field that sized itself to a long URL would push
    // the buttons beside it off the row.
    return QSize(80, sizeHint().height());
}

QRect AddressBar::textRect() const
{
    const ThemeProfile& profile = themeFor(this);
    int left = kPaddingH;
    if (securityGlyph(m_security) != Glyph::None) {
        left += profile.icons.small + kGap;
    }
    int right = width() - kPaddingH;
    if (m_trailing) {
        right -= m_trailing->width() + kGap;
    }
    return QRect(left, 0, qMax(0, right - left), height());
}

void AddressBar::placeTrailing()
{
    if (!m_trailing) {
        return;
    }
    const QSize hint = m_trailing->sizeHint();
    m_trailing->setGeometry(width() - kPaddingH - hint.width(),
                            (height() - hint.height()) / 2, hint.width(), hint.height());
}

void AddressBar::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    placeTrailing();
}

void AddressBar::enterEvent(QEnterEvent* event)
{
    m_hovered = true;
    update();
    QWidget::enterEvent(event);
}

void AddressBar::leaveEvent(QEvent* event)
{
    m_hovered = false;
    update();
    QWidget::leaveEvent(event);
}

bool AddressBar::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        updateGeometry();
        placeTrailing();
        update();
    }
    return QWidget::event(event);
}

void AddressBar::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const ResolvedStyle style = resolvedStyle();
    const int borderWidth = qMax(1, profile.border(BorderRole::Input));

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF plate = QRectF(rect()).adjusted(borderWidth / 2.0, borderWidth / 2.0,
                                                 -borderWidth / 2.0, -borderWidth / 2.0);
    painter.setPen(QPen(m_hovered ? style.hoverBorder : style.border, borderWidth));
    painter.setBrush(style.background);
    painter.drawRoundedRect(plate, style.radius, style.radius);

    const Glyph mark = securityGlyph(m_security);
    if (mark != Glyph::None) {
        const int side = profile.icons.small;
        paintGlyph(&painter, mark, QRectF(kPaddingH, (height() - side) / 2.0, side, side),
                   securityInk(profile, m_security));
    }

    const QRect box = textRect();
    if (box.width() <= 4) {
        return;
    }

    const QFont mono = roleMonoFont(this, TypeRole::Mono);
    painter.setFont(mono);
    const QFontMetrics fm(mono);

    if (m_url.isEmpty()) {
        painter.setPen(profile.colors.textDisabled);
        painter.drawText(box, Qt::AlignVCenter | Qt::AlignLeft,
                         fm.elidedText(m_placeholder, Qt::ElideRight, box.width()));
        return;
    }

    // `urlMarkup()` - the host, then the rest. Drawn as two runs rather than
    // one string so the host can carry its own weight and ink; see the header
    // for why that is not decoration.
    const int slash = m_url.indexOf(QLatin1Char('/'));
    const QString host = slash < 0 ? m_url : m_url.left(slash);
    const QString path = slash < 0 ? QString() : m_url.mid(slash);

    QFont hostFont = mono;
    hostFont.setWeight(QFont::Medium);
    const QFontMetrics hostMetrics(hostFont);

    const int hostWidth = qMin(hostMetrics.horizontalAdvance(host), box.width());
    painter.setFont(hostFont);
    painter.setPen(profile.colors.textPrimary);
    painter.drawText(QRect(box.left(), box.top(), hostWidth, box.height()),
                     Qt::AlignVCenter | Qt::AlignLeft,
                     hostMetrics.elidedText(host, Qt::ElideRight, hostWidth));

    const int rest = box.width() - hostWidth;
    if (!path.isEmpty() && rest > 4) {
        painter.setFont(mono);
        painter.setPen(style.foreground);
        painter.drawText(QRect(box.left() + hostWidth, box.top(), rest, box.height()),
                         Qt::AlignVCenter | Qt::AlignLeft,
                         fm.elidedText(path, Qt::ElideRight, rest));
    }
}

} // namespace novu::ui
