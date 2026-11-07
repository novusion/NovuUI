// The global tooltip switch.
//
// Off has to mean off for every hover path at once - the branded card, a plain
// QWidget::setToolTip, an item view's Qt::ToolTipRole and a component's own
// QEvent::ToolTip handler - because an application built from parts written at
// different times has all of them, and a setting that silences three of four
// reads as a broken setting. On has to bring every one of them back.

#include "novu/ui/Tooltip.h"

#include <QApplication>
#include <QHelpEvent>
#include <QStandardItemModel>
#include <QListView>
#include <QTest>
#include <QToolTip>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace {

// Stands in for a component with its own tooltip handler - Popover's elided
// entries, QTabBar, QMenu, QWebEngineView all work this way.
class OwnHandler final : public QWidget
{
public:
    int received = 0;

protected:
    bool event(QEvent* event) override
    {
        if (event->type() == QEvent::ToolTip) {
            ++received;
            return true;
        }
        return QWidget::event(event);
    }
};

// Shown and active. QToolTip closes itself on WindowActivate, so a tip shown
// before the activation that follows a show() arrives is gone a moment later.
bool present(QWidget* window)
{
    window->show();
    window->activateWindow();
    if (!QTest::qWaitForWindowExposed(window))
        return false;
    QTest::qWaitForWindowActive(window);
    QTest::qWait(50);
    return true;
}

void sendToolTip(QWidget* target, const QPoint& at = QPoint(4, 4))
{
    QHelpEvent help(QEvent::ToolTip, at, target->mapToGlobal(at));
    QApplication::sendEvent(target, &help);
}

}

class TooltipGateTest : public QObject
{
    Q_OBJECT

private slots:
    void cleanup()
    {
        setTooltipsEnabled(true);
        Tooltip::hideTip();
        QToolTip::hideText();
    }

    // An application that never touches the switch keeps its tooltips.
    void defaultIsOn()
    {
        QVERIFY(tooltipsEnabled());
    }

    void plainToolTipFollowsTheSwitch()
    {
        QWidget host;
        auto* plain = new QWidget(&host);
        plain->setToolTip(QStringLiteral("Rows per page"));
        plain->setGeometry(10, 10, 80, 30);
        host.resize(200, 100);
        QVERIFY(present(&host));

        setTooltipsEnabled(false);
        QVERIFY(!tooltipsEnabled());
        sendToolTip(plain);
        QVERIFY(!QToolTip::isVisible());

        setTooltipsEnabled(true);
        sendToolTip(plain);
        QTRY_VERIFY(QToolTip::isVisible());
        QCOMPARE(QToolTip::text(), QStringLiteral("Rows per page"));
    }

    void itemViewToolTipRoleFollowsTheSwitch()
    {
        QStandardItemModel model;
        auto* item = new QStandardItem(QStringLiteral("Fix the flicker"));
        item->setToolTip(QStringLiteral("Captured yesterday"));
        model.appendRow(item);
        QListView view;
        view.setModel(&model);
        view.resize(240, 120);
        QVERIFY(present(&view));
        const QPoint at = view.visualRect(model.index(0, 0)).center();

        setTooltipsEnabled(false);
        sendToolTip(view.viewport(), at);
        QVERIFY(!QToolTip::isVisible());

        setTooltipsEnabled(true);
        sendToolTip(view.viewport(), at);
        QTRY_VERIFY(QToolTip::isVisible());
        QCOMPARE(QToolTip::text(), QStringLiteral("Captured yesterday"));
    }

    // The filter sits in front of the receiver, so a component's own handler
    // is silenced too - which is what reaches modules built before the switch.
    void componentHandlerIsNotReachedWhenOff()
    {
        OwnHandler widget;
        widget.resize(80, 30);
        QVERIFY(present(&widget));

        setTooltipsEnabled(false);
        sendToolTip(&widget);
        QCOMPARE(widget.received, 0);

        setTooltipsEnabled(true);
        sendToolTip(&widget);
        QCOMPARE(widget.received, 1);
    }

    void brandedTipFollowsTheSwitch()
    {
        QWidget host;
        auto* button = new QWidget(&host);
        button->setGeometry(20, 40, 80, 30);
        host.resize(240, 140);
        Tooltip::install(button, TipContent::action(QStringLiteral("Timeline"),
                                                    QStringLiteral("Open the Timeline section.")));
        QVERIFY(present(&host));

        setTooltipsEnabled(false);
        Tooltip::showFor(button);
        QVERIFY(Tooltip::shownFor() == nullptr);
        QVERIFY(!Tooltip::instance().isVisible());

        // The hover path, not only showFor(): an Enter that would have
        // started the timer starts nothing.
        QEnterEvent enter(QPointF(4, 4), QPointF(4, 4), button->mapToGlobal(QPointF(4, 4)));
        QApplication::sendEvent(button, &enter);
        QTest::qWait(600);
        QVERIFY(Tooltip::shownFor() == nullptr);

        setTooltipsEnabled(true);
        Tooltip::showFor(button);
        QCOMPARE(Tooltip::shownFor(), button);
        QVERIFY(Tooltip::instance().isVisible());
        Tooltip::remove(button);
    }

    // Turning it off takes what is on screen away at once.
    void switchingOffHidesWhatIsShowing()
    {
        QWidget host;
        auto* button = new QWidget(&host);
        button->setGeometry(20, 40, 80, 30);
        host.resize(240, 140);
        Tooltip::install(button, QStringLiteral("Theme"));
        QVERIFY(present(&host));

        Tooltip::showFor(button);
        QCOMPARE(Tooltip::shownFor(), button);
        QToolTip::showText(host.mapToGlobal(QPoint(4, 4)), QStringLiteral("plain"), &host);
        QTRY_VERIFY(QToolTip::isVisible());

        setTooltipsEnabled(false);
        QVERIFY(Tooltip::shownFor() == nullptr);
        QTRY_VERIFY(!QToolTip::isVisible());
        Tooltip::remove(button);
    }

    // A message an editor shows about what was typed is not a hover
    // explanation and is left alone.
    void directShowTextIsNotAHoverAndStays()
    {
        QWidget host;
        host.resize(200, 80);
        QVERIFY(present(&host));

        setTooltipsEnabled(false);
        QToolTip::showText(host.mapToGlobal(QPoint(4, 40)),
                           QStringLiteral("That name is taken"), &host);
        QTRY_VERIFY(QToolTip::isVisible());
    }
};

QTEST_MAIN(TooltipGateTest)
#include "tst_tooltip_gate.moc"
