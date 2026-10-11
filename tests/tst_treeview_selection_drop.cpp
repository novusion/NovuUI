// TreeView multi-selection keyboard and drag & drop.
//
// Ctrl+A takes the visible rows a predicate accepts, Escape empties a
// selection, clear() leaves none behind, and a context menu acts on what is
// shown selected. A drag carries the selection in display order; a drop is
// refused onto the dragged rows and their subtrees, asked of the host's filter
// otherwise, and reported - never performed - by the tree.

#include "novu/ui/TreeView.h"

#include <QContextMenuEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QKeyEvent>
#include <QMimeData>
#include <QSignalSpy>
#include <QTest>

#include <memory>

using namespace novu::ui;

namespace {

// The handlers called directly. Qt routes a synthetic drag event through the
// drag manager, which only exists during a real drag; what is under test here
// is the tree's own answer, not Qt's delivery.
class ProbeTree : public TreeView
{
public:
    using TreeView::TreeView;
    using TreeView::contextMenuEvent;
    using TreeView::dragEnterEvent;
    using TreeView::dragMoveEvent;
    using TreeView::dropEvent;
    using TreeView::keyPressEvent;
};

// ws:a            row 0  (expanded)
//   cat:x         row 1  (expanded)
//     chat:1      row 2
//     chat:2      row 3
//   chat:3        row 4
// ws:b            row 5  (collapsed)
//   chat:4        hidden
void populate(TreeView& tree)
{
    tree.clear();
    tree.addNode(QString(), QStringLiteral("ws:a"), QStringLiteral("Alpha"),
                 Glyph::None, TreeView::Kind::Emphasis);
    tree.addNode(QStringLiteral("ws:a"), QStringLiteral("cat:x"), QStringLiteral("Ideas"));
    tree.addNode(QStringLiteral("cat:x"), QStringLiteral("chat:1"), QStringLiteral("One"));
    tree.addNode(QStringLiteral("cat:x"), QStringLiteral("chat:2"), QStringLiteral("Two"));
    tree.addNode(QStringLiteral("ws:a"), QStringLiteral("chat:3"), QStringLiteral("Three"));
    tree.addNode(QString(), QStringLiteral("ws:b"), QStringLiteral("Beta"),
                 Glyph::None, TreeView::Kind::Emphasis);
    tree.addNode(QStringLiteral("ws:b"), QStringLiteral("chat:4"), QStringLiteral("Four"));
    tree.setExpanded(QStringLiteral("ws:a"), true);
    tree.setExpanded(QStringLiteral("cat:x"), true);
}

QPoint centreOf(const TreeView& tree, const QString& id)
{
    return tree.rowRect(tree.rowOf(id)).center();
}

bool isChat(const QString& id) { return id.startsWith(QLatin1String("chat:")); }

bool moveAccepted(ProbeTree& tree, const QPoint& at, const QMimeData& mime)
{
    QDragMoveEvent move(at, Qt::MoveAction | Qt::CopyAction, &mime, Qt::LeftButton,
                        Qt::NoModifier);
    move.ignore();
    tree.dragMoveEvent(&move);
    return move.isAccepted();
}

} // namespace

class TreeViewSelectionDropTest : public QObject
{
    Q_OBJECT

private slots:
    void ctrlASelectsVisibleRowsTheFilterAccepts()
    {
        ProbeTree tree;
        tree.resize(260, 400);
        populate(tree);
        tree.setSelectionMode(TreeView::SelectionMode::Extended);
        tree.setSelectAllFilter(isChat);
        QSignalSpy changed(&tree, &TreeView::selectionChanged);

        QTest::keyClick(&tree, Qt::Key_A, Qt::ControlModifier);

        // chat:4 is a chat but inside the collapsed ws:b - not taken.
        const QStringList expected{QStringLiteral("chat:1"), QStringLiteral("chat:2"),
                                   QStringLiteral("chat:3")};
        QCOMPARE(tree.selectedIds(), expected);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(changed.last().first().toStringList(), expected);

        // Again: nothing changed, nothing announced.
        QTest::keyClick(&tree, Qt::Key_A, Qt::ControlModifier);
        QCOMPARE(changed.count(), 1);

        // No filter: every visible row.
        tree.setSelectAllFilter({});
        tree.selectAll();
        QCOMPARE(tree.selectionCount(), tree.visibleRowCount());
    }

