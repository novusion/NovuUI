// What a transient surface owes its user: it goes away.
//
// Popover is declared Qt::Popup, which is the mechanism that grabs the mouse
// and closes the menu on the first click elsewhere. These tests pin that the
// declaration actually reaches the shown window, and that the other ways out -
// Escape, a second menu, the window losing focus - end the same way. They are
// here because a menu that stays open over the application is not a cosmetic
// fault: it is a surface the user cannot dismiss.

#include "novu/ui/Popover.h"
#include "novu/ui/PopupSurface.h"

#include <QApplication>
#include <QMouseEvent>
#include <QLabel>
#include <QSignalSpy>
#include <QTest>
#include <QVBoxLayout>
#include <QWidget>

using novu::ui::Popover;
using novu::ui::PopupSurface;
using novu::ui::SelectButton;

namespace {

// A stand-in for the application underneath the menu.
QWidget* makeHost()
{
    auto* host = new QWidget;
    host->resize(600, 400);
    auto* layout = new QVBoxLayout(host);
    layout->addWidget(new QLabel(QStringLiteral("content"), host));
    return host;
}

Popover* makeMenu(QWidget* parent)
{
    auto* menu = new Popover(parent);
    menu->addItem(QStringLiteral("one"), QStringLiteral("One"));
    menu->addItem(QStringLiteral("two"), QStringLiteral("Two"));
    return menu;
}

}

class PopoverDismissalTest : public QObject
{
    Q_OBJECT

private slots:
    // The window type is the whole mechanism. If a later change to the
    // constructor drops Qt::Popup, every dismissal below stops working and
    // nothing else in the library would notice.
    void popoverIsDeclaredAPopupWindow()
    {
        QWidget* host = makeHost();
        host->show();
        Popover* menu = makeMenu(host);

        QCOMPARE(menu->windowFlags() & Qt::WindowType_Mask, Qt::Popup);
        QVERIFY(menu->isWindow());

        delete host;
    }

    // Shown as a popup, Qt registers it as *the* active popup - which is what
    // gives it the mouse grab that closes it on an outside click.
    void showingItMakesItTheActivePopup()
    {
        QWidget* host = makeHost();
        host->show();
        QVERIFY(QTest::qWaitForWindowExposed(host));

        Popover* menu = makeMenu(host);
        menu->popupAt(host->mapToGlobal(QPoint(10, 10)));

        QVERIFY(menu->isVisible());
        QCOMPARE(QApplication::activePopupWidget(), menu);

        menu->hide();
        delete host;
    }

    // Escape closes it, and says so.
    void escapeClosesItAndEmitsClosed()
    {
        QWidget* host = makeHost();
        host->show();
        QVERIFY(QTest::qWaitForWindowExposed(host));

        Popover* menu = makeMenu(host);
        QSignalSpy closed(menu, &Popover::closed);
        menu->popupAt(host->mapToGlobal(QPoint(10, 10)));
        QVERIFY(menu->isVisible());

        QTest::keyClick(menu, Qt::Key_Escape);

        QVERIFY(!menu->isVisible());
        QCOMPARE(closed.count(), 1);

        delete host;
    }

    // Opening a second menu must leave exactly one open.
    void openingASecondMenuClosesTheFirst()
    {
        QWidget* host = makeHost();
        host->show();
        QVERIFY(QTest::qWaitForWindowExposed(host));

        Popover* first = makeMenu(host);
        first->popupAt(host->mapToGlobal(QPoint(10, 10)));
        QCOMPARE(QApplication::activePopupWidget(), first);

        Popover* second = makeMenu(host);
        second->popupAt(host->mapToGlobal(QPoint(200, 10)));

        QCOMPARE(QApplication::activePopupWidget(), second);
        QVERIFY(!first->isVisible());
        QVERIFY(second->isVisible());

        second->hide();
        delete host;
    }

