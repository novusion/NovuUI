#include "novu/ui/PageHeader.h"

#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QPainter>
#include <QPen>
#include <QResizeEvent>
#include <QTextOption>

namespace novu::ui {

namespace {

// `.crumb { gap:6px; letter-spacing:.05em }`, with the separator at .45 opacity.
constexpr int kCrumbGap = 6;
constexpr double kSeparatorOpacity = 0.45;
// `<span class="sep">&rsaquo;</span>` - a single right-pointing angle
// quotation mark, not a slash. A slash reads as a path on disk; the
// approved design points the way through the application instead.
const QChar kSeparator = QChar(0x203A);

// `.page-head { gap:14px; padding:10px 20px }`.
constexpr int kHeaderPaddingH = 20;
constexpr int kHeaderPaddingV = 10;
constexpr int kHeaderGap = 14;

// `.page-head .ttl { gap:10px }` between the trail and the title, and the gap
// down to the description.
// `.ttl{gap:10px}` - between the trail, the title and the description when
// the three of them share one row.
constexpr int kInlineGap = 10;
// The second line, when the description has to take one.
constexpr int kInlineWrapGap = 4;

constexpr int kTrailGap = 4;
constexpr int kDescriptionGap = 3;

// `.page-head .desc { line-height:1.4 }`.
constexpr double kDescriptionLeading = 1.4;

QFont crumbFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleMonoFont(context, TypeRole::Eyebrow);
    font.setPixelSize(qMax(8, profile.type.xxs));
    font.setLetterSpacing(QFont::PercentageSpacing, 105);
    return font;
}

QFont titleFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::PageTitle, true);
    font.setPixelSize(profile.type.lg);
    // `letter-spacing:-.015em` - a title set tight, which is the one place the
    // design tracks *in* rather than out.
    font.setLetterSpacing(QFont::PercentageSpacing, 98.5);
    return font;
}

QFont descriptionFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::Caption);
    font.setPixelSize(profile.type.sm);
    return font;
}

int wrappedHeight(const QFontMetrics& fm, const QString& text, int width, double leading)
{
    if (text.isEmpty() || width <= 0) {
        return 0;
    }
    const QRect bounds = fm.boundingRect(QRect(0, 0, width, 10000),
                                         Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
                                         text);
    const int lines = qMax(1, qRound(double(bounds.height()) / qMax(1, fm.height())));
    return qRound(fm.height() * leading) * lines;
}

} // namespace

// == Breadcrumb =============================================================

Breadcrumb::Breadcrumb(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

Breadcrumb::Breadcrumb(const QStringList& path, QWidget* parent)
    : Breadcrumb(parent)
{
    m_path = path;
}

void Breadcrumb::setPath(const QStringList& path)
{
    if (m_path == path) {
        return;
    }
    m_path = path;
    updateGeometry();
    update();
}

QStringList Breadcrumb::visiblePath() const
{
    // `.crumb .cur { display:none }` — the last segment is the page title,
    // which is already on screen next to this.
    return m_path.size() <= 1 ? QStringList() : m_path.mid(0, m_path.size() - 1);
}

QSize Breadcrumb::sizeHint() const
{
    const QStringList visible = visiblePath();
    if (visible.isEmpty()) {
        return QSize(0, 0);
    }
    const QFontMetrics fm(crumbFont(this));
    int width = 0;
    for (const QString& segment : visible) {
        width += fm.horizontalAdvance(segment.toUpper()) + kCrumbGap
            + fm.horizontalAdvance(kSeparator) + kCrumbGap;
    }
    return QSize(width, fm.height());
}

QSize Breadcrumb::minimumSizeHint() const
{
    return QSize(0, sizeHint().height());
}

bool Breadcrumb::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        updateGeometry();
        update();
    }
    return QWidget::event(event);
}

