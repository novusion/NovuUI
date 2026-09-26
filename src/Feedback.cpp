#include "novu/ui/Feedback.h"

#include "novu/ui/Icon.h"
#include "novu/ui/Motion.h"
#include "novu/ui/StyleSheet.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QHBoxLayout>
#include <QTimer>
#include <QLabel>
#include <QFontMetrics>
#include <QLayout>
#include <QMargins>
#include <QAbstractTextDocumentLayout>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextLayout>
#include <QtMath>
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

namespace {

// `.empty svg{opacity:.5}` - the mark is the quietest thing in the column.
constexpr double kEmptyMarkOpacity = 0.5;

// `.note{padding:11px 14px}`.
// A single sentence is an inline notice, not a card. Seven pixels keeps the
// common one-line banner compact while heightForWidth() still adds as much
// room as wrapped prose or a trailing action actually needs.
constexpr int kBannerPaddingV = 7;
constexpr int kBannerPaddingH = 14;
// `.note{gap:10px;border-radius:11px}` and `.note svg{width:15px}`.
constexpr int kBannerGap = 10;
constexpr int kBannerRadius = 11;
constexpr int kBannerGlyph = 15;

// `.empty .s{max-width:48ch}` and `.bpage .inner{max-width:430px}` - the
// column is capped whatever the page is wide. A sentence that runs the width
// of a 1300px view is a sentence nobody finishes.
constexpr int kTextWidth = 430;

// The two rhythms the design draws, and the mark is what says which one this
// is: a bare glyph is an empty state inside a card, a plaque is a page-sized
// one. `.empty`: 12 / 5 / 16. `.bpage`: 16 / 6 / 14 / 18.
struct Rhythm
{
    int markGap;
    int detailGap;
    int textGap;
    int actionGap;
    double lineHeight;
};

constexpr Rhythm kBare{12, 5, 5, 16, 1.55};
constexpr Rhythm kPlaque{16, 6, 14, 18, 1.6};

} // namespace

EmptyState::EmptyState(const QString& glyph, const QString& title,
                       const QString& subtitle, QWidget* parent)
    : QWidget(parent)
{
    const ThemeProfile& profile = themeFor(this);

    // `.bpage .inner{max-width:430px}` - the column is a block of a fixed
    // measure standing in the middle of whatever it is given, not a paragraph
    // stretched across the page. The stretches either side are what centre it;
    // the weight on the middle is what lets it reach its cap on a wide page
    // and give way gracefully on a narrow one.
    auto* centred = new QHBoxLayout(this);
    centred->setContentsMargins(0, 0, 0, 0);
    centred->setSpacing(0);
    m_inner = new QWidget(this);
    m_inner->setMaximumWidth(kTextWidth);
    centred->addStretch(1);
    centred->addWidget(m_inner, 8);
    centred->addStretch(1);

    m_column = new QVBoxLayout(m_inner);
    // Every gap in this column is a margin the design states between two
    // named things, so the layout contributes none of its own.
    m_column->setContentsMargins(0, 0, 0, 0);
    m_column->setSpacing(0);
    m_column->setAlignment(Qt::AlignVCenter);

    auto* icon = label(glyph, m_inner, TypeRole::PageTitle, Ink::Muted);
    icon->setFont(iconFont(profile.icons.large + 6));
    icon->setAlignment(Qt::AlignCenter);
    m_glyphMark = icon;
    m_column->addWidget(icon);

    m_title = label(title, m_inner, TypeRole::CardTitle, Ink::Secondary, true);
    m_title->setAlignment(Qt::AlignCenter);
    m_column->addWidget(m_title);

    // `.bpage .u` - built here rather than on demand so the column never has
    // to be torn down around a line that appears later.
    m_detailLabel = label(QString(), m_inner, TypeRole::Mono, Ink::Muted);
    m_detailLabel->setAlignment(Qt::AlignCenter);
    m_detailLabel->setWordWrap(true);
    m_detailLabel->hide();
    m_column->addWidget(m_detailLabel);

    m_subtitle = label(subtitle, m_inner, TypeRole::Caption, Ink::Muted);
    m_subtitle->setAlignment(Qt::AlignCenter);
    m_subtitle->setWordWrap(true);
    m_column->addWidget(m_subtitle);

    m_actions = new QWidget(m_inner);
    auto* acts = new QHBoxLayout(m_actions);
    acts->setContentsMargins(0, 0, 0, 0);
    acts->setSpacing(profile.space(SpacingRole::ToolbarGap));
    acts->setAlignment(Qt::AlignCenter);
    // An empty actions row is not a row. It reappears the moment a caller puts
    // a button in it.
    m_actions->hide();
    m_actions->installEventFilter(this);
    m_column->addWidget(m_actions);

    applyRhythm();
}