    // A selector inside a content popup opens a nested popup. The menu takes
    // the active popup grab, but its owning surface must stay behind it.
    void nestedSelectKeepsOwningPopupOpen()
    {
        QWidget* host = makeHost();
        host->show();
        QVERIFY(QTest::qWaitForWindowExposed(host));

        auto* surface = new PopupSurface(host);
        auto* content = new QWidget;
        auto* layout = new QVBoxLayout(content);
        auto* select = new SelectButton(content);
        select->addOption(QStringLiteral("all"), QStringLiteral("All areas"));
        select->addOption(QStringLiteral("docs"), QStringLiteral("Documents"));
        layout->addWidget(select);
        surface->setContent(content);
        surface->popupAt(host->mapToGlobal(QPoint(20, 20)));

        QVERIFY(surface->isVisible());
        QCOMPARE(QApplication::activePopupWidget(), surface);

        select->click();
        QVERIFY(select->menu()->isVisible());
        QVERIFY(surface->isVisible());
        QCOMPARE(QApplication::activePopupWidget(), select->menu());

        QTest::keyClick(select->menu(), Qt::Key_Escape);
        QVERIFY(!select->menu()->isVisible());
        QVERIFY(surface->isVisible());

        select->click();
        const int documents = select->menu()->indexOfKey(QStringLiteral("docs"));
        QVERIFY(documents >= 0);
        QTest::mouseClick(select->menu(), Qt::LeftButton, Qt::NoModifier,
                          select->menu()->itemRect(documents).center());
        QCOMPARE(select->currentKey(), QStringLiteral("docs"));
        QVERIFY(!select->menu()->isVisible());
        QVERIFY(surface->isVisible());

        QTest::keyClick(surface, Qt::Key_Escape);
        QVERIFY(!surface->isVisible());

        surface->popupAt(host->mapToGlobal(QPoint(20, 20)));
        const QPoint outside(surface->width() + 40, surface->height() + 40);
        QMouseEvent press(QEvent::MouseButtonPress, QPointF(outside),
                          surface->mapToGlobal(outside), Qt::LeftButton,
                          Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(surface, &press);
        QVERIFY(!surface->isVisible());
        QVERIFY(!press.isAccepted());

        delete host;
    }

    // A click outside closes it. This is the case the user actually hits, and
    // the one a native child window - a web view, a video surface - can break
    // without anything in Qt noticing.
    void aClickOutsideClosesIt()
    {
        QWidget* host = makeHost();
        host->show();
        QVERIFY(QTest::qWaitForWindowExposed(host));

        Popover* menu = makeMenu(host);
        menu->popupAt(host->mapToGlobal(QPoint(10, 10)));
        QVERIFY(menu->isVisible());

        // A grabbing popup is handed presses that land anywhere, expressed in
        // its own coordinates - so a click below and right of the menu arrives
        // here with a position outside rect(). That is the real shape of the
        // event, and sending it directly is what makes this test meaningful on
        // a platform whose grab the test harness does not emulate.
        const QPoint outside(menu->width() + 40, menu->height() + 40);
        QMouseEvent press(QEvent::MouseButtonPress, QPointF(outside),
                          menu->mapToGlobal(outside), Qt::LeftButton,
                          Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(menu, &press);

        QVERIFY(!menu->isVisible());
        // The press must stay unhandled: swallowing it is what left the menu
        // open over the application in the first place.
        QVERIFY(!press.isAccepted());

        delete host;
    }

    // Losing the active window closes it: a transient menu must not hang over
    // another application.
    void losingWindowActivationClosesIt()
    {
        QWidget* host = makeHost();
        host->show();
        QVERIFY(QTest::qWaitForWindowExposed(host));

        Popover* menu = makeMenu(host);
        menu->popupAt(host->mapToGlobal(QPoint(10, 10)));
        QVERIFY(menu->isVisible());

        QEvent deactivate(QEvent::WindowDeactivate);
        QApplication::sendEvent(menu, &deactivate);

        QVERIFY(!menu->isVisible());

        delete host;
    }
};

QTEST_MAIN(PopoverDismissalTest)
#include "tst_popover_dismissal.moc"
