#include "novu/ui/NavList.h"

#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

namespace novu::ui {

namespace {

// `.navbtn { padding:7px 9px; gap:10px; border-radius:9px }`.
constexpr int kRowPaddingH = 9;
constexpr int kRowPaddingV = 7;
constexpr int kRowGap = 10;
constexpr int kRowRadiusBump = 5;   // over the profile's small radius

// `.navbtn svg { 16px }`, `.navbtn .tw { 16px }`.
constexpr int kGlyphSide = 16;
constexpr int kTwistySide = 16;

// `.navbtn.child { padding-left:31px }`, and the 6px square marker in it.
constexpr int kChildIndent = 31;
constexpr int kChildMarker = 6;

// `.navbtn .wsdot { 7px }`.
constexpr int kStatusDot = 7;

// `.navbtn .cnt { padding:1px 7px }`.
constexpr int kCountPaddingH = 7;
constexpr int kCountPaddingV = 1;

// `.navbtn.on::before { left:0; top:7px; bottom:7px; width:2.5px }`.
constexpr double kActiveBarWidth = 2.5;
constexpr int kActiveBarInset = 7;

// `.rail-sec { margin:18px 0 8px }`.
constexpr int kSectionTop = 18;
constexpr int kSectionBottom = 8;

// `[disabled] { opacity:.42 }`.
constexpr double kDisabledOpacity = 0.42;

QFont rowFont(const QWidget* context, bool child)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::Body);
    font.setPixelSize(child ? profile.type.sm - 1 : profile.type.sm);
    font.setWeight(QFont::Medium);
    return font;
}

QFont sectionFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleMonoFont(context, TypeRole::Eyebrow);
    font.setPixelSize(qMax(8, profile.type.xxs - 1));
    font.setLetterSpacing(QFont::PercentageSpacing, 113);
    return font;
}

QFont countFont(const QWidget* context)
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

} // namespace

NavList::NavList(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
}

int NavList::addItem(const QString& key, const QString& label, Glyph glyph)
{
    Row row;
    row.key = key;
    row.label = label;
    row.glyph = glyph;
    m_rows.append(row);
    updateGeometry();
    update();
    return m_rows.size() - 1;
}

int NavList::addChild(const QString& key, const QString& label)
{
    const int index = addItem(key, label);
    m_rows[index].child = true;
    return index;
}

int NavList::addSection(const QString& title)
{
    Row row;
    row.label = title;
    row.section = true;
    m_rows.append(row);
    updateGeometry();
    update();
    return m_rows.size() - 1;
}

void NavList::clear()
{
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
    updateGeometry();
    update();
}

void NavList::setExpanded(const QString& key, bool expanded)
{
    Row* target = row(key);
    if (!target || target->expanded == expanded) {
        return;
    }
    target->expanded = expanded;
    update();
}

bool NavList::isExpanded(const QString& key) const
{
    const Row* target = row(key);
    return target && target->expanded;
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
    if (!r || r->visible == visible) {
        return;
    }
    r->visible = visible;
    updateGeometry();
    update();
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
        return sectionHeight();
    }
    const QFontMetrics fm(rowFont(this, row.child));
    return qMax(fm.height(), kGlyphSide) + 2 * kRowPaddingV;
}

int NavList::sectionHeight() const
{
    return QFontMetrics(sectionFont(this)).height() + kSectionTop + kSectionBottom;
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
    const int left = row.child ? kChildIndent : kRowPaddingH;
    return QRect(left, box.top() + (box.height() - kTwistySide) / 2,
                 kTwistySide, kTwistySide);
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
    spec.foreground = token(ColorRole::TextDisabled);
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
    spec.checkedForeground = token(ColorRole::AccentLight);
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
    return QSize(m_collapsed ? sizeHint().width() : 96, sizeHint().height());
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
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    const int index = indexAt(event->pos());
    if (index < 0 || m_rows.at(index).section || !m_rows.at(index).enabled) {
        m_pressed = -1;
        QWidget::mousePressEvent(event);
        return;
    }
    m_pressed = index;
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
    m_pressed = -1;
    m_pressedTwisty = false;

    const int released = indexAt(event->pos());
    const bool releasedTwisty =
        released == pressed && twistyRect(released).contains(event->pos());

    if (released == pressed && releasedTwisty == pressedTwisty) {
        if (pressedTwisty) {
            Row& row = m_rows[pressed];
            row.expanded = !row.expanded;
            emit expandToggled(row.key, row.expanded);
        } else {
            choose(pressed);
        }
    }
    update();
}

