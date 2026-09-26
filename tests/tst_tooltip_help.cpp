// The progressive tip, and the ways out of it.
//
// One hover gives the name; keeping the pointer there adds the long form.
// That second level is only useful if a person knows it is coming, and only
// safe if it goes away reliably - a tip left over another window or over a
// menu is worse than no tip at all. Both halves are checked here.

#include "novu/ui/Tooltip.h"

#include <QKeyEvent>
#include <QLabel>
#include <QTest>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace {

TipContent twoLevelTip()
{
    return TipContent{
        QStringLiteral("Light or dark"),
        QStringLiteral("Ctrl+Shift+L"),
        QStringLiteral("Switches the window between the two."),
        {},
        QStringLiteral("Every surface moves together - the panes, the windows, "
                       "the menus and the console.")};
}

}

class TooltipHelpTest : public QObject
{
    Q_OBJECT

private slots:
    void cleanup() { Tooltip::hideTip(); }

    // The content model is five fields, so a caller never has to build a tip
    // out of newlines.
    void aTipKeepsEveryFieldItWasGiven()
    {
        QWidget host;
        auto* button = new QLabel(QStringLiteral("theme"), &host);
        Tooltip::install(button, twoLevelTip());

        const TipContent stored = Tooltip::contentFor(button);
        QCOMPARE(stored.title, QStringLiteral("Light or dark"));
        QCOMPARE(stored.shortcut, QStringLiteral("Ctrl+Shift+L"));
        QCOMPARE(stored.brief, QStringLiteral("Switches the window between the two."));
        QVERIFY(!stored.detail.isEmpty());
    }

    // The short form arrives first. The long form is not drawn yet, which is
    // the distinction the two levels are made of.
    void theFirstLevelShowsWithoutTheDetail()
    {
        QWidget host;
        auto* button = new QLabel(QStringLiteral("theme"), &host);
        Tooltip::install(button, twoLevelTip());
        host.show();
        QVERIFY(QTest::qWaitForWindowExposed(&host));
        QVERIFY(button->isVisible());
        Tooltip::showFor(button);

        QCOMPARE(Tooltip::shownFor(), button);
        QVERIFY(!Tooltip::instance().detailVisible());
        QCOMPARE(Tooltip::instance().visibleContent().title,
                 QStringLiteral("Light or dark"));
    }

    // Escape dismisses it. Without this a tip could outlive the reason it
    // appeared, with the pointer parked and the keyboard busy elsewhere.
    void escapeDismissesIt()
    {
        QWidget host;
        auto* button = new QLabel(QStringLiteral("theme"), &host);
        Tooltip::install(button, twoLevelTip());
        host.show();
        QVERIFY(QTest::qWaitForWindowExposed(&host));
        QVERIFY(button->isVisible());
        Tooltip::showFor(button);
        QVERIFY(Tooltip::shownFor() != nullptr);

        QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        QApplication::sendEvent(button, &escape);

        QCOMPARE(Tooltip::shownFor(), nullptr);
    }

    // A tip belongs to the window the pointer is in. Losing activation means
    // the person is elsewhere, possibly in another application.
    void losingTheWindowDismissesIt()
    {
        QWidget host;
        auto* button = new QLabel(QStringLiteral("theme"), &host);
        Tooltip::install(button, twoLevelTip());
        host.show();
        QVERIFY(QTest::qWaitForWindowExposed(&host));
        QVERIFY(button->isVisible());
        Tooltip::showFor(button);
        QVERIFY(Tooltip::shownFor() != nullptr);

        QEvent deactivate(QEvent::WindowDeactivate);
        QApplication::sendEvent(button, &deactivate);

        QCOMPARE(Tooltip::shownFor(), nullptr);
    }

    void leavingTheWidgetDismissesIt()
    {
        QWidget host;
        auto* button = new QLabel(QStringLiteral("theme"), &host);
        Tooltip::install(button, twoLevelTip());
        host.show();
        QVERIFY(QTest::qWaitForWindowExposed(&host));
        QVERIFY(button->isVisible());
        Tooltip::showFor(button);
        QVERIFY(Tooltip::shownFor() != nullptr);

        QEvent leave(QEvent::Leave);
        QApplication::sendEvent(button, &leave);

        QCOMPARE(Tooltip::shownFor(), nullptr);
    }

    // Moving quickly across a row of icons must leave nothing behind: the tip
    // is one reused window, so the last target is the only one showing.
    void sweepingAcrossControlsLeavesOneTipAtMost()
    {
        QWidget host;
        auto* layout = new QVBoxLayout(&host);

        QList<QLabel*> buttons;
        for (int i = 0; i < 5; ++i) {
            auto* button = new QLabel(QStringLiteral("b%1").arg(i), &host);
            layout->addWidget(button);
            Tooltip::install(button, TipContent{QStringLiteral("Control %1").arg(i),
                                                {}, QStringLiteral("does a thing"), {}, {}});
            buttons.append(button);
        }

        host.show();
        QVERIFY(QTest::qWaitForWindowExposed(&host));

        for (QLabel* button : buttons) {
            Tooltip::showFor(button);
        }

        // One window, showing the last thing asked for.
        QCOMPARE(Tooltip::shownFor(), buttons.last());
        QCOMPARE(Tooltip::instance().visibleContent().title,
                 QStringLiteral("Control 4"));
    }

    // Removing the tip takes it off the widget, so a control that stops
    // meaning something stops claiming to.
    void removingATipLeavesNothingBehind()
    {
        QWidget host;
        auto* button = new QLabel(QStringLiteral("theme"), &host);
        Tooltip::install(button, twoLevelTip());
        QVERIFY(!Tooltip::contentFor(button).isEmpty());

        Tooltip::remove(button);
        QVERIFY(Tooltip::contentFor(button).isEmpty());
    }

    // Every installed description is listable, which is what lets a help
    // window and a hover read the same sentence instead of two copies.
    void everyDescriptionIsAvailableToAHelpWindow()
    {
        QWidget host;
        auto* button = new QLabel(QStringLiteral("theme"), &host);
        Tooltip::install(button, twoLevelTip());

        bool found = false;
        for (const TipContent& tip : Tooltip::all()) {
            if (tip.title == QStringLiteral("Light or dark")) {
                found = true;
                break;
            }
        }
        QVERIFY(found);
    }
};

QTEST_MAIN(TooltipHelpTest)
#include "tst_tooltip_help.moc"
