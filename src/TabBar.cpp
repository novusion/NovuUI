#include "novu/ui/TabBar.h"

#include "novu/ui/Fields.h"
#include "novu/ui/Motion.h"
#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/Tooltip.h"

#include <QContextMenuEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFontMetrics>
#include <QHash>
#include <QKeyEvent>
#include <QLineEdit>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QRegion>
#include <QPainterPath>
#include <QPen>
#include <QResizeEvent>
#include <QPointer>
#include <QShowEvent>
#include <QStyle>
#include <QTimer>
#include <QToolTip>
#include <QWheelEvent>

#include <utility>

namespace novu::ui {

namespace {

// `.btabs-row` is the band and `.btab` is what sits in it:
// `.btabs{padding:6px 6px 0;gap:3px}` over `.btab{padding:6px 9px 7px}`, which
// is a 32px tab standing on 6px of the band - and the 6px is why a tab reads
// as meeting the page below it rather than floating in a strip.
constexpr int kHeight = 39;  // 38 of band, and the rule under it
constexpr int kInsetTop = 6;
constexpr int kInsetLeft = 6;
// `.tab{padding:10px 12px}`.
constexpr int kPaddingH = 12;
constexpr int kGap = 8;
constexpr int kRadius = 8;

// The mark at the leading edge, and the close box at the trailing one.
constexpr int kMark = 14;
// `.btab .fav` - the lettered mark is a tile rather than a bare glyph, so it
// is a shade larger and carries its own edge.
constexpr int kMarkTile = 17;
constexpr int kMarkTileRadius = 5;
// `.btab .sdot` - the state dot that a lettered tab shows after its label.
constexpr int kStateDot = 6;
constexpr int kCloseSide = 16;
constexpr int kCloseGlyph = 9;

// What a tab may shrink to before the strip starts scrolling instead. Below
// this the label is gone and the tab is only its mark and its close box, which
// is the point at which shrinking stops helping.
constexpr int kMaxWidth = 220;
constexpr int kMinWidth = 92;
constexpr int kTabGap = 3;

// `.tab.on::after{left:8px;right:8px}` - the rule under an underlined tab is
// inset from both ends, so it reads as marking the word rather than the box.
constexpr double kUnderlineInset = 8.0;

// The trailing "+".
constexpr int kNewTabWidth = 28;

// `.btab.on::after` — the accent underline on the active tab, and the per-tab
// colour when one is set.
constexpr double kUnderlineHeight = 2.0;

// `.btab.sleeping` — a tab whose content has been discarded.
constexpr double kAsleepOpacity = 0.5;

// How far a press has to travel before it becomes a reorder rather than a
// click. Without it every click on a tab is a one-pixel drag.
constexpr int kDragThreshold = 6;

// A drag held this close to either end of the strip scrolls it, by this much
// per tick - fast enough to cross thirty tabs, slow enough to stop on one.
constexpr int kAutoScrollZone = 28;
constexpr int kAutoScrollStep = 14;
constexpr int kAutoScrollIntervalMs = 16;

// One wheel notch moves the strip by about half a crowded tab.
constexpr double kWheelPixelsPerNotch = 48.0;

// The fade at a clipped end, which is what says there is more strip there.
constexpr int kEdgeFade = 18;

// The speaker's strike when a tab is muted.
constexpr double kMuteStrike = 1.4;

QFont tabFont(const QWidget* context, bool current)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::Body);
    // `.tab{font-size:12.5px;font-weight:500}` - the sheet gives every tab
    // the same weight and marks the current one with colour and the 2px rule
    // under it, not with a heavier face. `md` is the step 12.5 rounds to.
    font.setPixelSize(profile.type.md);
    font.setWeight(QFont::Medium);
    Q_UNUSED(current)
    return font;
}

} // namespace

// State added after the bar's layout was frozen (see TabBar.h). One per bar,
// owned by it as a child and found through a pointer map, so asking for it is
// a hash lookup rather than a walk over the bar's children.
struct TabBarExtras final : public QObject
{
    explicit TabBarExtras(TabBar* bar)
        : QObject(bar), owner(bar)
    {
        registry().insert(bar, this);
    }
    ~TabBarExtras() override { registry().remove(owner); }

    static QHash<const TabBar*, TabBarExtras*>& registry()
    {
        static QHash<const TabBar*, TabBarExtras*> map;
        return map;
    }
    static TabBarExtras* of(const TabBar* bar) { return registry().value(bar); }

    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == editor && !editKey.isEmpty()) {
            if (event->type() == QEvent::KeyPress) {
                if (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape) {
                    owner->finishInlineEdit(false, false);
                    return true;
                }
            } else if (event->type() == QEvent::FocusOut) {
                // A popup taking focus (the tooltip carrying a rejection, a
                // context menu) is not the person leaving the field.
                if (static_cast<QFocusEvent*>(event)->reason() != Qt::PopupFocusReason) {
                    owner->finishInlineEdit(true, true);
                }
            }
        }
        return QObject::eventFilter(watched, event);
    }

    TabBar* owner = nullptr;
    QHash<QString, TabBar::Audio> audio;
    int scroll = 0;
    QTimer* autoScroll = nullptr;
    int autoScrollDirection = 0;
    QPoint lastDragPosition;
    int hoveredAudio = -1;
    int pressedAudio = -1;
    QPointer<QLineEdit> editor;
    QString editKey;
    bool editRejected = false;
    bool editFinishing = false;
};

TabBar::TabBar(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(kHeight);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::PointingHandCursor);
    auto* extras = new TabBarExtras(this);
    extras->autoScroll = new QTimer(extras);
    extras->autoScroll->setInterval(kAutoScrollIntervalMs);
    connect(extras->autoScroll, &QTimer::timeout, this, [this] { autoScrollStep(); });
}

// == shape ==================================================================

int TabBar::addTab(const QString& key, const QString& text, Glyph glyph)
{
    return insertTab(m_tabs.size(), key, text, glyph);
}

int TabBar::insertTab(int index, const QString& key, const QString& text, Glyph glyph)
{
    if (key.isEmpty() || indexOfKey(key) >= 0) {
        // A duplicate key would give two tabs one identity, and every signal
        // about either would name the same document.
        return -1;
    }
    Tab tab;
    tab.key = key;
    tab.label = text;
    tab.glyph = glyph;
    // `.tab` draws no close box, whenever the tab arrives.
    tab.closable = m_variant != Variant::Underline;

    const int at = qBound(0, index, m_tabs.size());
    m_tabs.insert(at, tab);
    if (m_current < 0) {
        m_current = at;
    } else if (at <= m_current) {
        ++m_current;
    }
    updateGeometry();
    update();
    return at;
}

