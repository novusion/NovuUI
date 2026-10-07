// NavList drop targets: which rows accept a drop, and what a drop reports.
//
// A rail row becomes a drop target only when its owner says so, for one MIME
// type. Everything else - other rows, captions, empty space, a drag carrying
// something else - refuses. A drop reports the row and the payload and moves
// nothing itself. The targets belong to the list, not to the rows, so a rail
// rebuilt from data keeps them.

#include "novu/ui/NavList.h"

#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QSignalSpy>
#include <QTest>

using namespace novu::ui;

namespace {

const QString kMime = QStringLiteral("application/x-test-ids");

// The handlers called directly. Qt routes a synthetic drag event through the
// drag manager, which only exists during a real drag; what is under test here
// is the list's own answer, not Qt's delivery.
class ProbeNavList : public NavList
{
public:
    using NavList::NavList;
    using NavList::dragMoveEvent;
    using NavList::dropEvent;
};

void populate(NavList& list)
{
    list.clear();
    list.addSection(QStringLiteral("Collections"), QStringLiteral("sec:collections"));
    list.addItem(QStringLiteral("vl:personal"), QStringLiteral("Personal"));
    list.addItem(QStringLiteral("vl:dev"), QStringLiteral("Development"));
    list.addSection(QStringLiteral("Types"));
    list.addItem(QStringLiteral("vc:password"), QStringLiteral("Passwords"));
}

bool acceptsMoveAt(ProbeNavList& list, const QPoint& at, const QMimeData& mime)
{
    QDragMoveEvent move(at, Qt::MoveAction, &mime, Qt::LeftButton, Qt::NoModifier);
    move.ignore();
    list.dragMoveEvent(&move);
    return move.isAccepted();
}

// The y of the first pixel row that the list reports for `key`, found by
// probing with a drop of the right type.
int yOfTarget(ProbeNavList& list, const QMimeData& mime)
{
    for (int y = 0; y < list.height(); ++y) {
        if (acceptsMoveAt(list, QPoint(40, y), mime)) {
            return y;
        }
    }
    return -1;
}

} // namespace

class NavListDropTest : public QObject
{
    Q_OBJECT

private slots:
    void onlyDeclaredRowsAccept()
    {
        ProbeNavList list;
        list.resize(238, 400);
        populate(list);
        list.show();
        QMimeData mime;
        mime.setData(kMime, QByteArrayLiteral("[\"a\"]"));

        // Before any targets are declared, nothing accepts.
        QVERIFY(!list.acceptDrops());
        QCOMPARE(yOfTarget(list, mime), -1);

        list.setDropTargets(kMime, {QStringLiteral("vl:dev")});
        QVERIFY(list.acceptDrops());
        const int y = yOfTarget(list, mime);
        QVERIFY(y >= 0);

        // Exactly one band of pixel rows accepts: the vl:dev row. Count them
        // and make sure every accepting row is contiguous.
        int accepting = 0;
        int last = -2;
        bool contiguous = true;
        for (int py = 0; py < list.height(); ++py) {
            if (acceptsMoveAt(list, QPoint(40, py), mime)) {
                if (last >= 0 && py != last + 1) {
                    contiguous = false;
                }
                last = py;
                ++accepting;
            }
        }
        QVERIFY(accepting > 0);
        QVERIFY(contiguous);
    }

    void aDifferentPayloadIsRefused()
    {
        ProbeNavList list;
        list.resize(238, 400);
        populate(list);
        list.show();
        list.setDropTargets(kMime, {QStringLiteral("vl:dev")});
        QMimeData right;
        right.setData(kMime, QByteArrayLiteral("[]"));
        const int y = yOfTarget(list, right);
        QVERIFY(y >= 0);
        QMimeData wrong;
        wrong.setText(QStringLiteral("a secret pasted from somewhere"));
        QVERIFY(!acceptsMoveAt(list, QPoint(40, y), wrong));
    }

    void aDropReportsRowAndPayload()
    {
        ProbeNavList list;
        list.resize(238, 400);
        populate(list);
        list.show();
        list.setDropTargets(kMime, {QStringLiteral("vl:dev")});
        QSignalSpy dropped(&list, &NavList::rowDropped);
        QMimeData mime;
        mime.setData(kMime, QByteArrayLiteral("[\"id-1\",\"id-2\"]"));
        const int y = yOfTarget(list, mime);
        QVERIFY(y >= 0);

        QDropEvent drop(QPointF(40, y), Qt::MoveAction, &mime, Qt::LeftButton,
                        Qt::NoModifier);
        list.dropEvent(&drop);
        QCOMPARE(dropped.count(), 1);
        QCOMPARE(dropped.at(0).at(0).toString(), QStringLiteral("vl:dev"));
        QCOMPARE(dropped.at(0).at(1).toByteArray(), QByteArrayLiteral("[\"id-1\",\"id-2\"]"));

        // Not on a target: nothing is reported.
        QDropEvent miss(QPointF(40, 1), Qt::MoveAction, &mime, Qt::LeftButton,
                        Qt::NoModifier);
        list.dropEvent(&miss);
        QCOMPARE(dropped.count(), 1);
    }

    void targetsSurviveARebuild()
    {
        ProbeNavList list;
        list.resize(238, 400);
        populate(list);
        list.show();
        list.setDropTargets(kMime, {QStringLiteral("vl:dev")});
        populate(list);
        QMimeData mime;
        mime.setData(kMime, QByteArrayLiteral("[]"));
        QVERIFY(yOfTarget(list, mime) >= 0);
    }

    void emptyTargetsTurnDropsOff()
    {
        ProbeNavList list;
        populate(list);
        list.setDropTargets(kMime, {QStringLiteral("vl:dev")});
        QVERIFY(list.acceptDrops());
        list.setDropTargets(kMime, {});
        QVERIFY(!list.acceptDrops());
    }
};

QTEST_MAIN(NavListDropTest)
#include "tst_navlist_drop.moc"
