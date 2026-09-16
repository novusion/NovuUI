#include "novu/ui/TreeView.h"

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
#include <QResizeEvent>
#include <QScrollBar>

#include <utility>

namespace novu::ui {

namespace {

// `.trow { gap:6px; padding:0 6px 0 4px }` and `--tree-step`.
constexpr int kRowGap = 6;
constexpr int kRowPaddingLeft = 4;
constexpr int kRowPaddingRight = 6;
// kIndentStep is gone: the step is profile.controls.treeStep, read at paint
// and layout time. A constant here was a fourth tree step in a codebase whose
// design had just finished collapsing three.

// `.trow .cv { 14px }`, `.trow .ico { 14px }`, `.trow .tmarks svg { 12px }`.
constexpr int kChevron = 14;
constexpr int kIcon = 14;
constexpr int kMarkGlyph = 12;
constexpr int kMarkGap = 5;

// `.trow .cnt { padding:1px 6px }`.
constexpr int kCountPaddingH = 6;
constexpr int kTagPaddingH = 7;

// `.trow.ws-on::before { left:-2px; top:3px; bottom:3px; width:2px }`.
constexpr double kActiveBarWidth = 2.0;
constexpr int kActiveBarInset = 3;

// `.tkids { border-left:1px solid var(--hair) }` — the guide down each level.
constexpr int kGuideInset = 8;

constexpr int kRowHeight = 28;
constexpr int kCompactRowHeight = 24;

constexpr double kDisabledOpacity = 0.42;
constexpr double kMarkIdleOpacity = 0.7;

QFont nameFont(const QWidget* context, TreeView::Kind kind)
{
    const ThemeProfile& profile = themeFor(context);
    if (kind == TreeView::Kind::Group) {
        // `.trow.k-group .tname` — the tracked uppercase caption.
        QFont font = roleFont(context, TypeRole::Eyebrow, true);
        font.setPixelSize(qMax(8, profile.type.xs));
        font.setLetterSpacing(QFont::PercentageSpacing, 105.5);
        return font;
    }
    QFont font = roleFont(context, TypeRole::Body);
    font.setPixelSize(profile.type.sm);
    font.setWeight(kind == TreeView::Kind::Emphasis ? QFont::DemiBold : QFont::Normal);
    return font;
}

QFont pillFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleMonoFont(context, TypeRole::Mono);
    font.setPixelSize(qMax(8, profile.type.xxs));
    return font;
}

SemanticRole roleFor(Badge::Tone tone)
{
    switch (tone) {
    case Badge::Tone::Success: return SemanticRole::Success;
    case Badge::Tone::Warning: return SemanticRole::Warning;
    case Badge::Tone::Danger:  return SemanticRole::Danger;
    case Badge::Tone::Info:    return SemanticRole::Info;
    case Badge::Tone::Accent:
    case Badge::Tone::Neutral: break;
    }
    return SemanticRole::Neutral;
}

} // namespace

TreeView::TreeView(QWidget* parent)
    : QAbstractScrollArea(parent)
{
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFocusPolicy(Qt::StrongFocus);
    viewport()->setAutoFillBackground(false);
    viewport()->setMouseTracking(true);
    viewport()->setCursor(Qt::PointingHandCursor);
}

// == shape ==================================================================

const TreeView::Node* TreeView::node(const QString& id) const
{
    const auto it = m_nodes.constFind(id);
    return it == m_nodes.constEnd() ? nullptr : &it.value();
}

TreeView::Node* TreeView::node(const QString& id)
{
    const auto it = m_nodes.find(id);
    return it == m_nodes.end() ? nullptr : &it.value();
}