void TabBar::removeTab(const QString& key)
{
    const int index = indexOfKey(key);
    if (index < 0) {
        return;
    }
    TabBarExtras* extras = TabBarExtras::of(this);
    if (extras && extras->editKey == key) {
        // The tab is gone, so there is nothing left to rename.
        finishInlineEdit(false, false);
    }
    m_tabs.remove(index);
    if (extras) {
        extras->audio.remove(key);
        extras->hoveredAudio = -1;
        extras->pressedAudio = -1;
    }

    if (m_tabs.isEmpty()) {
        m_current = -1;
    } else if (index < m_current) {
        --m_current;
    } else if (index == m_current) {
        // Closing the current tab moves to the one that took its place, or to
        // the last if it was the last - which is what every editor does.
        m_current = qMin(index, m_tabs.size() - 1);
        emit currentChanged(currentKey());
    }

    m_hovered = -1;
    m_pressed = -1;
    m_dragIndex = -1;
    updateMotion();
    updateGeometry();
    placeInlineEditor();
    update();
}

void TabBar::clear()
{
    if (TabBarExtras* extras = TabBarExtras::of(this)) {
        if (!extras->editKey.isEmpty()) {
            finishInlineEdit(false, false);
        }
        extras->audio.clear();
        extras->scroll = 0;
        extras->hoveredAudio = -1;
        extras->pressedAudio = -1;
    }
    m_tabs.clear();
    m_current = -1;
    m_hovered = -1;
    m_pressed = -1;
    m_dragIndex = -1;
    updateMotion();
    updateGeometry();
    update();
}

int TabBar::indexOfKey(const QString& key) const
{
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs.at(i).key == key) {
            return i;
        }
    }
    return -1;
}

QString TabBar::keyAt(int index) const
{
    return index >= 0 && index < m_tabs.size() ? m_tabs.at(index).key : QString();
}

QStringList TabBar::keys() const
{
    QStringList result;
    result.reserve(m_tabs.size());
    for (const Tab& tab : m_tabs) {
        result.append(tab.key);
    }
    return result;
}

QString TabBar::currentKey() const
{
    return keyAt(m_current);
}

const TabBar::Tab* TabBar::find(const QString& key) const
{
    const int index = indexOfKey(key);
    return index < 0 ? nullptr : &m_tabs.at(index);
}

TabBar::Tab* TabBar::find(const QString& key)
{
    return const_cast<Tab*>(std::as_const(*this).find(key));
}

void TabBar::setCurrentIndex(int index)
{
    if (index < -1 || index >= m_tabs.size() || index == m_current) {
        return;
    }
    m_current = index;
    ensureVisible(currentKey());
    update();
    emit currentChanged(currentKey());
}

void TabBar::setCurrentKey(const QString& key)
{
    setCurrentIndex(indexOfKey(key));
}

// == content ================================================================

void TabBar::setLabel(const QString& key, const QString& text)
{
    Tab* tab = find(key);
    if (!tab || tab->label == text) {
        return;
    }
    tab->label = text;
    updateGeometry();
    placeInlineEditor();
    update();
}

QString TabBar::label(const QString& key) const
{
    const Tab* tab = find(key);
    return tab ? tab->label : QString();
}

void TabBar::setGlyph(const QString& key, Glyph glyph)
{
    Tab* tab = find(key);
    if (!tab || tab->glyph == glyph) {
        return;
    }
    tab->glyph = glyph;
    update();
}

void TabBar::setMarkText(const QString& key, const QString& text)
{
    Tab* tab = find(key);
    if (!tab || tab->mark == text) {
        return;
    }
    tab->mark = text;
    updateGeometry();
    update();
}

QString TabBar::markText(const QString& key) const
{
    const Tab* tab = find(key);
    return tab ? tab->mark : QString();
}

void TabBar::setState(const QString& key, State state)
{
    Tab* tab = find(key);
    if (!tab || tab->state == state) {
        return;
    }
    tab->state = state;
    // The clock is held open only while something is actually turning.
    updateMotion();
    update();
}

TabBar::State TabBar::state(const QString& key) const
{
    const Tab* tab = find(key);
    return tab ? tab->state : State::Idle;
}

void TabBar::setColor(const QString& key, const QColor& colour)
{
    Tab* tab = find(key);
    if (!tab || tab->color == colour) {
        return;
    }
    // Kept on the tab rather than in a parallel list beside it. Every
    // implementation that kept colours alongside got them out of step the first
    // time a tab was dragged.
    tab->color = colour;
    update();
}

QColor TabBar::color(const QString& key) const
{
    const Tab* tab = find(key);
    return tab ? tab->color : QColor();
}

void TabBar::setVariant(Variant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    // An underlined strip has no close boxes at all - `.tab` draws none - so
    // asking for the variant is also asking for that. Tabs added *after* the
    // variant was set are handled in addTab(): asking here only, a caller that
    // sets the variant on an empty bar and then fills it - which is the
    // ordinary order - got a close box on every tab.
    if (variant == Variant::Underline) {
        for (Tab& tab : m_tabs) {
            tab.closable = false;
        }
    }
    updateGeometry();
    update();
}

void TabBar::setClosable(const QString& key, bool closable)
{
    Tab* tab = find(key);
    if (!tab || tab->closable == closable) {
        return;
    }
    tab->closable = closable;
    updateGeometry();
    update();
}

void TabBar::setAudio(const QString& key, Audio audio)
{
    TabBarExtras* extras = TabBarExtras::of(this);
    if (!extras || !find(key)) {
        return;
    }
    const Audio previous = extras->audio.value(key, Audio::None);
    if (previous == audio) {
        return;
    }
    if (audio == Audio::None) {
        extras->audio.remove(key);
    } else {
        extras->audio.insert(key, audio);
    }
    // The speaker takes room after the label only while it is there.
    if ((previous == Audio::None) != (audio == Audio::None)) {
        updateGeometry();
        placeInlineEditor();
    }
    update();
}

TabBar::Audio TabBar::audio(const QString& key) const
{
    const TabBarExtras* extras = TabBarExtras::of(this);
    return extras ? extras->audio.value(key, Audio::None) : Audio::None;
}

void TabBar::setReorderable(bool reorderable)
{
    if (m_reorderable == reorderable) {
        return;
    }
    m_reorderable = reorderable;
    m_dragIndex = -1;
}

void TabBar::setNewTabButton(bool enabled)
{
    if (m_newTab == enabled) {
        return;
    }
    m_newTab = enabled;
    updateGeometry();
    update();
}