void Breadcrumb::paintEvent(QPaintEvent*)
{
    const QStringList visible = visiblePath();
    if (visible.isEmpty()) {
        return;
    }
    const ThemeProfile& profile = themeFor(this);
    const QFont font = crumbFont(this);
    const QFontMetrics fm(font);

    QPainter painter(this);
    painter.setFont(font);

    int x = 0;
    for (int i = 0; i < visible.size(); ++i) {
        const QString segment = visible.at(i).toUpper();
        const int segmentWidth = fm.horizontalAdvance(segment);
        painter.setPen(profile.colors.textDisabled);
        painter.drawText(QRect(x, 0, segmentWidth, height()),
                         Qt::AlignLeft | Qt::AlignVCenter, segment);
        x += segmentWidth;

        // `.crumb .sep:last-child { display:none }` — no trailing slash.
        if (i + 1 >= visible.size()) {
            break;
        }
        x += kCrumbGap;
        QColor faded = profile.colors.textDisabled;
        faded.setAlphaF(faded.alphaF() * kSeparatorOpacity);
        painter.setPen(faded);
        const int sepWidth = fm.horizontalAdvance(kSeparator);
        painter.drawText(QRect(x, 0, sepWidth, height()),
                         Qt::AlignLeft | Qt::AlignVCenter, QString(kSeparator));
        x += sepWidth + kCrumbGap;
    }
}

// == PageHeader =============================================================

PageHeader::PageHeader(const QString& title, QWidget* parent)
    : QWidget(parent)
    , m_title(title)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    m_actions = new QWidget(this);
    auto* layout = new QHBoxLayout(m_actions);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(themeFor(this).space(SpacingRole::ToolbarGap));
    m_actions->setVisible(false);
}

PageHeader::PageHeader(const QString& title, const QString& description, QWidget* parent)
    : PageHeader(title, parent)
{
    m_description = description;
}

void PageHeader::setTitle(const QString& title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    update();
}

void PageHeader::setDescription(const QString& description)
{
    if (m_description == description) {
        return;
    }
    m_description = description;
    updateGeometry();
    relayout();
    update();
}

void PageHeader::setPath(const QStringList& path)
{
    if (m_path == path) {
        return;
    }
    m_path = path;
    updateGeometry();
    relayout();
    update();
}

void PageHeader::setEyebrowWidget(QWidget* widget)
{
    if (m_eyebrow == widget) return;
    if (m_eyebrow) m_eyebrow->deleteLater();
    m_eyebrow = widget;
    if (m_eyebrow) { m_eyebrow->setParent(this); m_eyebrow->show(); }
    updateGeometry(); relayout(); update();
}

QHBoxLayout* PageHeader::actions() const
{
    return qobject_cast<QHBoxLayout*>(m_actions->layout());
}

void PageHeader::setDivider(bool divider)
{
    if (m_divider == divider) {
        return;
    }
    m_divider = divider;
    update();
}

void PageHeader::setLayout(Layout layout)
{
    if (m_layout == layout) {
        return;
    }
    m_layout = layout;
    updateGeometry();
    relayout();
    update();
}

int PageHeader::textWidth(int width) const
{
    int available = width - 2 * kHeaderPaddingH;
    if (actions()->count() > 0) {
        available -= m_actions->sizeHint().width() + kHeaderGap;
    }
    return qMax(0, available);
}