void TreeView::addNode(const QString& parentId, const QString& id, const QString& text,
                       Glyph glyph, Kind kind)
{
    if (id.isEmpty() || m_nodes.contains(id)) {
        // A duplicate id would give two rows one identity: the second would win
        // every lookup and the first would be unreachable while still drawn.
        return;
    }

    Node entry;
    entry.id = id;
    entry.parent = parentId;
    entry.label = text;
    entry.glyph = glyph;
    entry.kind = kind;

    if (parentId.isEmpty()) {
        entry.depth = 0;
        m_roots.append(id);
    } else {
        Node* parent = node(parentId);
        if (!parent) {
            return;   // an orphan would never be reached by the flatten
        }
        entry.depth = parent->depth + 1;
        parent->children.append(id);
    }

    m_nodes.insert(id, entry);
    rebuildRows();
}

void TreeView::removeSubtree(const QString& id)
{
    Node* entry = node(id);
    if (!entry) {
        return;
    }
    const QStringList children = entry->children;
    for (const QString& child : children) {
        removeSubtree(child);
    }
    m_nodes.remove(id);
}

void TreeView::removeNode(const QString& id)
{
    Node* entry = node(id);
    if (!entry) {
        return;
    }
    const QString parentId = entry->parent;
    if (parentId.isEmpty()) {
        m_roots.removeAll(id);
    } else if (Node* parent = node(parentId)) {
        parent->children.removeAll(id);
    }
    removeSubtree(id);
    if (!m_nodes.contains(m_current)) {
        m_current.clear();
    }
    rebuildRows();
}

void TreeView::clear()
{
    m_nodes.clear();
    m_roots.clear();
    m_rows.clear();
    m_current.clear();
    m_hovered = -1;
    m_pressed = -1;
    updateScrollRange();
    viewport()->update();
}

bool TreeView::contains(const QString& id) const
{
    return m_nodes.contains(id);
}

QString TreeView::parentOf(const QString& id) const
{
    const Node* entry = node(id);
    return entry ? entry->parent : QString();
}

QStringList TreeView::childrenOf(const QString& id) const
{
    const Node* entry = node(id);
    return entry ? entry->children : QStringList();
}

int TreeView::depthOf(const QString& id) const
{
    const Node* entry = node(id);
    return entry ? entry->depth : -1;
}

void TreeView::flatten(const QString& id, int depth)
{
    Node* entry = node(id);
    if (!entry) {
        return;
    }
    entry->depth = depth;
    m_rows.append(id);
    if (!entry->expanded) {
        return;
    }
    for (const QString& child : entry->children) {
        flatten(child, depth + 1);
    }
}

void TreeView::rebuildRows()
{
    m_rows.clear();
    for (const QString& root : m_roots) {
        flatten(root, 0);
    }
    // Hover indices point into the row list, so they are stale the moment the
    // shape changes. Dropping them is cheaper and safer than re-finding them.
    m_hovered = -1;
    m_hoveredChevron = false;
    updateScrollRange();
    viewport()->update();
}

void TreeView::updateScrollRange()
{
    const int perPage = qMax(1, viewport()->height() / rowHeight());
    QScrollBar* bar = verticalScrollBar();
    bar->setPageStep(perPage);
    bar->setSingleStep(1);
    bar->setRange(0, qMax(0, m_rows.size() - perPage));
}

// == content ================================================================

void TreeView::setLabel(const QString& id, const QString& text)
{
    Node* entry = node(id);
    if (!entry || entry->label == text) {
        return;
    }
    entry->label = text;
    viewport()->update();
}

QString TreeView::label(const QString& id) const
{
    const Node* entry = node(id);
    return entry ? entry->label : QString();
}

void TreeView::setGlyph(const QString& id, Glyph glyph)
{
    Node* entry = node(id);
    if (!entry || entry->glyph == glyph) {
        return;
    }
    entry->glyph = glyph;
    viewport()->update();
}

void TreeView::setCount(const QString& id, int count)
{
    Node* entry = node(id);
    if (!entry || entry->count == count) {
        return;
    }
    entry->count = count;
    viewport()->update();
}

