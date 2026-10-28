// A control as a row of a Popover - the colour row under "Marker".
//
// The menu owns it, places it in its row at the row's width and the height it
// asks for at that width, steps over it with the keyboard, and a click lands
// on the control itself. clear() takes it away with the rows.

#include "novu/ui/Popover.h"
#include "novu/ui/Swatch.h"

#include <QPointer>
#include <QSignalSpy>
#include <QTest>

using namespace novu::ui;

class PopoverControlTest : public QObject
{
    Q_OBJECT

private slots:
    void controlRowIsPlacedAndClickable()
    {
        QWidget anchor;
        anchor.resize(200, 40);
        anchor.show();
        QVERIFY(QTest::qWaitForWindowExposed(&anchor));

        Popover menu(&anchor);
        menu.addItem(QStringLiteral("rename"), QStringLiteral("Rename"));
        menu.addGroup(QStringLiteral("Marker"));
        auto* swatch = new Swatch;
        swatch->setColors({QColor("#2979ff"), QColor("#00c853"), QColor("#ffb300"), QColor("#d50000")}, true, true);
        const int row = menu.addControl(swatch);
        menu.addItem(QStringLiteral("delete"), QStringLiteral("Delete"));
        QCOMPARE(row, 2);
        QCOMPARE(swatch->parentWidget(), &menu);

        menu.popupAt(anchor.mapToGlobal(QPoint(10, 10)));
        QVERIFY(QTest::qWaitForWindowExposed(&menu));
        QVERIFY(swatch->isVisible());
        // inside the menu, between the group and the next item, its own height at its width
        const QRect g = swatch->geometry();
        QVERIFY2(QRect(QPoint(0, 0), menu.size()).contains(g), "the control sits inside the menu");
        QVERIFY(g.width() > 0);
        QCOMPARE(g.height(), swatch->heightForWidth(g.width()));

        // a click on a swatch is the swatch's
        QSignalSpy picked(swatch, &Swatch::picked);
        QTest::mouseClick(swatch, Qt::LeftButton, Qt::NoModifier, swatch->swatchRect(1).center());
        QCOMPARE(picked.count(), 1);

        // clear() takes it with the rows
        QPointer<Swatch> gone(swatch);
        menu.clear();
        QVERIFY(gone.isNull());
    }
};

QTEST_MAIN(PopoverControlTest)
#include "tst_popover_control.moc"