int PageHeader::textHeight(int width) const
{
    const QFontMetrics titles(titleFont(this));

    // One baseline, so the band is as tall as the tallest of the three and
    // nothing else: `.page-head` is `padding:10px 20px` around a single row.
    //
    // `.ttl{flex-wrap:wrap}`, though: when the description does not fit beside
    // the title it takes a line of its own rather than being cut, and the band
    // grows by that line. A heading that elides its own explanation explains
    // nothing.
    if (m_layout == Layout::Inline) {
        int height = titles.height();
        if (m_path.size() > 1) {
            height = qMax(height, QFontMetrics(crumbFont(this)).height());
        }
        if (!m_description.isEmpty()) {
            const QFontMetrics descriptions(descriptionFont(this));
            height = qMax(height, descriptions.height());
            if (inlineDescriptionWraps(width)) {
                height += kInlineWrapGap + descriptions.height();
            }
        }
        return height;
    }

    int height = titles.height();
    if (m_eyebrow) height += m_eyebrow->sizeHint().height() + kTrailGap;

    // The same rule Breadcrumb applies: a one-segment path is entirely the
    // title, so there is no trail and no line for one.
    if (m_path.size() > 1) {
        height += QFontMetrics(crumbFont(this)).height() + kTrailGap;
    }
    if (!m_description.isEmpty()) {
        height += kDescriptionGap
            + wrappedHeight(QFontMetrics(descriptionFont(this)), m_description,
                            textWidth(width), kDescriptionLeading);
    }
    return height;
}

int PageHeader::heightForWidth(int width) const
{
    const int content = qMax(textHeight(width),
                             actions()->count() > 0 ? m_actions->sizeHint().height() : 0);
    return content + 2 * kHeaderPaddingV;
}

QSize PageHeader::sizeHint() const
{
    const QFontMetrics titles(titleFont(this));
    int width = 2 * kHeaderPaddingH + titles.horizontalAdvance(m_title);
    if (m_eyebrow) width = qMax(width, 2 * kHeaderPaddingH + m_eyebrow->sizeHint().width());
    if (actions()->count() > 0) {
        width += kHeaderGap + m_actions->sizeHint().width();
    }
    return QSize(width, heightForWidth(qMax(width, 320)));
}

QSize PageHeader::minimumSizeHint() const
{
    // The floor is the header with nothing wrapped. A heightForWidth widget
    // whose minimum reports its *narrow-case* height carries that height
    // around forever - a layout will not give it less - so a one-row inline
    // header would claim the three rows it needs at 220px and move the whole
    // page down behind it.
    const QFontMetrics titles(titleFont(this));
    int floor = titles.height();
    if (m_layout == Layout::Stacked) {
        if (m_eyebrow) floor += kTrailGap + m_eyebrow->sizeHint().height();
        if (!m_path.isEmpty()) {
            floor += kHeaderGap + QFontMetrics(roleFont(this, TypeRole::Eyebrow)).height();
        }
        if (!m_description.isEmpty()) {
            floor += kHeaderGap
                + QFontMetrics(roleFont(this, TypeRole::Caption)).height();
        }
    }
    if (actions()->count() > 0) {
        floor = qMax(floor, m_actions->sizeHint().height());
    }
    return QSize(220, floor + 2 * kHeaderPaddingV);
}

void PageHeader::relayout()
{
    if (m_eyebrow) {
        const QSize hint = m_eyebrow->sizeHint();
        const int top = kHeaderPaddingV + (height() - 2 * kHeaderPaddingV - textHeight(width())) / 2;
        m_eyebrow->setGeometry(kHeaderPaddingH, top, hint.width(), hint.height());
    }
    const bool any = actions()->count() > 0;
    m_actions->setVisible(any);
    if (!any) {
        return;
    }
    const QSize hint = m_actions->sizeHint();
    m_actions->setGeometry(width() - kHeaderPaddingH - hint.width(),
                           (height() - hint.height()) / 2,
                           hint.width(), hint.height());
}

void PageHeader::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    relayout();
}

bool PageHeader::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange
        || event->type() == QEvent::LayoutRequest) {
        updateGeometry();
        relayout();
        update();
    }
    return QWidget::event(event);
}

StyleSpec PageHeader::styleSpec() const
{
    StyleSpec spec;
    // No plate: a page header sits on the page, which is the design's
    // `.content { background:var(--window) }`. Only the rule is drawn.
    spec.background = token(ColorRole::Transparent);
    spec.border = token(ColorRole::BorderSubtle);
    spec.foreground = token(ColorRole::TextPrimary);
    spec.radius = SurfaceRole::Toolbar;
    spec.borderWidth = BorderRole::Divider;
    spec.type = TypeRole::PageTitle;
    return spec;
}

