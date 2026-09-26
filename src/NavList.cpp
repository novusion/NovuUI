#include "novu/ui/NavList.h"

#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"

#include <QContextMenuEvent>
#include <QEvent>
#include <QFontMetrics>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

namespace novu::ui {

namespace {

// `.navbtn { padding:7px 9px; gap:10px; border-radius:9px }`. The vertical
// padding is a floor rather than the height: what a row measures comes from
// the profile's `controls.rowHeight`, and this only stops a large type scale
// from being clipped by a small token.
constexpr int kRowPaddingH = 9;
constexpr int kRowPaddingV = 2;
constexpr int kRowGap = 10;
constexpr int kRowRadiusBump = 5;   // over the profile's small radius

// `.navbtn svg { 16px }`, `.navbtn .tw { 16px }`.
constexpr int kGlyphSide = 16;
constexpr int kTwistySide = 16;

// The disclosure chevrons, as the sheet draws them: 11px in a 16px target,
// and heavier than the icon set's own 1.7 so a control does not read as a
// hairline. `.navbtn .tw svg{width:11px;stroke-width:2}` and
// `.sb-bhead .cv svg{width:11px;stroke-width:2.2}`.
constexpr int kTwistyGlyph = 11;
constexpr double kTwistyStroke = 2.0;
constexpr int kCaptionGlyph = 11;
constexpr double kCaptionStroke = 2.2;

// The glyph's own box, centred in the target it is hit-tested by.
QRectF centredSquare(const QRect& target, int side)
{
    return QRectF(target.center().x() - side / 2.0, target.center().y() - side / 2.0,
                  side, side);
}

// `.navbtn.child { padding-left:31px }`, and the 6px square marker in it.
// One tree level advances by five pixels from its parent's text axis. The
// remaining width is the row furniture (twisty, status and leaf marker), not
// hierarchy spacing, so it must not be counted as another indentation step.
constexpr int kChildIndent = 57;
// Where a level's rule sits: half a nesting step in from the row's own
// padding, which is where the indent the children take begins.
constexpr int kIndentStep = 17;
constexpr int kChildMarker = 6;

// `.navbtn .wsdot { 7px }`.
constexpr int kStatusDot = 7;

// `.navbtn .cnt { padding:1px 7px }`.
constexpr int kCountPaddingH = 7;
constexpr int kCountPaddingV = 1;

// `.navbtn.on::before { left:0; top:7px; bottom:7px; width:2.5px }`.
constexpr double kActiveBarWidth = 2.5;
constexpr int kActiveBarInset = 7;

// `.rail-sec { margin:18px 0 8px }` — NovuSpace's values, which the profile
// now states as SpacingRole::SectionGap and the small spacing step.
constexpr int kSectionActionSide = 16;

// `[disabled] { opacity:.42 }`.
constexpr double kDisabledOpacity = 0.42;

// The label's own floor. Not zero: a row that can show its glyph and its
// badge but not one character of its name is a row that answers "what is
// this" with nothing, and eliding everything away is indistinguishable from
// the label having been forgotten. Wide enough for an ellipsis and a couple
// of characters at the row's own type size, which is the same reasoning
// every elided label in this list already uses.
constexpr int kMinLabelWidth = 28;

QFont rowFont(const QWidget* context, bool child)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::Body);
    // `.navbtn{font-size:12.5px}` and `.navbtn.child{font-size:12px}`. 12.5
    // is not an integer pixel size, and the profile's own rounding of it is
    // `type.md` (13) - the same step Body already stands for. The child row
    // is exactly `type.sm`. Both were one step short of what the sheet says.
    font.setPixelSize(child ? profile.type.sm : profile.type.md);
    font.setWeight(QFont::Medium);
    return font;
}

QFont sectionFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleMonoFont(context, TypeRole::Eyebrow);
    font.setPixelSize(qMax(8, profile.type.xxs - 1));
    // `.sb-bhead{letter-spacing:.14em}` - 14% of the size, so 114.
    font.setLetterSpacing(QFont::PercentageSpacing, 114);
    return font;
}

QFont countFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleMonoFont(context, TypeRole::Mono);
    font.setPixelSize(qMax(8, profile.type.xxs));
    return font;
}

// A rail's ink, as the profile names it.
ColorRole inkRole(NavInk ink)
{
    switch (ink) {
    case NavInk::Secondary: return ColorRole::TextSecondary;
    case NavInk::Primary:   return ColorRole::TextPrimary;
    case NavInk::Accent:    return ColorRole::AccentLight;
    case NavInk::Muted:     break;
    }
    return ColorRole::TextDisabled;
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

} // namespace

NavList::NavList(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
    // A list draws rows and nothing between them, so it has no ground of its
    // own - and it must not invent one.
    //
    // It used to fill from QPalette::Window to give a scroll something to
    // paint over. That is the wrong ground twice: the rail it sits in is a
    // *gradient*, which no single palette colour can match, and the fill is
    // the widget's own rect, which is exactly as tall as its rows - so the
    // rail came out with a flat panel over the top of it that ended at a
    // different height in every module, which is precisely how a shared
    // sidebar stops looking shared.
    //
    // The scroll case is the host's: Sidebar's viewport paints the rail's
    // gradient on every exposure, so there is nothing left on the glass.
    setAttribute(Qt::WA_TranslucentBackground, false);
}

int NavList::addItem(const QString& key, const QString& label, Glyph glyph)
{
    Row row;
    row.key = key;
    row.label = label;
    row.glyph = glyph;
    m_rows.append(row);
    applyDisclosure();
    update();
    return m_rows.size() - 1;
}

int NavList::addChild(const QString& key, const QString& label, Glyph glyph)
{
    const int index = addItem(key, label, glyph);
    m_rows[index].child = true;
    // A child added under a shut parent must not appear, which is the whole
    // point of the list owning the fold: a caller streaming rows in cannot be
    // expected to check the state of the row above each one.
    applyDisclosure();
    return index;
}

