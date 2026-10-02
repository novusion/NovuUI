#include "novu/ui/Button.h"

#include "novu/ui/Tooltip.h"

#include "novu/ui/Motion.h"
#include "novu/ui/StyleSheet.h"
#include "novu/ui/StyleSpec.h"
#include "novu/ui/ThemeManager.h"

#include <QConicalGradient>
#include <QEvent>
#include <QFontMetrics>
#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QPaintEvent>

namespace novu::ui {

namespace {

// Horizontal padding comes from the product profile, alongside height.
// Between a leading glyph and the caption. The design's `gap`.
constexpr int kIconGap = 7;

// The travelling ring on a Primary button: `.btn.primary::before`, a 1.5px
// conic gradient turning once every `animation.ringMs`.
//
// The stops are the design's, converted once: CSS measures a conic gradient
// clockwise from twelve o'clock, QConicalGradient counter-clockwise from three,
// and a negative start angle is what reconciles the two. Almost all of the ring
// is the accent at 12% - a hairline - with one bright arc, white at its peak,
// sweeping round it.
constexpr double kRingWidth = 1.5;
constexpr double kRingFaintAlpha = 0.12;
constexpr double kRingBrightAlpha = 0.90;
constexpr double kRingFaintEnd = 258.0 / 360.0;
constexpr double kRingBrightStart = 322.0 / 360.0;
constexpr double kRingPeak = 344.0 / 360.0;

int paddingFor(const QWidget* widget,ControlSize size)
{
    const auto& c=themeFor(widget).controls;
    return size==ControlSize::Small?c.buttonPaddingSmall:size==ControlSize::Large?c.buttonPaddingLarge:c.buttonPadding;
}

} // namespace

Button::Button(QWidget* parent)
    : QPushButton(parent)
{
    // The button is exactly its sizeHint(); nothing may compress it.
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);
    connectToClock();
    applyVariant();
}

Button::Button(const QString& text, Variant variant, QWidget* parent)
    : QPushButton(text, parent)
    , m_variant(variant)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);
    connectToClock();
    applyVariant();
}

void Button::setVariant(Variant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    applyVariant();
}

void Button::setControlSize(ControlSize size)
{
    if (m_size == size) {
        return;
    }
    m_size = size;
    applyVariant();
}

void Button::setGlyph(Glyph glyph)
{
    if (m_glyph == glyph) {
        return;
    }
    m_glyph = glyph;
    applyVariant();
}

void Button::setTrailingGlyph(Glyph glyph)
{
    if (m_trailingGlyph == glyph) {
        return;
    }
    m_trailingGlyph = glyph;
    updateGeometry();
    update();
}

void Button::setBusy(bool busy)
{
    if (m_busy == busy) {
        return;
    }
    m_busy = busy;
    applyVariant();
}

// The ring repaints itself, because Pulse::subscribe() already calls update().
// The spinner cannot: its angle lives in a QIcon, which has to be replaced. So
// this one connection exists for the spinner, and costs a bool test per tick on
// every other button - and the clock is not running at all unless something on
// screen is animating.
void Button::connectToClock()
{
    connect(&Pulse::instance(), &Pulse::tick, this, [this] {
        if (m_busy) {
            applyGlyph();
        }
    });
}

// Subscribes to the shared clock only while this button actually has something
// turning, and drops out again when it does not. A window full of Secondary
// buttons keeps the timer stopped.
void Button::updateMotion()
{
    const bool wants = (m_variant == Variant::Primary || m_busy)
        && isEnabled() && motion::enabled(this);
    if (wants == m_animating) {
        return;
    }
    m_animating = wants;
    if (wants) {
        Pulse::instance().subscribe(this);
    } else {
        Pulse::instance().release(this);
    }
}

