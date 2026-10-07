#include "novu/ui/TreeView.h"

#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QRadialGradient>
#include <QPainterPath>
#include <QPen>
#include <QResizeEvent>
#include <QScrollBar>

#include <utility>

namespace novu::ui {

namespace {

// `.trow { gap:6px; padding:0 6px 0 4px }` and `--tree-step`.
// `.wsdot{width:6px}` and the halo around it, matching StatusDot's own.
constexpr int kStatusDot = 6;
constexpr int kStatusHalo = 2;
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
// The hover action sits in the trailing zone at a mark's size: it is
// one more small glyph at the end of a row, not a button bolted on.
constexpr int kRowAction = 14;
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
        // `.trow.k-group .tname{ font-family:var(--f-head); font-weight:600;
        //  font-size:calc(var(--fs-sm)); letter-spacing:.055em;
        //  text-transform:uppercase }`
        //
        // The head face, stated by that rule - NOT the Eyebrow role, which
        // this asked for before. Eyebrow declares itself monospace in the
        // NovuAI profile, so every group caption in every tree came out in
        // the mono stack: ESANTI and HOUDINI wore a typewriter's serifed
        // capital I, and a navigation rail read as technical output. A
        // display role resolves to the profile's displayFamily, which is
        // `--f-head`, and the weight is stated outright rather than reached
        // by bolding a role that already has an opinion.
        //
        // The size is `sm` for the same reason: the rule says fs-sm and this
        // was drawing xs, a step too small.
        QFont font = roleFont(context, TypeRole::CardTitle, false, QFont::DemiBold);
        font.setPixelSize(qMax(8, profile.type.sm));
        font.setLetterSpacing(QFont::PercentageSpacing, 105.5);
        return font;
    }
    // `.trow .tname` inherits the body face; the kinds differ only in weight
    // and colour. `.trow.k-ws .tname{font-weight:500}` - Medium, not DemiBold:
    // a workspace is the heavier of the two rows, one step up from a category,
    // not as heavy as the group heading above it.
    QFont font = roleFont(context, TypeRole::Body);
    font.setPixelSize(profile.type.sm);
    font.setWeight(kind == TreeView::Kind::Emphasis ? QFont::Medium : QFont::Normal);
    return font;
}

QFont pillFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleMonoFont(context, TypeRole::Mono);
    font.setPixelSize(qMax(8, profile.type.xxs));
    return font;
}

QString fileTypeText(const QString& label, Glyph glyph)
{
    if (glyph != Glyph::File && glyph != Glyph::Code
        && glyph != Glyph::Doc && glyph != Glyph::Terminal) {
        return {};
    }
    const int dot = label.lastIndexOf(QLatin1Char('.'));
    if (dot <= 0 || dot == label.size() - 1) return {};
    QString type = label.mid(dot + 1).toUpper();
    if (type == QLatin1String("CXX") || type == QLatin1String("CC")) type = QStringLiteral("CPP");
    else if (type == QLatin1String("HPP") || type == QLatin1String("HXX")) type = QStringLiteral("H");
    else if (type == QLatin1String("YAML")) type = QStringLiteral("YML");
    else if (type == QLatin1String("MARKDOWN")) type = QStringLiteral("MD");
    else if (type == QLatin1String("JAVASCRIPT")) type = QStringLiteral("JS");
    else if (type == QLatin1String("TYPESCRIPT")) type = QStringLiteral("TS");
    return type.size() <= 4 ? type : type.left(4);
}

