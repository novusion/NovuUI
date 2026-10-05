#include "novu/ui/StatusBar.h"

#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QPainter>
#include <QPen>
#include <QRadialGradient>
#include <QResizeEvent>

#include <utility>

namespace novu::ui {

namespace {

// `.statusbar { gap:10px; padding:0 16px }`. The height is a profile token -
// the design states 32 for NovuSpace, 26 for NovuAI and 24 for NovuExplorer,
// and a constant here meant the other two could not have theirs.
constexpr int kGap = 10;
constexpr int kPaddingH = 16;

// `.statusbar .sdot { width:7px }` and the gap to its word.
constexpr int kDotDiameter = 7;
constexpr int kEntryGlyph = 12;
constexpr int kDotGap = 5;
// The glow, as a multiple of the dot. `box-shadow:0 0 6px` on a 7px dot.
constexpr double kGlowScale = 2.7;
// The label sits clear of the glow's own radius, not just the dot's - a
// fixed kDotGap left the fade's tail touching the first letter.
constexpr int kStatusLabelGap = 9;

// The divider is a character rather than a rule in the design - a thin
// separator glyph in the hairline colour.
const QChar kSeparatorGlyph = QChar(0x2502);   // BOX DRAWINGS LIGHT VERTICAL

QFont barFont(const QWidget* context)
{
    // The strip's own role. It falls back to Body, which is what every
    // profile that has not stated one gets, so this reads the same as before
    // until a design says otherwise.
    return roleFont(context, TypeRole::StatusBar);
}

QFont valueFont(const QWidget* context)
{
    QFont font = barFont(context);
    font.setWeight(QFont::Medium);
    return font;
}

QFont monoFont(const QWidget* context)
{
    // Monospace at the strip's own size, not at the Mono role's: a mono run
    // beside a proportional one in the same strip has to sit on the same line.
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleMonoFont(context, TypeRole::StatusBar);
    font.setPointSizeF(qMax(8.0, profile.text(TypeRole::StatusBar).size - 1.0) * 72.0 / context->logicalDpiY());
    Q_UNUSED(profile);
    return font;
}

SemanticRole roleFor(StatusDot::Tone tone)
{
    switch (tone) {
    case StatusDot::Tone::Success: return SemanticRole::Success;
    case StatusDot::Tone::Warning: return SemanticRole::Warning;
    case StatusDot::Tone::Danger:  return SemanticRole::Danger;
    case StatusDot::Tone::Info:    return SemanticRole::Info;
    case StatusDot::Tone::Accent:
    case StatusDot::Tone::Neutral: break;
    }
    return SemanticRole::Neutral;
}

QColor toneColor(const QWidget* context, StatusDot::Tone tone)
{
    const ThemeProfile& profile = themeFor(context);
    if (tone == StatusDot::Tone::Accent) {
        return profile.colors.accentLight;
    }
    return profile.semantic.role(roleFor(tone)).text;
}

} // namespace

StatusBar::StatusBar(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(themeFor(this).window.statusBarHeight);
}

int StatusBar::addText(const QString& text, Slot slot)
{
    Entry entry;
    entry.handle = m_nextHandle++;
    entry.kind = Item::Text;
    entry.slot = slot;
    entry.text = text;
    m_items.append(entry);
    update();
    return entry.handle;
}

int StatusBar::addValue(const QString& text, Slot slot)
{
    const int handle = addText(text, slot);
    m_items.last().kind = Item::Value;
    return handle;
}

int StatusBar::addMono(const QString& text, Slot slot)
{
    const int handle = addText(text, slot);
    m_items.last().kind = Item::Mono;
    return handle;
}

int StatusBar::addStatus(const QString& text, StatusDot::Tone tone, Slot slot)
{
    const int handle = addText(text, slot);
    m_items.last().kind = Item::Status;
    m_items.last().tone = tone;
    return handle;
}

void StatusBar::setItemGlyph(int id, Glyph glyph)
{
    Entry* entry = find(id);
    if (!entry || entry->glyph == glyph) {
        return;
    }
    entry->glyph = glyph;
    updateGeometry();
    update();
}

int StatusBar::addSeparator(Slot slot)
{
    const int handle = addText(QString(), slot);
    m_items.last().kind = Item::Separator;
    return handle;
}

const StatusBar::Entry* StatusBar::find(int handle) const
{
    for (const Entry& entry : m_items) {
        if (entry.handle == handle) {
            return &entry;
        }
    }
    return nullptr;
}

StatusBar::Entry* StatusBar::find(int handle)
{
    return const_cast<Entry*>(std::as_const(*this).find(handle));
}

void StatusBar::setItemText(int handle, const QString& text)
{
    Entry* entry = find(handle);
    if (!entry || entry->kind == Item::Separator || entry->text == text) {
        return;
    }
    entry->text = text;
    update();
}

QString StatusBar::itemText(int handle) const
{
    const Entry* entry = find(handle);
    return entry ? entry->text : QString();
}

void StatusBar::setItemTone(int handle, StatusDot::Tone tone)
{
    Entry* entry = find(handle);
    if (!entry || entry->tone == tone) {
        return;
    }
    entry->tone = tone;
    update();
}

void StatusBar::setItemVisible(int handle, bool visible)
{
    Entry* entry = find(handle);
    if (!entry || entry->visible == visible) {
        return;
    }
    entry->visible = visible;
    update();
}

bool StatusBar::isItemVisible(int handle) const
{
    const Entry* entry = find(handle);
    return entry && entry->visible;
}

void StatusBar::clear()
{
    m_items.clear();
    update();
}

QRect StatusBar::itemRect(int handle) const
{
    const Entry* entry = find(handle);
    return entry ? entry->placed : QRect();
}

void StatusBar::setOffline(bool offline)
{
    if (m_offline == offline) {
        return;
    }
    m_offline = offline;
    update();
}

void StatusBar::setTrailingWidget(QWidget* widget)
{
    if (m_trailing == widget) {
        return;
    }
    if (m_trailing) {
        m_trailing->setParent(nullptr);
        m_trailing->deleteLater();
    }
    m_trailing = widget;
    if (m_trailing) {
        m_trailing->setParent(this);
        m_trailing->show();
    }
    layoutTrailing();
    // The leading host is bounded by what the trailing one leaves, so it has
    // to be measured again whenever that changes.
    layoutLeading();
    update();
}

void StatusBar::setLeadingWidget(QWidget* widget)
{
    if (m_leading == widget) {
        return;
    }
    if (m_leading) {
        m_leading->setParent(nullptr);
        m_leading->deleteLater();
    }
    m_leading = widget;
    if (m_leading) {
        m_leading->setParent(this);
        m_leading->show();
    }
    layoutLeading();
    update();
}

void StatusBar::layoutLeading()
{
    if (!m_leading) {
        return;
    }
    const QSize hint = m_leading->sizeHint();
    // Embedded controls may be almost as tall as the strip itself (NovuAI's
    // 23 px compact SelectButton inside its 26 px status bar).  Reserving
    // three pixels on both sides reduced the host to 20 px and clipped the
    // child's rounded bottom border.  The embedded host gets the strip's full
    // height when needed; each compact control keeps its own stated height and
    // is centred by the host layout.
    const int widgetHeight = qMin(hint.height(), height());
    // The width it asks for, bounded by the width there is.
    //
    // A size hint is a request, and this one was granted whatever it said: a
    // leading host that grew - a model picker, then Load and Unload, then a
    // progress bar beside them - pushed the strip's own readings right until
    // they ran into the trailing ones, and two sets of text were drawn over
    // each other. The strip's items are painted, so neither could clip the
    // other and the collision was simply visible.
    //
    // Room for the trailing widget is reserved here; the entries themselves
    // then take what is left, and the ones that do not fit are dropped rather
    // than overprinted (see paintEvent).
    int available = width() - 2 * kPaddingH;
    if (m_trailing) {
        available -= m_trailing->sizeHint().width() + kGap;
    }
    m_leading->setGeometry(kPaddingH, (height() - widgetHeight) / 2,
                           qMax(0, qMin(hint.width(), available)), widgetHeight);
}

// The mark in front of a reading, and the box the text then gets. Returns the
// box unchanged when the entry carries no glyph, so the two cases are one line
// at the call site.
QRect StatusBar::paintEntryGlyph(QPainter* painter, const Entry& entry,
                                 const QRect& box, const QColor& ink) const
{
    if (entry.glyph == Glyph::None) {
        return box;
    }
    const QFont saved = painter->font();
    const QPen pen = painter->pen();
    paintGlyph(painter, entry.glyph,
               QRectF(box.left(), box.center().y() - kEntryGlyph / 2.0 + 0.5,
                      kEntryGlyph, kEntryGlyph),
               ink);
    painter->setFont(saved);
    painter->setPen(pen);
    return box.adjusted(kEntryGlyph + kDotGap, 0, 0, 0);
}

int StatusBar::entryWidth(const Entry& entry) const
{
    const int mark = entry.glyph == Glyph::None ? 0 : kEntryGlyph + kDotGap;
    switch (entry.kind) {
    case Item::Separator:
        return QFontMetrics(barFont(this)).horizontalAdvance(kSeparatorGlyph);
    case Item::Mono:
        return mark + QFontMetrics(monoFont(this)).horizontalAdvance(entry.text);
    case Item::Value:
        return mark + QFontMetrics(valueFont(this)).horizontalAdvance(entry.text);
    case Item::Status:
        return kDotDiameter + kStatusLabelGap
            + QFontMetrics(barFont(this)).horizontalAdvance(entry.text);
    case Item::Text:
        break;
    }
    return mark + QFontMetrics(barFont(this)).horizontalAdvance(entry.text);
}

void StatusBar::layoutTrailing()
{
    if (!m_trailing) {
        return;
    }
    const QSize hint = m_trailing->sizeHint();
    const int widgetHeight = qMin(hint.height(), height());
    m_trailing->setGeometry(width() - kPaddingH - hint.width(),
                            (height() - widgetHeight) / 2,
                            hint.width(), widgetHeight);
}

StyleSpec StatusBar::styleSpec() const
{
    StyleSpec spec;
    spec.background = token(ColorRole::HeaderBg);
    spec.foreground = token(ColorRole::TextDisabled);
    spec.border = token(ColorRole::Border);
    spec.radius = SurfaceRole::Toolbar;
    spec.borderWidth = BorderRole::Divider;
    spec.type = TypeRole::StatusBar;
    return spec;
}

ResolvedStyle StatusBar::resolvedStyle() const
{
    return resolve(styleSpec(), this, height());
}

QSize StatusBar::sizeHint() const
{
    int width = 2 * kPaddingH;
    int visible = 0;
    for (const Entry& entry : m_items) {
        if (!entry.visible) {
            continue;
        }
        width += entryWidth(entry);
        ++visible;
    }
    if (visible > 1) {
        width += kGap * (visible - 1);
    }
    if (m_trailing) {
        width += kGap + m_trailing->sizeHint().width();
    }
    if (m_leading) {
        width += kGap + m_leading->sizeHint().width();
    }
    return QSize(width, themeFor(this).window.statusBarHeight);
}

QSize StatusBar::minimumSizeHint() const
{
    return QSize(120, themeFor(this).window.statusBarHeight);
}

void StatusBar::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    layoutLeading();
    layoutTrailing();
}