QSize Button::sizeHint() const
{
    // Derived from this button's own font and its own padding rather than from
    // the style sheet, because QSS padding is not reflected in
    // QPushButton::sizeHint() until the widget has been polished. Depending on
    // that timing means a caption can be painted clipped.
    const ThemeProfile& profile = themeFor(this);
    const QFontMetrics fm(font());
    const int padding = paddingFor(this,m_size);
    // The glyph and the gap after it are part of the button, so they are part
    // of the measurement. Left to Qt's own icon handling the caption would be
    // measured and the icon would not, and a captioned icon button would come
    // out one glyph too narrow.
    const int lead = (m_glyph == Glyph::None && !m_statusDot)
        ? 0
        : glyphSizeFor(this, m_size) + kIconGap;
    // The trailing glyph is measured the same way. QPushButton knows about
    // one icon and it is the leading one, so without this the chevron would
    // be painted over the last letter of the caption.
    const int trail = m_trailingGlyph == Glyph::None
        ? 0
        : glyphSizeFor(this, m_size) + kIconGap;
    return QSize(fm.horizontalAdvance(text()) + lead + trail + 2 * padding + (profile.controls.buttonPaddingIncludesBorder?0:2*profile.border(BorderRole::Input)),
                 qMax(profile.controlHeight(m_size), fm.height() + 8));
}

bool Button::event(QEvent* event)
{
    // The profile is resolved from the widget tree, and at construction the
    // tree may not carry a scope yet — a button built before its page is
    // parented would otherwise keep the default product's palette forever.
    if (event->type() == ThemeChangeEvent::type()
        || event->type() == QEvent::PolishRequest
        || event->type() == QEvent::ParentChange
        // A disabled button does not animate: the design dims it to 42% and
        // nothing about a control the user cannot press should be moving.
        || event->type() == QEvent::EnabledChange) {
        applyVariant();
    }
    return QPushButton::event(event);
}