ResolvedStyle PageHeader::resolvedStyle() const
{
    return resolve(styleSpec(), this, height());
}

// Does the description still fit beside the title at this width?
//
// The same question `flex-wrap` asks, answered once so heightForWidth() and
// the paint pass cannot disagree about how tall the band is.
bool PageHeader::inlineDescriptionWraps(int width) const
{
    if (m_layout != Layout::Inline || m_description.isEmpty()) {
        return false;
    }
    const QFontMetrics crumbs(crumbFont(this));
    const QFontMetrics titles(titleFont(this));
    const QFontMetrics descriptions(descriptionFont(this));

    int used = 0;
    const QStringList visible =
        m_path.size() <= 1 ? QStringList() : m_path.mid(0, m_path.size() - 1);
    for (int i = 0; i < visible.size(); ++i) {
        used += crumbs.horizontalAdvance(visible.at(i).toUpper());
        if (i + 1 < visible.size()) {
            used += 2 * kCrumbGap + crumbs.horizontalAdvance(kSeparator);
        }
    }
    if (!visible.isEmpty()) {
        used += kInlineGap;
    }
    used += titles.horizontalAdvance(m_title) + kInlineGap;
    return used + descriptions.horizontalAdvance(m_description) > textWidth(width);
}

void PageHeader::paintInline(QPainter* painter, const ThemeProfile& profile, int top,
                             int available) const
{
    const QFont crumbs = crumbFont(this);
    const QFont titles = titleFont(this);
    const QFont descriptions = descriptionFont(this);
    const QFontMetrics crumbMetrics(crumbs);
    const QFontMetrics titleMetrics(titles);
    const QFontMetrics descriptionMetrics(descriptions);

    // `align-items:baseline`: one baseline for all three, placed by whichever
    // of them reaches highest above it.
    int ascent = titleMetrics.ascent();
    const QStringList visible =
        m_path.size() <= 1 ? QStringList() : m_path.mid(0, m_path.size() - 1);
    if (!visible.isEmpty()) {
        ascent = qMax(ascent, crumbMetrics.ascent());
    }
    if (!m_description.isEmpty()) {
        ascent = qMax(ascent, descriptionMetrics.ascent());
    }
    const int baseline = top + ascent;

    const int limit = kHeaderPaddingH + available;
    int x = kHeaderPaddingH;

    if (!visible.isEmpty()) {
        painter->setFont(crumbs);
        for (int i = 0; i < visible.size(); ++i) {
            const QString segment = visible.at(i).toUpper();
            painter->setPen(profile.colors.textDisabled);
            painter->drawText(QPointF(x, baseline), segment);
            x += crumbMetrics.horizontalAdvance(segment);
            if (i + 1 >= visible.size()) {
                break;
            }
            x += kCrumbGap;
            QColor faded = profile.colors.textDisabled;
            faded.setAlphaF(faded.alphaF() * kSeparatorOpacity);
            painter->setPen(faded);
            painter->drawText(QPointF(x, baseline), QString(kSeparator));
            x += crumbMetrics.horizontalAdvance(kSeparator) + kCrumbGap;
        }
        x += kInlineGap;
    }

    painter->setFont(titles);
    painter->setPen(profile.colors.textPrimary);
    const QString title =
        titleMetrics.elidedText(m_title, Qt::ElideRight, qMax(0, limit - x));
    painter->drawText(QPointF(x, baseline), title);
    x += titleMetrics.horizontalAdvance(title);

    if (m_description.isEmpty()) {
        return;
    }
    painter->setFont(descriptions);
    painter->setPen(profile.colors.textDisabled);
    if (inlineDescriptionWraps(width())) {
        // Its own line, at the text origin, `.ttl{gap:10px}` under the title.
        const int second = baseline + kInlineWrapGap + descriptionMetrics.height();
        painter->drawText(QPointF(kHeaderPaddingH, second),
                          descriptionMetrics.elidedText(m_description, Qt::ElideRight,
                                                        available));
        return;
    }
    x += kInlineGap;
    painter->drawText(QPointF(x, baseline),
                      descriptionMetrics.elidedText(m_description, Qt::ElideRight,
                                                    qMax(0, limit - x)));
}

