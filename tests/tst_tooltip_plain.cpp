// A plain setToolTip() string, and an item view's Qt::ToolTipRole, are drawn
// as the Novusion card once adoptPlainToolTips() is on - the OS tooltip is a
// QLabel, which the base stylesheet leaves without a background.

#include "novu/ui/Tooltip.h"

#include <QApplication>
#include <QHelpEvent>
#include <QListWidget>
#include <QPushButton>
#include <QTest>
#include <QVBoxLayout>

using namespace novu::ui;

class TooltipPlainTest : public QObject
{
    Q_OBJECT

private slots:
    void fromTextSplits()
    {
        QCOMPARE(Tooltip::fromText(QStringLiteral("Open Run")).title, QStringLiteral("Open Run"));
        const TipContent lines = Tooltip::fromText(QStringLiteral("Workflow · Agent\nClick to show the run's card"));
        QCOMPARE(lines.title, QStringLiteral("Workflow · Agent"));
        QCOMPARE(lines.brief, QStringLiteral("Click to show the run's card"));
        const TipContent colon = Tooltip::fromText(
            QStringLiteral("Graph: the session's agents, live - who talks, who waits, a handover waiting for you"));
        QCOMPARE(colon.title, QStringLiteral("Graph"));
        QVERIFY(colon.brief.startsWith(QStringLiteral("the session's agents")));
        const TipContent sentence = Tooltip::fromText(
            QStringLiteral("The agent in this chat reads this record, so it can continue the task later on."));
        QVERIFY(sentence.title.isEmpty());
        QVERIFY(!sentence.brief.isEmpty());
        QCOMPARE(Tooltip::fromText(QStringLiteral("<b>Bold</b> name")).title, QStringLiteral("Bold name"));
        QVERIFY(Tooltip::fromText(QStringLiteral("   ")).isEmpty());
    }

    void plainTipBecomesTheCard()
    {
        Tooltip::adoptPlainToolTips();
        Tooltip::adoptPlainToolTips();   // idempotent
        QWidget host;
        auto* column = new QVBoxLayout(&host);
        auto* button = new QPushButton(QStringLiteral("Go"), &host);
        button->setToolTip(QStringLiteral("Show this run's card in the conversation"));
        auto* list = new QListWidget(&host);
        auto* item = new QListWidgetItem(QStringLiteral("row"), list);
        item->setToolTip(QStringLiteral("A row's own tip"));
        column->addWidget(button);
        column->addWidget(list);
        host.resize(400, 300);
        host.show();
        QVERIFY(QTest::qWaitForWindowExposed(&host));

        QHelpEvent onButton(QEvent::ToolTip, button->rect().center(), button->mapToGlobal(button->rect().center()));
        QApplication::sendEvent(button, &onButton);
        QCOMPARE(Tooltip::shownFor(), button);
        QCOMPARE(Tooltip::instance().visibleContent().brief, QStringLiteral("Show this run's card in the conversation"));
        Tooltip::hideTip();

        const QPoint at = list->visualItemRect(item).center();
        QHelpEvent onRow(QEvent::ToolTip, at, list->viewport()->mapToGlobal(at));
        QApplication::sendEvent(list->viewport(), &onRow);
        QCOMPARE(Tooltip::shownFor(), list->viewport());
        QCOMPARE(Tooltip::instance().visibleContent().title, QStringLiteral("A row's own tip"));
        Tooltip::hideTip();

        // an installed card is the installed card, not the plain string
        Tooltip::install(button, TipContent::action(QStringLiteral("Go"), QStringLiteral("Goes.")));
        QApplication::sendEvent(button, &onButton);
        QVERIFY(Tooltip::shownFor() == nullptr || Tooltip::instance().visibleContent().brief == QStringLiteral("Goes."));
        Tooltip::hideTip();
    }
};

QTEST_MAIN(TooltipPlainTest)
#include "tst_tooltip_plain.moc"