StyleSpec Button::styleSpec() const
{
    // The variant table, as references. Nothing below is a colour, so a profile
    // change is a repaint rather than a rebuild, and a test can read the
    // contract back without photographing the button.
    StyleSpec spec;
    spec.radius = SurfaceRole::Button;
    spec.borderWidth = BorderRole::Input;
    spec.disabledForeground = token(ColorRole::TextDisabled);
    spec.disabledBorder = token(ColorRole::BorderSubtle);

    // A row action drops one step on the type scale and one on the radius
    // scale, which is what the design draws for `.btn.sm`. Both are said as
    // steps, never as pixels.
    const bool small = m_size == ControlSize::Small;
    spec.type = small ? TypeRole::Caption : TypeRole::Body;
    // `.btn{font-weight:500}` - a button's caption is set one step above body
    // text in every approved design. Primary states 600 below; the status
    // variants keep this one.
    spec.weight = QFont::Medium;
    if (small) {
        spec.radiusStepOverride = static_cast<int>(themeFor(this).controls.smallButtonRadius);
    }

    switch (m_variant) {
    case Variant::Primary:
        // Solid accent. The border matches the fill so a Primary and a Ghost
        // button with the same caption stay the same size.
        spec.background = token(ColorRole::Accent);
        spec.foreground = token(ColorRole::TextOnAccent);
        spec.border = token(ColorRole::Accent);
        spec.hoverBackground = token(ColorRole::AccentLight);
        spec.hoverBorder = token(ColorRole::AccentLight);
        // `.btn.primary{font-weight:600}` - demibold, which is where this
        // design system's type scale ends; 700 is a weight none of the
        // mockups draw.
        spec.weight = QFont::DemiBold;
        break;

    case Variant::Accent:
        spec.background = token(ColorRole::Accent, 0.08);
        spec.foreground = token(ColorRole::Accent);
        spec.border = token(ColorRole::Accent, 0.5);
        spec.hoverBackground = token(ColorRole::Accent, 0.14);
        spec.hoverForeground = token(ColorRole::AccentLight);
        spec.hoverBorder = token(ColorRole::Accent);
        break;

    // Through the shared palette rather than re-deriving the tint. The 0.14
    // fill here against Badge's 0.16 was the whole difference between a Danger
    // button and a Danger badge sitting next to each other.
    //
    // A status control hovers into its *own* tint. Hovering every variant into
    // the accent made a Danger button turn amber under the pointer, which is
    // the one colour the design says a status may never wear.
    case Variant::Danger:
    case Variant::Success:
    case Variant::Warning: {
        const SemanticRole status = m_variant == Variant::Danger  ? SemanticRole::Danger
                                  : m_variant == Variant::Success ? SemanticRole::Success
                                                                  : SemanticRole::Warning;
        spec.background = statusToken(status, ColorRole::StatusFill);
        spec.foreground = statusToken(status, ColorRole::StatusText);
        spec.border = statusToken(status, ColorRole::StatusBorder);
        spec.hoverBackground = statusToken(status, ColorRole::StatusHover);
        spec.hoverBorder = statusToken(status, ColorRole::StatusText);
        break;
    }

    case Variant::Outlined:
        // `.btn` at rest: a hairline border, a transparent plate. Only on
        // hover does it lift onto the page's own surface
        // (`:hover{border-color:var(--text3);background:var(--bg);
        // color:var(--text)}`) - the one difference from Secondary, which is
        // already filled before the pointer ever arrives.
        spec.background = token(ColorRole::Transparent);
        spec.foreground = token(ColorRole::TextSecondary);
        spec.border = token(ColorRole::BorderHover);
        spec.hoverBackground = token(ColorRole::PanelBg);
        spec.hoverBorder = token(ColorRole::TextDisabled);
        spec.hoverForeground = token(ColorRole::TextPrimary);
        break;

    case Variant::Ghost:
        // No outline at rest: the design gives a ghost button a transparent
        // border and grows the control outline on hover, which is what keeps a
        // toolbar of low-emphasis actions from reading as a row of boxes.
        spec.background = token(ColorRole::Transparent);
        spec.foreground = token(ColorRole::TextDisabled);
        spec.border = token(ColorRole::Transparent);
        spec.hoverBackground = token(ColorRole::PanelBgAlt);
        spec.hoverBorder = token(ColorRole::BorderHover);
        break;

    case Variant::Secondary:
    default:
        // A lift off the card, not a well: a secondary button sits on top of
        // the surface it belongs to.
        //
        // BorderHover is the *control* outline (--hair2), not the surface one
        // (--hair). At --hair a button standing on the card it belongs to has
        // no visible edge, which is the difference between a button and a word.
        spec.background = token(ColorRole::SurfaceHover);
        spec.foreground = token(ColorRole::TextPrimary);
        spec.border = token(ColorRole::BorderHover);
        spec.hoverBackground = token(ColorRole::SurfaceActive);
        spec.hoverBorder = token(ColorRole::TextDisabled);
        break;
    }
    return spec;
}

ResolvedStyle Button::resolvedStyle() const
{
    return resolve(styleSpec(), this, themeFor(this).controlHeight(m_size));
}