void PageHeader::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const int available = textWidth(width());

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (m_divider) {
        const int borderWidth = qMax(1, profile.border(BorderRole::Divider));
        // `.page-head{border-bottom:1px solid var(--hair)}` - the rule the
        // design draws, not the subtler one a card uses inside itself.
        painter.setPen(QPen(profile.colors.border, borderWidth));
        paintHairline(&painter, QPoint(0, height() - borderWidth),
                      QPoint(width() - 1, height() - borderWidth),
                      profile.colors.border, borderWidth);
    }

    int y = kHeaderPaddingV
        + (height() - 2 * kHeaderPaddingV - textHeight(width())) / 2;

    if (m_layout == Layout::Inline) {
        paintInline(&painter, profile, y, available);
        return;
    }

    if (m_eyebrow) y += m_eyebrow->sizeHint().height() + kTrailGap;

    // The trail. Drawn here rather than by a child Breadcrumb: the whole point
    // of painting the header is that it is one widget, and a trail plus a title
    // plus a description as three labels is the shape this replaces.
    const QStringList visible =
        m_path.size() <= 1 ? QStringList() : m_path.mid(0, m_path.size() - 1);
    if (!visible.isEmpty()) {
        const QFont font = crumbFont(this);
        const QFontMetrics fm(font);
        painter.setFont(font);

        int x = kHeaderPaddingH;
        for (int i = 0; i < visible.size(); ++i) {
            const QString segment = visible.at(i).toUpper();
            const int segmentWidth = fm.horizontalAdvance(segment);
            painter.setPen(profile.colors.textDisabled);
            painter.drawText(QRect(x, y, segmentWidth, fm.height()),
                             Qt::AlignLeft | Qt::AlignVCenter, segment);
            x += segmentWidth;
            if (i + 1 >= visible.size()) {
                break;
            }
            x += kCrumbGap;
            QColor faded = profile.colors.textDisabled;
            faded.setAlphaF(faded.alphaF() * kSeparatorOpacity);
            painter.setPen(faded);
            const int sepWidth = fm.horizontalAdvance(kSeparator);
            painter.drawText(QRect(x, y, sepWidth, fm.height()),
                             Qt::AlignLeft | Qt::AlignVCenter, QString(kSeparator));
            x += sepWidth + kCrumbGap;
        }
        y += fm.height() + kTrailGap;
    }

    const QFont titles = titleFont(this);
    const QFontMetrics titleMetrics(titles);
    painter.setFont(titles);
    painter.setPen(profile.colors.textPrimary);
    painter.drawText(QRect(kHeaderPaddingH, y, available, titleMetrics.height()),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     titleMetrics.elidedText(m_title, Qt::ElideRight, available));
    y += titleMetrics.height();

    if (m_description.isEmpty()) {
        return;
    }
    y += kDescriptionGap;
    const QFont descriptions = descriptionFont(this);
    const QFontMetrics descriptionMetrics(descriptions);
    QTextOption wrap(Qt::AlignLeft | Qt::AlignTop);
    wrap.setWrapMode(QTextOption::WordWrap);
    painter.setFont(descriptions);
    painter.setPen(profile.colors.textDisabled);
    painter.drawText(QRectF(kHeaderPaddingH, y, available,
                            wrappedHeight(descriptionMetrics, m_description, available,
                                          kDescriptionLeading)),
                     m_description, wrap);
}

} // namespace novu::ui