void NavList::setBadge(const QString& key, const QString& text, CountTone tone)
{
    Row* target = row(key);
    if (!target) {
        return;
    }
    target->badge = text;
    target->countTone = tone;
    updateGeometry();
    update();
}

int NavList::addSection(const QString& title, const QString& key)
{
    Row row;
    row.key = key;
    row.label = title;
    row.section = true;
    // `renderRail()` wraps every group marker in a `.sb-block` whose head is
    // a toggle, in every module - so a caption folds unless an application
    // says otherwise, rather than only when one remembers to ask. It was
    // opt-in, and exactly one of the four modules opted in: that is how the
    // same rail ended up behaving four different ways.
    row.expandable = !key.isEmpty();
    // Shut, if this caption was shut the last time it was here.
    //
    // An application refreshes its rail by clear() + rebuild - that is what a
    // count changing or a workspace being renamed costs - and a caption that
    // forgets it was folded on every refresh is a caption nobody can fold.
    // The design says it outright: *collapsing WORKSPACES does not forget
    // which workspaces were open inside it*, and neither does it forget that
    // WORKSPACES itself is closed. The key is the caller's own stable
    // identifier, which is exactly what makes this safe across a rebuild.
    row.expanded = !m_shutSections.contains(key);
    m_rows.append(row);
    applyDisclosure();
    update();
    return m_rows.size() - 1;
}

void NavList::clear()
{
    // m_shutSections deliberately survives: see addSection().
    m_rows.clear();
    m_current = -1;
    m_hovered = -1;
    m_pressed = -1;
    updateGeometry();
    update();
}

int NavList::indexOfKey(const QString& key) const
{
    for (int i = 0; i < m_rows.size(); ++i) {
        if (!m_rows.at(i).section && m_rows.at(i).key == key) {
            return i;
        }
    }
    return -1;
}

QString NavList::keyAt(int index) const
{
    return index >= 0 && index < m_rows.size() ? m_rows.at(index).key : QString();
}

QStringList NavList::keys() const
{
    QStringList result;
    for (const Row& row : m_rows) {
        if (!row.section) {
            result.append(row.key);
        }
    }
    return result;
}

QString NavList::currentKey() const
{
    return keyAt(m_current);
}

NavList::Row* NavList::row(const QString& key)
{
    const int index = indexOfKey(key);
    return index < 0 ? nullptr : &m_rows[index];
}

const NavList::Row* NavList::row(const QString& key) const
{
    const int index = indexOfKey(key);
    return index < 0 ? nullptr : &m_rows.at(index);
}

// Captions are addressed separately, because indexOfKey() deliberately skips
// them -- a caption is not somewhere you can be, so it must never answer
// setCurrentKey(). Routing the section setters through it meant they looked
// up a row that by definition could not be found, and setSectionCollapsible()
// and setSectionAction() silently did nothing in every application that
// called them.
int NavList::indexOfSection(const QString& key) const
{
    if (key.isEmpty()) {
        return -1;
    }
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows.at(i).section && m_rows.at(i).key == key) {
            return i;
        }
    }
    return -1;
}

int NavList::indexOfRow(const Row& row) const
{
    const qsizetype offset = &row - m_rows.constData();
    return offset >= 0 && offset < m_rows.size() ? int(offset) : 0;
}

NavList::Row* NavList::sectionRow(const QString& key)
{
    const int index = indexOfSection(key);
    return index < 0 ? nullptr : &m_rows[index];
}

const NavList::Row* NavList::sectionRow(const QString& key) const
{
    const int index = indexOfSection(key);
    return index < 0 ? nullptr : &m_rows.at(index);
}

void NavList::setCurrentIndex(int index)
{
    if (index < -1 || index >= m_rows.size()) {
        return;
    }
    // A caption cannot be current: it is not somewhere you can be.
    if (index >= 0 && m_rows.at(index).section) {
        return;
    }
    if (m_current == index) {
        return;
    }
    m_current = index;
    update();
    emit currentChanged(currentKey());
}

void NavList::setCurrentKey(const QString& key)
{
    setCurrentIndex(indexOfKey(key));
}

void NavList::choose(int index)
{
    if (index < 0 || index >= m_rows.size()) {
        return;
    }
    const Row& row = m_rows.at(index);
    if (row.section || !row.enabled) {
        return;
    }
    setCurrentIndex(index);
    emit activated(row.key);
}

void NavList::setLabel(const QString& key, const QString& label)
{
    Row* target = row(key);
    if (!target || target->label == label) {
        return;
    }
    target->label = label;
    updateGeometry();
    update();
}

void NavList::setGlyph(const QString& key, Glyph glyph)
{
    Row* target = row(key);
    if (!target || target->glyph == glyph) {
        return;
    }
    target->glyph = glyph;
    update();
}

void NavList::setCount(const QString& key, int count, CountTone tone)
{
    Row* target = row(key);
    if (!target || (target->count == count && target->countTone == tone)) {
        return;
    }
    target->count = count;
    target->countTone = tone;
    updateGeometry();
    update();
}

void NavList::setStatus(const QString& key, StatusDot::Tone tone)
{
    Row* target = row(key);
    if (!target || (target->hasStatus && target->status == tone)) {
        return;
    }
    target->hasStatus = true;
    target->status = tone;
    update();
}

void NavList::clearStatus(const QString& key)
{
    Row* target = row(key);
    if (!target || !target->hasStatus) {
        return;
    }
    target->hasStatus = false;
    update();
}

void NavList::setRowEnabled(const QString& key, bool enabled)
{
    Row* target = row(key);
    if (!target || target->enabled == enabled) {
        return;
    }
    target->enabled = enabled;
    update();
}