void Button::applyVariant()
{
    // Resolved here and used here. The values are handed to the style sheet and
    // to the icon and then dropped - the button keeps the spec, never this.
    const ResolvedStyle style = resolvedStyle();
    const qreal disabledOpacity=themeFor(this).controls.buttonDisabledOpacity;
    auto* opacity=qobject_cast<QGraphicsOpacityEffect*>(graphicsEffect());
    if(disabledOpacity>=0&&!isEnabled()&&!graphicsEffect()){
        opacity=new QGraphicsOpacityEffect(this);opacity->setProperty("novuButtonOpacity",true);setGraphicsEffect(opacity);
    }
    if(opacity&&opacity->property("novuButtonOpacity").toBool())opacity->setOpacity(isEnabled()||disabledOpacity<0?1.0:qBound(0.0,disabledOpacity,1.0));

    setFont(style.font);
    applyGlyph();
    updateMotion();

    const auto css = [](const QColor& c) {
        return c.alpha() == 0 ? QStringLiteral("transparent") : rgba(c, c.alphaF());
    };

    // Padding is intentionally NOT declared here - it lives in sizeHint() above
    // so that geometry has exactly one source of truth.
    //
    // `padding: 0px` is not a contradiction of that, it is what enforces it.
    // Saying nothing about padding leaves QStyleSheetStyle free to lay the
    // label out inside the *native* button margin as well as the border, which
    // is space sizeHint() never counted: a small button then paints its caption
    // into a box a dozen pixels narrower than the one it asked for, and clips
    // it. Stating zero makes the painted content rect the rect this class
    // measured.
    setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; color: %2; border: %3px solid %4;"
        " border-radius: %5px; padding: 0px; text-align: center; }"
        "QPushButton:hover { background: %6; border-color: %7; }"
        "QPushButton:disabled { color: %8; border-color: %9; background: %10; }")
        .arg(css(style.background), style.foreground.name())
        .arg(style.borderWidth)
        .arg(css(style.border))
        .arg(style.radius)
        .arg(css(style.hoverBackground), css(style.hoverBorder),
             disabledOpacity>=0?style.foreground.name():style.disabledForeground.name(),
             css(disabledOpacity>=0?style.border:style.disabledBorder),
             disabledOpacity>=0?css(style.background):QStringLiteral("transparent")));

    updateGeometry();
}

void Button::setStatusDot(StatusDot::Tone tone)
{
    if (m_statusDot && m_statusDotTone == tone) {
        return;
    }
    m_statusDot = true;
    m_statusDotTone = tone;
    applyGlyph();
    updateGeometry();
    update();
}

void Button::clearStatusDot()
{
    if (!m_statusDot) {
        return;
    }
    m_statusDot = false;
    applyGlyph();
    updateGeometry();
    update();
}

// The dot as the button's icon rather than as a child widget or a hand-placed
// circle in paintEvent().
//
// The caption's position is QPushButton's to decide - it centres the icon and
// the text as one group, and the padding that feeds that comes from a style
// sheet. Painting a dot at a computed x would be guessing at that arithmetic
// and would drift the first time the padding changed. Handing it over as the
// icon means the layout is the one Qt already does for a leading glyph, and
// the dot is re-rendered from the tone on every theme change for the same
// reason the glyph is.
QPixmap Button::statusDotPixmap() const
{
    const int size = glyphSizeFor(this, m_size);
    const qreal ratio = devicePixelRatioF();
    QPixmap pixmap(QSize(size, size) * ratio);
    pixmap.setDevicePixelRatio(ratio);
    pixmap.fill(Qt::transparent);

    // The same token StatusDot resolves, so the dot on a button and the dot in
    // a list are one colour with one meaning.
    const QColor ink = statusDotToken(m_statusDotTone).resolve(themeFor(this));
    // The design's dot is smaller than a glyph's box; the rest of the box is
    // the breathing room StatusDot gives its own halo.
    const double diameter = size * 0.5;
    const double inset = (size - diameter) / 2.0;

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(ink);
    painter.drawEllipse(QRectF(inset, inset, diameter, diameter));
    return pixmap;
}

void Button::applyGlyph()
{
    // A state outranks a decoration: see setStatusDot(). The spinner still
    // outranks both, because a button that is working is not reporting.
    if (m_statusDot && !m_busy) {
        const int size = glyphSizeFor(this, m_size);
        setIcon(QIcon(statusDotPixmap()));
        setIconSize(QSize(size, size));
        return;
    }
    const Glyph shown = m_busy ? Glyph::Refresh : m_glyph;
    if (shown == Glyph::None) {
        setIcon(QIcon());
        return;
    }
    const int size = glyphSizeFor(this, m_size);
    // Resolved per call rather than cached. The tick is cheap; a stored ink is
    // the exact thing that stops following a theme switch.
    const QColor ink = styleSpec().foreground.resolve(themeFor(this));
    if (m_busy) {
        const double angle = motion::enabled(this)
            ? motion::phase(themeFor(this).animation.spinMs) * 360.0
            : 0.0;
        setIcon(QIcon(glyphPixmap(shown, size, ink, 0.0, angle)));
    } else {
        setIcon(glyphIcon(shown, size, ink,this));
    }
    setIconSize(QSize(size, size));
}

