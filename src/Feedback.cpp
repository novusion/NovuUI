#include "novu/ui/Feedback.h"

#include "novu/ui/Icon.h"
#include "novu/ui/Motion.h"
#include "novu/ui/StyleSheet.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QFontMetrics>
#include <QLayout>
#include <QMargins>
#include <QPainter>
#include <QPainterPath>
#include <QShowEvent>
#include <QVBoxLayout>

#include <cmath>

namespace novu::ui {

namespace {

// The same roles Badge maps, resolved through the same shared palette - an
// InfoBanner and a Badge saying the same thing must not be two different
// colours. Info in particular used to be the application accent here too.
// A banner asks for this much and is happy with less. Neither number is
// measured from the text, which is the point.
constexpr int kPreferredWidth = 420;
constexpr int kMinimumWidth = 120;

SemanticRole roleFor(Badge::Tone tone)
{
    switch (tone) {
    case Badge::Tone::Success: return SemanticRole::Success;
    case Badge::Tone::Warning: return SemanticRole::Warning;
    case Badge::Tone::Danger:  return SemanticRole::Danger;
    case Badge::Tone::Info:    return SemanticRole::Info;
    case Badge::Tone::Neutral: break;
    }
    return SemanticRole::Neutral;
}

const SemanticColor& toneColors(const QWidget* context, Badge::Tone tone)
{
    return themeFor(context).semantic.role(roleFor(tone));
}

QColor toneColor(const QWidget* context, Badge::Tone tone)
{
    return toneColors(context, tone).text;
}

} // namespace

EmptyState::EmptyState(const QString& glyph, const QString& title,
                       const QString& subtitle, QWidget* parent)
    : QWidget(parent)
{
    const ThemeProfile& profile = themeFor(this);

    auto* column = new QVBoxLayout(this);
    column->setSpacing(profile.space(SpacingRole::FormRowGap) / 2);
    column->setAlignment(Qt::AlignCenter);

    auto* icon = label(glyph, this, TypeRole::PageTitle, Ink::Muted);
    icon->setFont(iconFont(profile.icons.large + 6));
    icon->setAlignment(Qt::AlignCenter);
    column->addWidget(icon);

    m_title = label(title, this, TypeRole::CardTitle, Ink::Secondary, true);
    m_title->setAlignment(Qt::AlignCenter);
    column->addWidget(m_title);

    m_subtitle = label(subtitle, this, TypeRole::Caption, Ink::Muted);
    m_subtitle->setAlignment(Qt::AlignCenter);
    column->addWidget(m_subtitle);
}

void EmptyState::setTitle(const QString& title)
{
    m_title->setText(title);
}

void EmptyState::setSubtitle(const QString& subtitle)
{
    m_subtitle->setText(subtitle);
}

InfoBanner::InfoBanner(const QString& text, Badge::Tone tone, QWidget* parent,
                       QWidget* trailing)
    : QFrame(parent)
    , m_tone(tone)
{
    const ThemeProfile& profile = themeFor(this);

    // Painted, not styled - see the header for why.
    setAttribute(Qt::WA_StyledBackground, false);
    setFixedHeight(profile.controlHeight(ControlSize::Medium) + 6);

    // A banner is exactly as wide as it is given. Its sentence must never be
    // what decides that - a long one elides, it does not push.
    //
    // This is not a detail. The gallery hands this component a linker error,
    // which is one unbroken four-hundred-character line, and a banner whose
    // width followed its text dragged the whole window off the screen. Any
    // component that reports a failure will one day be handed a message
    // nobody budgeted for, so not growing is part of what a banner *is*.
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

    // Through the setter, not the member: the tooltip that carries the full
    // message when the strip elides is set there, and assigning m_text directly
    // meant a banner given its text at construction - which is all of them -
    // silently had none.
    setText(text);

    auto* row = new QHBoxLayout(this);
    // The medium step, not the form-row gap: a banner is a strip of prose and
    // the tighter gap left its sentence sitting on the tinted edge.
    row->setContentsMargins(profile.spacing.md, 0, profile.spacing.md, 0);
    // The sentence is painted, not laid out: it is one line of non-interactive
    // prose, and a QLabel for it would be a second widget whose own sizeHint is
    // the thing that has to be fought. The stretch reserves its lane.
    row->addStretch(1);

    if (trailing) {
        trailing->setParent(this);
        row->addWidget(trailing);
    }
}

QSize InfoBanner::sizeHint() const
{
    // A preferred width, not a measured one. Height is the whole of what this
    // component insists on.
    return QSize(kPreferredWidth, height());
}

QSize InfoBanner::minimumSizeHint() const
{
    return QSize(kMinimumWidth, height());
}

void InfoBanner::setText(const QString& text)
{
    if (m_text == text) {
        return;
    }
    m_text = text;
    // The full sentence stays reachable even when the strip is too narrow for
    // it. Eliding without this loses the half of the message that mattered.
    setToolTip(text);
    update();
}

void InfoBanner::setTone(Badge::Tone tone)
{
    if (m_tone == tone) {
        return;
    }
    m_tone = tone;
    update();
}

void InfoBanner::paintEvent(QPaintEvent*)
{
    const QColor tone = toneColor(this, m_tone);
    // Button, not Badge.
    //
    // A badge is a pill because it is a word-sized token. A banner is a
    // control-height strip of prose, and asking for the badge radius turned
    // it into a lozenge — most visibly when it carries a trailing action,
    // where a fully rounded strip wrapped a button with 6px corners and the
    // two shapes contradicted each other.
    //
    // Button is the role a banner shares its rhythm with: it stands at
    // control height, next to controls, and often contains one.
    const int radius = resolveRadius(themeFor(this).radius(SurfaceRole::Button), height());

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath path;
    path.addRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);