void TabBar::setTrailingWidget(QWidget* widget)
{
    if (m_trailing == widget) {
        return;
    }
    delete m_trailing;
    m_trailing = widget;
    if (m_trailing) {
        m_trailing->setParent(this);
        m_trailing->show();
    }
    updateGeometry();
    placeTrailing();
    update();
}

void TabBar::placeTrailing()
{
    if (!m_trailing) {
        return;
    }
    const QSize hint = m_trailing->sizeHint();
    m_trailing->setGeometry(width() - hint.width(), (height() - hint.height()) / 2,
                            hint.width(), hint.height());
}

void TabBar::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    placeTrailing();
    // A narrower strip can push the current tab out of view; it stays in it.
    if (m_current >= 0) {
        ensureVisible(currentKey());
    }
    placeInlineEditor();
}

// == geometry ===============================================================

int TabBar::tabWidth(const Tab& tab) const
{
    const QFontMetrics fm(tabFont(this, false));
    // Plus a pixel of slack at each end. The label box comes out as exactly
    // the advance, and elidedText() gives way at equal widths - so a tab
    // measured to the pixel painted `NovuAI access` as `NovuAI acce...`.
    int width = 2 * kPaddingH + fm.horizontalAdvance(tab.label) + 2;
    if (!tab.mark.isEmpty()) {
        width += kMarkTile + kGap;
        if (tab.state == State::Failed || tab.state == State::Asleep) {
            width += kGap + kStateDot;
        }
    } else if (tab.glyph != Glyph::None || tab.state != State::Idle) {
        width += kMark + kGap;
    }
    if (audio(tab.key) != Audio::None) {
        width += kGap + kMark;
    }
    if (tab.closable) {
        width += kGap + kCloseSide;
    }
    // The floor and the cap are a *browser's* rules. `.btab{max-width:250px}`
    // keeps twenty open documents legible, and the floor keeps a close box
    // reachable on a tab titled "1". Neither applies to `.tab`, the underlined
    // strip, which is `padding:10px 12px` around its word and nothing else:
    // forced to 92 apiece, eight of them spread edge to edge across a drawer
    // and elided `Indicators` to `Indic...` with blank plate on both sides.
    if (m_variant == Variant::Underline) {
        return width;
    }
    return qBound(kMinWidth, width, kMaxWidth);
}

QVector<int> TabBar::laidOutWidths() const
{
    QVector<int> widths;
    widths.reserve(m_tabs.size());
    if (m_tabs.isEmpty()) {
        return widths;
    }

    int total = 0;
    for (const Tab& tab : m_tabs) {
        const int each = tabWidth(tab);
        widths << each;
        total += each;
    }

    const int available = width() - 2 * kInsetLeft
        - (m_newTab ? kNewTabWidth + 4 + kTabGap : 0)
        - (m_trailing ? m_trailing->sizeHint().width() : 0)
        - kTabGap * (m_tabs.size() - 1);

    // Each tab is as wide as its own title needs, up to the cap - which is
    // what `.btab{max-width:250px}` says and what a browser looks like with
    // three tabs open. Equal widths are the *crowded* case, not the normal
    // one: forcing two tabs to the width of the longer title leaves a strip
    // of blank plate beside the shorter one.
    if (total <= available) {
        return widths;
    }

    // Out of room: they shrink together, down to the floor, because tabs that
    // gave up their width one at a time would move every close box on every
    // keystroke of a page title.
    const int even = qBound(kMinWidth, available / m_tabs.size(), kMaxWidth);
    for (int& each : widths) {
        each = even;
    }
    return widths;
}

int TabBar::laidOutWidth() const
{
    const QVector<int> widths = laidOutWidths();
    return widths.isEmpty() ? 0 : widths.first();
}

QRect TabBar::tabRect(int index) const
{
    if (index < 0 || index >= m_tabs.size()) {
        return QRect();
    }
    const QVector<int> widths = laidOutWidths();
    int left = kInsetLeft - effectiveScroll();
    for (int i = 0; i < index; ++i) {
        left += widths.at(i) + kTabGap;
    }
    // The rule at the bottom edge is the page's, not the strip's: a tab ends
    // on it, as `.btab{border-bottom:none}` says.
    return QRect(left, kInsetTop, widths.at(index), height() - kInsetTop - 1);
}

QRect TabBar::tabsViewport() const
{
    const int trailing = m_trailing ? m_trailing->sizeHint().width() : 0;
    const int newTab = m_newTab ? kNewTabWidth + 4 + kTabGap : 0;
    const int span = qMax(0, width() - 2 * kInsetLeft - newTab - trailing);
    return QRect(kInsetLeft, 0, span, height());
}

int TabBar::contentWidth() const
{
    const QVector<int> widths = laidOutWidths();
    int total = 0;
    for (int each : widths) {
        total += each;
    }
    if (!widths.isEmpty()) {
        total += kTabGap * (widths.size() - 1);
    }
    return total;
}

int TabBar::maximumScrollOffset() const
{
    return qMax(0, contentWidth() - tabsViewport().width());
}

int TabBar::effectiveScroll() const
{
    const TabBarExtras* extras = TabBarExtras::of(this);
    if (!extras) {
        return 0;
    }
    return qBound(0, extras->scroll, maximumScrollOffset());
}

int TabBar::scrollOffset() const
{
    return effectiveScroll();
}

void TabBar::setScrollOffset(int pixels)
{
    TabBarExtras* extras = TabBarExtras::of(this);
    if (!extras) {
        return;
    }
    const int clamped = qBound(0, pixels, maximumScrollOffset());
    if (clamped == effectiveScroll() && clamped == extras->scroll) {
        return;
    }
    extras->scroll = clamped;
    m_hovered = -1;
    m_hoveredClose = false;
    extras->hoveredAudio = -1;
    placeInlineEditor();
    update();
}

void TabBar::ensureVisible(const QString& key)
{
    const int index = indexOfKey(key);
    if (index < 0 || maximumScrollOffset() == 0) {
        return;
    }
    const QRect box = tabRect(index);
    const QRect viewport = tabsViewport();
    int scroll = effectiveScroll();
    if (box.left() < viewport.left()) {
        scroll -= viewport.left() - box.left();
    } else if (box.right() > viewport.right()) {
        scroll += box.right() - viewport.right();
    }
    setScrollOffset(scroll);
}