void Button::paintEvent(QPaintEvent* event)
{
    // The spinner is the button's icon, so QPushButton has already drawn it at
    // whatever angle the last tick set. Turning it here instead would mean
    // calling setIcon() from inside a paint event, and the two would chase each
    // other for as long as the button stayed busy.
    QPushButton::paintEvent(event);

    if (m_trailingGlyph != Glyph::None) {
        QPainter trailing(this);
        trailing.setRenderHint(QPainter::Antialiasing, true);
        const int size = glyphSizeFor(this, m_size);
        const QColor ink = styleSpec().foreground.resolve(themeFor(this));
        const QRectF box(width() - paddingFor(this,m_size) - size,
                         (height() - size) / 2.0, size, size);
        paintGlyph(&trailing, m_trailingGlyph, box, ink);
    }

    if (m_variant != Variant::Primary || !isEnabled()) {
        return;
    }
    QPainter painter(this);
    paintRing(&painter);
}

void Button::paintRing(QPainter* painter)
{
    const ThemeProfile& profile = themeFor(this);

    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRectF ring = QRectF(rect()).adjusted(kRingWidth / 2.0, kRingWidth / 2.0,
                                                -kRingWidth / 2.0, -kRingWidth / 2.0);
    const double radius = resolveRadius(m_size == ControlSize::Small
                                            ? profile.radii.value(profile.controls.smallButtonRadius)
                                            : profile.radius(SurfaceRole::Button),
                                        height());

    // The arc itself belongs to the motion layer: a button is one of several
    // things that wear it, and the stop positions are the design's, not this
    // widget's. The accent is resolved here, from this widget's own profile,
    // so the ring follows a theme change like everything else.
    motion::paintRunningRing(painter, ring, profile.colors.accent, kRingWidth, radius,
                             profile.animation.ringMs, motion::enabled(this));
}

Button* primaryButton(const QString& text, QWidget* parent)
{
    return new Button(text, Button::Variant::Primary, parent);
}

Button* secondaryButton(const QString& text, QWidget* parent)
{
    return new Button(text, Button::Variant::Secondary, parent);
}

Button* outlinedButton(const QString& text, QWidget* parent)
{
    return new Button(text, Button::Variant::Outlined, parent);
}

Button* ghostButton(const QString& text, QWidget* parent)
{
    return new Button(text, Button::Variant::Ghost, parent);
}

Button* dangerButton(const QString& text, QWidget* parent)
{
    return new Button(text, Button::Variant::Danger, parent);
}

Button* successButton(const QString& text, QWidget* parent)
{
    return new Button(text, Button::Variant::Success, parent);
}

Button* warningButton(const QString& text, QWidget* parent)
{
    return new Button(text, Button::Variant::Warning, parent);
}

Button* detachButton(QWidget* parent)
{
    // The icon set's own mark, not a Unicode character.
    //
    // It was U+29C9 "two joined squares" inside the caption, which is a glyph
    // from whatever text font happened to resolve it - a different weight, a
    // different size and a different vertical position from every other icon
    // in the same header, and missing outright on a machine without it. The
    // design says `iact("external", ...)`, and External is that mark.
    //
    // `iact()` is `class="btn sm ico"` - `.btn`'s hairline border at rest,
    // like every other button. Ghost draws no border until hover, which is
    // right for chrome (a title bar) and wrong here: a card header's detach
    // control is a row action, not furniture.
    auto* button = new Button(QObject::tr("Detach"), Button::Variant::Outlined, parent);
    button->setGlyph(Glyph::External);
    Tooltip::install(button, TipContent::action(
        QObject::tr("Detach"),
        QObject::tr("Opens this panel in a window of its own, so it stays "
                    "readable while you work elsewhere.")));
    return button;
}

} // namespace novu::ui