    QColor fill = tone;
    fill.setAlphaF(0.10f);
    p.fillPath(path, fill);

    QColor outline = tone;
    outline.setAlphaF(0.35f);
    p.setPen(QPen(outline, 1));
    p.drawPath(path);

    if (m_text.isEmpty()) {
        return;
    }

    // The text lane stops where the trailing widget begins, so a banner with a
    // button in it elides against the button rather than under it.
    const QMargins margins = layout() ? layout()->contentsMargins() : QMargins();
    QRect lane = rect().adjusted(margins.left(), 0, -margins.right(), 0);
    for (QObject* child : children()) {
        if (auto* widget = qobject_cast<QWidget*>(child); widget && widget->isVisible()) {
            lane.setRight(qMin(lane.right(), widget->geometry().left() - margins.left()));
        }
    }

    const QFont font = roleFont(this, TypeRole::Caption);
    p.setFont(font);
    p.setPen(tone);
    p.drawText(lane, Qt::AlignVCenter | Qt::AlignLeft,
               QFontMetrics(font).elidedText(m_text, Qt::ElideRight, lane.width()));
}

namespace {

// The design's `.meter { height:6px; border-radius:4px }`, and the legend gap.
constexpr int kTrackHeight = 6;
constexpr int kTrackRadius = 3;
constexpr int kLegendGap = 5;

// NovuAdmin's thresholds, kept verbatim: a disk has to read as full before
// anyone reads the number.
constexpr int kWarningPercent = 75;
constexpr int kCriticalPercent = 90;

} // namespace

ProgressBar::ProgressBar(int value, QWidget* parent)
    : QWidget(parent)
    , m_value(qBound(0, value, 100))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

ProgressBar::ProgressBar(int value, Tone tone, QWidget* parent)
    : ProgressBar(value, parent)
{
    m_tone = tone;
}

void ProgressBar::setValue(int value)
{
    // Clamped: a caller computing a percentage from two live counters will hand
    // over 101 sooner or later, and a fill wider than its track is a fill that
    // paints over the card it is in.
    const int clamped = qBound(0, value, 100);
    if (m_value == clamped && !m_unknown) {
        return;
    }
    m_value = clamped;
    // Setting a value is a measurement, so it is no longer unknown.
    m_unknown = false;
    update();
}

void ProgressBar::setTone(Tone tone)
{
    if (m_tone == tone) {
        return;
    }
    m_tone = tone;
    update();
}

