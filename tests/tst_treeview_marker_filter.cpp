// TreeView's colour markers and its row filter.
//
// A marker is a thin bar before the icon in the marker's colour - and nothing
// else on the row changes colour; an invalid colour takes it away. The row
// filter hides what it refuses with its subtree and removes nothing: clearing
// it brings every row back.

#include "novu/ui/TreeView.h"

#include <QImage>
#include <QSet>
#include <QTest>

using namespace novu::ui;

namespace {

bool hasColour(const QImage& image, const QColor& colour)
{
    for (int y = 0; y < image.height(); ++y)
        for (int x = 0; x < image.width(); ++x)
            if (image.pixelColor(x, y).rgb() == colour.rgb()) return true;
    return false;
}

} // namespace

class TreeViewMarkerFilterTest : public QObject
{
    Q_OBJECT

private slots:
    void markerIsABarBeforeTheIcon()
    {
        TreeView tree;
        tree.resize(260, 200);
        tree.addNode(QString(), QStringLiteral("ws"), QStringLiteral("Workspace"), Glyph::Folder);
        tree.addNode(QStringLiteral("ws"), QStringLiteral("chat"), QStringLiteral("Chat"), Glyph::Chat);
        tree.setExpanded(QStringLiteral("ws"), true);
        tree.show();
        QVERIFY(QTest::qWaitForWindowExposed(&tree));

        const QColor red("#d50000");
        const QRect row = tree.rowRect(tree.rowOf(QStringLiteral("chat")));
        QVERIFY(!hasColour(tree.viewport()->grab(row).toImage(), red));
        tree.setMarker(QStringLiteral("chat"), red);
        QCOMPARE(tree.marker(QStringLiteral("chat")), red);
        const QImage marked = tree.viewport()->grab(row).toImage();
        QVERIFY2(hasColour(marked, red), "the bar is painted in the marker's colour");
        // a bar: 3px wide, before the label - the right part of the row keeps its colours
        QVERIFY(!hasColour(marked.copy(row.width() / 2, 0, row.width() / 2, row.height()), red));
        // the workspace row is untouched
        QVERIFY(!hasColour(tree.viewport()->grab(tree.rowRect(tree.rowOf(QStringLiteral("ws")))).toImage(), red));
        tree.setMarker(QStringLiteral("chat"), QColor());
        QVERIFY(!tree.marker(QStringLiteral("chat")).isValid());
        QVERIFY(!hasColour(tree.viewport()->grab(row).toImage(), red));
    }

    void filterHidesAndNeverRemoves()
    {
        TreeView tree;
        tree.addNode(QString(), QStringLiteral("ws"), QStringLiteral("Workspace"));
        tree.addNode(QStringLiteral("ws"), QStringLiteral("cat"), QStringLiteral("Category"));
        tree.addNode(QStringLiteral("cat"), QStringLiteral("c1"), QStringLiteral("One"));
        tree.addNode(QStringLiteral("ws"), QStringLiteral("c2"), QStringLiteral("Two"));
        tree.setExpanded(QStringLiteral("ws"), true);
        tree.setExpanded(QStringLiteral("cat"), true);
        QCOMPARE(tree.visibleRowCount(), 4);
        const int nodes = tree.nodeCount();

        // only c2 and the parent it needs
        const QSet<QString> pass{QStringLiteral("ws"), QStringLiteral("c2")};
        tree.setRowFilter([pass](const QString& id) { return pass.contains(id); });
        QVERIFY(tree.hasRowFilter());
        QCOMPARE(tree.visibleRowCount(), 2);
        QCOMPARE(tree.rowOf(QStringLiteral("c1")), -1);
        QVERIFY(tree.rowOf(QStringLiteral("c2")) >= 0);
        QCOMPARE(tree.nodeCount(), nodes);           // nothing removed

        tree.setRowFilter({});
        QVERIFY(!tree.hasRowFilter());
        QCOMPARE(tree.visibleRowCount(), 4);
    }
};

QTEST_MAIN(TreeViewMarkerFilterTest)
#include "tst_treeview_marker_filter.moc"