void TreeView::setTag(const QString& id, const QString& text, Badge::Tone tone)
{
    Node* entry = node(id);
    if (!entry || (entry->tag == text && entry->tagTone == tone)) {
        return;
    }
    entry->tag = text;
    entry->tagTone = tone;
    viewport()->update();
}

void TreeView::setMarks(const QString& id, const QVector<Glyph>& marks)
{
    Node* entry = node(id);
    if (!entry || entry->marks == marks) {
        return;
    }
    entry->marks = marks;
    viewport()->update();
}

void TreeView::setNodeKind(const QString& id, Kind kind)
{
    Node* entry = node(id);
    if (!entry || entry->kind == kind) {
        return;
    }
    entry->kind = kind;
    viewport()->update();
}

void TreeView::setMark(const QString& id, Mark value)
{
    Node* entry = node(id);
    if (!entry || entry->mark == value) {
        return;
    }
    entry->mark = value;
    viewport()->update();
}

TreeView::Mark TreeView::mark(const QString& id) const
{
    const Node* entry = node(id);
    return entry ? entry->mark : Mark::None;
}

void TreeView::setNodeEnabled(const QString& id, bool enabled)
{
    Node* entry = node(id);
    if (!entry || entry->enabled == enabled) {
        return;
    }
    entry->enabled = enabled;
    viewport()->update();
}

// == state ==================================================================

bool TreeView::isExpanded(const QString& id) const
{
    const Node* entry = node(id);
    return entry && entry->expanded;
}

void TreeView::setExpanded(const QString& id, bool expanded)
{
    Node* entry = node(id);
    if (!entry || entry->expanded == expanded) {
        return;
    }
    // A node with no children still records the state: children often arrive
    // after the expand, and forgetting it would collapse the node again the
    // moment they did.
    entry->expanded = expanded;
    rebuildRows();
}

void TreeView::expandAll()
{
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        it.value().expanded = true;
    }
    rebuildRows();
}

void TreeView::collapseAll()
{
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        it.value().expanded = false;
    }
    rebuildRows();
}

void TreeView::revealNode(const QString& id)
{
    const Node* entry = node(id);
    if (!entry) {
        return;
    }
    QString parentId = entry->parent;
    bool changed = false;
    while (!parentId.isEmpty()) {
        Node* parent = node(parentId);
        if (!parent) {
            break;
        }
        if (!parent->expanded) {
            parent->expanded = true;
            changed = true;
        }
        parentId = parent->parent;
    }
    if (changed) {
        rebuildRows();
    }

    const int row = rowOf(id);
    if (row < 0) {
        return;
    }
    QScrollBar* bar = verticalScrollBar();
    if (row < bar->value()) {
        bar->setValue(row);
    } else if (row > bar->value() + bar->pageStep() - 1) {
        bar->setValue(qMin(bar->maximum(), row - bar->pageStep() + 1));
    }
}

QString TreeView::currentId() const
{
    return m_current;
}

void TreeView::setCurrentId(const QString& id)
{
    if (m_current == id || (!id.isEmpty() && !m_nodes.contains(id))) {
        return;
    }
    m_current = id;
    viewport()->update();
    emit currentChanged(m_current);
}

// == geometry ===============================================================

int TreeView::rowHeight() const
{
    const QFontMetrics fm(nameFont(this, Kind::Item));
    return qMax(m_compact ? kCompactRowHeight : kRowHeight, fm.height() + 6);
}

void TreeView::setCompact(bool compact)
{
    if (m_compact == compact) {
        return;
    }
    m_compact = compact;
    updateScrollRange();
    viewport()->update();
}

int TreeView::firstVisibleRow() const
{
    return verticalScrollBar()->value();
}

QRect TreeView::rowRect(int row) const
{
    if (row < 0 || row >= m_rows.size()) {
        return QRect();
    }
    const int height = rowHeight();
    return QRect(0, (row - firstVisibleRow()) * height, viewport()->width(), height);
}