void ProgressBar::setUnknown(bool unknown)
{
    if (m_unknown == unknown) {
        return;
    }
    m_unknown = unknown;
    update();
}

void ProgressBar::setLegend(const QString& leading, const QString& trailing)
{
    if (m_leading == leading && m_trailing == trailing) {
        return;
    }
    const bool had = !m_leading.isEmpty() || !m_trailing.isEmpty();
    m_leading = leading;
    m_trailing = trailing;
    if (had != (!m_leading.isEmpty() || !m_trailing.isEmpty())) {
        updateGeometry();   // the legend line changes the bar's height
    }
    update();
}

SemanticRole ProgressBar::effectiveRole() const
{
    switch (m_tone) {
    case Tone::Success: return SemanticRole::Success;
    case Tone::Warning: return SemanticRole::Warning;
    case Tone::Danger:  return SemanticRole::Danger;
    case Tone::Threshold:
        if (m_value >= kCriticalPercent) {
            return SemanticRole::Danger;
        }
        if (m_value >= kWarningPercent) {
            return SemanticRole::Warning;
        }
        return SemanticRole::Success;
    case Tone::Accent:
        break;
    }
    // Accent is not a status and must not resolve to one: work in progress is
    // the product doing something, not a state of the world.
    return SemanticRole::Neutral;
}

StyleSpec ProgressBar::styleSpec() const
{
    StyleSpec spec;
    spec.background = token(ColorRole::PanelBgAlt);
    spec.border = token(ColorRole::Transparent);
    spec.radius = SurfaceRole::Badge;
    spec.borderWidth = BorderRole::Divider;
    spec.type = TypeRole::Caption;
    spec.foreground = m_tone == Tone::Accent
        ? token(ColorRole::Accent)
        : statusToken(effectiveRole(), ColorRole::StatusText);
    return spec;
}

ResolvedStyle ProgressBar::resolvedStyle() const
{
    return resolve(styleSpec(), this, kTrackHeight);
}

QSize ProgressBar::sizeHint() const
{
    int height = kTrackHeight;
    if (!m_leading.isEmpty() || !m_trailing.isEmpty()) {
        const ThemeProfile& profile = themeFor(this);
        QFont font = roleFont(this, TypeRole::Caption);
        font.setPixelSize(profile.type.xs);
        height += kLegendGap + QFontMetrics(font).height();
    }
    return QSize(120, height);
}

QSize ProgressBar::minimumSizeHint() const
{
    return QSize(48, sizeHint().height());
}

bool ProgressBar::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        updateGeometry();
        update();
    }
    return QWidget::event(event);
}

void ProgressBar::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);

    const QRectF track(0, 0, width(), kTrackHeight);
    painter.setBrush(profile.colors.panelBgAlt);
    painter.drawRoundedRect(track, kTrackRadius, kTrackRadius);

    // Unknown draws the track and stops. "Not measured" and "idle" must not
    // look the same, and a zero-length fill is what makes them.
    if (!m_unknown && m_value > 0) {
        const QColor fill = m_tone == Tone::Accent
            ? profile.colors.accent
            : profile.semantic.role(effectiveRole()).text;
        painter.setBrush(fill);
        painter.drawRoundedRect(QRectF(0, 0, track.width() * m_value / 100.0, kTrackHeight),
                                kTrackRadius, kTrackRadius);
    }

    if (m_leading.isEmpty() && m_trailing.isEmpty()) {
        return;
    }

    QFont font = roleFont(this, TypeRole::Caption);
    font.setPixelSize(profile.type.xs);
    const QFontMetrics fm(font);
    painter.setFont(font);

    const QRect legend(0, kTrackHeight + kLegendGap, width(), fm.height());
    if (!m_leading.isEmpty()) {
        // The reading itself takes the bar's colour, so the number and the fill
        // agree without a caller tinting the label.
        painter.setPen(m_unknown ? profile.colors.textDisabled
                                 : (m_tone == Tone::Accent
                                        ? profile.colors.textSecondary
                                        : profile.semantic.role(effectiveRole()).text));
        painter.drawText(legend, Qt::AlignLeft | Qt::AlignVCenter, m_leading);
    }
    if (!m_trailing.isEmpty()) {
        painter.setPen(profile.colors.textDisabled);
        painter.drawText(legend, Qt::AlignRight | Qt::AlignVCenter, m_trailing);
    }
}

