// The same words already shown are not shown twice: a button pressed again
// gives the toast that is there, kept a while longer - another message, or
// the same words in another tone, is a toast of its own.

#include "novu/ui/Toast.h"

#include <QTest>
#include <QWidget>

using namespace novu::ui;

class ToastDedupeTest : public QObject
{
    Q_OBJECT

private slots:
    void sameMessageOnce()
    {
        QWidget host;
        host.resize(600, 400);
        host.show();
        QVERIFY(QTest::qWaitForWindowExposed(&host));
        Toast* first = Toast::post(&host, QStringLiteral("Waits for FX-1 Fix F1"), Toast::Tone::Warning);
        Toast* again = Toast::post(&host, QStringLiteral("Waits for FX-1 Fix F1"), Toast::Tone::Warning);
        QCOMPARE(again, first);
        QCOMPARE(Toast::shownOn(&host).size(), 1);
        Toast::post(&host, QStringLiteral("Waits for FX-1 Fix F1"), Toast::Tone::Info);
        Toast::post(&host, QStringLiteral("Something else"), Toast::Tone::Warning);
        QCOMPARE(Toast::shownOn(&host).size(), 3);
        Toast::dismissAll(&host);
    }
};

QTEST_MAIN(ToastDedupeTest)
#include "tst_toast_dedupe.moc"