// One nesting step, from the profile. Three call sites wanted it and only one
// of them had a ThemeProfile already in hand.
int TreeView::indentStep() const
{
    return themeFor(this).controls.treeStep;
}

QRect TreeView::chevronRect(int row) const
{
    if (row < 0 || row >= m_rows.size()) {
        return QRect();
    }
    const Node* entry = node(m_rows.at(row));
    if (!entry || entry->children.isEmpty()) {
        return QRect();
    }
    const QRect box = rowRect(row);
    const int left = kRowPaddingLeft + entry->depth * indentStep();
    return QRect(left, box.top() + (box.height() - kChevron) / 2, kChevron, kChevron);
}

int TreeView::rowAt(const QPoint& position) const
{
    const int height = rowHeight();
    if (height <= 0 || position.y() < 0) {
        return -1;
    }
    const int row = firstVisibleRow() + position.y() / height;
    return row >= 0 && row < m_rows.size() ? row : -1;
}

QString TreeView::idAt(const QPoint& position) const
{
    const int row = rowAt(position);
    return row < 0 ? QString() : m_rows.at(row);
}

int TreeView::rowOf(const QString& id) const
{
    return m_rows.indexOf(id);
}

// == interaction ============================================================

void TreeView::resizeEvent(QResizeEvent* event)
{
    QAbstractScrollArea::resizeEvent(event);
    updateScrollRange();
}

void TreeView::scrollContentsBy(int dx, int dy)
{
    QAbstractScrollArea::scrollContentsBy(dx, dy);
    viewport()->update();
}

bool TreeView::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        updateScrollRange();
        viewport()->update();
    }
    return QAbstractScrollArea::event(event);
}

void TreeView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QAbstractScrollArea::mousePressEvent(event);
        return;
    }
    const QPoint position = event->position().toPoint();
    const int row = rowAt(position);
    if (row < 0) {
        m_pressed = -1;
        return;
    }
    const Node* entry = node(m_rows.at(row));
    if (!entry || !entry->enabled) {
        m_pressed = -1;
        return;
    }
    m_pressed = row;
    // The chevron is a target of its own: opening a folder and selecting it are
    // different actions on the same line.
    m_pressedChevron = chevronRect(row).contains(position);
    viewport()->update();
}

void TreeView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || m_pressed < 0) {
        QAbstractScrollArea::mouseReleaseEvent(event);
        return;
    }
    const QPoint position = event->position().toPoint();
    const int pressed = m_pressed;
    const bool pressedChevron = m_pressedChevron;
    m_pressed = -1;
    m_pressedChevron = false;

    const int released = rowAt(position);
    const bool releasedChevron =
        released == pressed && chevronRect(released).contains(position);

    if (released != pressed || releasedChevron != pressedChevron) {
        viewport()->update();
        return;   // a cancelled gesture
    }

    const QString id = m_rows.at(pressed);
    if (pressedChevron) {
        setExpanded(id, !isExpanded(id));
        emit expandToggled(id, isExpanded(id));
        return;
    }

    setCurrentId(id);
    emit activated(id);
}

void TreeView::mouseMoveEvent(QMouseEvent* event)
{
    const QPoint position = event->position().toPoint();
    const int row = rowAt(position);
    const bool onChevron = row >= 0 && chevronRect(row).contains(position);
    if (row != m_hovered || onChevron != m_hoveredChevron) {
        m_hovered = row;
        m_hoveredChevron = onChevron;
        viewport()->update();
    }
    QAbstractScrollArea::mouseMoveEvent(event);
}

void TreeView::leaveEvent(QEvent* event)
{
    if (m_hovered != -1) {
        m_hovered = -1;
        m_hoveredChevron = false;
        viewport()->update();
    }
    QAbstractScrollArea::leaveEvent(event);
}