QString formatBytes(qint64 bytes)
{
    // 1024, and the short names these tools actually print. Stated once,
    // because NovuAdmin had two of these and they disagreed about whether a
    // kilobyte was 1000 or 1024 - which showed up as a file that was 1.0 MB in
    // the table and 0.98 MB in its own properties.
    static const char* const units[] = {"B", "kB", "MB", "GB", "TB", "PB"};
    const bool negative = bytes < 0;
    double value = std::abs(double(bytes));
    int unit = 0;
    while (value >= 1024.0 && unit < 5) {
        value /= 1024.0;
        ++unit;
    }
    // Bytes are never fractional; above that one decimal is what a person
    // reads without the number jittering.
    const QString text = unit == 0
        ? QString::number(qint64(value))
        : QString::number(value, 'f', value < 10.0 ? 1 : 0);
    return QStringLiteral("%1%2 %3")
        .arg(negative ? QStringLiteral("-") : QString(), text,
             QLatin1String(units[unit]));
}

// == waiting ================================================================

namespace {

// The design's `.sp` is 13px with a 1.7px border inside a 12.5px line. Held as
// a ratio rather than as two numbers so a Large spinner is the same drawing.
constexpr double kSpinnerStrokeRatio = 0.13;
constexpr double kSpinnerStrokeMin = 1.4;

// `.skel { height:11px }` and `.skel.tall { height:24px }`.
constexpr int kSkeletonTextHeight = 11;
constexpr int kSkeletonBlockHeight = 24;

// `.skelrow { padding:11px 13px }`, and the gap between plates in a row.
constexpr int kSkeletonRowPaddingV = 11;
constexpr int kSkeletonRowPaddingH = 13;
constexpr int kSkeletonRowGap = 12;

// A placeholder plate is the card colour lifted towards the hover colour, which
// is what the design's `linear-gradient(card, card-h, card)` is. Taken from the
// profile so a light theme shimmers pale rather than dark.
QColor skeletonBase(const QWidget* widget)
{
    return themeFor(widget).colors.panelBgAlt;
}

QColor skeletonHighlight(const QWidget* widget)
{
    return themeFor(widget).colors.surfaceHover;
}

} // namespace

// -- Spinner ----------------------------------------------------------------

Spinner::Spinner(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

Spinner::Spinner(ControlSize size, QWidget* parent)
    : Spinner(parent)
{
    m_size = size;
}

void Spinner::setControlSize(ControlSize size)
{
    if (m_size == size) {
        return;
    }
    m_size = size;
    updateGeometry();
    update();
}

void Spinner::setSpinning(bool spinning)
{
    if (m_spinning == spinning) {
        return;
    }
    m_spinning = spinning;
    updateMotion();
    update();
}

void Spinner::setHeadColor(const QColor& color)
{
    m_head = color;
    update();
}

QSize Spinner::sizeHint() const
{
    const int side = glyphSizeFor(this, m_size);
    return QSize(side, side);
}

void Spinner::updateMotion()
{
    // Three conditions, all of them necessary: a spinner told to stop, one that
    // is off screen, and one in a profile with motion switched off must none of
    // them hold the clock open.
    const bool wanted = m_spinning && isVisible() && motion::enabled(this);
    if (wanted == m_subscribed) {
        return;
    }
    m_subscribed = wanted;
    if (wanted) {
        Pulse::instance().subscribe(this);
    } else {
        Pulse::instance().release(this);
    }
}

void Spinner::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    updateMotion();
}

void Spinner::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    updateMotion();
}

bool Spinner::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        // A profile that disables motion has to stop an already-running
        // spinner, not merely paint the next frame differently.
        updateMotion();
        updateGeometry();
        update();
    }
    return QWidget::event(event);
}

void Spinner::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const QRectF box = QRectF(rect());
    const double stroke = qMax(kSpinnerStrokeMin, box.width() * kSpinnerStrokeRatio);

    QPainter painter(this);
    motion::paintSpinner(&painter, box,
                         profile.colors.border,
                         m_head.isValid() ? m_head : profile.colors.accentLight,
                         stroke, profile.animation.spinMs,
                         m_spinning && motion::enabled(this));
}