void NavList::setIndentOffset(const QString& key, int pixels)
{
    Row* target = row(key);
    const int inset = qMax(0, pixels);
    if (!target || target->indentOffset == inset) {
        return;
    }
    target->indentOffset = inset;
    updateGeometry();
    update();
}

void NavList::setTreeGuides(bool enabled)
{
    if (m_treeGuides == enabled) {
        return;
    }
    m_treeGuides = enabled;
    update();
}

bool NavList::isRowEnabled(const QString& key) const
{
    const Row* target = row(key);
    return target && target->enabled;
}

void NavList::setExpandable(const QString& key, bool expandable)
{
    Row* target = row(key);
    if (!target || target->expandable == expandable) {
        return;
    }
    target->expandable = expandable;
    applyDisclosure();
    update();
}

void NavList::setExpanded(const QString& key, bool expanded)
{
    Row* target = row(key);
    if (!target || target->expanded == expanded) {
        return;
    }
    target->expanded = expanded;
    rememberFold(*target);
    // A caption and an expandable item both hide something; which one it is
    // is the pass's problem, not this setter's.
    applyDisclosure();
    update();
}

void NavList::setSectionCollapsible(const QString& key, bool collapsible)
{
    Row* target = sectionRow(key);
    if (!target) {
        return;
    }
    target->expandable = collapsible;
    if (collapsible && !target->expanded) {
        target->expanded = true;
    }
    applyDisclosure();
    update();
}

void NavList::setSectionAction(const QString& key, Glyph glyph, const QString& tooltip)
{
    Row* target = sectionRow(key);
    if (!target) {
        return;
    }
    target->action = glyph;
    target->actionTip = tooltip;
    update();
}

QRect NavList::sectionActionRect(int index) const
{
    if (index < 0 || index >= m_rows.size()) {
        return QRect();
    }
    const Row& row = m_rows.at(index);
    if (!row.section || row.action == Glyph::None || m_collapsed) {
        return QRect();
    }
    const QRect box = rowRect(index);
    const int top = box.top() + sectionTop(index)
        + (box.height() - sectionTop(index) - sectionBottom() - kSectionActionSide) / 2;
    return QRect(width() - kRowPaddingH - kSectionActionSide, top,
                 kSectionActionSide, kSectionActionSide);
}

void NavList::setStatusPlacement(StatusPlacement placement)
{
    if (m_statusPlacement == placement) {
        return;
    }
    m_statusPlacement = placement;
    updateGeometry();
    update();
}

// A collapsed section takes everything under it with it, up to the next
// section. Visibility rather than removal: the rows keep their state, and a
// hidden row is already zero-height everywhere else in this class.
// The whole disclosure model, in one pass and one place.
//
// A row is drawn unless something above it is shut. There are exactly three
// things that can shut it and they compose rather than override: the caller's
// own filter, the caption of the section it is in, and - for a child - the
// expandable row it hangs from. Working them out per row, in list order, is
// why this is a single loop: a caption resets the parent state because a new
// section starts a new group, and a top-level row ends the previous group
// whether or not it opens one of its own.
// Which captions are shut, by key, so a rebuild can put them back. Only
// captions: what an expandable *row* reveals is the caller's data, and a
// caller that rebuilds the list decides for itself which branches it sends.
void NavList::rememberFold(const Row& row)
{
    if (!row.section || row.key.isEmpty()) {
        return;
    }
    if (row.expanded) {
        m_shutSections.remove(row.key);
    } else {
        m_shutSections.insert(row.key);
    }
}

void NavList::applyDisclosure()
{
    bool sectionShut = false;
    bool parentShut = false;
    for (Row& row : m_rows) {
        if (row.section) {
            sectionShut = row.expandable && !row.expanded;
            parentShut = false;
            continue;
        }
        if (!row.child) {
            parentShut = row.expandable && !row.expanded;
            row.visible = !row.hiddenByFilter && !sectionShut;
            continue;
        }
        row.visible = !row.hiddenByFilter && !sectionShut && !parentShut;
    }
    updateGeometry();
}

bool NavList::isExpanded(const QString& key) const
{
    // A caption or an expandable item, whichever carries the key.
    const Row* target = row(key);
    if (!target) {
        target = sectionRow(key);
    }
    return target && target->expanded;
}

void NavList::setSectionBadge(const QString& key, const QString& text)
{
    Row* target = sectionRow(key);
    if (!target || target->sectionBadge == text) {
        return;
    }
    target->sectionBadge = text;
    update();
}

void NavList::setSectionExpanded(const QString& key, bool expanded)
{
    Row* target = sectionRow(key);
    if (!target || target->expanded == expanded) {
        return;
    }
    target->expanded = expanded;
    rememberFold(*target);
    applyDisclosure();
    update();
}

void NavList::setCollapsed(bool collapsed)
{
    if (m_collapsed == collapsed) {
        return;
    }
    m_collapsed = collapsed;
    updateGeometry();
    update();
}

QString NavList::label(const QString& key) const
{
    const Row* r = row(key);
    return r ? r->label : QString();
}

bool NavList::isRowVisible(const QString& key) const
{
    const Row* r = row(key);
    return r ? r->visible : false;
}

void NavList::setRowVisible(const QString& key, bool visible)
{
    Row* r = row(key);
    if (!r || r->hiddenByFilter == !visible) {
        return;
    }
    // The caller's own answer is recorded separately from what is drawn: a
    // row that a filter keeps must still stay hidden while the caption above
    // it is shut, and must come back when it opens.
    r->hiddenByFilter = !visible;
    applyDisclosure();
    update();
}

int NavList::leadingZoneWidth(const Row& row) const
{
    int left = (row.child ? kChildIndent : kRowPaddingH) + row.indentOffset;
    if (row.expandable) {
        left += kTwistySide + kRowGap;
    }
    if (row.glyph != Glyph::None) {
        left += kGlyphSide + kRowGap;
    }
    if (row.hasStatus && m_statusPlacement == StatusPlacement::Leading) {
        left += kStatusDot + kRowGap;
    }
    return left;
}