void TreeView::keyPressEvent(QKeyEvent* event)
{
    const int row = rowOf(m_current);

    switch (event->key()) {
    case Qt::Key_Down:
        if (row + 1 < m_rows.size()) {
            setCurrentId(m_rows.at(row + 1));
            revealNode(m_current);
        }
        return;
    case Qt::Key_Up:
        if (row > 0) {
            setCurrentId(m_rows.at(row - 1));
            revealNode(m_current);
        }
        return;
    case Qt::Key_Right: {
        // Right opens a closed node and steps into an open one, which is what
        // every tree in every file manager does and what a user's hands expect.
        const Node* entry = node(m_current);
        if (entry && !entry->children.isEmpty()) {
            if (!entry->expanded) {
                setExpanded(m_current, true);
                emit expandToggled(m_current, true);
            } else {
                setCurrentId(entry->children.first());
            }
        }
        return;
    }
    case Qt::Key_Left: {
        const Node* entry = node(m_current);
        if (entry && entry->expanded && !entry->children.isEmpty()) {
            setExpanded(m_current, false);
            emit expandToggled(m_current, false);
        } else if (entry && !entry->parent.isEmpty()) {
            setCurrentId(entry->parent);
            revealNode(m_current);
        }
        return;
    }
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (!m_current.isEmpty()) {
            emit activated(m_current);
        }
        return;
    default:
        break;
    }
    QAbstractScrollArea::keyPressEvent(event);
}

// == appearance =============================================================

StyleSpec TreeView::styleSpec() const
{
    StyleSpec spec;
    spec.background = token(ColorRole::Transparent);
    spec.foreground = token(ColorRole::TextSecondary);
    spec.border = token(ColorRole::Transparent);
    spec.radius = SurfaceRole::Toolbar;
    spec.borderWidth = BorderRole::Divider;
    spec.type = TypeRole::Body;
    return spec;
}

StyleSpec TreeView::rowStyleSpec(int row) const
{
    StyleSpec spec = styleSpec();
    spec.radius = SurfaceRole::Button;
    spec.radiusStepOverride = static_cast<int>(RadiusStep::Small);
    spec.hoverBackground = token(ColorRole::PanelBg);
    spec.hoverForeground = token(ColorRole::TextPrimary);
    // `.trow.sel { background:rgba(--acc,.14) }`.
    spec.checkedBackground = token(ColorRole::Accent, 0.14);
    spec.checkedForeground = token(ColorRole::TextPrimary);
    spec.disabledForeground = token(ColorRole::TextDisabled, kDisabledOpacity);

    if (row < 0 || row >= m_rows.size()) {
        return spec;
    }
    const Node* entry = node(m_rows.at(row));
    if (!entry) {
        return spec;
    }
    if (entry->kind == Kind::Group) {
        spec.type = TypeRole::Eyebrow;
        spec.foreground = token(ColorRole::TextDisabled);
    }
    if (entry->mark == Mark::Active) {
        spec.foreground = token(ColorRole::AccentLight);
    } else if (entry->mark == Mark::Detached) {
        spec.foreground = token(ColorRole::TextDisabled);
    } else if (entry->mark == Mark::Dropping) {
        spec.background = token(ColorRole::Accent, 0.20);
        spec.border = token(ColorRole::Accent);
    }
    return spec;
}

ResolvedStyle TreeView::resolvedStyle() const
{
    return resolve(styleSpec(), this, rowHeight());
}