    void ctrlAIsIgnoredInSingleMode()
    {
        ProbeTree tree;
        tree.resize(260, 400);
        populate(tree);
        tree.setCurrentId(QStringLiteral("chat:1"));
        QSignalSpy changed(&tree, &TreeView::selectionChanged);

        QKeyEvent press(QEvent::KeyPress, Qt::Key_A, Qt::ControlModifier);
        tree.keyPressEvent(&press);
        QVERIFY(!press.isAccepted());
        QCOMPARE(tree.selectedIds(), QStringList{QStringLiteral("chat:1")});
        QCOMPARE(changed.count(), 0);
    }

    void escapeClearsANonEmptySelectionOnly()
    {
        ProbeTree tree;
        tree.resize(260, 400);
        populate(tree);
        tree.setSelectionMode(TreeView::SelectionMode::Extended);
        tree.setSelectedIds({QStringLiteral("chat:1"), QStringLiteral("chat:3")});
        QSignalSpy changed(&tree, &TreeView::selectionChanged);

        QKeyEvent first(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        tree.keyPressEvent(&first);
        QVERIFY(first.isAccepted());
        QCOMPARE(tree.selectionCount(), 0);
        QCOMPARE(changed.count(), 1);
        QVERIFY(changed.last().first().toStringList().isEmpty());

        // Nothing left to clear: Escape belongs to the parent.
        QKeyEvent second(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        tree.keyPressEvent(&second);
        QVERIFY(!second.isAccepted());
        QCOMPARE(changed.count(), 1);
    }

    void clearDropsTheSelection()
    {
        ProbeTree tree;
        tree.resize(260, 400);
        populate(tree);
        tree.setSelectionMode(TreeView::SelectionMode::Extended);
        tree.setSelectedIds({QStringLiteral("chat:1"), QStringLiteral("chat:2")});
        QSignalSpy changed(&tree, &TreeView::selectionChanged);

        tree.clear();
        QCOMPARE(tree.selectionCount(), 0);
        QVERIFY(tree.selectedIds().isEmpty());
        QCOMPARE(changed.count(), 1);

        // Rows re-added under the same ids are not selected again.
        populate(tree);
        QVERIFY(!tree.isSelected(QStringLiteral("chat:1")));
        QCOMPARE(changed.count(), 1);   // clearing an empty one says nothing
    }

    void contextMenuActsOnWhatIsShownSelected()
    {
        ProbeTree tree;
        tree.resize(260, 400);
        populate(tree);
        tree.setSelectionMode(TreeView::SelectionMode::Extended);
        tree.setSelectedIds({QStringLiteral("chat:1"), QStringLiteral("chat:3")});
        QSignalSpy changed(&tree, &TreeView::selectionChanged);
        QSignalSpy menu(&tree, &TreeView::contextMenuRequested);

        // Inside the selection: kept.
        const QPoint inside = centreOf(tree, QStringLiteral("chat:3"));
        QContextMenuEvent onSelected(QContextMenuEvent::Mouse, inside, tree.mapToGlobal(inside));
        tree.contextMenuEvent(&onSelected);
        QCOMPARE(tree.selectedIds(),
                 (QStringList{QStringLiteral("chat:1"), QStringLiteral("chat:3")}));
        QCOMPARE(changed.count(), 0);
        QCOMPARE(menu.count(), 1);
        QCOMPARE(menu.last().first().toString(), QStringLiteral("chat:3"));
        QCOMPARE(tree.currentId(), QStringLiteral("chat:3"));

        // Outside it: that row alone, announced before the menu.
        const QPoint outside = centreOf(tree, QStringLiteral("chat:2"));
        QContextMenuEvent onOther(QContextMenuEvent::Mouse, outside, tree.mapToGlobal(outside));
        tree.contextMenuEvent(&onOther);
        QCOMPARE(tree.selectedIds(), QStringList{QStringLiteral("chat:2")});
        QCOMPARE(changed.count(), 1);
        QCOMPARE(changed.last().first().toStringList(), QStringList{QStringLiteral("chat:2")});
        QCOMPARE(menu.count(), 2);
        QCOMPARE(tree.currentId(), QStringLiteral("chat:2"));
    }

    void dragCarriesTheSelectionInDisplayOrder()
    {
        ProbeTree tree;
        tree.resize(260, 400);
        populate(tree);
        tree.setSelectionMode(TreeView::SelectionMode::Extended);
        tree.setDragEnabled(true);
        tree.setDraggableFilter(isChat);
        QVERIFY(tree.dragEnabled());

        // Marked bottom-up, and including a non-draggable row and a row that
        // a collapse is about to hide.
        tree.setSelectedIds({QStringLiteral("chat:3"), QStringLiteral("ws:b"),
                             QStringLiteral("chat:2"), QStringLiteral("chat:1")});
        tree.setExpanded(QStringLiteral("cat:x"), false);

        const QStringList expected{QStringLiteral("chat:1"), QStringLiteral("chat:2"),
                                   QStringLiteral("chat:3")};
        QCOMPARE(tree.dragIdsFor(QStringLiteral("chat:3")), expected);

        // A row outside the selection goes alone; a non-draggable one not at all.
        QCOMPARE(tree.dragIdsFor(QStringLiteral("chat:4")),
                 QStringList{QStringLiteral("chat:4")});
        QVERIFY(tree.dragIdsFor(QStringLiteral("ws:b")).isEmpty());
        QVERIFY(!tree.isDraggable(QStringLiteral("cat:x")));

        // The payload round-trips, under the tree type and as text.
        std::unique_ptr<QMimeData> mime(TreeView::createDragMimeData(expected));
        QVERIFY(mime->hasFormat(QStringLiteral("application/x-novu-tree-ids")));
        QCOMPARE(TreeView::dragMimeType(), QStringLiteral("application/x-novu-tree-ids"));
        QCOMPARE(TreeView::dragIdsFromMimeData(mime.get()), expected);
        QCOMPARE(mime->text(), expected.join(QLatin1Char('\n')));

        // Single mode drags the pressed row only.
        tree.setSelectionMode(TreeView::SelectionMode::Single);
        tree.setCurrentId(QStringLiteral("chat:3"));
        QCOMPARE(tree.dragIdsFor(QStringLiteral("chat:3")),
                 QStringList{QStringLiteral("chat:3")});
    }

    void dropFilterRefusesSelfAndDescendants()
    {
        ProbeTree tree;
        tree.resize(260, 400);
        populate(tree);
        int asked = 0;
        tree.setDropFilter([&asked](const QStringList& ids, const QString& target) {
            ++asked;
            Q_UNUSED(ids);
            return !isChat(target);
        });

        const QStringList category{QStringLiteral("cat:x")};
        // Onto itself and into its own subtree: refused without asking.
        QVERIFY(!tree.canDrop(category, QStringLiteral("cat:x")));
        QVERIFY(!tree.canDrop(category, QStringLiteral("chat:1")));
        QCOMPARE(asked, 0);
        // A valid target, asked of the host.
        QVERIFY(tree.canDrop(category, QStringLiteral("ws:b")));
        QCOMPARE(asked, 1);
        // The host's own refusal.
        QVERIFY(!tree.canDrop({QStringLiteral("chat:1")}, QStringLiteral("chat:3")));

        // The same answers through the drag handlers, with the highlight.
        std::unique_ptr<QMimeData> mime(
            TreeView::createDragMimeData({QStringLiteral("chat:1"), QStringLiteral("chat:2")}));
        QVERIFY(!moveAccepted(tree, centreOf(tree, QStringLiteral("chat:1")), *mime));
        QVERIFY(tree.dropTargetId().isEmpty());
        QVERIFY(!moveAccepted(tree, centreOf(tree, QStringLiteral("chat:3")), *mime));
        QVERIFY(moveAccepted(tree, centreOf(tree, QStringLiteral("ws:b")), *mime));
        QCOMPARE(tree.dropTargetId(), QStringLiteral("ws:b"));

        // A payload of another type is refused outright.
        QMimeData foreign;
        foreign.setText(QStringLiteral("chat:1"));
        QVERIFY(!moveAccepted(tree, centreOf(tree, QStringLiteral("ws:b")), foreign));
        QVERIFY(tree.dropTargetId().isEmpty());
    }

    void noFilterNoDrops()
    {
        ProbeTree tree;
        tree.resize(260, 400);
        populate(tree);
        QVERIFY(!tree.acceptDrops());
        QVERIFY(!tree.canDrop({QStringLiteral("chat:1")}, QStringLiteral("ws:b")));
        tree.setDropFilter([](const QStringList&, const QString&) { return true; });
        QVERIFY(tree.acceptDrops());
        QVERIFY(tree.viewport()->acceptDrops());
    }

    void dropReportsIdsAndTargetAndMovesNothing()
    {
        ProbeTree tree;
        tree.resize(260, 400);
        populate(tree);
        tree.setDropFilter([](const QStringList&, const QString& target) {
            return !isChat(target);
        });
        QSignalSpy dropped(&tree, &TreeView::rowsDropped);
        const QStringList ids{QStringLiteral("chat:1"), QStringLiteral("chat:3")};
        std::unique_ptr<QMimeData> mime(TreeView::createDragMimeData(ids));

        // Refused target: nothing reported.
        QDropEvent refused(QPointF(centreOf(tree, QStringLiteral("chat:2"))), Qt::MoveAction,
                           mime.get(), Qt::LeftButton, Qt::NoModifier);
        tree.dropEvent(&refused);
        QVERIFY(!refused.isAccepted());
        QCOMPARE(dropped.count(), 0);

        QVERIFY(moveAccepted(tree, centreOf(tree, QStringLiteral("ws:b")), *mime));
        QDropEvent drop(QPointF(centreOf(tree, QStringLiteral("ws:b"))), Qt::MoveAction,
                        mime.get(), Qt::LeftButton, Qt::NoModifier);
        tree.dropEvent(&drop);
        QVERIFY(drop.isAccepted());
        QCOMPARE(drop.dropAction(), Qt::MoveAction);
        QCOMPARE(dropped.count(), 1);
        QCOMPARE(dropped.last().at(0).toStringList(), ids);
        QCOMPARE(dropped.last().at(1).toString(), QStringLiteral("ws:b"));

        // The tree moved nothing and the highlight is gone.
        QCOMPARE(tree.parentOf(QStringLiteral("chat:1")), QStringLiteral("cat:x"));
        QCOMPARE(tree.parentOf(QStringLiteral("chat:3")), QStringLiteral("ws:a"));
        QVERIFY(tree.dropTargetId().isEmpty());
    }

    void hoveringACollapsedParentOpensIt()
    {
        ProbeTree tree;
        tree.resize(260, 400);
        populate(tree);
        QCOMPARE(tree.autoExpandDelay(), 700);
        tree.setAutoExpandDelay(60);
        tree.setDropFilter([](const QStringList&, const QString& target) {
            return !isChat(target);
        });
        QSignalSpy toggled(&tree, &TreeView::expandToggled);
        std::unique_ptr<QMimeData> mime(
            TreeView::createDragMimeData({QStringLiteral("chat:1")}));

        QVERIFY(!tree.isExpanded(QStringLiteral("ws:b")));
        QVERIFY(moveAccepted(tree, centreOf(tree, QStringLiteral("ws:b")), *mime));
        QVERIFY(!tree.isExpanded(QStringLiteral("ws:b")));   // not at once
        QTRY_VERIFY_WITH_TIMEOUT(tree.isExpanded(QStringLiteral("ws:b")), 2000);
        QCOMPARE(toggled.count(), 1);
        QCOMPARE(toggled.last().at(0).toString(), QStringLiteral("ws:b"));
        QCOMPARE(toggled.last().at(1).toBool(), true);
        QVERIFY(tree.rowOf(QStringLiteral("chat:4")) >= 0);
        // The rebuild kept the target lit.
        QCOMPARE(tree.dropTargetId(), QStringLiteral("ws:b"));

        // Leaving before the delay opens nothing.
        tree.setExpanded(QStringLiteral("cat:x"), false);
        QVERIFY(!moveAccepted(tree, centreOf(tree, QStringLiteral("chat:3")), *mime));
        QVERIFY(moveAccepted(tree, centreOf(tree, QStringLiteral("cat:x")), *mime));
        QDragLeaveEvent leave;
        QCoreApplication::sendEvent(tree.viewport(), &leave);
        QTest::qWait(200);
        QVERIFY(!tree.isExpanded(QStringLiteral("cat:x")));
        QVERIFY(tree.dropTargetId().isEmpty());
    }
};

QTEST_MAIN(TreeViewSelectionDropTest)
#include "tst_treeview_selection_drop.moc"