QRect TabBar::closeRect(int index) const
{
    if (index < 0 || index >= m_tabs.size() || !m_tabs.at(index).closable) {
        return QRect();
    }
    const QRect box = tabRect(index);
    if (box.width() < kMinWidth) {
        return QRect();
    }
    return QRect(box.right() - kPaddingH - kCloseSide + 1,
                 box.top() + (box.height() - kCloseSide) / 2, kCloseSide, kCloseSide);
}

QRect TabBar::newTabRect() const
{
    if (!m_newTab) {
        return QRect();
    }
    // After the last tab, or pinned at the end of the band once the strip
    // scrolls - the "+" is never scrolled away with the tabs.
    int left = kInsetLeft;
    if (!m_tabs.isEmpty()) {
        left += qMin(contentWidth(), tabsViewport().width()) + kTabGap;
    }
    // `.btab-new{margin:0 0 4px 4px}` - it sits on the page's edge with the
    // tabs, not centred in the band above them.
    return QRect(left + 4, height() - kNewTabWidth - 5, kNewTabWidth, kNewTabWidth);
}

int TabBar::indexAt(const QPoint& position) const
{
    // A tab scrolled under either end is not there to be hit.
    const QRect viewport = tabsViewport();
    if (position.x() < viewport.left() || position.x() > viewport.right()) {
        return -1;
    }
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (tabRect(i).contains(position)) {
            return i;
        }
    }
    return -1;
}

QSize TabBar::sizeHint() const
{
    int width = 0;
    for (const Tab& tab : m_tabs) {
        width += tabWidth(tab) + kTabGap;
    }
    if (m_newTab) {
        width += kNewTabWidth;
    }
    return QSize(qMax(width, kMinWidth), kHeight);
}

QRect TabBar::audioRect(int index) const
{
    if (index < 0 || index >= m_tabs.size() || audio(m_tabs.at(index).key) == Audio::None) {
        return QRect();
    }
    const Tab& tab = m_tabs.at(index);
    const QRect box = tabRect(index);
    const QRect close = closeRect(index);
    int rightEdge = close.isNull() ? box.right() - kPaddingH : close.left() - kGap;
    const bool lettered = !tab.mark.isEmpty() && tab.state != State::Loading;
    if (lettered && (tab.state == State::Failed || tab.state == State::Asleep)) {
        rightEdge -= kStateDot + kGap;
    }
    const QRect speaker(rightEdge - kMark + 1, box.top() + (box.height() - kMark) / 2,
                        kMark, kMark);
    // Too narrow a tab keeps its mark and close box and loses the speaker
    // before it loses the label entirely.
    return speaker.left() > box.left() + kPaddingH + kMarkTile ? speaker : QRect();
}

QSize TabBar::minimumSizeHint() const
{
    return QSize(kMinWidth + (m_newTab ? kNewTabWidth : 0), kHeight);
}

// == motion =================================================================

void TabBar::updateMotion()
{
    bool anyLoading = false;
    for (const Tab& tab : m_tabs) {
        if (tab.state == State::Loading) {
            anyLoading = true;
            break;
        }
    }
    const bool wanted = anyLoading && isVisible() && motion::enabled(this);
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

void TabBar::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    updateMotion();
}

void TabBar::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    if (TabBarExtras* extras = TabBarExtras::of(this)) {
        extras->autoScroll->stop();
    }
    updateMotion();
}

bool TabBar::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        updateMotion();
        updateGeometry();
        update();
    }
    // Handled here rather than in wheelEvent()/contextMenuEvent() overrides,
    // which would change the class modules were built against.
    if (event->type() == QEvent::Wheel && handleWheel(static_cast<QWheelEvent*>(event))) {
        return true;
    }
    if (event->type() == QEvent::ContextMenu
        && handleContextMenu(static_cast<QContextMenuEvent*>(event))) {
        return true;
    }
    return QWidget::event(event);
}

bool TabBar::handleWheel(QWheelEvent* event)
{
    if (maximumScrollOffset() == 0) {
        return false;
    }
    // A trackpad gives pixels, a wheel gives notches. Either axis scrolls:
    // the strip has only one, and most wheels only turn the vertical one.
    const QPoint pixels = event->pixelDelta();
    const QPoint angle = event->angleDelta();
    int delta = 0;
    if (!pixels.isNull()) {
        delta = qAbs(pixels.x()) > qAbs(pixels.y()) ? pixels.x() : pixels.y();
    } else {
        const int notches = qAbs(angle.x()) > qAbs(angle.y()) ? angle.x() : angle.y();
        delta = qRound(notches / 120.0 * kWheelPixelsPerNotch);
    }
    if (delta == 0) {
        return false;
    }
    setScrollOffset(effectiveScroll() - delta);
    event->accept();
    return true;
}

bool TabBar::handleContextMenu(QContextMenuEvent* event)
{
    int index = -1;
    QPoint global = event->globalPos();
    if (event->reason() == QContextMenuEvent::Mouse) {
        index = indexAt(event->pos());
    } else if (m_current >= 0) {
        index = m_current;
        ensureVisible(currentKey());
        const QRect box = tabRect(index);
        global = mapToGlobal(QPoint(box.left() + kPaddingH, box.bottom()));
    }
    if (index < 0) {
        return false;
    }
    event->accept();
    // A copy: the menu can close the tab while the slot still holds the key.
    const QString key = m_tabs.at(index).key;
    emit contextMenuRequested(key, global);
    return true;
}

// == renaming in place ======================================================
//
// One QLineEdit for the bar, laid over the label band of the tab being
// renamed - the same convention, and the same rules, as NavList's.

bool TabBar::beginInlineEdit(const QString& key, const QString& text)
{
    TabBarExtras* extras = TabBarExtras::of(this);
    const int index = indexOfKey(key);
    if (!extras || index < 0) {
        return false;
    }
    if (!extras->editor) {
        extras->editor = lineEdit(QString(), this);
        extras->editor->setObjectName(QStringLiteral("novuTabInlineEditor"));
        extras->editor->installEventFilter(extras);
        connect(extras->editor, &QLineEdit::returnPressed, this,
                [this] { finishInlineEdit(true, false); });
        connect(extras->editor, &QLineEdit::textEdited, this, [extras] {
            // Typing again clears a previous rejection's mark.
            extras->editor->setProperty("invalid", false);
            extras->editor->style()->unpolish(extras->editor);
            extras->editor->style()->polish(extras->editor);
            extras->editor->setToolTip(QString());
        });
    }
    extras->editKey = key;
    extras->editRejected = false;
    extras->editor->setProperty("invalid", false);
    extras->editor->setToolTip(QString());
    extras->editor->setText(text.isNull() ? m_tabs.at(index).label : text);
    ensureVisible(key);
    placeInlineEditor();
    extras->editor->show();
    extras->editor->raise();
    extras->editor->setFocus(Qt::OtherFocusReason);
    extras->editor->selectAll();
    return true;
}