void TreeView::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);

    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing, true);

    const int height = rowHeight();
    const int first = firstVisibleRow();
    const int rows = qMin(m_rows.size() - first, viewport()->height() / height + 2);
    const QFont pills = pillFont(this);
    const QFontMetrics pillMetrics(pills);

    for (int i = 0; i < rows; ++i) {
        const int row = first + i;
        if (row < 0 || row >= m_rows.size()) {
            break;
        }
        const Node* entry = node(m_rows.at(row));
        if (!entry) {
            continue;
        }
        const QRect box = rowRect(row);
        const ResolvedStyle style = resolve(rowStyleSpec(row), this, height);
        const bool current = m_rows.at(row) == m_current;
        const bool hot = row == m_hovered && !current && entry->enabled;

        // `.tkids` — one guide per level the row is under, drawn before the
        // plate so a selected row's tint sits over it rather than under it.
        painter.setPen(QPen(profile.colors.borderSubtle, 1));
        for (int level = 1; level <= entry->depth; ++level) {
            const int x = kRowPaddingLeft + (level - 1) * indentStep() + kGuideInset;
            painter.drawLine(x, box.top(), x, box.bottom());
        }

        // A panel that is not the active one keeps its selection, dimmed.
        // Without it a two-panel window has two rows that both look current;
        // with a clear-on-focus-out instead, it has none.
        const double selectionStrength =
            selectionIsActive(this) ? 1.0 : inactiveSelectionOpacity;

        const double radius = profile.radii.small + 1;
        if (entry->mark == Mark::Dropping) {
            painter.setPen(QPen(profile.colors.accent, 1));
            painter.setBrush(withAlpha(profile.colors.accent, 0.20));
            painter.drawRoundedRect(QRectF(box).adjusted(0.5, 0.5, -0.5, -0.5),
                                    radius, radius);
        } else if (current) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(withAlpha(style.checkedBackground,
                                       style.checkedBackground.alphaF()
                                           * selectionStrength));
            painter.drawRoundedRect(QRectF(box), radius, radius);
        } else if (hot) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(style.hoverBackground);
            painter.drawRoundedRect(QRectF(box), radius, radius);
        }

        if (entry->mark == Mark::Active) {
            QPainterPath bar;
            bar.addRoundedRect(QRectF(0.0, box.top() + kActiveBarInset, kActiveBarWidth,
                                      box.height() - 2.0 * kActiveBarInset),
                               kActiveBarWidth / 2.0, kActiveBarWidth / 2.0);
            painter.setPen(Qt::NoPen);
            painter.setBrush(profile.colors.accent);
            painter.drawPath(bar);
        }

        QColor ink = current ? style.checkedForeground
                             : (hot ? style.hoverForeground : style.foreground);
        if (!entry->enabled) {
            ink = withAlpha(ink, kDisabledOpacity);
        }

        int x = kRowPaddingLeft + entry->depth * indentStep();

        // The chevron, or the space it would take. A node with no children
        // still reserves it, or names at the same depth stagger by 14px
        // depending on whether they happen to have anything inside them.
        if (!entry->children.isEmpty()) {
            const QRect chevron = chevronRect(row);
            if (m_hoveredChevron && row == m_hovered) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(profile.colors.surfaceHover);
                painter.drawRoundedRect(QRectF(chevron), profile.radii.xs + 1,
                                        profile.radii.xs + 1);
            }
            paintGlyph(&painter, Glyph::ChevronDown,
                       QRectF(chevron).adjusted(3, 3, -3, -3),
                       withAlpha(ink, 0.85), entry->expanded ? 0.0 : -90.0);
        }
        x += kChevron + kRowGap;

        if (entry->glyph != Glyph::None) {
            QColor iconInk = ink;
            if (entry->mark == Mark::Active) {
                iconInk = profile.colors.accent;
            } else if (entry->mark == Mark::Detached) {
                iconInk = withAlpha(ink, kMarkIdleOpacity);
            }
            paintGlyph(&painter, entry->glyph,
                       QRectF(x, box.center().y() - kIcon / 2.0, kIcon, kIcon), iconInk);
            x += kIcon + kRowGap;
        }

        // The trailing zone, measured from the right so the name gets what is
        // left rather than the other way round.
        int rightEdge = box.right() - kRowPaddingRight;

        if (!entry->marks.isEmpty()) {
            // `.trow .tmarks { opacity:.7 }`, full on hover. They are properties
            // of the node rather than actions on it, so they never take a click.
            const double opacity = (row == m_hovered) ? 1.0 : kMarkIdleOpacity;
            for (int m = entry->marks.size() - 1; m >= 0; --m) {
                const QRectF mark(rightEdge - kMarkGlyph,
                                  box.center().y() - kMarkGlyph / 2.0,
                                  kMarkGlyph, kMarkGlyph);
                paintGlyph(&painter, entry->marks.at(m), mark,
                           withAlpha(profile.colors.textDisabled, opacity));
                rightEdge -= kMarkGlyph + kMarkGap;
            }
            rightEdge -= kRowGap - kMarkGap;
        }

        if (!entry->tag.isEmpty()) {
            const SemanticColor tone = profile.semantic.role(roleFor(entry->tagTone));
            const int tagWidth = pillMetrics.horizontalAdvance(entry->tag) + 2 * kTagPaddingH;
            const int tagHeight = pillMetrics.height() + 2;
            const QRectF pill(rightEdge - tagWidth, box.center().y() - tagHeight / 2.0,
                              tagWidth, tagHeight);
            painter.setPen(QPen(tone.border, 1));
            painter.setBrush(tone.fill);
            painter.drawRoundedRect(pill, tagHeight / 2.0, tagHeight / 2.0);
            painter.setFont(pills);
            painter.setPen(tone.text);
            painter.drawText(pill, Qt::AlignCenter, entry->tag);
            rightEdge = static_cast<int>(pill.left()) - kRowGap;
        }

        if (entry->count >= 0) {
            const QString text = QString::number(entry->count);
            const int pillWidth = pillMetrics.horizontalAdvance(text) + 2 * kCountPaddingH;
            const int pillHeight = pillMetrics.height() + 2;
            const QRectF pill(rightEdge - pillWidth, box.center().y() - pillHeight / 2.0,
                              pillWidth, pillHeight);
            // `.trow.k-group .cnt { background:transparent }` — a caption's
            // count is a number beside a heading, not a chip in a list.
            painter.setPen(Qt::NoPen);
            painter.setBrush(entry->kind == Kind::Group ? QBrush(Qt::NoBrush)
                                                        : QBrush(profile.colors.panelBgAlt));
            painter.drawRoundedRect(pill, pillHeight / 2.0, pillHeight / 2.0);
            painter.setFont(pills);
            painter.setPen(profile.colors.textDisabled);
            painter.drawText(pill, Qt::AlignCenter, text);
            rightEdge = static_cast<int>(pill.left()) - kRowGap;
        }

        QFont font = nameFont(this, entry->kind);
        if (entry->mark == Mark::Detached) {
            // `.trow.detached .tname { font-style:italic }` — present, but not
            // loaded. Italic rather than a colour, because the row is already
            // muted and a second muted grey would be invisible.
            font.setItalic(true);
        }
        const QFontMetrics fm(font);
        painter.setFont(font);
        painter.setPen(ink);
        const QString text = entry->kind == Kind::Group ? entry->label.toUpper()
                                                        : entry->label;
        const QRect nameBox(x, box.top(), qMax(0, rightEdge - x), box.height());
        painter.drawText(nameBox, Qt::AlignVCenter | Qt::AlignLeft,
                         fm.elidedText(text, Qt::ElideMiddle, nameBox.width()));
    }

    if (hasFocus() && !m_current.isEmpty()) {
        const int row = rowOf(m_current);
        if (row >= first && row < first + rows) {
            painter.setPen(QPen(profile.colors.accent, profile.border(BorderRole::Focus)));
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(QRectF(rowRect(row)).adjusted(1, 1, -1, -1),
                                    profile.radii.small + 1, profile.radii.small + 1);
        }
    }
}

} // namespace novu::ui
