#pragma once

#include "novu/ui/Badge.h"
#include "novu/ui/Icon.h"
#include "novu/ui/StyleSpec.h"

#include <QAbstractScrollArea>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

namespace novu::ui {

// A structure of nested things: the approved NovuAI design's `.tree`, `.trow`
// and `.tkids`.
//
// NavList is one indent deep on purpose — the NovuSpace design's note is that
// a sidebar must show two levels *without turning into a file browser*. This is
// the file browser: arbitrary depth, an indent guide down each level, and a row
// that can carry a chevron, an icon, a name, marks, a count and a tag.
//
// The two are not one component with a flag. A navigation list is a set of
// places, all of which exist when it is built; a tree is a structure that is
// discovered as it is opened, and its rows are nodes with parents. Merging them
// would put a parent pointer on every sidebar row and a "flat" mode on every
// tree.
//
// ── one widget, whatever the depth ───────────────────────────────────────
//
// Rows are painted. A workspace tree with two hundred visible nodes, each of
// which the HTML draws as a button containing a chevron, an icon, a label, two
// marks and a count, is a thousand widgets — and most of them are hidden most
// of the time, so they are a thousand widgets that exist to be collapsed.
//
// The visible rows are flattened into a vector when the shape changes, and only
// the ones on screen are drawn. Opening a folder of ten thousand files costs
// one flatten and one paint of twenty rows.
//
// ── why this does not derive from MonoLineView ───────────────────────────
//
// It virtualises the same way, and that is the whole of the resemblance.
// MonoLineView is monospace *output*: a fixed face, no hover, no selection, no
// hit zones, and a contract that says a row is a line of text. A tree is an
// interactive control in the proportional face with three targets per row.
// A shared base would need a font flag, a hit-testing hook and a selection
// model none of its other subclasses want, which is a base class shaped by its
// most demanding user rather than by what its users have in common.
class TreeView : public QAbstractScrollArea
{
    Q_OBJECT

public:
    // What a row *is*. The design draws four and they differ in typography and
    // in whether the row is a destination at all.
    enum class Kind {
        Item,      // `.trow` — a thing you can open
        Group,     // `.trow.k-group` — a tracked uppercase caption with children
        Emphasis,  // `.trow.k-ws` — the heavier row a workspace gets
    };

    // Per-row state that is not selection. The design's `ws-on`, `detached`
    // and `drop-on`.
    enum class Mark {
        None,
        Active,     // the workspace currently in scope: accent bar and ink
        Detached,   // present but not loaded: italic, muted
        Dropping,   // a drag is over this row
    };

    explicit TreeView(QWidget* parent = nullptr);

    // ── shape ────────────────────────────────────────────────────────────
    //
    // `parentId` empty adds a root. Ids are the caller's own and are never
    // shown, so they survive translation and re-ordering.
    void addNode(const QString& parentId, const QString& id, const QString& label,
                 Glyph glyph = Glyph::None, Kind kind = Kind::Item);
    void removeNode(const QString& id);
    void clear();

    bool contains(const QString& id) const;
    QString parentOf(const QString& id) const;
    QStringList childrenOf(const QString& id) const;
    int depthOf(const QString& id) const;
    int nodeCount() const { return m_nodes.size(); }
    // How many rows are on screen at all - the flattened, expanded set. This is
    // what the scroll bar measures, and what a test asserting "collapsing hid
    // the children" should read.
    int visibleRowCount() const { return m_rows.size(); }

    // ── content ──────────────────────────────────────────────────────────
    void setLabel(const QString& id, const QString& label);
    QString label(const QString& id) const;
    void setGlyph(const QString& id, Glyph glyph);
    // -1 removes the pill.
    void setCount(const QString& id, int count);
    // `.trow .tag` — a short word after the name: a branch, a state, a format.
    void setTag(const QString& id, const QString& text,
                Badge::Tone tone = Badge::Tone::Neutral);
    // `.trow .tmarks` — small glyphs at the right, drawn faint until the row is
    // hovered. For the properties of a node rather than actions on it: modified,
    // shared, pinned.
    void setMarks(const QString& id, const QVector<Glyph>& marks);
    void setNodeKind(const QString& id, Kind kind);
    void setMark(const QString& id, Mark mark);
    Mark mark(const QString& id) const;
    void setNodeEnabled(const QString& id, bool enabled);

    // ── state ────────────────────────────────────────────────────────────
    bool isExpanded(const QString& id) const;
    void setExpanded(const QString& id, bool expanded);
    void expandAll();
    void collapseAll();
    // Opens every ancestor so `id` is on screen, and scrolls to it.
    void revealNode(const QString& id);

    QString currentId() const;
    void setCurrentId(const QString& id);

    // ── geometry ─────────────────────────────────────────────────────────
    QRect rowRect(int row) const;
    QRect chevronRect(int row) const;
    int rowAt(const QPoint& position) const;
    QString idAt(const QPoint& position) const;
    int rowOf(const QString& id) const;

    int rowHeight() const;
    // The design's `--row-h` against its 24px chat rows: a density for the whole
    // tree rather than a height per row, because per-row heights turn every hit
    // test into a search.
    bool isCompact() const { return m_compact; }
    void setCompact(bool compact);

    StyleSpec styleSpec() const;
    StyleSpec rowStyleSpec(int row) const;
    ResolvedStyle resolvedStyle() const;

signals:
    void activated(const QString& id);
    void currentChanged(const QString& id);
    void expandToggled(const QString& id, bool expanded);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool event(QEvent* event) override;

private:
    int indentStep() const;
    struct Node
    {
        QString id;
        QString parent;
        QStringList children;
        QString label;
        QString tag;
        Badge::Tone tagTone = Badge::Tone::Neutral;
        QVector<Glyph> marks;
        Glyph glyph = Glyph::None;
        Kind kind = Kind::Item;
        Mark mark = Mark::None;
        int count = -1;
        int depth = 0;
        bool expanded = false;
        bool enabled = true;
    };

    void rebuildRows();
    void flatten(const QString& id, int depth);
    void updateScrollRange();
    const Node* node(const QString& id) const;
    Node* node(const QString& id);
    void removeSubtree(const QString& id);
    int firstVisibleRow() const;

    QHash<QString, Node> m_nodes;
    QStringList m_roots;
    // The expanded set, flattened in display order. Rebuilt whenever the shape
    // or an expansion changes; everything else - hover, selection, paint - is
    // an index into this.
    QVector<QString> m_rows;

    QString m_current;
    int m_hovered = -1;
    bool m_hoveredChevron = false;
    int m_pressed = -1;
    bool m_pressedChevron = false;
    bool m_compact = false;
};

} // namespace novu::ui