QColor fileTypeInk(const ThemeProfile& profile, const QString& type)
{
    if (type == QLatin1String("PY")) return profile.semantic.info.text;
    if (type == QLatin1String("CPP") || type == QLatin1String("C")
        || type == QLatin1String("H")) return profile.colors.accentLight;
    if (type == QLatin1String("MD") || type == QLatin1String("TXT"))
        return profile.semantic.success.text;
    if (type == QLatin1String("YML") || type == QLatin1String("JSON")
        || type == QLatin1String("TOML") || type == QLatin1String("INI"))
        return profile.semantic.warning.text;
    if (type == QLatin1String("JS") || type == QLatin1String("TS")
        || type == QLatin1String("QML")) return profile.semantic.warning.text;
    if (type == QLatin1String("SH") || type == QLatin1String("PS1")
        || type == QLatin1String("BAT")) return profile.semantic.info.text;
    return profile.colors.textDisabled;
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
    m_selected.intersect(QSet<QString>(m_nodes.keyBegin(), m_nodes.keyEnd()));
    if (!m_nodes.contains(m_selectionAnchor)) {
        m_selectionAnchor.clear();
    }
    if (!m_nodes.contains(m_current)) {
    m_current.clear();
    m_selected.clear();
    m_selectionAnchor.clear();
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

void TreeView::setStatus(const QString& id, StatusDot::Tone tone)
{
    Node* entry = node(id);
    if (!entry || (entry->hasStatus && entry->status == tone)) {
        return;
    }
    entry->hasStatus = true;
    entry->status = tone;
    viewport()->update();
}

void TreeView::clearStatus(const QString& id)
{
    Node* entry = node(id);
    if (!entry || !entry->hasStatus) {
        return;
    }
    entry->hasStatus = false;
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

void TreeView::setNodeExpandable(const QString& id, bool expandable)
{
    Node* entry = node(id);
    if (!entry || entry->expandable == expandable) {
        return;
    }
    entry->expandable = expandable;
    viewport()->update();
}

bool TreeView::isNodeExpandable(const QString& id) const
{
    const Node* entry = node(id);
    return entry && canExpand(*entry);
}

bool TreeView::canExpand(const Node& entry)
{
    return !entry.children.isEmpty() || entry.expandable;
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


void TreeView::setSelectionMode(SelectionMode mode)
{
    if (m_selectionMode == mode) {
        return;
    }
    m_selectionMode = mode;
    if (mode == SelectionMode::Single) {
        // Dropping back to one row keeps the current one and discards the
        // rest, rather than clearing everything: the person is left marking
        // what they were already on.
        const bool had = m_selected.size() > 1;
        m_selected.clear();
        if (!m_current.isEmpty()) {
            m_selected.insert(m_current);
        }
        if (had) {
            viewport()->update();
            emit selectionChanged(selectedIds());
        }
    }
}

QStringList TreeView::selectedIds() const
{
    // Row order, not set order. A caller listing what it is about to delete
    // shows them the way the rail does; a hash's order would be arbitrary and
    // would change between runs.
    QStringList ids;
    ids.reserve(m_selected.size());
    for (const QString& id : m_rows) {
        if (m_selected.contains(id)) {
            ids << id;
        }
    }
    // A selected row inside a collapsed parent is not in m_rows and would be
    // lost here. It is still selected, so it goes on the end rather than
    // silently out of the answer.
    for (const QString& id : m_selected) {
        if (!ids.contains(id)) {
            ids << id;
        }
    }
    return ids;
}

void TreeView::setSelectedIds(const QStringList& ids)
{
    QSet<QString> next;
    for (const QString& id : ids) {
        if (m_nodes.contains(id)) {
            next.insert(id);
        }
    }
    if (next == m_selected) {
        return;
    }
    m_selected = next;
    viewport()->update();
    emit selectionChanged(selectedIds());
}

void TreeView::clearSelection()
{
    if (m_selected.isEmpty()) {
        return;
    }
    m_selected.clear();
    viewport()->update();
    emit selectionChanged(selectedIds());
}

// Applies one click's worth of selection. Returns true when the row should
// also be ACTIVATED - opened - which a plain click does and a modified one
// deliberately does not: adding a fourth chat to a selection must not open
// the fourth chat and throw away the view the person was building the
// selection for.
bool TreeView::applySelectionForClick(const QString& id, Qt::KeyboardModifiers modifiers)
{
    const QSet<QString> before = m_selected;

    if (m_selectionMode == SelectionMode::Single
        || !(modifiers & (Qt::ControlModifier | Qt::ShiftModifier))) {
        m_selected.clear();
        m_selected.insert(id);
        m_selectionAnchor = id;
        if (m_selected != before) {
            emit selectionChanged(selectedIds());
        }
        return true;
    }

    if (modifiers & Qt::ShiftModifier) {
        // A range over what is ON SCREEN. Rows hidden inside a collapsed
        // parent are not between two visible rows in any sense a person can
        // see, and taking them would select things the gesture never crossed.
        const int anchorRow = m_selectionAnchor.isEmpty() ? -1 : rowOf(m_selectionAnchor);
        const int targetRow = rowOf(id);
        if (anchorRow >= 0 && targetRow >= 0) {
            m_selected.clear();
            const int from = qMin(anchorRow, targetRow);
            const int to = qMax(anchorRow, targetRow);
            for (int row = from; row <= to; ++row) {
                m_selected.insert(m_rows.at(row));
            }
        } else {
            m_selected.insert(id);
            m_selectionAnchor = id;
        }
    } else {
        // Ctrl: add, or take away. Taking away is the half that makes a
        // selection correctable without starting over.
        if (m_selected.contains(id)) {
            m_selected.remove(id);
        } else {
            m_selected.insert(id);
        }
        m_selectionAnchor = id;
    }

    if (m_selected != before) {
        viewport()->update();
        emit selectionChanged(selectedIds());
    }
    return false;
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
    // In Single mode the selection IS the current row, so a caller that sets
    // the current row programmatically - creating a workspace and selecting
    // it, say - gets a selection that agrees with it rather than an empty one.
    if (m_selectionMode == SelectionMode::Single) {
        m_selected.clear();
        if (!id.isEmpty()) {
            m_selected.insert(id);
        }
        m_selectionAnchor = id;
    }
    viewport()->update();
    emit currentChanged(m_current);
}

void TreeView::activate(const QString& id)
{
    if (id.isEmpty() || !m_nodes.contains(id)) {
        return;
    }
    // Opening a row programmatically is a plain click: it becomes the whole
    // selection. A caller that wanted to add to one would set the selection
    // itself; this is the "go here" call.
    const QSet<QString> before = m_selected;
    m_selected.clear();
    m_selected.insert(id);
    m_selectionAnchor = id;
    setCurrentId(id);
    if (m_selected != before) {
        emit selectionChanged(selectedIds());
    }
    emit activated(id);
}

// == geometry ===============================================================

void TreeView::setRowHeight(int height){m_rowHeight=qMax(0,height);updateScrollRange();updateGeometry();viewport()->update();}
void TreeView::setMeta(const QString& id,const QString& text){if(auto* item=node(id)){item->meta=text;viewport()->update();}}
QString TreeView::meta(const QString& id)const{const auto* item=node(id);return item?item->meta:QString();}
int TreeView::rowHeight() const
{
    if(m_rowHeight>0)return m_rowHeight;
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
    if(!m_branchesVisible)return {};
    if (row < 0 || row >= m_rows.size()) {
        return QRect();
    }
    const Node* entry = node(m_rows.at(row));
    if (!entry || !canExpand(*entry)) {
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
    if (m_editor) {
        layOutEditor();
    }
    QAbstractScrollArea::scrollContentsBy(dx, dy);
    viewport()->update();
}

// =============================================================================
// Gestures
// =============================================================================

void TreeView::setRowAction(const QString& id, Glyph glyph, const QString& tooltip)
{
    auto it = m_nodes.find(id);
    if (it == m_nodes.end()) {
        return;
    }
    it->action = glyph;
    it->actionTip = tooltip;
    viewport()->update();
}

void TreeView::clearRowAction(const QString& id)
{
    setRowAction(id, Glyph::None, QString());
}

QRect TreeView::rowActionRect(int row) const
{
    const QRect box = rowRect(row);
    if (!box.isValid()) {
        return {};
    }
    return QRect(box.right() - kRowPaddingRight - kRowAction,
                 box.center().y() - kRowAction / 2, kRowAction, kRowAction);
}

int TreeView::contentLeft(int row) const
{
    if (row < 0 || row >= m_rows.size()) {
        return kRowPaddingLeft;
    }
    const Node* entry = node(m_rows.at(row));
    if (!entry) {
        return kRowPaddingLeft;
    }
    int x = kRowPaddingLeft + entry->depth * indentStep();
    if (m_branchesVisible) {
        x += kChevron + kRowGap;
    }
    // The same two shapes paintEvent puts here, measured the same way: a file
    // type chip when the name has an extension worth naming, an icon
    // otherwise.
    const QString fileType = fileTypeText(entry->label, entry->glyph);
    if (!fileType.isEmpty()) {
        QFont typeFont = roleMonoFont(this, TypeRole::Mono);
        typeFont.setPixelSize(qMax(8, themeFor(this).type.xxs));
        const QFontMetrics typeMetrics(typeFont, viewport());
        x += qMax(20, typeMetrics.horizontalAdvance(fileType) + 6) + kRowGap;
    } else if (entry->glyph != Glyph::None) {
        x += kIcon + kRowGap;
    }
    return x;
}

void TreeView::beginRename(const QString& id)
{
    if (!m_nodes.contains(id)) {
        return;
    }
    if (m_editor) {
        endRename(true);
    }
    revealNode(id);
    const int row = rowOf(id);
    if (row < 0) {
        return;   // an ancestor refused to open; nothing to edit in place
    }

    m_editing = id;
    m_editor = new QLineEdit(viewport());
    // Named so the stylesheet can take the field shape off it. A dot in an
    // object name reads as a class separator in QSS, so this one has none.
    m_editor->setObjectName(QStringLiteral("novuTreeRename"));
    m_editor->setFrame(false);
    m_editor->setText(label(id));
    m_editor->selectAll();
    m_editor->installEventFilter(this);
    layOutEditor();
    m_editor->show();
    m_editor->setFocus(Qt::OtherFocusReason);
    viewport()->update();
}

void TreeView::layOutEditor()
{
    if (!m_editor) {
        return;
    }
    const int row = rowOf(m_editing);
    if (row < 0) {
        // Scrolled out of the visible set. Committing here would write a name
        // the user can no longer see; the edit is abandoned instead.
        endRename(false);
        return;
    }
    const QRect box = rowRect(row);
    const int left = contentLeft(row);
    m_editor->setGeometry(QRect(box.left() + left, box.top() + 2,
                                qMax(40, box.right() - kRowPaddingRight - box.left() - left),
                                box.height() - 4));
}

void TreeView::commitRename() { endRename(true); }
void TreeView::cancelRename() { endRename(false); }

void TreeView::endRename(bool commit)
{
    // Committing on focus-out means the editor is destroyed from inside its
    // own event handling, and Qt hands focus on during that - straight back
    // into here. Once is enough.
    if (!m_editor || m_endingRename) {
        return;
    }
    m_endingRename = true;

    const QString id = m_editing;
    const QString text = m_editor->text().trimmed();
    QLineEdit* editor = m_editor;
    m_editor = nullptr;
    m_editing.clear();
    editor->removeEventFilter(this);
    editor->hide();
    editor->deleteLater();
    setFocus(Qt::OtherFocusReason);
    viewport()->update();
    m_endingRename = false;

    // An empty name and an unchanged one are both "nothing happened". The
    // caller is told nothing was renamed rather than being asked to work out
    // that the new name equals the old one.
    if (!commit || text.isEmpty() || text == label(id)) {
        emit renameCancelled(id);
        return;
    }
    // The label is NOT written here. Whether the rename took is the service's
    // answer, and a row that renamed itself optimistically and then failed is
    // a lie the tree told on the service's behalf.
    emit renameCommitted(id, text);
}

bool TreeView::eventFilter(QObject* watched, QEvent* event)
{
    if (m_editor && watched == m_editor) {
        if (event->type() == QEvent::KeyPress) {
            const int key = static_cast<QKeyEvent*>(event)->key();
            if (key == Qt::Key_Return || key == Qt::Key_Enter) {
                endRename(true);
                return true;
            }
            if (key == Qt::Key_Escape) {
                endRename(false);
                return true;
            }
        } else if (event->type() == QEvent::FocusOut) {
            endRename(true);
            return false;
        }
    }
    return QAbstractScrollArea::eventFilter(watched, event);
}

void TreeView::contextMenuEvent(QContextMenuEvent* event)
{
    const int row = rowAt(event->pos());
    if (row < 0 || row >= m_rows.size()) {
        emit contextMenuRequested(QString(), event->globalPos());
        event->accept();
        return;
    }
    const QString id = m_rows.at(row);
    // Current first, then the menu: everything the menu offers is about this
    // node, and it must be the node the pointer is on.
    setCurrentId(id);
    emit contextMenuRequested(id, event->globalPos());
    event->accept();
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
    // So is the row action, for the same reason the chevron is: pressing the
    // plus on a row is not pressing the row.
    if (entry->action != Glyph::None && rowActionRect(row).contains(position)) {
        m_pressed = -1;
        m_pressedAction = row;
        viewport()->update();
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
    if (event->button() == Qt::LeftButton && m_pressedAction >= 0) {
        const int pressedAction = m_pressedAction;
        m_pressedAction = -1;
        viewport()->update();
        const QPoint position = event->position().toPoint();
        if (rowAt(position) == pressedAction
            && rowActionRect(pressedAction).contains(position)
            && pressedAction < m_rows.size()) {
            emit rowActionTriggered(m_rows.at(pressedAction));
        }
        return;
    }
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

    // Selection first, because what the click MEANS depends on the modifier:
    // a plain click opens the row, a modified one only marks it.
    const bool shouldActivate = applySelectionForClick(id, event->modifiers());
    setCurrentId(id);
    if (shouldActivate) {
        emit activated(id);
    } else {
        viewport()->update();
    }
}

void TreeView::mouseMoveEvent(QMouseEvent* event)
{
    const QPoint position = event->position().toPoint();
    const int row = rowAt(position);
    const bool onChevron = row >= 0 && chevronRect(row).contains(position);
    // The action says what it does while the pointer is on it. A bare glyph
    // that appears on hover and is never explained is a guess.
    const Node* hoveredNode = row >= 0 && row < m_rows.size() ? node(m_rows.at(row)) : nullptr;
    const bool onAction = hoveredNode && hoveredNode->action != Glyph::None
        && rowActionRect(row).contains(position);
    viewport()->setToolTip(onAction ? hoveredNode->actionTip : QString());

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
    case Qt::Key_F2:
        if (!m_current.isEmpty()) {
            beginRename(m_current);
        }
        return;
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
        if (entry && canExpand(*entry)) {
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
        if (entry && entry->expanded && canExpand(*entry)) {
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
        // Marked but not current. Drawn like the current row and a shade
        // lighter, so a run of four selected rows reads as one block with the
        // keyboard sitting on one of them - rather than as four current rows,
        // which would say the keyboard is in four places.
        const bool selected = !current && m_selected.contains(m_rows.at(row));
        const bool hot = row == m_hovered && !current && !selected && entry->enabled;

        // `.tkids` — one guide per level the row is under, drawn before the
        // plate so a selected row's tint sits over it rather than under it.
        painter.setPen(QPen(profile.colors.borderSubtle, 1));
        for (int level = 1; m_guidesVisible && level <= entry->depth; ++level) {
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
        } else if (selected) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(withAlpha(style.checkedBackground,
                                       style.checkedBackground.alphaF()
                                           * selectionStrength * 0.55));
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

        // The chevron, or the space it would take. A node with nothing
        // expandable about it still reserves it, or names at the same depth
        // stagger by 14px depending on whether they happen to have anything
        // inside them.
        if (m_branchesVisible && canExpand(*entry)) {
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
        if(m_branchesVisible)x += kChevron + kRowGap;

        const QString fileType = fileTypeText(entry->label, entry->glyph);
        if (!fileType.isEmpty()) {
            QFont typeFont = roleMonoFont(this, TypeRole::Mono);
            typeFont.setPixelSize(qMax(8, profile.type.xxs));
            const QFontMetrics typeMetrics(typeFont, viewport());
            const int typeWidth = qMax(20, typeMetrics.horizontalAdvance(fileType) + 6);
            const int typeHeight = 16;
            const QRectF typeBox(x, box.center().y() - typeHeight / 2.0,
                                 typeWidth, typeHeight);
            QColor typeInk = fileTypeInk(profile, fileType);
            if (!entry->enabled) typeInk = withAlpha(typeInk, kDisabledOpacity);
            QColor typeFill = typeInk;
            typeFill.setAlphaF(0.10);
            QColor typeEdge = typeInk;
            typeEdge.setAlphaF(0.42);
            painter.setFont(typeFont);
            painter.setPen(QPen(typeEdge, 1));
            painter.setBrush(typeFill);
            painter.drawRoundedRect(typeBox.adjusted(0.5, 0.5, -0.5, -0.5), 3, 3);
            painter.setPen(typeInk);
            painter.drawText(typeBox, Qt::AlignCenter, fileType);
            x += typeWidth + kRowGap;
        } else if (entry->glyph != Glyph::None) {
            // Quieter than the name it labels, where the profile says so.
            QColor iconInk = profile.controls.treeIconOpacity < 1.0
                                 ? withAlpha(ink, profile.controls.treeIconOpacity)
                                 : ink;
            if (entry->mark == Mark::Active) {
                iconInk = profile.colors.accent;
            } else if (entry->mark == Mark::Detached) {
                iconInk = withAlpha(ink, kMarkIdleOpacity);
            }
            // A row's own icon is drawn solid where the shape allows it.
            //
            // A dense tree is read by the shape of the column, and outlines at
            // this size have too little mass to hold one - the rows blur into
            // each other. Same glyph, same box, same ink: only the weight
            // changes, and only for the glyphs whose outline is a single
            // closed shape.
            //
            // Here rather than in a caller, because this is where a glyph
            // means "a node of a tree" - every tree gets the same answer
            // without each one deciding for itself.
            paintGlyphFilled(&painter, entry->glyph,
                             QRectF(x, box.center().y() - kIcon / 2.0, kIcon, kIcon),
                             iconInk, this);
            x += kIcon + kRowGap;
        }

        // The trailing zone, measured from the right so the name gets what is
        // left rather than the other way round.
        int rightEdge = box.right() - kRowPaddingRight;

        // The hover action takes the outermost slot. It is subtracted whether
        // or not it is drawn, so arriving with the pointer reveals a glyph
        // and moves nothing.
        if (entry->action != Glyph::None) {
            const QRect slot = rowActionRect(row);
            if (row == m_hovered) {
                if (m_pressedAction == row) {
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(profile.colors.surfaceHover);
                    painter.drawRoundedRect(QRectF(slot), profile.radii.xs + 1,
                                            profile.radii.xs + 1);
                }
                paintGlyph(&painter, entry->action, QRectF(slot).adjusted(2, 2, -2, -2),
                           withAlpha(ink, 0.9));
            }
            rightEdge = slot.left() - kRowGap;
        }
        if(!entry->meta.isEmpty()){
            auto font=roleMonoFont(this,TypeRole::Mono);font.setPointSizeF(9.0*72/logicalDpiY());const QFontMetrics metrics(font,viewport());const int width=metrics.horizontalAdvance(entry->meta);
            painter.setFont(font);painter.setPen(current?profile.colors.accent:profile.colors.textDisabled);painter.drawText(QRect(rightEdge-width,box.top(),width,box.height()),Qt::AlignVCenter,entry->meta);rightEdge-=width+kRowGap;
        }


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

        if (entry->hasStatus) {
            // `.wsdot{width:6px}` with the same halo StatusDot draws, so a dot
            // in a tree row and a dot in a nav row are the same dot.
            // statusDotToken() is StatusDot's own table, not a copy of it, so
            // a dot painted in a row and a StatusDot widget are the same
            // colour in every theme.
            const QColor dotColour = statusDotToken(entry->status).resolve(profile);
            const double radius = kStatusDot / 2.0;
            const QPointF centre(rightEdge - radius, box.center().y());
            QColor halo = dotColour;
            halo.setAlphaF(halo.alphaF() * 0.55);
            QRadialGradient glow(centre, radius + kStatusHalo);
            glow.setColorAt(0.0, halo);
            halo.setAlphaF(0.0);
            glow.setColorAt(1.0, halo);
            painter.setPen(Qt::NoPen);
            painter.setBrush(glow);
            painter.drawEllipse(centre, radius + kStatusHalo, radius + kStatusHalo);
            painter.setBrush(dotColour);
            painter.drawEllipse(centre, radius, radius);
            rightEdge -= kStatusDot + kRowGap;
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

        // The row being renamed draws no name.
        //
        // The inline editor is deliberately transparent so it sits in the
        // row rather than on top of it - see `QLineEdit#novuTreeRename` in
        // the sheet - which means anything this paints underneath shows
        // through it. The old label did, so starting a rename on "New Chat"
        // left "New Chat" behind the caret and whatever was typed landed on
        // top of it.
        //
        // The editor already carries the text, selected, so there is nothing
        // to draw here: the name is the editor's for as long as it is up.
        if (entry->id == m_editing) {
            continue;
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

    if (hasFocus() && !m_current.isEmpty() && !focusRingSuppressed(this)) {
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