bool StatusBar::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        updateGeometry();
        layoutLeading();
        layoutTrailing();
        update();
    }
    return QWidget::event(event);
}

void StatusBar::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // The design's `linear-gradient(180deg, titlebar-top, titlebar-bot)`. The
    // same two tokens the title bar uses, so the top and bottom of a window
    // match without either component knowing about the other.
    QLinearGradient ground(0, 0, 0, height());
    ground.setColorAt(0.0, profile.window.titleBarBg);
    ground.setColorAt(1.0, profile.window.titleBarBgBottom);
    painter.fillRect(rect(), ground);

    painter.setPen(QPen(profile.colors.border, qMax(1, profile.border(BorderRole::Divider))));
    paintHairline(&painter, QPoint(0, 0), QPoint(width() - 1, 0),
                  painter.pen().color(), qMax(1, painter.pen().width()));

    const QFont text = barFont(this);
    const QFont value = valueFont(this);
    const QFont mono = monoFont(this);

    int left = kPaddingH;
    if (m_leading) {
        left = m_leading->geometry().right() + kGap;
    }
    int right = width() - kPaddingH;
    if (m_trailing) {
        right = m_trailing->geometry().left() - kGap;
    }

    // Trailing items are placed from the right edge inwards, in reverse, so the
    // first one added is the one nearest the edge.
    for (int i = m_items.size() - 1; i >= 0; --i) {
        Entry& entry = m_items[i];
        if (entry.slot != Slot::Trailing) {
            continue;
        }
        if (!entry.visible) {
            entry.placed = QRect();
            continue;
        }
        const int own = entryWidth(entry);
        entry.placed = QRect(right - own, 0, own, height());
        right -= own + kGap;
    }

    for (Entry& entry : m_items) {
        if (entry.slot != Slot::Leading) {
            continue;
        }
        if (!entry.visible) {
            entry.placed = QRect();
            continue;
        }
        const int own = entryWidth(entry);
        // A reading that does not fit is not drawn. The two sets grow towards
        // each other - leading from the left, trailing from the right - and
        // nothing used to stop them meeting, so in a narrow strip the branch
        // and the bridge state were painted on top of one another. Dropping
        // the ones with no room left is the honest answer: a strip that shows
        // four of its six readings is readable, and one that shows all six
        // overlapping is not.
        if (left + own > right) {
            entry.placed = QRect();
            continue;
        }
        entry.placed = QRect(left, 0, own, height());
        left += own + kGap;
    }

    for (const Entry& entry : m_items) {
        if (!entry.visible || entry.placed.isNull() || entry.placed.width() <= 0) {
            continue;
        }
        const QRect box = entry.placed;

        switch (entry.kind) {
        case Item::Separator:
            painter.setFont(text);
            painter.setPen(profile.colors.border);
            painter.drawText(box, Qt::AlignCenter, QString(kSeparatorGlyph));
            break;

        case Item::Value:
            painter.setFont(value);
            painter.setPen(profile.colors.textSecondary);
            painter.drawText(paintEntryGlyph(&painter, entry, box,
                                             profile.colors.textDisabled),
                             Qt::AlignVCenter | Qt::AlignLeft, entry.text);
            break;

        case Item::Mono:
            painter.setFont(mono);
            painter.setPen(profile.colors.textDisabled);
            painter.drawText(paintEntryGlyph(&painter, entry, box,
                                             profile.colors.textDisabled),
                             Qt::AlignVCenter | Qt::AlignLeft, entry.text);
            break;

        case Item::Status: {
            // Offline overrides every transport light at once, which is what
            // the design's `.statusbar.offline .sdot` says and what losing a
            // connection actually means.
            const QColor hue = m_offline
                ? profile.semantic.danger.text
                : toneColor(this, entry.tone);

            const QRectF dot(box.left(), (height() - kDotDiameter) / 2.0,
                             kDotDiameter, kDotDiameter);

            // `box-shadow: 0 0 6px rgba(hue,.7)` - a halo that fades out,
            // rather than a second ring, or the dot gains an outline it does
            // not have in the design. A single stop at each end, not a
            // plateau in the middle: a flat inner ring is what read as a hard
            // edge where the fade stopped rather than a breath around the
            // dot.
            const QPointF centre = dot.center();
            const double glow = kDotDiameter * kGlowScale / 2.0;
            QRadialGradient halo(centre, glow);
            QColor inner = hue;
            inner.setAlphaF(0.42);
            QColor outer = hue;
            outer.setAlphaF(0.0);
            halo.setColorAt(0.0, inner);
            halo.setColorAt(1.0, outer);
            painter.setPen(Qt::NoPen);
            painter.setBrush(halo);
            painter.drawEllipse(centre, glow, glow);

            painter.setBrush(hue);
            painter.drawEllipse(dot);

            painter.setFont(text);
            painter.setPen(profile.colors.textDisabled);
            painter.drawText(box.adjusted(kDotDiameter + kStatusLabelGap, 0, 0, 0),
                             Qt::AlignVCenter | Qt::AlignLeft, entry.text);
            break;
        }

        case Item::Text:
            painter.setFont(text);
            painter.setPen(profile.colors.textDisabled);
            painter.drawText(box, Qt::AlignVCenter | Qt::AlignLeft, entry.text);
            break;
        }
    }
}

} // namespace novu::ui