void NavList::mouseMoveEvent(QMouseEvent* event)
{
    const int index = indexAt(event->pos());
    const int hovered = (index >= 0 && !m_rows.at(index).section && m_rows.at(index).enabled)
        ? index
        : -1;
    const bool onTwisty = hovered >= 0 && twistyRect(hovered).contains(event->pos());
    if (hovered != m_hovered || onTwisty != m_hoveredTwisty) {
        m_hovered = hovered;
        m_hoveredTwisty = onTwisty;
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

void NavList::keyPressEvent(QKeyEvent* event)
{
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
            painter.setFont(sections);
            painter.setPen(profile.colors.textDisabled);
            painter.drawText(box.adjusted(kRowPaddingH, kSectionTop, -kRowPaddingH,
                                          -kSectionBottom),
                             Qt::AlignLeft | Qt::AlignVCenter, row.label.toUpper());
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
        int left = row.child ? kChildIndent : kRowPaddingH;
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
            paintGlyph(&painter, Glyph::ChevronDown, QRectF(twisty).adjusted(3, 3, -3, -3),
                       ink, row.expanded ? 0.0 : -90.0);
            left = twisty.right() + kRowGap;
        }

        if (row.child && !m_collapsed && !row.expandable) {
            // `.navbtn.child .dot` — a small square, accent when current.
            const QRectF marker(left - kChildMarker - kRowGap + 4,
                                box.center().y() - kChildMarker / 2.0,
                                kChildMarker, kChildMarker);
            painter.setPen(Qt::NoPen);
            painter.setBrush(current ? profile.colors.accent : profile.colors.border);
            painter.drawRoundedRect(marker, 2, 2);
        }

        if (row.glyph != Glyph::None) {
            const QRectF glyphBox(left, box.center().y() - kGlyphSide / 2.0,
                                  kGlyphSide, kGlyphSide);
            paintGlyph(&painter, row.glyph, glyphBox, ink);
            left = static_cast<int>(glyphBox.right()) + kRowGap;
        }

        if (row.hasStatus) {
            const QColor hue = row.status == StatusDot::Tone::Accent
                ? profile.colors.accentLight
                : profile.semantic.role(roleFor(row.status)).text;
            const QRectF dot(left, box.center().y() - kStatusDot / 2.0,
                             kStatusDot, kStatusDot);
            // `.wsdot.archived` is an outline rather than a fill: an archived
            // workspace is present without being live, and a filled grey dot
            // says "idle", which is a different thing.
            if (row.status == StatusDot::Tone::Neutral) {
                painter.setPen(QPen(profile.colors.border, 1));
                painter.setBrush(Qt::NoBrush);
            } else {
                painter.setPen(Qt::NoPen);
                painter.setBrush(hue);
            }
            painter.drawEllipse(dot);
            left = static_cast<int>(dot.right()) + kRowGap;
        }

        if (m_collapsed) {
            continue;   // no label, no count - the design hides both
        }

        // ── the trailing zone ────────────────────────────────────────────
        int rightEdge = width() - kRowPaddingH;
        if (row.count >= 0) {
            const QString text = QString::number(row.count);
            const int pillWidth = countMetrics.horizontalAdvance(text) + 2 * kCountPaddingH;
            const int pillHeight = countMetrics.height() + 2 * kCountPaddingV;
            const QRectF pill(rightEdge - pillWidth, box.center().y() - pillHeight / 2.0,
                              pillWidth, pillHeight);

            QColor pillInk = profile.colors.textDisabled;
            QColor pillBorder = profile.colors.border;
            QColor pillFill = profile.colors.windowBg;
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

            painter.setPen(QPen(pillBorder, 1));
            painter.setBrush(pillFill);
            painter.drawRoundedRect(pill, pillHeight / 2.0, pillHeight / 2.0);
            painter.setFont(counts);
            painter.setPen(pillInk);
            painter.drawText(pill, Qt::AlignCenter, text);
            rightEdge = static_cast<int>(pill.left()) - kRowGap;
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
    if (hasFocus() && m_current >= 0) {
        const QRect box = rowRect(m_current);
        painter.setPen(QPen(profile.colors.accent, profile.border(BorderRole::Focus)));
        painter.setBrush(Qt::NoBrush);
        const double radius = profile.radii.small + kRowRadiusBump;
        painter.drawRoundedRect(QRectF(box).adjusted(1.0, 1.0, -1.0, -1.0), radius, radius);
    }
}

} // namespace novu::ui