EmptyState::EmptyState(Glyph glyph, const QString& title, const QString& subtitle,
                       QWidget* parent)
    : EmptyState(QString(), title, subtitle, parent)
{
    const ThemeProfile& profile = themeFor(this);
    auto* icon = new QLabel(this);
    const int side = profile.icons.large + 6;
    icon->setPixmap(glyphPixmap(glyph, side,
                                withAlpha(profile.colors.textDisabled, kEmptyMarkOpacity),
                                devicePixelRatioF()));
    icon->setAlignment(Qt::AlignCenter);
    icon->setFixedSize(side, side);
    setMark(icon);
    // A glyph is still the bare mark, whatever widget carries it.
    m_plaque = false;
    applyRhythm();
}

void EmptyState::applyRhythm()
{
    const Rhythm& r = m_plaque ? kPlaque : kBare;

    m_title->setContentsMargins(0, r.markGap, 0, 0);
    if (m_plaque) {
        // `.bpage h3{font-size:15.5px}` against `.empty .t{14px}` - a page's
        // own empty state is a heading, one inside a card is a line of text.
        applyText(m_title, TypeRole::PageTitle, Ink::Default, true);
        m_title->setAlignment(Qt::AlignCenter);
    }
    m_detailLabel->setContentsMargins(0, r.detailGap, 0, 0);
    m_subtitle->setContentsMargins(0, m_detail.isEmpty() ? r.detailGap : r.textGap, 0, 0);
    setLineHeight(m_subtitle, r.lineHeight);
    // Rich text takes the ink from the document, not from the palette the
    // plain path set, so the sentence is re-inked after it is re-rendered.
    applyText(m_subtitle, TypeRole::Caption, Ink::Muted);
    m_subtitle->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    m_subtitle->setMaximumWidth(kTextWidth);
    if (auto* acts = actions()) {
        acts->setContentsMargins(0, r.actionGap, 0, 0);
    }
    if (m_footerRow) {
        m_footerRow->setContentsMargins(0, r.actionGap, 0, 0);
    }

    if (m_mark) {
        // A plaque is a square: the column must not be allowed to talk it out
        // of its own height when it has room to spare.
        m_mark->setFixedSize(m_mark->sizeHint());
    }
}

void EmptyState::setTitle(const QString& title)
{
    m_title->setText(title);
}

void EmptyState::setTextMaximumWidth(int width)
{
    m_textWidth = qMax(1, width);
    m_inner->setMaximumWidth(m_textWidth);
    m_subtitle->setMaximumWidth(m_textWidth);
    m_subtitle->updateGeometry();
    updateGeometry();
}

void EmptyState::setSubtitle(const QString& subtitle)
{
    // Through the helper: the sentence is carrying a line height, and a plain
    // setText() would replace the styled block with a bare string.
    setLabelText(m_subtitle, subtitle);
}

void EmptyState::setMark(QWidget* mark)
{
    if (m_mark == mark) {
        return;
    }
    delete m_mark;
    m_mark = mark;
    m_plaque = m_mark != nullptr;
    rebuildMarkRow();
    applyRhythm();
}

void EmptyState::rebuildMarkRow()
{
    // Exactly one mark is shown. Two would be a glyph and a plaque saying the
    // same thing one above the other.
    if (m_glyphMark) {
        m_glyphMark->setVisible(m_mark == nullptr);
    }
    if (!m_mark) {
        return;
    }
    m_mark->setParent(m_inner);
    auto* row = new QHBoxLayout;
    row->setContentsMargins(0, 0, 0, 0);
    row->addStretch(1);
    row->addWidget(m_mark);
    row->addStretch(1);
    m_column->insertLayout(0, row);
    m_mark->show();
}

