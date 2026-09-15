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

// `.statusbar { height:32px; gap:10px; padding:0 16px }`.
constexpr int kHeight = 32;
constexpr int kGap = 10;
constexpr int kPaddingH = 16;

// `.statusbar .sdot { width:7px }` and the gap to its word.
constexpr int kDotDiameter = 7;
constexpr int kDotGap = 5;
// The glow, as a multiple of the dot. `box-shadow:0 0 6px` on a 7px dot.
constexpr double kGlowScale = 2.7;

// The divider is a character rather than a rule in the design - a thin
// separator glyph in the hairline colour.
const QChar kSeparatorGlyph = QChar(0x2502);   // BOX DRAWINGS LIGHT VERTICAL

QFont barFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::Body);
    font.setPixelSize(profile.type.sm);
    return font;
}

QFont valueFont(const QWidget* context)
{
    QFont font = barFont(context);
    font.setWeight(QFont::Medium);
    return font;
}

QFont monoFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleMonoFont(context, TypeRole::Mono);
    font.setPixelSize(qMax(8, profile.type.xxs));
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
    setFixedHeight(kHeight);
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
    const int widgetHeight = qMin(hint.height(), height() - 6);
    m_leading->setGeometry(kPaddingH, (height() - widgetHeight) / 2,
                           hint.width(), widgetHeight);
}

int StatusBar::entryWidth(const Entry& entry) const
{
    switch (entry.kind) {
    case Item::Separator:
        return QFontMetrics(barFont(this)).horizontalAdvance(kSeparatorGlyph);
    case Item::Mono:
        return QFontMetrics(monoFont(this)).horizontalAdvance(entry.text);
    case Item::Value:
        return QFontMetrics(valueFont(this)).horizontalAdvance(entry.text);
    case Item::Status:
        return kDotDiameter + kDotGap
            + QFontMetrics(barFont(this)).horizontalAdvance(entry.text);
    case Item::Text:
        break;
    }
    return QFontMetrics(barFont(this)).horizontalAdvance(entry.text);
}

void StatusBar::layoutTrailing()
{
    if (!m_trailing) {
        return;
    }
    const QSize hint = m_trailing->sizeHint();
    const int widgetHeight = qMin(hint.height(), height() - 6);
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
    spec.type = TypeRole::Body;
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
    return QSize(width, kHeight);
}

QSize StatusBar::minimumSizeHint() const
{
    return QSize(120, kHeight);
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
    painter.drawLine(0, 0, width(), 0);

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
            painter.drawText(box, Qt::AlignVCenter | Qt::AlignLeft, entry.text);
            break;

        case Item::Mono:
            painter.setFont(mono);
            painter.setPen(profile.colors.textDisabled);
            painter.drawText(box, Qt::AlignVCenter | Qt::AlignLeft, entry.text);
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
            // not have in the design.
            const QPointF centre = dot.center();
            const double glow = kDotDiameter * kGlowScale / 2.0;
            QRadialGradient halo(centre, glow);
            QColor inner = hue;
            inner.setAlphaF(0.55);
            QColor outer = hue;
            outer.setAlphaF(0.0);
            halo.setColorAt(0.0, inner);
            halo.setColorAt(0.45, inner);
            halo.setColorAt(1.0, outer);
            painter.setPen(Qt::NoPen);
            painter.setBrush(halo);
            painter.drawEllipse(centre, glow, glow);

            painter.setBrush(hue);
            painter.drawEllipse(dot);

            painter.setFont(text);
            painter.setPen(profile.colors.textDisabled);
            painter.drawText(box.adjusted(kDotDiameter + kDotGap, 0, 0, 0),
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