int NavList::trailingZoneWidth(const Row& row) const
{
    int right = kRowPaddingH;
    if (row.count >= 0 || !row.badge.isEmpty()) {
        const QString text = row.badge.isEmpty() ? QString::number(row.count) : row.badge;
        right += QFontMetrics(countFont(this)).horizontalAdvance(text)
            + (m_plainCounts ? 0 : 2 * kCountPaddingH)
            + kRowGap;
    }
    if (row.hasStatus && m_statusPlacement == StatusPlacement::Trailing) {
        right += kStatusDot + kRowGap;
    }
    return right;
}

int NavList::rowMinimumWidth(const Row& row) const
{
    if (row.section || !row.visible) {
        return 0;
    }
    return leadingZoneWidth(row) + trailingZoneWidth(row) + kMinLabelWidth;
}

int NavList::rowHeight(const Row& row) const
{
    // A hidden row is zero-height, which is the whole of hiding it: rowRect()
    // collapses, an empty rect contains no click, and paintEvent draws nothing
    // into it. One condition rather than a filter in four places.
    if (!row.visible) {
        return 0;
    }
    if (row.section) {
        // paintEvent() never draws `.sb-bhead` on a collapsed rail - the
        // caption reserving its full height anyway was a gap nothing sat in,
        // wide enough to read as several rows missing rather than as one
        // hidden caption.
        if (m_collapsed) {
            return 0;
        }
        return sectionHeight(indexOfRow(row));
    }
    // The profile states the navigation row height. The content is the
    // floor, so a product with a larger type scale cannot be clipped by a
    // smaller token.
    const ThemeProfile& profile = themeFor(this);
    const QFontMetrics fm(rowFont(this, row.child));
    return qMax(profile.controls.navRowHeight,
                qMax(fm.height(), kGlyphSide) + 2 * kRowPaddingV);
}

int NavList::sectionTop(int index) const
{
    const ThemeProfile& profile = themeFor(this);
    // A caption directly under another caption is not being separated from
    // anything: the pair is one heading and its first group -- the approved
    // sidebars write it as a block head with a section inside it -- and the
    // full gap between them opens a hole where the design has a step.
    if (index > 0 && m_rows.at(index - 1).section) {
        return profile.spacing.xs;
    }
    return profile.space(SpacingRole::SectionGap);
}

int NavList::sectionBottom() const
{
    return themeFor(this).controls.navSectionBottom;
}

int NavList::sectionHeight(int index) const
{
    return QFontMetrics(sectionFont(this)).height() + sectionTop(index)
        + sectionBottom();
}

QRect NavList::rowRect(int index) const
{
    if (index < 0 || index >= m_rows.size()) {
        return QRect();
    }
    int y = 0;
    for (int i = 0; i < index; ++i) {
        y += rowHeight(m_rows.at(i));
    }
    return QRect(0, y, width(), rowHeight(m_rows.at(index)));
}

QRect NavList::twistyRect(int index) const
{
    if (index < 0 || index >= m_rows.size()) {
        return QRect();
    }
    const Row& row = m_rows.at(index);
    if (!row.expandable || m_collapsed) {
        return QRect();
    }
    const QRect box = rowRect(index);
    const int left = (row.child ? kChildIndent : kRowPaddingH) + row.indentOffset;
    if (row.section) {
        // Centred on the caption rather than on the whole block: a section is
        // mostly the margin above it.
        const int top = box.top() + sectionTop(index)
            + (box.height() - sectionTop(index) - sectionBottom() - kTwistySide) / 2;
        return QRect(left, top, kTwistySide, kTwistySide);
    }
    return QRect(left, box.top() + (box.height() - kTwistySide) / 2,
                 kTwistySide, kTwistySide);
}

int NavList::rowMinimumWidth(int index) const
{
    if (index < 0 || index >= m_rows.size()) {
        return 0;
    }
    return rowMinimumWidth(m_rows.at(index));
}

int NavList::indexAt(const QPoint& position) const
{
    int y = 0;
    for (int i = 0; i < m_rows.size(); ++i) {
        const int height = rowHeight(m_rows.at(i));
        if (position.y() >= y && position.y() < y + height) {
            return i;
        }
        y += height;
    }
    return -1;
}

int NavList::neighbour(int from, int step) const
{
    for (int i = from + step; i >= 0 && i < m_rows.size(); i += step) {
        // Hidden rows are skipped here too: a filtered list that the arrow
        // keys still walk through is a list that jumps to a row nobody can see.
        if (!m_rows.at(i).section && m_rows.at(i).enabled && m_rows.at(i).visible) {
            return i;
        }
    }
    return -1;
}

StyleSpec NavList::styleSpec() const
{
    StyleSpec spec;
    spec.background = token(ColorRole::Transparent);
    spec.foreground = token(inkRole(themeFor(this).controls.navRestInk));
    spec.border = token(ColorRole::Transparent);
    spec.radius = SurfaceRole::Toolbar;
    spec.borderWidth = BorderRole::Divider;
    spec.type = TypeRole::Body;
    return spec;
}

StyleSpec NavList::rowStyleSpec(int index) const
{
    StyleSpec spec = styleSpec();
    spec.radius = SurfaceRole::Button;

    if (index < 0 || index >= m_rows.size()) {
        return spec;
    }
    const Row& row = m_rows.at(index);
    if (row.section) {
        spec.type = TypeRole::Eyebrow;
        return spec;
    }

    // `.navbtn:hover { background:rgba(127,140,160,.09) }` — a wash over
    // whatever the rail is, not a surface swap.
    spec.hoverBackground = token(ColorRole::SurfaceHover, 0.55);
    spec.hoverForeground = token(ColorRole::TextSecondary);
    // `.navbtn.on { background:rgba(--rl,.13); color:--acc-l }`.
    spec.checkedBackground = token(ColorRole::Accent, 0.13);
    spec.checkedForeground =
        token(inkRole(themeFor(this).controls.navCurrentInk));
    spec.disabledForeground = token(ColorRole::TextDisabled, kDisabledOpacity);
    return spec;
}