void EmptyState::setDetail(const QString& detail)
{
    m_detail = detail;
    m_detailLabel->setText(detail);
    m_detailLabel->setVisible(!detail.isEmpty());
    applyRhythm();
}

QHBoxLayout* EmptyState::actions() const
{
    return qobject_cast<QHBoxLayout*>(m_actions->layout());
}

void EmptyState::setFooter(QWidget* footer)
{
    if (m_footer == footer) {
        return;
    }
    delete m_footer;
    m_footer = footer;
    if (!m_footer) {
        return;
    }
    m_footer->setParent(m_inner);
    m_footerRow = new QHBoxLayout;
    m_footerRow->setContentsMargins(0, 0, 0, 0);
    m_footerRow->addStretch(1);
    m_footerRow->addWidget(m_footer);
    m_footerRow->addStretch(1);
    m_column->addLayout(m_footerRow);
    m_footer->show();
    applyRhythm();
}

bool EmptyState::eventFilter(QObject* watched, QEvent* event)
{
    // A caller that put a button in actions() gets the row back without having
    // to say so a second time - and a caller that only asked for the layout
    // and added nothing is not charged its margin.
    if (watched == m_actions
        && (event->type() == QEvent::ChildAdded || event->type() == QEvent::ChildRemoved)) {
        // Reparenting emits ChildAdded before QLayout inserts its item.
        QTimer::singleShot(0,this,[this]{m_actions->setVisible(actions() && actions()->count() > 0);});
    }
    return QWidget::eventFilter(watched, event);
}

InfoBanner::InfoBanner(const QString& text, Badge::Tone tone, QWidget* parent,
                       QWidget* trailing)
    : QFrame(parent)
    , m_tone(tone)
{
    // Painted, not styled - see the header for why.
    setAttribute(Qt::WA_StyledBackground, false);

    // A banner is exactly as wide as it is given. Its sentence must never be
    // what decides that - a long one wraps, it does not push.
    //
    // This is not a detail. The gallery hands this component a linker error,
    // which is one unbroken four-hundred-character line, and a banner whose
    // width followed its text dragged the whole window off the screen. Any
    // component that reports a failure will one day be handed a message
    // nobody budgeted for, so not growing is part of what a banner *is*.
    //
    // Vertically it is the opposite: `.note` is a block with
    // `line-height:1.55` and no height of its own, so the sentence decides.
    // The strip used to be one control-height line with the rest elided,
    // which threw away the half of the message that said what to do.
    QSizePolicy policy(QSizePolicy::Ignored, QSizePolicy::Minimum);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);

    // Through the setter, not the member: the tooltip that carries the full
    // message is set there, and assigning m_text directly meant a banner given
    // its text at construction - which is all of them - silently had none.
    setText(text);

    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(kBannerPaddingH, kBannerPaddingV, kBannerPaddingH,
                            kBannerPaddingV);
    row->setSpacing(kBannerGap);
    // The sentence is painted, not laid out: it is prose nobody interacts
    // with, and a QLabel for it would be a second widget whose own sizeHint is
    // the thing that has to be fought. The stretch reserves its lane.
    row->addStretch(1);

    if (trailing) {
        trailing->setParent(this);
        // `.note .btn{align-self:center}`.
        row->addWidget(trailing, 0, Qt::AlignVCenter);
    }
}

// `.note svg` - the glyph says which kind of note this is, and it is the only
// part of one that carries the tone. Derived from the tone unless a caller
// states one, because the two always agree in the design.
Glyph InfoBanner::glyphForTone(Badge::Tone tone)
{
    switch (tone) {
    case Badge::Tone::Warning:
    case Badge::Tone::Danger:  return Glyph::Warning;
    case Badge::Tone::Success: return Glyph::Check;
    case Badge::Tone::Info:
    case Badge::Tone::Neutral:
    case Badge::Tone::Accent:  break;
    }
    return Glyph::Info;
}

void InfoBanner::setGlyph(Glyph glyph)
{
    m_glyphFromTone = false;
    if (m_glyph == glyph) {
        return;
    }
    m_glyph = glyph;
    syncHeight();
    updateGeometry();
    update();
}