// -- Skeleton ---------------------------------------------------------------

Skeleton::Skeleton(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

Skeleton::Skeleton(Line line, QWidget* parent)
    : Skeleton(parent)
{
    m_line = line;
}

void Skeleton::setLine(Line line)
{
    if (m_line == line) {
        return;
    }
    m_line = line;
    updateGeometry();
    update();
}

void Skeleton::setFill(double fraction)
{
    const double clamped = qBound(0.05, fraction, 1.0);
    if (qFuzzyCompare(m_fill + 1.0, clamped + 1.0)) {
        return;
    }
    m_fill = clamped;
    update();
}

QSize Skeleton::sizeHint() const
{
    return QSize(120, m_line == Line::Block ? kSkeletonBlockHeight : kSkeletonTextHeight);
}

QSize Skeleton::minimumSizeHint() const
{
    return QSize(24, sizeHint().height());
}

void Skeleton::updateMotion()
{
    const bool wanted = isVisible() && motion::enabled(this);
    if (wanted == m_subscribed) {
        return;
    }
    m_subscribed = wanted;
    if (wanted) {
        Pulse::instance().subscribe(this);
    } else {
        Pulse::instance().release(this);
    }
}

void Skeleton::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    updateMotion();
}

void Skeleton::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    updateMotion();
}

bool Skeleton::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        updateMotion();
        updateGeometry();
        update();
    }
    return QWidget::event(event);
}

void Skeleton::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    QRectF plate(rect());
    plate.setWidth(plate.width() * m_fill);

    QPainter painter(this);
    motion::paintShimmer(&painter, plate, profile.radii.small,
                         skeletonBase(this), skeletonHighlight(this),
                         profile.animation.shimmerMs, motion::enabled(this));
}

// -- SkeletonList -----------------------------------------------------------

SkeletonList::SkeletonList(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

SkeletonList::SkeletonList(int rows, const QVector<double>& columns, QWidget* parent)
    : SkeletonList(parent)
{
    m_rows = qMax(1, rows);
    if (!columns.isEmpty()) {
        m_columns = columns;
    }
}

void SkeletonList::setRowCount(int rows)
{
    const int clamped = qMax(1, rows);
    if (m_rows == clamped) {
        return;
    }
    m_rows = clamped;
    updateGeometry();
    update();
}

void SkeletonList::setColumns(const QVector<double>& columns)
{
    if (columns.isEmpty() || columns == m_columns) {
        return;
    }
    m_columns = columns;
    update();
}

QSize SkeletonList::sizeHint() const
{
    const ThemeProfile& profile = themeFor(this);
    const int row = qMax(profile.table.rowHeight,
                         kSkeletonTextHeight + 2 * kSkeletonRowPaddingV);
    return QSize(320, row * m_rows);
}

QSize SkeletonList::minimumSizeHint() const
{
    return QSize(120, sizeHint().height());
}

void SkeletonList::updateMotion()
{
    const bool wanted = isVisible() && motion::enabled(this);
    if (wanted == m_subscribed) {
        return;
    }
    m_subscribed = wanted;
    if (wanted) {
        Pulse::instance().subscribe(this);
    } else {
        Pulse::instance().release(this);
    }
}

void SkeletonList::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    updateMotion();
}

void SkeletonList::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    updateMotion();
}

bool SkeletonList::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        updateMotion();
        updateGeometry();
        update();
    }
    return QWidget::event(event);
}

