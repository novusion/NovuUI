// TreeView's running status: a turning arrow in place of the dot.
//
// A spinning status paints differently from a still one and turns between two
// grabs; setStatus() keeps the spin and clearStatus() ends it; the one timer
// runs only while some node spins and stops once none does - after a clear,
// a removal or the spin being set off - so an idle tree costs nothing.

#include "novu/ui/TreeView.h"

#include <QImage>
#include <QTest>
#include <QTimer>

using namespace novu::ui;

namespace {

bool spinTimerActive(const TreeView& tree)
{
    for (const QTimer* timer : tree.findChildren<QTimer*>()) {
        if (timer->interval() == 33 && timer->isActive()) {
            return true;
        }
    }
    return false;
}

QImage statusArea(TreeView& tree, const QString& id)
{
    const QRect row = tree.rowRect(tree.rowOf(id));
    // the trailing edge, where the status sits
    const QRect area(row.right() - 40, row.top(), 40, row.height());
    return tree.viewport()->grab(area).toImage();
}

} // namespace

class TreeViewStatusSpinnerTest : public QObject
{
    Q_OBJECT

private slots:
    void spinsOnlyWhileAsked()
    {
        TreeView tree;
        tree.resize(260, 200);
        tree.addNode(QString(), QStringLiteral("chat:1"), QStringLiteral("One"));
        tree.addNode(QString(), QStringLiteral("chat:2"), QStringLiteral("Two"));
        tree.show();
        QVERIFY(QTest::qWaitForWindowExposed(&tree));

        tree.setStatus(QStringLiteral("chat:1"), StatusDot::Tone::Success);
        QVERIFY(!tree.isStatusSpinning(QStringLiteral("chat:1")));
        QVERIFY(!spinTimerActive(tree));
        const QImage dot = statusArea(tree, QStringLiteral("chat:1"));

        tree.setStatusSpinning(QStringLiteral("chat:1"), true);
        QVERIFY(tree.isStatusSpinning(QStringLiteral("chat:1")));
        QVERIFY(spinTimerActive(tree));
        const QImage arrow = statusArea(tree, QStringLiteral("chat:1"));
        QVERIFY2(arrow != dot, "a running status must not paint as the still dot");

        // it turns: two grabs a quarter of a turn apart differ
        QTest::qWait(375);
        QVERIFY2(statusArea(tree, QStringLiteral("chat:1")) != arrow, "the arrow must turn");

        // a tone change keeps the spin
        tree.setStatus(QStringLiteral("chat:1"), StatusDot::Tone::Warning);
        QVERIFY(tree.isStatusSpinning(QStringLiteral("chat:1")));

        // spin off -> still dot, and the timer stops on its next tick
        tree.setStatusSpinning(QStringLiteral("chat:1"), false);
        QTRY_VERIFY_WITH_TIMEOUT(!spinTimerActive(tree), 500);

        // clearStatus ends it
        tree.setStatusSpinning(QStringLiteral("chat:2"), true);
        tree.setStatus(QStringLiteral("chat:2"), StatusDot::Tone::Success);
        QVERIFY(spinTimerActive(tree));
        tree.clearStatus(QStringLiteral("chat:2"));
        QVERIFY(!tree.isStatusSpinning(QStringLiteral("chat:2")));
        QTRY_VERIFY_WITH_TIMEOUT(!spinTimerActive(tree), 500);

        // a removed spinning row does not keep the timer alive
        tree.setStatus(QStringLiteral("chat:2"), StatusDot::Tone::Success);
        tree.setStatusSpinning(QStringLiteral("chat:2"), true);
        QVERIFY(spinTimerActive(tree));
        tree.removeNode(QStringLiteral("chat:2"));
        QTRY_VERIFY_WITH_TIMEOUT(!spinTimerActive(tree), 500);

        // and neither does clear()
        tree.setStatus(QStringLiteral("chat:1"), StatusDot::Tone::Success);
        tree.setStatusSpinning(QStringLiteral("chat:1"), true);
        tree.clear();
        QTRY_VERIFY_WITH_TIMEOUT(!spinTimerActive(tree), 500);
    }

    void manyRowsShareOneTimer()
    {
        TreeView tree;
        tree.resize(260, 400);
        for (int i = 0; i < 12; ++i) {
            const QString id = QStringLiteral("chat:%1").arg(i);
            tree.addNode(QString(), id, id);
            tree.setStatus(id, StatusDot::Tone::Success);
            tree.setStatusSpinning(id, true);
        }
        int active = 0;
        for (const QTimer* timer : tree.findChildren<QTimer*>()) {
            active += timer->isActive() ? 1 : 0;
        }
        QCOMPARE(active, 1);
    }
};

QTEST_MAIN(TreeViewStatusSpinnerTest)
#include "tst_treeview_status_spinner.moc"