int InfoBanner::textLaneWidth(int width) const
{
    int lane = width - 2 * kBannerPaddingH;
    const Glyph shown = m_glyphFromTone ? glyphForTone(m_tone) : m_glyph;
    if (shown != Glyph::None) {
        lane -= kBannerGlyph + kBannerGap;
    }
    // Whatever a caller put at the trailing edge takes its own lane, so the
    // prose wraps against the button rather than under it.
    for (QObject* child : children()) {
        // Geometry is commonly requested before the banner's parent is shown.
        // isVisible() is false in that state even for a trailing action that
        // will participate in the layout, so it made the initial text lane
        // too wide and the banner two pixels too short. Only an explicitly
        // hidden child is absent from the composition.
        if (auto* widget = qobject_cast<QWidget*>(child); widget && !widget->isHidden()) {
            lane -= widget->sizeHint().width() + kBannerGap;
        }
    }
    return qMax(kMinimumWidth / 2, lane);
}

void InfoBanner::rebuildDocument()
{
    if (!m_document) {
        m_document = std::make_unique<QTextDocument>();
        m_document->setDocumentMargin(0);
    }
    const ThemeProfile& profile = themeFor(this);
    QFont font = roleFont(this, TypeRole::Caption);
    m_document->setDefaultFont(font);
    // `.note{color:var(--text2);line-height:1.55}` with `.note b{color:
    // var(--text);font-weight:600}`. The prose is *not* the tone colour: a
    // whole sentence in amber reads as a warning about itself, and the design
    // tints only the plate, the outline and the glyph.
    m_document->setDefaultStyleSheet(
        QStringLiteral("body{color:%1;} b{color:%2;font-weight:600;}")
            .arg(profile.colors.textSecondary.name(), profile.colors.textPrimary.name()));

    // line-height:1.55 is room a *wrapped* sentence needs between its own
    // lines - it is not room one line needs above or below it. Applied
    // unconditionally, a one-line banner (the common case) measured taller
    // than its sentence, and because a text layout hangs that extra leading
    // mostly under the baseline rather than splitting it evenly, the strip
    // read as more padding below the words than above them even though
    // kBannerPaddingV is the same number on both edges. So: lay the sentence
    // out at its own natural height first, and only reach for the taller
    // line-height once there is more than one line for it to space out.
    m_document->setHtml(QStringLiteral("<body>%1</body>").arg(m_text));
    m_document->setTextWidth(textLaneWidth(width()));
    m_document->documentLayout()->documentSize();   // forces the wrap
    if (m_document->firstBlock().layout()->lineCount() > 1) {
        m_document->setHtml(QStringLiteral("<body style=\"line-height:155%\">%1</body>")
                                .arg(m_text));
        m_document->setTextWidth(textLaneWidth(width()));
    }
}

QSize InfoBanner::sizeHint() const
{
    // A preferred width, not a measured one.
    return QSize(kPreferredWidth, heightForWidth(kPreferredWidth));
}

QSize InfoBanner::minimumSizeHint() const
{
    // The height it *has*, not the height it would need at 120px. syncHeight()
    // has already fixed the height to the width the banner was given, and a
    // minimum computed from the narrowest allowed width reported a strip four
    // hundred pixels tall that every clipping check then flagged as cut off.
    return QSize(kMinimumWidth, height() > 0 ? height() : heightForWidth(kPreferredWidth));
}

int InfoBanner::heightForWidth(int width) const
{
    const_cast<InfoBanner*>(this)->rebuildDocument();
    m_document->setTextWidth(textLaneWidth(width));
    int inner = qCeil(m_document->size().height());
    for (QObject* child : children()) {
        if (auto* widget = qobject_cast<QWidget*>(child); widget && !widget->isHidden()) {
            inner = qMax(inner, widget->sizeHint().height());
        }
    }
    const Glyph shown = m_glyphFromTone ? glyphForTone(m_tone) : m_glyph;
    if (shown != Glyph::None) {
        inner = qMax(inner, kBannerGlyph);
    }
    return inner + 2 * kBannerPaddingV;
}

void InfoBanner::resizeEvent(QResizeEvent* event)
{
    QFrame::resizeEvent(event);
    syncHeight();
}