void SkeletonList::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const int rowHeight = qMax(profile.table.rowHeight,
                               kSkeletonTextHeight + 2 * kSkeletonRowPaddingV);

    double total = 0.0;
    for (const double column : m_columns) {
        total += qMax(0.0, column);
    }
    if (total <= 0.0) {
        return;
    }

    const QColor base = skeletonBase(this);
    const QColor highlight = skeletonHighlight(this);
    const bool animated = motion::enabled(this);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const double gaps = kSkeletonRowGap * (m_columns.size() - 1);
    const double usable = width() - 2.0 * kSkeletonRowPaddingH - gaps;

    for (int row = 0; row < m_rows; ++row) {
        const double top = row * rowHeight;

        // The separator, drawn between rows and not after the last one - the
        // design's `.skelrow:last-child { border-bottom:none }`. A table's own
        // grid line, so the placeholder and the table rule at the same weight.
        if (row + 1 < m_rows) {
            painter.setPen(QPen(profile.table.gridLine, profile.border(BorderRole::Table)));
            painter.drawLine(QPointF(0.0, top + rowHeight - 0.5),
                             QPointF(width(), top + rowHeight - 0.5));
        }

        double x = kSkeletonRowPaddingH;
        for (const double column : m_columns) {
            const double plateWidth = usable * qMax(0.0, column) / total;
            if (plateWidth <= 0.0) {
                continue;
            }
            const QRectF plate(x, top + (rowHeight - kSkeletonTextHeight) / 2.0,
                               plateWidth, kSkeletonTextHeight);
            motion::paintShimmer(&painter, plate, profile.radii.small, base, highlight,
                                 profile.animation.shimmerMs, animated);
            x += plateWidth + kSkeletonRowGap;
        }
    }
}

// -- StatusLine -------------------------------------------------------------

StatusLine::StatusLine(const QString& text, State state, QWidget* parent)
    : QWidget(parent)
    , m_text(text)
    , m_state(state)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // The spinner is a real child rather than a painted arc, and this is the
    // one place in the library where that is the cheaper answer: it owns the
    // clock subscription, so a line that is not running holds nothing open and
    // this class needs no motion code of its own.
    m_spinner = new Spinner(ControlSize::Small, this);
    m_spinner->setVisible(state == State::Running);
}

void StatusLine::setText(const QString& text)
{
    if (m_text == text) {
        return;
    }
    m_text = text;
    updateGeometry();
    update();
}

void StatusLine::setState(State state, const QString& text)
{
    if (!text.isEmpty()) {
        m_text = text;
    }
    m_state = state;
    m_spinner->setVisible(state == State::Running);
    m_spinner->setSpinning(state == State::Running);
    updateGeometry();
    update();
}

QSize StatusLine::sizeHint() const
{
    const ThemeProfile& profile = themeFor(this);
    const QFontMetrics fm(roleFont(this, TypeRole::Body));
    const int mark = glyphSizeFor(this, ControlSize::Small);
    return QSize(mark + profile.spacing.sm + fm.horizontalAdvance(m_text),
                 qMax(mark, fm.height()));
}

bool StatusLine::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        updateGeometry();
        update();
    }
    return QWidget::event(event);
}

void StatusLine::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const int mark = glyphSizeFor(this, ControlSize::Small);
    const QRectF markBox(0.0, (height() - mark) / 2.0, mark, mark);

    // The spinner child is positioned here rather than by a layout: the whole
    // widget is one row of two things, and a QHBoxLayout to place a 14px square
    // against a painted sentence is a layout that exists to do arithmetic.
    m_spinner->setGeometry(markBox.toRect());

    QColor ink = profile.colors.textSecondary;
    Glyph glyph = Glyph::None;
    switch (m_state) {
    case State::Done:
        ink = profile.semantic.success.text;
        glyph = Glyph::Check;
        break;
    case State::Failed:
        ink = profile.semantic.danger.text;
        glyph = Glyph::Close;
        break;
    case State::Running:
        ink = profile.colors.textPrimary;
        break;
    case State::Idle:
        ink = profile.colors.textDisabled;
        break;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    if (glyph != Glyph::None) {
        paintGlyph(&painter, glyph, markBox, ink);
    }

    painter.setFont(roleFont(this, TypeRole::Body));
    painter.setPen(ink);
    QRect textBox = rect();
    textBox.setLeft(static_cast<int>(markBox.right()) + profile.spacing.sm);
    painter.drawText(textBox, Qt::AlignVCenter | Qt::AlignLeft, m_text);
}

// -- Steps ------------------------------------------------------------------