bool TabBar::isInlineEditing() const
{
    const TabBarExtras* extras = TabBarExtras::of(this);
    return extras && extras->editor && extras->editor->isVisible()
        && !extras->editKey.isEmpty();
}

QString TabBar::inlineEditKey() const
{
    const TabBarExtras* extras = TabBarExtras::of(this);
    return extras ? extras->editKey : QString();
}

void TabBar::rejectInlineEdit(const QString& message)
{
    TabBarExtras* extras = TabBarExtras::of(this);
    if (!extras || !extras->editor) {
        return;
    }
    extras->editRejected = true;
    extras->editor->setProperty("invalid", true);
    extras->editor->style()->unpolish(extras->editor);
    extras->editor->style()->polish(extras->editor);
    extras->editor->setToolTip(message);
    if (!message.isEmpty() && extras->editor->isVisible()) {
        QToolTip::showText(extras->editor->mapToGlobal(QPoint(0, extras->editor->height())),
                           message, extras->editor);
    }
}

void TabBar::cancelInlineEdit()
{
    finishInlineEdit(false, false);
}

void TabBar::finishInlineEdit(bool submit, bool fromFocusOut)
{
    TabBarExtras* extras = TabBarExtras::of(this);
    if (!extras || extras->editFinishing || extras->editKey.isEmpty() || !extras->editor) {
        return;
    }
    extras->editFinishing = true;
    const QString key = extras->editKey;
    if (submit) {
        extras->editRejected = false;
        emit inlineEditCommitted(key, extras->editor->text());
        if (extras->editRejected && !fromFocusOut) {
            // Kept open with the reason beside it; the person fixes the name.
            extras->editFinishing = false;
            extras->editor->setFocus(Qt::OtherFocusReason);
            return;
        }
        const bool rejected = extras->editRejected;
        extras->editKey.clear();
        extras->editor->hide();
        extras->editFinishing = false;
        if (rejected) {
            // Focus left with an unacceptable name: abandon rather than steal
            // focus back.
            emit inlineEditCancelled(key);
        }
        return;
    }
    extras->editKey.clear();
    extras->editor->hide();
    extras->editFinishing = false;
    emit inlineEditCancelled(key);
}

void TabBar::placeInlineEditor()
{
    TabBarExtras* extras = TabBarExtras::of(this);
    if (!extras || !extras->editor || extras->editKey.isEmpty()) {
        return;
    }
    const int index = indexOfKey(extras->editKey);
    if (index < 0) {
        finishInlineEdit(false, false);
        return;
    }
    const Tab& tab = m_tabs.at(index);
    const QRect box = tabRect(index);
    int left = box.left() + kPaddingH;
    if (!tab.mark.isEmpty()) {
        left += kMarkTile + kGap;
    } else if (tab.glyph != Glyph::None || tab.state != State::Idle) {
        left += kMark + kGap;
    }
    const QRect close = closeRect(index);
    const int right = close.isNull() ? box.right() - kPaddingH + 4 : close.left() - 4;
    const int editorHeight = qMin(box.height() - 4, extras->editor->sizeHint().height());
    extras->editor->setGeometry(left - 4, box.top() + (box.height() - editorHeight) / 2,
                                qMax(80, right - left + 4), editorHeight);
}

// == interaction ============================================================

void TabBar::focusInEvent(QFocusEvent* event)
{
    m_ring.noteFocusIn(event->reason());
    QWidget::focusInEvent(event);
    update();
}

void TabBar::mousePressEvent(QMouseEvent* event)
{
    // Clicking a tab is not asking for the keyboard's outline: `.btab.on` is
    // a plate, a hairline and the 2px accent rule across its top, and a ring
    // drawn over that is the same statement a third time.
    m_ring.noteMousePress();
    const QPoint position = event->position().toPoint();

    // Middle-click closes, everywhere it exists. It is the one gesture users
    // bring with them from every browser and every editor.
    if (event->button() == Qt::MiddleButton) {
        const int index = indexAt(position);
        if (index >= 0 && m_tabs.at(index).closable) {
            const QString key = m_tabs.at(index).key;
            emit closeRequested(key);
        }
        return;
    }
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    if (newTabRect().contains(position)) {
        m_pressedNew = true;
        update();
        return;
    }

    const int index = indexAt(position);
    if (TabBarExtras* extras = TabBarExtras::of(this)) {
        extras->pressedAudio = index >= 0 && audioRect(index).contains(position) ? index : -1;
        if (extras->pressedAudio >= 0) {
            m_pressed = -1;
            update();
            return;
        }
    }
    m_pressed = index;
    m_pressedClose = index >= 0 && closeRect(index).contains(position);
    m_dragOrigin = position;
    m_dragIndex = -1;
    update();
}