// The height, set rather than merely offered.
//
// heightForWidth() alone is not enough here: the banner's horizontal policy is
// Ignored - it must never let its sentence decide its width - and a layout
// that is told to ignore an item's width hint does not reliably ask that item
// how tall it would be at the width it ended up with. The banner came out
// 1256x0: the right width, and no height at all. So the height is computed
// from the width it actually has and fixed, which is what the strip version of
// this component did before it learned to wrap.
void InfoBanner::syncHeight()
{
    rebuildDocument();
    const int wanted = heightForWidth(width());
    if (minimumHeight() != wanted || maximumHeight() != wanted) {
        setFixedHeight(wanted);
    }
}

void InfoBanner::setText(const QString& text)
{
    if (m_text == text) {
        return;
    }
    m_text = text;
    // The full sentence stays reachable from the tooltip, with the markup
    // taken back out of it.
    QString plain = text;
    plain.remove(QRegularExpression(QStringLiteral("<[^>]*>")));
    setToolTip(plain);
    syncHeight();
    updateGeometry();
    update();
}

void InfoBanner::setTone(Badge::Tone tone)
{
    if (m_tone == tone) {
        return;
    }
    m_tone = tone;
    rebuildDocument();
    update();
}

void InfoBanner::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const QColor tone = toneColor(this, m_tone);
    // `.note{border-radius:11px}` - stated, not borrowed from a control role.
    // A note is a block of prose the width of the page, and the button radius
    // it used to take made a short one read as a lozenge.
    const int radius = kBannerRadius;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath path;
    path.addRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);

    // The design's own two numbers per tone: an info note is the faintest
    // (.07/.25) and a bad one the strongest (.10/.35), because a failure has
    // to be seen from across the page and a hint must not be.
    const double fillAlpha = m_tone == Badge::Tone::Danger ? 0.10
                           : m_tone == Badge::Tone::Info   ? 0.07
                                                           : 0.08;
    const double edgeAlpha = m_tone == Badge::Tone::Danger ? 0.35
                           : m_tone == Badge::Tone::Info   ? 0.25
                                                           : 0.30;
    QColor fill = tone;
    fill.setAlphaF(static_cast<float>(fillAlpha));
    p.fillPath(path, fill);

    QColor outline = tone;
    outline.setAlphaF(static_cast<float>(edgeAlpha));
    p.setPen(QPen(outline, 1));
    p.drawPath(path);

    int left = kBannerPaddingH;
    const Glyph shown = m_glyphFromTone ? glyphForTone(m_tone) : m_glyph;
    if (shown != Glyph::None) {
        // `margin-top:2px` - the glyph sits on the first line's cap height
        // rather than on the middle of a paragraph it may be three lines of.
        paintGlyph(&p, shown,
                   QRectF(left, kBannerPaddingV + 2, kBannerGlyph, kBannerGlyph), tone);
        left += kBannerGlyph + kBannerGap;
    }

    if (m_text.isEmpty()) {
        return;
    }
    const_cast<InfoBanner*>(this)->rebuildDocument();
    p.save();
    p.translate(left, kBannerPaddingV);
    QAbstractTextDocumentLayout::PaintContext context;
    context.palette.setColor(QPalette::Text, profile.colors.textSecondary);
    m_document->documentLayout()->draw(&p, context);
    p.restore();
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
    spec.background = m_trackBackground;
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
    return resolve(styleSpec(), this, m_trackHeight);
}

QSize ProgressBar::sizeHint() const
{
    int height = m_trackHeight;
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

    const QRectF track(0, 0, width(), m_trackHeight);
    painter.setBrush(m_trackBackground.resolve(profile));
    painter.drawRoundedRect(track, m_trackRadius, m_trackRadius);

    // Unknown draws the track and stops. "Not measured" and "idle" must not
    // look the same, and a zero-length fill is what makes them.
    if (!m_unknown && m_value > 0) {
        const QColor fill = m_tone == Tone::Accent
            ? profile.colors.accent
            : profile.semantic.role(effectiveRole()).text;
        painter.setBrush(fill);
        painter.drawRoundedRect(QRectF(0, 0, track.width() * m_value / 100.0, m_trackHeight),
                                m_trackRadius, m_trackRadius);
    }

    if (m_leading.isEmpty() && m_trailing.isEmpty()) {
        return;
    }

    QFont font = roleFont(this, TypeRole::Caption);
    font.setPixelSize(profile.type.xs);
    const QFontMetrics fm(font);
    painter.setFont(font);

    const QRect legend(0, m_trackHeight + kLegendGap, width(), fm.height());
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