namespace {

// `.steps { gap:7px }`, `.steps .s { padding:2px 8px; border-radius:999px }`.
constexpr int kStepGap = 7;
constexpr int kStepPaddingH = 8;
constexpr int kStepPaddingV = 2;

QFont stepFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleMonoFont(context, TypeRole::Eyebrow);
    font.setPixelSize(qMax(8, profile.type.xxs));
    // The design's .08em tracking. At this size a run of uppercase words set
    // solid is one grey smear.
    font.setLetterSpacing(QFont::PercentageSpacing, 108);
    return font;
}

} // namespace

Steps::Steps(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

Steps::Steps(const QStringList& steps, QWidget* parent, int current)
    : Steps(parent)
{
    m_steps = steps;
    m_current = qBound(0, current, qMax(0, steps.size()));
}

int Steps::addStep(const QString& label)
{
    m_steps.append(label);
    updateGeometry();
    update();
    return m_steps.size() - 1;
}

void Steps::clear()
{
    m_steps.clear();
    m_current = 0;
    updateGeometry();
    update();
}

QString Steps::stepText(int index) const
{
    return index >= 0 && index < m_steps.size() ? m_steps.at(index) : QString();
}

void Steps::setCurrentIndex(int index)
{
    // size() is a legal value and means "finished": every pill done, none
    // current. Clamping to size()-1 would leave the last stage looking like it
    // is still running after it has succeeded.
    const int clamped = qBound(0, index, m_steps.size());
    if (m_current == clamped) {
        return;
    }
    m_current = clamped;
    updateGeometry();
    update();
}

void Steps::markComplete()
{
    setCurrentIndex(m_steps.size());
}

bool Steps::isComplete() const
{
    return !m_steps.isEmpty() && m_current >= m_steps.size();
}

int Steps::stepWidth(int index) const
{
    if (index < 0 || index >= m_steps.size()) {
        return 0;
    }
    return QFontMetrics(stepFont(this)).horizontalAdvance(m_steps.at(index).toUpper())
        + 2 * kStepPaddingH;
}

QRect Steps::stepRect(int index) const
{
    if (index < 0 || index >= m_steps.size()) {
        return QRect();
    }
    int x = 0;
    for (int i = 0; i < index; ++i) {
        x += stepWidth(i) + kStepGap;
    }
    return QRect(x, 0, stepWidth(index), height());
}

QSize Steps::sizeHint() const
{
    if (m_steps.isEmpty()) {
        return QSize(0, 0);
    }
    int width = kStepGap * (m_steps.size() - 1);
    for (int i = 0; i < m_steps.size(); ++i) {
        width += stepWidth(i);
    }
    return QSize(width, QFontMetrics(stepFont(this)).height() + 2 * kStepPaddingV + 2);
}

bool Steps::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        updateGeometry();
        update();
    }
    return QWidget::event(event);
}

void Steps::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const QFont font = stepFont(this);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setFont(font);

    for (int i = 0; i < m_steps.size(); ++i) {
        const QRectF pill = QRectF(stepRect(i)).adjusted(0.5, 0.5, -0.5, -0.5);
        const double radius = pill.height() / 2.0;

        QColor ink = profile.colors.textDisabled;
        QColor border = profile.colors.border;
        QColor fill = Qt::transparent;

        if (i < m_current) {
            // Done: the success ink and its border, but no plate. A row of
            // filled green pills behind the one that is running is louder than
            // the thing it is reporting on.
            ink = profile.semantic.success.text;
            border = profile.semantic.success.border;
        } else if (i == m_current) {
            // Running: the accent, with the plate. The only filled pill, so the
            // eye lands on it.
            ink = profile.colors.accentLight;
            border = withAlpha(profile.colors.accent, 0.5);
            fill = withAlpha(profile.colors.accent, 0.12);
        }

        painter.setPen(QPen(border, qMax(1, profile.border(BorderRole::Card))));
        painter.setBrush(fill.alpha() > 0 ? QBrush(fill) : QBrush(Qt::NoBrush));
        painter.drawRoundedRect(pill, radius, radius);

        painter.setPen(ink);
        painter.drawText(pill, Qt::AlignCenter, m_steps.at(i).toUpper());
    }
}

} // namespace novu::ui