void TabBar::mouseMoveEvent(QMouseEvent* event)
{
    const QPoint position = event->position().toPoint();

    if (m_pressed >= 0 && !m_pressedClose && m_reorderable) {
        // A press only becomes a reorder once it has travelled. Without the
        // threshold every click on a tab is a one-pixel drag.
        if (m_dragIndex < 0
            && (position - m_dragOrigin).manhattanLength() > kDragThreshold) {
            m_dragIndex = m_pressed;
        }
        if (m_dragIndex >= 0) {
            dragTo(position);
            updateAutoScroll(position);
            return;
        }
    }

    const int index = indexAt(position);
    const bool onClose = index >= 0 && closeRect(index).contains(position);
    const bool onNew = newTabRect().contains(position);
    TabBarExtras* extras = TabBarExtras::of(this);
    const int onAudio = index >= 0 && audioRect(index).contains(position) ? index : -1;
    const bool audioChanged = extras && extras->hoveredAudio != onAudio;
    if (index != m_hovered || onClose != m_hoveredClose || onNew != m_hoveredNew
        || audioChanged) {
        // A shrunken tab hides its label, so the tip is the only way to read
        // it. Installed only when the label is actually elided.
        if (onAudio >= 0) {
            Tooltip::install(this, audio(m_tabs.at(onAudio).key) == Audio::Muted
                                       ? tr("Unmute tab") : tr("Mute tab"));
        } else if (index != m_hovered || audioChanged) {
            if (index >= 0 && tabRect(index).width() <= kMinWidth + 8) {
                Tooltip::install(this, m_tabs.at(index).label);
            } else {
                Tooltip::remove(this);
            }
        }
        m_hovered = index;
        m_hoveredClose = onClose;
        m_hoveredNew = onNew;
        if (extras) {
            extras->hoveredAudio = onAudio;
        }
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void TabBar::dragTo(const QPoint& position)
{
    if (m_dragIndex < 0) {
        return;
    }
    if (TabBarExtras* extras = TabBarExtras::of(this)) {
        extras->lastDragPosition = position;
    }
    // Held past an end of the band, the drag is over the nearest tab there:
    // that is the tab the auto-scroll is bringing into view.
    const QRect viewport = tabsViewport();
    const QPoint probe(qBound(viewport.left(), position.x(), viewport.right()),
                       qBound(kInsetTop, position.y(), height() - 2));
    const int over = indexAt(probe);
    if (over < 0 || over == m_dragIndex) {
        return;
    }
    const QString key = m_tabs.at(m_dragIndex).key;
    const int from = m_dragIndex;
    m_tabs.move(m_dragIndex, over);
    // The colour and the state travel with the tab, because they live on it.
    if (m_current == from) {
        m_current = over;
    } else if (from < m_current && over >= m_current) {
        --m_current;
    } else if (from > m_current && over <= m_current) {
        ++m_current;
    }
    m_dragIndex = over;
    m_pressed = over;
    placeInlineEditor();
    update();
    emit tabMoved(key, from, over);
}

void TabBar::updateAutoScroll(const QPoint& position)
{
    TabBarExtras* extras = TabBarExtras::of(this);
    if (!extras) {
        return;
    }
    const QRect viewport = tabsViewport();
    int direction = 0;
    if (m_dragIndex >= 0 && maximumScrollOffset() > 0) {
        if (position.x() < viewport.left() + kAutoScrollZone && effectiveScroll() > 0) {
            direction = -1;
        } else if (position.x() > viewport.right() - kAutoScrollZone
                   && effectiveScroll() < maximumScrollOffset()) {
            direction = 1;
        }
    }
    extras->autoScrollDirection = direction;
    if (direction == 0) {
        extras->autoScroll->stop();
    } else if (!extras->autoScroll->isActive()) {
        extras->autoScroll->start();
    }
}

void TabBar::autoScrollStep()
{
    TabBarExtras* extras = TabBarExtras::of(this);
    if (!extras || m_dragIndex < 0 || extras->autoScrollDirection == 0) {
        if (extras) {
            extras->autoScroll->stop();
        }
        return;
    }
    const int before = effectiveScroll();
    setScrollOffset(before + extras->autoScrollDirection * kAutoScrollStep);
    if (effectiveScroll() == before) {
        extras->autoScroll->stop();
        return;
    }
    // The strip moved under a pointer that did not: the tab under it changed.
    dragTo(extras->lastDragPosition);
}

void TabBar::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    const QPoint position = event->position().toPoint();

    if (m_pressedNew) {
        m_pressedNew = false;
        if (newTabRect().contains(position)) {
            emit newTabRequested();
        }
        update();
        return;
    }

    if (TabBarExtras* extras = TabBarExtras::of(this)) {
        extras->autoScroll->stop();
        extras->autoScrollDirection = 0;
        const int pressedAudio = std::exchange(extras->pressedAudio, -1);
        if (pressedAudio >= 0) {
            if (indexAt(position) == pressedAudio && audioRect(pressedAudio).contains(position)) {
                const QString key = m_tabs.at(pressedAudio).key;
                emit audioToggleRequested(key);
            }
            update();
            return;
        }
    }

    const int pressed = m_pressed;
    const bool pressedClose = m_pressedClose;
    const bool dragged = m_dragIndex >= 0;
    m_pressed = -1;
    m_pressedClose = false;
    m_dragIndex = -1;

    if (pressed < 0 || dragged) {
        update();
        return;   // a reorder is not also a selection
    }

    const int released = indexAt(position);
    const bool releasedClose = released >= 0 && closeRect(released).contains(position);
    if (released != pressed || releasedClose != pressedClose) {
        update();
        return;   // cancelled
    }

    if (pressedClose) {
        // The bar does not remove the tab: a document with unsaved changes has
        // to be able to ask first, and a bar that had already closed it would
        // be asking about something gone.
        // The key goes out as a copy: a slot that removes the tab destroys
        // the string a reference into m_tabs would still point at.
        const QString key = m_tabs.at(pressed).key;
        emit closeRequested(key);
    } else {
        setCurrentIndex(pressed);
    }
    update();
}

void TabBar::leaveEvent(QEvent* event)
{
    if (m_hovered != -1 || m_hoveredClose || m_hoveredNew) {
        m_hovered = -1;
        m_hoveredClose = false;
        m_hoveredNew = false;
        Tooltip::remove(this);
        update();
    }
    QWidget::leaveEvent(event);
}

void TabBar::keyPressEvent(QKeyEvent* event)
{
    m_ring.noteKeyPress();
    switch (event->key()) {
    case Qt::Key_Right:
        if (m_current + 1 < m_tabs.size()) {
            setCurrentIndex(m_current + 1);
        }
        return;
    case Qt::Key_Left:
        if (m_current > 0) {
            setCurrentIndex(m_current - 1);
        }
        return;
    case Qt::Key_W:
        if (event->modifiers().testFlag(Qt::ControlModifier) && m_current >= 0
            && m_tabs.at(m_current).closable) {
            const QString key = m_tabs.at(m_current).key;
            emit closeRequested(key);
            return;
        }
        break;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

// == appearance =============================================================

StyleSpec TabBar::styleSpec() const
{
    StyleSpec spec;
    spec.background = token(ColorRole::HeaderBg);
    spec.border = token(ColorRole::Border);
    spec.foreground = token(ColorRole::TextDisabled);
    spec.radius = SurfaceRole::Toolbar;
    spec.borderWidth = BorderRole::Divider;
    spec.type = TypeRole::Body;
    return spec;
}

StyleSpec TabBar::tabStyleSpec(int index) const
{
    StyleSpec spec = styleSpec();
    spec.radius = SurfaceRole::Button;
    spec.background = token(ColorRole::Transparent);
    spec.border = token(ColorRole::Transparent);
    spec.hoverBackground = token(ColorRole::SurfaceHover, 0.55);
    spec.hoverForeground = token(ColorRole::TextSecondary);
    spec.checkedBackground = token(ColorRole::PanelBg);
    spec.checkedForeground = token(ColorRole::TextPrimary);

    if (index >= 0 && index < m_tabs.size()) {
        switch (m_tabs.at(index).state) {
        case State::Failed:
            spec.foreground = statusToken(SemanticRole::Danger, ColorRole::StatusText);
            break;
        case State::Asleep:
            spec.foreground = token(ColorRole::TextDisabled, kAsleepOpacity);
            spec.checkedForeground = token(ColorRole::TextSecondary, kAsleepOpacity);
            break;
        case State::Idle:
        case State::Loading:
        case State::Modified:
            break;
        }
    }
    return spec;
}

ResolvedStyle TabBar::resolvedStyle() const
{
    return resolve(styleSpec(), this, height());
}

void TabBar::paintEvent(QPaintEvent* event)
{
    // What animates (loading rings), collected while painting, so the shared
    // clock repaints only those - see Pulse.
    QRegion motionRegion;
    const ThemeProfile& profile = themeFor(this);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Browser tabs stand on their own panel band (`.btabs-row`). Underline
    // tabs map to `.tabs`, whose background is transparent so the strip stays
    // part of the surrounding card instead of reading as another frame.
    if (m_variant == Variant::Browser)
        painter.fillRect(rect(), profile.colors.panelBgAlt);
    // Filled, not stroked: an antialiased 1px line lands on a half pixel and
    // comes out as two grey rows where the design draws one hairline.
    const int rule = qMax(1, profile.border(BorderRole::Divider));
    painter.fillRect(QRect(0, height() - rule, width(), rule), profile.colors.border);

    // The tabs are drawn inside their band only: once the strip scrolls, a
    // tab half under the "+" or the trailing actions is cut at the band's
    // edge rather than drawn over them.
    const QRect viewport = tabsViewport();
    painter.save();
    painter.setClipRect(viewport.adjusted(0, 0, 1, 0));
    const TabBarExtras* extras = TabBarExtras::of(this);
    for (int index = 0; index < m_tabs.size(); ++index) {
        const Tab& tab = m_tabs.at(index);
        const QRect box = tabRect(index);
        if (!box.intersects(viewport)) {
            continue;
        }
        if (tab.state == State::Loading) {
            motionRegion += box;
        }
        // Only what this paint was asked for: a ring's repaint does not
        // re-resolve and redraw every other tab.
        if (!box.intersects(event->rect())) {
            continue;
        }
        const ResolvedStyle style = resolve(tabStyleSpec(index), this, box.height());
        const bool current = index == m_current;
        const bool hot = index == m_hovered && !current;

        // The plate. Only the top corners are round: the tab meets the page
        // below it, and a fully rounded tab reads as a chip that happens to be
        // near the edge.
        if ((current || hot) && m_variant == Variant::Browser) {
            QPainterPath plate;
            plate.moveTo(box.left(), box.bottom() + 1);
            plate.lineTo(box.left(), box.top() + kRadius);
            plate.quadTo(box.left(), box.top(), box.left() + kRadius, box.top());
            plate.lineTo(box.right() - kRadius, box.top());
            plate.quadTo(box.right(), box.top(), box.right(), box.top() + kRadius);
            plate.lineTo(box.right(), box.bottom() + 1);
            plate.closeSubpath();
            painter.setPen(Qt::NoPen);
            painter.setBrush(current ? style.checkedBackground : style.hoverBackground);
            painter.drawPath(plate);
        }

        if (current) {
            // The underline: the tab's own colour when it has one, the accent
            // otherwise. NovuExplorer's contribution, and the reason the colour
            // lives on the tab. `.tab.on::after{left:8px;right:8px;height:2px;
            // border-radius:2px}` - inset from both ends in the underlined
            // strip, full width under a browser tab's plate.
            const QColor line = tab.color.isValid() ? tab.color : profile.colors.accent;
            painter.setPen(Qt::NoPen);
            painter.setBrush(line);
            const double inset = m_variant == Variant::Underline ? kUnderlineInset : 0.0;
            const QRectF rule(box.left() + inset, box.bottom() - kUnderlineHeight + 1,
                              box.width() - 2 * inset, kUnderlineHeight);
            if (m_variant == Variant::Underline) {
                painter.drawRoundedRect(rule, kUnderlineHeight, kUnderlineHeight);
            } else {
                painter.drawRect(rule);
            }
        } else if (tab.color.isValid()) {
            // Inactive tabs keep a trace of their colour, or a coloured set is
            // only ever one tab's worth of information. The same rule as the
            // current tab's, at a lower weight, so it reads as the same mark.
            painter.setPen(Qt::NoPen);
            painter.setBrush(withAlpha(tab.color, 0.55));
            const double inset = m_variant == Variant::Underline ? kUnderlineInset : 0.0;
            painter.drawRect(QRectF(box.left() + inset, box.bottom() - kUnderlineHeight + 1,
                                    box.width() - 2 * inset, kUnderlineHeight));
        }

        QColor ink = current ? style.checkedForeground
                             : (hot ? style.hoverForeground : style.foreground);
        if (tab.state == State::Asleep && !current) {
            // `.btab.sleeping{opacity:.62}` - and `.on` cancels it, because a
            // suspended tab you are looking at is not a tab in the background.
            ink = withAlpha(ink, kAsleepOpacity);
        }

        int left = box.left() + kPaddingH;
        const QRect close = closeRect(index);
        int rightEdge = close.isNull() ? box.right() - kPaddingH : close.left() - kGap;

        // ── the mark ─────────────────────────────────────────────────────
        //
        // A lettered mark is the document's own, and only a page still
        // arriving takes its place. Everything else states itself after the
        // label instead of overwriting what identifies the tab.
        const bool lettered = !tab.mark.isEmpty() && tab.state != State::Loading;
        if (lettered) {
            const QRectF tile(left, box.center().y() - kMarkTile / 2.0, kMarkTile,
                              kMarkTile);
            QLinearGradient plate(tile.topLeft(), tile.bottomRight());
            plate.setColorAt(0.0, current ? profile.colors.surfaceHover
                                          : profile.colors.panelBgAlt);
            plate.setColorAt(1.0, current ? profile.colors.surfaceActive
                                          : profile.colors.surfaceHover);
            painter.setPen(QPen(profile.colors.borderHover, 1));
            painter.setBrush(plate);
            painter.drawRoundedRect(tile.adjusted(0.5, 0.5, -0.5, -0.5), kMarkTileRadius,
                                    kMarkTileRadius);

            QFont letter = roleFont(this, TypeRole::Body);
            letter.setPixelSize(profile.type.xxs);
            letter.setWeight(QFont::DemiBold);
            painter.setFont(letter);
            painter.setPen(tab.state == State::Asleep
                               ? withAlpha(profile.colors.textSecondary, kAsleepOpacity)
                               : profile.colors.textSecondary);
            painter.drawText(tile, Qt::AlignCenter, tab.mark);
            left += kMarkTile + kGap;
        }

        const QRectF markBox(left, box.center().y() - kMark / 2.0, kMark, kMark);
        switch (lettered ? State::Idle : tab.state) {
        case State::Loading:
            motion::paintSpinner(&painter, markBox.adjusted(1, 1, -1, -1),
                                 profile.colors.border, profile.colors.accentLight, 1.6,
                                 profile.animation.spinMs, motion::enabled(this));
            left += kMark + kGap;
            break;
        case State::Failed:
            painter.setPen(Qt::NoPen);
            painter.setBrush(profile.semantic.danger.text);
            painter.drawEllipse(markBox.adjusted(4, 4, -4, -4));
            left += kMark + kGap;
            break;
        case State::Modified:
            painter.setPen(Qt::NoPen);
            painter.setBrush(profile.colors.accentLight);
            painter.drawEllipse(markBox.adjusted(4, 4, -4, -4));
            left += kMark + kGap;
            break;
        case State::Idle:
        case State::Asleep:
            if (tab.glyph != Glyph::None) {
                paintGlyph(&painter, tab.glyph, markBox,
                           tab.state == State::Asleep ? withAlpha(ink, kAsleepOpacity) : ink);
                left += kMark + kGap;
            }
            break;
        }

        // ── the state dot ──────────────────────────────────────
        if (lettered && (tab.state == State::Failed || tab.state == State::Asleep)) {
            const QRectF dot(rightEdge - kStateDot, box.center().y() - kStateDot / 2.0,
                             kStateDot, kStateDot);
            painter.setPen(Qt::NoPen);
            painter.setBrush(tab.state == State::Failed ? profile.semantic.danger.text
                                                        : profile.colors.borderHover);
            painter.drawEllipse(dot);
            rightEdge -= kStateDot + kGap;
        }

        // ── the speaker ──────────────────────────────────────────────────
        const QRect speaker = audioRect(index);
        if (!speaker.isNull()) {
            const bool muted = audio(tab.key) == Audio::Muted;
            const bool hotAudio = extras && extras->hoveredAudio == index;
            if (hotAudio) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(profile.colors.surfaceHover);
                painter.drawRoundedRect(QRectF(speaker).adjusted(-2, -2, 2, 2),
                                        profile.radii.small, profile.radii.small);
            }
            const QColor speakerInk = muted ? profile.colors.textDisabled
                                            : (hotAudio ? profile.colors.textPrimary
                                                        : profile.colors.textSecondary);
            paintGlyph(&painter, Glyph::Audio, QRectF(speaker), speakerInk);
            if (muted) {
                painter.setPen(QPen(speakerInk, kMuteStrike, Qt::SolidLine, Qt::RoundCap));
                painter.setBrush(Qt::NoBrush);
                painter.drawLine(QPointF(speaker.left() + 1, speaker.top() + 1),
                                 QPointF(speaker.right(), speaker.bottom()));
            }
            rightEdge = speaker.left() - kGap;
        }

        // ── the label ────────────────────────────────────────────────────
        const QFont font = tabFont(this, current);
        const QFontMetrics fm(font);
        painter.setFont(font);
        painter.setPen(ink);
        const QRect textBox(left, box.top(), qMax(0, rightEdge - left), box.height());
        if (textBox.width() > 8) {
            painter.drawText(textBox, Qt::AlignVCenter | Qt::AlignLeft,
                             fm.elidedText(tab.label, Qt::ElideRight, textBox.width()));
        }

        // ── the close box ────────────────────────────────────────────────
        //
        // Drawn on the current tab and on the hovered one, and nowhere else. A
        // strip of thirty close boxes is thirty invitations to lose work.
        if (!close.isNull() && (current || index == m_hovered)) {
            const bool hotClose = m_hoveredClose && index == m_hovered;
            if (hotClose) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(m_pressedClose ? profile.window.closePressedBg
                                                : profile.window.closeHoverBg);
                painter.drawRoundedRect(QRectF(close), profile.radii.small,
                                        profile.radii.small);
            }
            const double inset = (kCloseSide - kCloseGlyph) / 2.0;
            paintGlyph(&painter, Glyph::Close,
                       QRectF(close).adjusted(inset, inset, -inset, -inset),
                       hotClose ? profile.window.closeHoverGlyph
                                : profile.colors.textDisabled);
        }
    }

    painter.restore();

    // ── the ends of a scrolled strip ─────────────────────────────────────
    //
    // A fade into the band's own colour where tabs continue out of sight.
    if (m_variant == Variant::Browser) {
        const QColor band = profile.colors.panelBgAlt;
        QColor clear = band;
        clear.setAlpha(0);
        const int scroll = effectiveScroll();
        const int bandHeight = height() - rule;
        if (scroll > 0) {
            QLinearGradient fade(viewport.left(), 0, viewport.left() + kEdgeFade, 0);
            fade.setColorAt(0.0, band);
            fade.setColorAt(1.0, clear);
            painter.fillRect(QRect(viewport.left(), 0, kEdgeFade, bandHeight), fade);
        }
        if (scroll < maximumScrollOffset()) {
            QLinearGradient fade(viewport.right() - kEdgeFade, 0, viewport.right() + 1, 0);
            fade.setColorAt(0.0, clear);
            fade.setColorAt(1.0, band);
            painter.fillRect(QRect(viewport.right() - kEdgeFade + 1, 0, kEdgeFade, bandHeight),
                             fade);
        }
    }

    // ── the new-tab button ───────────────────────────────────────────────
    const QRect plus = newTabRect();
    if (!plus.isNull()) {
        if (m_hoveredNew) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(m_pressedNew ? profile.colors.surfaceActive
                                          : profile.colors.surfaceHover);
            painter.drawRoundedRect(QRectF(plus), profile.radii.small, profile.radii.small);
        }
        paintGlyph(&painter, Glyph::Plus, QRectF(plus).adjusted(7, 7, -7, -7),
                   m_hoveredNew ? profile.colors.textPrimary : profile.colors.textDisabled);
    }

    if (m_ring.visibleOn(this) && m_current >= 0) {
        painter.setClipRect(viewport.adjusted(0, 0, 1, 0));
        painter.setPen(QPen(profile.colors.accent, profile.border(BorderRole::Focus)));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(QRectF(tabRect(m_current)).adjusted(1, 1, -1, -2),
                                kRadius, kRadius);
    }

    if (property("novuMotionRegion").value<QRegion>() != motionRegion) {
        setProperty("novuMotionRegion", QVariant::fromValue(motionRegion));
    }
}

} // namespace novu::ui