ResolvedStyle NavList::resolvedStyle() const
{
    return resolve(styleSpec(), this, 0);
}

QSize NavList::sizeHint() const
{
    int height = 0;
    int width = 0;
    for (const Row& row : m_rows) {
        height += rowHeight(row);
        if (m_collapsed) {
            continue;
        }
        if (row.section) {
            width = qMax(width, 2 * kRowPaddingH
                                    + QFontMetrics(sectionFont(this))
                                          .horizontalAdvance(row.label.toUpper()));
            continue;
        }
        int own = 2 * kRowPaddingH + kRowGap
            + QFontMetrics(rowFont(this, row.child)).horizontalAdvance(row.label);
        own += row.indentOffset;
        if (row.child) {
            own += kChildIndent - kRowPaddingH;
        } else if (row.glyph != Glyph::None) {
            own += kGlyphSide + kRowGap;
        }
        if (row.expandable) {
            own += kTwistySide + kRowGap;
        }
        if (row.hasStatus) {
            own += kStatusDot + kRowGap;
        }
        if (row.count >= 0) {
            own += kRowGap + 2 * kCountPaddingH
                + QFontMetrics(countFont(this)).horizontalAdvance(QString::number(row.count));
        }
        width = qMax(width, own);
    }
    if (m_collapsed) {
        width = kGlyphSide + 2 * kRowPaddingH + 8;
    }
    return QSize(qMax(width, 48), height);
}

QSize NavList::minimumSizeHint() const
{
    if (m_collapsed) {
        return QSize(sizeHint().width(), sizeHint().height());
    }
    // The widest floor any one visible row actually needs - its own glyph,
    // twisty, dot and badge, plus enough for the label to still say
    // something - rather than a flat number unrelated to what is in the
    // list. A container that respects this can never hand a row less room
    // than its own furniture requires, which is what made a badge start
    // drawing over the glyph next to it: nothing said the row could not go
    // narrower than that.
    int floor = 0;
    for (const Row& r : m_rows) {
        floor = qMax(floor, rowMinimumWidth(r));
    }
    return QSize(qMax(floor, 48), sizeHint().height());
}

bool NavList::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        updateGeometry();
        update();
    }
    return QWidget::event(event);
}

void NavList::mousePressEvent(QMouseEvent* event)
{
    // A click marks the row; it does not ask for the keyboard's ring.
    m_ring.noteMousePress();
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    const int index = indexAt(event->pos());
    if (index >= 0 && m_rows.at(index).section) {
        // A section heading is not selectable, but its disclosure and its
        // action are both targets.
        const bool onAction = sectionActionRect(index).contains(event->pos());
        // `<button class="sb-bhead" data-block-toggle=...>` - the whole head
        // folds the block, not the 16px chevron alone. The `+` is hit-tested
        // first, because the design requires that creating something must not
        // collapse the block it is created in.
        const bool onTwisty = !onAction && m_rows.at(index).expandable;
        if (onAction || onTwisty) {
            m_pressed = index;
            m_pressedAction = onAction;
            m_pressedTwisty = onTwisty;
            update();
            return;
        }
    }
    if (index < 0 || m_rows.at(index).section || !m_rows.at(index).enabled) {
        m_pressed = -1;
        QWidget::mousePressEvent(event);
        return;
    }
    m_pressed = index;
    m_pressedAction = false;
    // The twisty is a separate target: expanding a workspace and opening it
    // are different actions on the same line.
    m_pressedTwisty = twistyRect(index).contains(event->pos());
    update();
}

void NavList::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || m_pressed < 0) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    const int pressed = m_pressed;
    const bool pressedTwisty = m_pressedTwisty;
    const bool pressedAction = m_pressedAction;
    m_pressed = -1;
    m_pressedTwisty = false;
    m_pressedAction = false;

    const int released = indexAt(event->pos());

    if (pressedAction) {
        if (released == pressed && sectionActionRect(released).contains(event->pos())) {
            emit sectionActionTriggered(m_rows.at(pressed).key);
        }
        update();
        return;
    }

    const bool releasedTwisty = released == pressed
        && (m_rows.at(qBound(0, released, m_rows.size() - 1)).section
                ? m_rows.at(released).expandable
                : twistyRect(released).contains(event->pos()));

    if (released == pressed && releasedTwisty == pressedTwisty) {
        if (pressedTwisty) {
            Row& row = m_rows[pressed];
            row.expanded = !row.expanded;
            // The rows under it go with it, here, once - not in every
            // application that happens to put a tree in a rail.
            const QString key = row.key;
            const bool open = row.expanded;
            rememberFold(row);
            applyDisclosure();
            emit expandToggled(key, open);
        } else {
            choose(pressed);
        }
    }
    update();
}

