// A switch that is clicked shows that it was clicked.
//
// The bug this pins, from NovuAI's Share window: clicking a provider switch
// changed the permission - the caption under it said "Shared" and the count
// said "1 allowed" - while the knob stayed drawn on the left. The checked
// state and the drawing had come apart. Each case here clicks the way a
// person does and then asks where the knob is, not only what isChecked() says.

#include "novu/ui/Toggle.h"

#include <QLabel>
#include <QTest>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

class ToggleClickStateTest : public QObject
{
    Q_OBJECT

private slots:
    void aClickMovesTheKnob();
    void aClickInAPaneShownLaterMovesTheKnob();
    void aSecondClickMovesItBack();
    void aSwitchBuiltOnIsDrawnOn();
    void aHandlerThatEditsTheRowDoesNotStopTheKnob();
    void hidingThePaneMidSlideStillEndsOn();

private:
    static void settle() { QTest::qWait(450); }
    static void click(Toggle* toggle)
    {
        QTest::mouseClick(toggle, Qt::LeftButton, {}, toggle->rect().center());
    }
};

void ToggleClickStateTest::aClickMovesTheKnob()
{
    QWidget host;
    auto* column = new QVBoxLayout(&host);
    Toggle* toggle = toggleRow(column, &host, QStringLiteral("Share Codex CLI"),
                               QStringLiteral("This device · Available"), false);
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));
    QCOMPARE(toggle->knobPosition(), 0.0);

    click(toggle);
    settle();
    QVERIFY(toggle->isChecked());
    QCOMPARE(toggle->knobPosition(), 1.0);
}

void ToggleClickStateTest::aClickInAPaneShownLaterMovesTheKnob()
{
    // The Share window builds Local, CLI and API at once and shows one.
    QWidget host;
    auto* column = new QVBoxLayout(&host);
    auto* pane = new QWidget(&host);
    auto* paneLayout = new QVBoxLayout(pane);
    Toggle* toggle = toggleRow(paneLayout, pane, QStringLiteral("Share Claude API"),
                               QStringLiteral("Server · Available"), false);
    column->addWidget(pane);
    pane->setVisible(false);
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));

    pane->setVisible(true);
    QTest::qWait(50);
    click(toggle);
    settle();
    QVERIFY(toggle->isChecked());
    QCOMPARE(toggle->knobPosition(), 1.0);
}

void ToggleClickStateTest::aSecondClickMovesItBack()
{
    QWidget host;
    auto* column = new QVBoxLayout(&host);
    Toggle* toggle = toggleRow(column, &host, QStringLiteral("Share NovuAI Agent"), {}, false);
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));
    click(toggle);
    settle();
    click(toggle);
    settle();
    QVERIFY(!toggle->isChecked());
    QCOMPARE(toggle->knobPosition(), 0.0);
}

void ToggleClickStateTest::aSwitchBuiltOnIsDrawnOn()
{
    QWidget host;
    auto* column = new QVBoxLayout(&host);
    Toggle* toggle = toggleRow(column, &host, QStringLiteral("Share Gemini CLI"), {}, true);
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));
    QCOMPARE(toggle->knobPosition(), 1.0);
}

void ToggleClickStateTest::aHandlerThatEditsTheRowDoesNotStopTheKnob()
{
    // What the Share row does on toggled: rewrite the caption under the switch
    // and the count above it.
    QWidget host;
    auto* column = new QVBoxLayout(&host);
    auto* count = new QLabel(QStringLiteral("none allowed"), &host);
    column->addWidget(count);
    auto* rowHost = new QWidget(&host);
    auto* rowLayout = new QVBoxLayout(rowHost);
    Toggle* toggle = toggleRow(rowLayout, rowHost, QStringLiteral("Share Codex CLI"),
                               QStringLiteral("This device · Available"), false);
    column->addWidget(rowHost);
    QLabel* hint = rowHost->findChildren<QLabel*>().value(1);
    QVERIFY(hint);
    connect(toggle, &Toggle::toggled, toggle, [hint, count](bool on) {
        hint->setText(on ? QStringLiteral("Shared · This device · Available")
                         : QStringLiteral("This device · Available"));
        count->setText(on ? QStringLiteral("1 allowed") : QStringLiteral("none allowed"));
    });
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));

    click(toggle);
    settle();
    QCOMPARE(hint->text(), QStringLiteral("Shared · This device · Available"));
    QCOMPARE(toggle->knobPosition(), 1.0);
}

void ToggleClickStateTest::hidingThePaneMidSlideStillEndsOn()
{
    // Local -> CLI -> Local straight after a click.
    QWidget host;
    auto* column = new QVBoxLayout(&host);
    auto* pane = new QWidget(&host);
    auto* paneLayout = new QVBoxLayout(pane);
    Toggle* toggle = toggleRow(paneLayout, pane, QStringLiteral("Share Codex CLI"), {}, false);
    column->addWidget(pane);
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));

    click(toggle);
    pane->setVisible(false);
    settle();
    pane->setVisible(true);
    QTest::qWait(50);
    QVERIFY(toggle->isChecked());
    QCOMPARE(toggle->knobPosition(), 1.0);
}

QTEST_MAIN(ToggleClickStateTest)
#include "tst_toggle_click_state.moc"