void NavList::mouseMoveEvent(QMouseEvent* event)
{
    const int index = indexAt(event->pos());
    const bool onSection = index >= 0 && m_rows.at(index).section;
    // A section heading does not highlight, but its two affordances do, so it
    // still counts as hovered while the pointer is over one of them.
    const bool onAction = onSection && sectionActionRect(index).contains(event->pos());
    // A caption that folds is a control, and the sheet gives it a hover
    // plate of its own - `.sb-bhead:hover`. One that does not fold still
    // highlights only its `+`.
    const bool onFoldableCaption = onSection && m_rows.at(index).expandable;
    const int hovered = (index >= 0
                         && ((!onSection && m_rows.at(index).enabled) || onAction
                             || onFoldableCaption))
        ? index
        : -1;
    const bool onTwisty = hovered >= 0 && !onSection
        && twistyRect(hovered).contains(event->pos());
    if (hovered != m_hovered || onTwisty != m_hoveredTwisty
        || onAction != m_hoveredAction) {
        m_hovered = hovered;
        m_hoveredTwisty = onTwisty;
        m_hoveredAction = onAction;
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void NavList::leaveEvent(QEvent* event)
{
    if (m_hovered != -1 || m_hoveredTwisty) {
        m_hovered = -1;
        m_hoveredTwisty = false;
        update();
    }
    QWidget::leaveEvent(event);
}


void NavList::contextMenuEvent(QContextMenuEvent* event)
{
    // A caption has no menu: it is not a thing, it is a label over things.
    const int index = event->reason() == QContextMenuEvent::Keyboard
        ? m_current
        : indexAt(event->pos());
    if (index < 0 || index >= m_rows.size() || m_rows.at(index).section) {
        event->ignore();
        return;
    }
    event->accept();
    emit contextMenuRequested(m_rows.at(index).key, event->globalPos());
}

void NavList::focusInEvent(QFocusEvent* event)
{
    // The same FocusRing every other chrome control shares - ModuleBar,
    // TabBar - rather than this list keeping its own copy of "which
    // QFocusEvent reasons count as the keyboard". A second copy is how one
    // of the two drifted: this one used to exclude only mouse and popup
    // reasons and treated a window merely activating as a keystroke, so a
    // Gallery page that had never been clicked still opened with a row
    // wearing a ring nobody had tabbed to.
    m_ring.noteFocusIn(event->reason());
    QWidget::focusInEvent(event);
    update();
}

void NavList::keyPressEvent(QKeyEvent* event)
{
    // Driving the list from the keyboard is exactly when the ring is for.
    m_ring.noteKeyPress();
    switch (event->key()) {
    case Qt::Key_Down: {
        const int next = neighbour(m_current, 1);
        if (next >= 0) {
            setCurrentIndex(next);
        }
        return;
    }
    case Qt::Key_Up: {
        const int previous = neighbour(m_current < 0 ? m_rows.size() : m_current, -1);
        if (previous >= 0) {
            setCurrentIndex(previous);
        }
        return;
    }
    case Qt::Key_Right:
    case Qt::Key_Left: {
        // The twisty, from the keyboard. Only on a row that has one — on any
        // other row the arrow keys belong to whatever contains this list.
        if (m_current >= 0 && m_rows.at(m_current).expandable) {
            const bool open = event->key() == Qt::Key_Right;
            if (m_rows.at(m_current).expanded != open) {
                m_rows[m_current].expanded = open;
                applyDisclosure();
                update();
                emit expandToggled(m_rows.at(m_current).key, open);
            }
            return;
        }
        break;
    }
    case Qt::Key_Space:
    case Qt::Key_Return:
    case Qt::Key_Enter:
        choose(m_current);
        return;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void NavList::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QFont sections = sectionFont(this);
    const QFont counts = countFont(this);
    const QFontMetrics countMetrics(counts);

    int y = 0;
    for (int index = 0; index < m_rows.size(); ++index) {
        const Row& row = m_rows.at(index);
        // Disclosure and filtering keep rows in the model with zero height.
        // Do not paint their out-of-flow furniture (twisty, status dot or
        // badge): unlike text, those primitives are centred around the empty
        // row rectangle and would otherwise leak below a collapsed section.
        if (!row.visible) {
            continue;
        }
        const int height = rowHeight(row);
        const QRect box(0, y, width(), height);
        y += height;

        if (!box.intersects(rect())) {
            continue;   // scrolled out of view; nothing to draw
        }

        // ── a caption ────────────────────────────────────────────────────
        if (row.section) {
            if (m_collapsed) {
                continue;   // the design hides `.sb-bhead` on a collapsed rail
            }
            // `.sb-bhead{border-radius:7px}` with `:hover{background:
            // rgba(127,140,160,.09); color:var(--text2); opacity:1}`.
            const bool captionHot = index == m_hovered && !m_hoveredAction;
            if (captionHot && row.expandable) {
                // A section heading is an interactive sidebar item too. Its
                // outer block also carries the gap between groups, but the
                // visible hover plate must use the same height as every
                // navigation row rather than shrinking around the caption.
                const int plateHeight = profile.controls.navRowHeight;
                const QRect plate(kRowPaddingH - 3,
                                  box.bottom() - plateHeight + 1,
                                  box.width() - 2 * (kRowPaddingH - 3),
                                  plateHeight);
                painter.setPen(Qt::NoPen);
                painter.setBrush(token(ColorRole::SurfaceHover, 0.55).resolve(profile));
                painter.drawRoundedRect(QRectF(plate), profile.radii.small,
                                        profile.radii.small);
            }
            painter.setFont(sections);
            painter.setPen(captionHot && row.expandable ? profile.colors.textSecondary
                                                        : profile.colors.textDisabled);
            int captionLeft = kRowPaddingH;
            if (row.expandable) {
                const QRect twisty = twistyRect(index);
                // `.sb-bhead .cv svg{width:11px;height:11px;stroke-width:2.2}`.
                paintGlyph(&painter, Glyph::ChevronDown,
                           centredSquare(twisty, kCaptionGlyph),
                           profile.colors.textDisabled, row.expanded ? 0.0 : -90.0,
                           kCaptionStroke);
                captionLeft = twisty.right() + 6;
            }
            if (row.action != Glyph::None) {
                const QRect act = sectionActionRect(index);
                paintGlyph(&painter, row.action, QRectF(act).adjusted(3, 3, -3, -3),
                           index == m_hovered && m_hoveredAction
                               ? profile.colors.textSecondary
                               : profile.colors.textDisabled);
            }
            const QRect captionBox = box.adjusted(captionLeft, sectionTop(index),
                                                  -kRowPaddingH - kSectionActionSide,
                                                  -sectionBottom());
            painter.drawText(captionBox, Qt::AlignLeft | Qt::AlignVCenter,
                             row.label.toUpper());
            if (!row.sectionBadge.isEmpty()) {
                // Right after the name, not against the far edge: the count
                // belongs to the word, and a caption is short.
                const int nameWidth =
                    QFontMetrics(sections).horizontalAdvance(row.label.toUpper());
                painter.setFont(counts);
                painter.drawText(captionBox.adjusted(nameWidth + kRowGap, 0, 0, 0),
                                 Qt::AlignLeft | Qt::AlignVCenter, row.sectionBadge);
                painter.setFont(sections);
            }
            continue;
        }

        const ResolvedStyle style = resolve(rowStyleSpec(index), this, height);
        const bool current = index == m_current;
        const bool hot = index == m_hovered && !current;

        // ── the plate ────────────────────────────────────────────────────
        //
        // Dimmed while this panel is not the active one, so a window with a
        // rail and a tree does not show two current rows. See
        // selectionIsActive() - the rule is NovuExplorer's, and it is right.
        const double selectionStrength =
            selectionIsActive(this) ? 1.0 : inactiveSelectionOpacity;

        const double radius = profile.radii.small + kRowRadiusBump;
        if (current && style.checkedBackground.alpha() > 0) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(withAlpha(style.checkedBackground,
                                       style.checkedBackground.alphaF()
                                           * selectionStrength));
            painter.drawRoundedRect(QRectF(box), radius, radius);
        } else if (hot && style.hoverBackground.alpha() > 0) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(style.hoverBackground);
            painter.drawRoundedRect(QRectF(box), radius, radius);
        }

        // `.navbtn.on::before` — the marker against the rail's left edge. It
        // is what tells the current row from a merely hovered one when the two
        // tints are close, so it is drawn even at the collapsed width.
        if (current) {
            QPainterPath bar;
            bar.addRoundedRect(QRectF(0.0, box.top() + kActiveBarInset, kActiveBarWidth,
                                      box.height() - 2.0 * kActiveBarInset),
                               kActiveBarWidth / 2.0, kActiveBarWidth / 2.0);
            painter.setPen(Qt::NoPen);
            painter.setBrush(withAlpha(profile.colors.accent, selectionStrength));
            painter.drawPath(bar);
        }

        QColor ink = style.foreground;
        if (current) {
            ink = style.checkedForeground;
        } else if (hot) {
            ink = style.hoverForeground;
        }
        if (!row.enabled) {
            ink = withAlpha(ink, kDisabledOpacity);
        }

        // ── the leading zone ─────────────────────────────────────────────
        int left = (row.child ? kChildIndent : kRowPaddingH) + row.indentOffset;
        if (m_collapsed) {
            left = (width() - kGlyphSide) / 2;
        }

        if (row.expandable && !m_collapsed) {
            const QRect twisty = twistyRect(index);
            if (m_hoveredTwisty && index == m_hovered) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(withAlpha(profile.colors.textSecondary, 0.16));
                painter.drawRoundedRect(QRectF(twisty), profile.radii.xs + 2,
                                        profile.radii.xs + 2);
            }
            // `.tw` points right when shut and is rotated 90 degrees when open.
            // Drawn from the same chevron the disclosure uses, so the two
            // cannot disagree about which way "open" points.
            // `.navbtn .tw svg{width:11px;height:11px;stroke-width:2}` inside
            // the 16px target - it was drawn at 10 in the body weight, which
            // is what made the disclosure read as a hairline.
            paintGlyph(&painter, Glyph::ChevronDown, centredSquare(twisty, kTwistyGlyph),
                       ink, row.expanded ? 0.0 : -90.0, kTwistyStroke);
            left = twisty.right() + kRowGap;
        }

        if (row.child && !m_collapsed) {
            if (m_treeGuides
                || profile.controls.navChildMark == NavChildMark::Guide) {
                // A classic tree branch: consecutive children share the
                // vertical rule and the last child closes it at its elbow.
                painter.setPen(QPen(profile.colors.borderSubtle, 1));
                const double guide = kRowPaddingH + row.indentOffset
                    + kChildIndent / 2.0;
                bool hasNextSibling = false;
                for (int next = index + 1; next < m_rows.size(); ++next) {
                    const Row& candidate = m_rows.at(next);
                    if (!candidate.visible)
                        continue;
                    hasNextSibling = candidate.child;
                    break;
                }
                painter.drawLine(QPointF(guide, box.top()),
                                 QPointF(guide, hasNextSibling
                                                   ? box.bottom() + 1
                                                   : box.center().y()));
                painter.drawLine(QPointF(guide, box.center().y()),
                                 QPointF(left - 8, box.center().y()));
            } else if (!row.expandable) {
                // `.navbtn.child .dot` — a small square, accent when current.
                const QRectF marker(left - kChildMarker - kRowGap + 4,
                                    box.center().y() - kChildMarker / 2.0,
                                    kChildMarker, kChildMarker);
                painter.setPen(Qt::NoPen);
                painter.setBrush(current ? profile.colors.accent
                                         : profile.colors.border);
                painter.drawRoundedRect(marker, 2, 2);
            }
        }

        const QRectF glyphBox(left, box.center().y() - kGlyphSide / 2.0,
                              kGlyphSide, kGlyphSide);
        if (row.glyph != Glyph::None) {
            paintGlyph(&painter, row.glyph, glyphBox,
                       current && m_accentCurrentGlyph ? profile.colors.accent : ink);
            left = static_cast<int>(glyphBox.right()) + kRowGap;
        } else if (m_collapsed) {
            // A glyph-less row - most often a child added without one - still
            // needs a mark in an icon-only rail, or the height its row
            // reserves is a gap with nothing in it rather than a row. The
            // same leaf marker an expanded child with no twisty already
            // draws beside its label, centred here instead.
            const QRectF marker(glyphBox.center().x() - kChildMarker / 2.0,
                                glyphBox.center().y() - kChildMarker / 2.0,
                                kChildMarker, kChildMarker);
            painter.setPen(Qt::NoPen);
            painter.setBrush(current ? profile.colors.accent : profile.colors.border);
            painter.drawRoundedRect(marker, 2, 2);
            left = static_cast<int>(glyphBox.right()) + kRowGap;
        }

        // One drawing, either side of the row. `.wsdot.archived` is an
        // outline rather than a fill: an archived workspace is present
        // without being live, and a filled grey dot says "idle", which is a
        // different thing.
        const auto paintStatus = [&](double x) {
            const QColor hue = row.status == StatusDot::Tone::Accent
                ? profile.colors.accentLight
                : profile.semantic.role(roleFor(row.status)).text;
            const QRectF dot(x, box.center().y() - kStatusDot / 2.0,
                             kStatusDot, kStatusDot);
            if (row.status == StatusDot::Tone::Neutral) {
                painter.setPen(QPen(profile.colors.border, 1));
                painter.setBrush(Qt::NoBrush);
            } else {
                painter.setPen(Qt::NoPen);
                painter.setBrush(hue);
            }
            painter.drawEllipse(dot);
        };

        if (row.hasStatus && (m_statusPlacement == StatusPlacement::Leading
                              || m_collapsed)) {
            paintStatus(left);
            left += kStatusDot + kRowGap;
        }

        if (m_collapsed) {
            continue;   // no label, no count - the design hides both
        }

        // ── the trailing zone ────────────────────────────────────────────
        int rightEdge = width() - kRowPaddingH;
        // A container that honours minimumSizeHint() never reaches this, but
        // one forced narrower anyway - a fixed width set below the row's own
        // floor, a splitter mid-drag - must not draw a badge over the glyph
        // it would otherwise land on. Losing the badge for one frame is
        // correct; painting it on top of something else never is.
        const bool badgeFits = (row.count >= 0 || !row.badge.isEmpty())
            && rightEdge - trailingZoneWidth(row) + kRowPaddingH >= left;
        if (badgeFits) {
            // A word says what a number cannot, so it wins where both are set.
            const QString text = row.badge.isEmpty() ? QString::number(row.count)
                                                     : row.badge;
            const int pillWidth = countMetrics.horizontalAdvance(text)
                + (m_plainCounts ? 0 : 2 * kCountPaddingH);
            const int pillHeight = countMetrics.height() + 2 * kCountPaddingV;
            const QRectF pill(rightEdge - pillWidth, box.center().y() - pillHeight / 2.0,
                              pillWidth, pillHeight);

            const bool filled =
                profile.controls.navCountStyle == NavCountStyle::Filled;
            QColor pillInk = profile.colors.textDisabled;
            // `.sb-count{background:var(--card)}` carries no outline of its
            // own; `.cnt` is an outline over the window. One or the other.
            QColor pillBorder = filled ? QColor(Qt::transparent)
                                       : profile.colors.border;
            QColor pillFill = filled ? profile.colors.surfaceHover
                                     : profile.colors.windowBg;
            switch (row.countTone) {
            case CountTone::Alert:
                pillInk = profile.semantic.warning.text;
                pillBorder = profile.semantic.warning.border;
                pillFill = profile.semantic.warning.fill;
                break;
            case CountTone::Bad:
                pillInk = profile.semantic.danger.text;
                pillBorder = profile.semantic.danger.border;
                pillFill = profile.semantic.danger.fill;
                break;
            case CountTone::Normal:
                if (current) {
                    // `.navbtn.on .cnt` — the pill joins the row rather than
                    // staying a neutral chip on a tinted plate.
                    pillInk = profile.colors.accentLight;
                    pillBorder = withAlpha(profile.colors.accent, 0.42);
                    pillFill = Qt::transparent;
                }
                break;
            }
            if (!row.enabled) {
                pillInk = withAlpha(pillInk, kDisabledOpacity);
                pillBorder = withAlpha(pillBorder, kDisabledOpacity);
            }

            if (!m_plainCounts) {
                painter.setPen(QPen(pillBorder, 1));
                painter.setBrush(pillFill);
                painter.drawRoundedRect(pill, pillHeight / 2.0, pillHeight / 2.0);
            }
            painter.setFont(counts);
            painter.setPen(pillInk);
            painter.drawText(pill, Qt::AlignCenter, text);
            rightEdge = static_cast<int>(pill.left()) - kRowGap;
        }

        if (row.hasStatus && m_statusPlacement == StatusPlacement::Trailing
            && rightEdge - kStatusDot >= left) {
            paintStatus(rightEdge - kStatusDot);
            rightEdge -= kStatusDot + kRowGap;
        }

        const QFont font = rowFont(this, row.child);
        const QFontMetrics fm(font);
        painter.setFont(font);
        painter.setPen(ink);
        const QRect textBox(left, box.top(), qMax(0, rightEdge - left), box.height());
        painter.drawText(textBox, Qt::AlignVCenter | Qt::AlignLeft,
                         fm.elidedText(row.label, Qt::ElideRight, textBox.width()));
    }

    // The focus ring goes round the current row, not round the list: a list
    // that outlines itself tells you nothing about where the arrow keys will
    // take you next.
    if (m_ring.visibleOn(this) && m_current >= 0) {
        const QRect box = rowRect(m_current);
        painter.setPen(QPen(profile.colors.accent, profile.border(BorderRole::Focus)));
        painter.setBrush(Qt::NoBrush);
        const double radius = profile.radii.small + kRowRadiusBump;
        painter.drawRoundedRect(QRectF(box).adjusted(1.0, 1.0, -1.0, -1.0), radius, radius);
    }
}

} // namespace novu::ui
