// ConnectionStrip: an elided line must still be readable in full.
//
// The rail is narrow and the strip is where a module reports its connection,
// including why it failed. A transport error elided to "NovuTrade private
// transport is unav..." with no other place to read it is a message nobody can
// act on - which is how "The NovuTrade module is inco..." was read as
// "incompatible". This pins the tooltip: complete host and detail once either
// is cut, nothing when both fit.

#include "novu/ui/Sidebar.h"

#include <QTest>

using namespace novu::ui;

class ConnectionStripTooltipTest : public QObject
{
    Q_OBJECT

private slots:
    void elidedTextIsInTheTooltip()
    {
        ConnectionStrip strip;
        const QString host = QStringLiteral("trade.novuhub.com");
        const QString detail = QStringLiteral(
            "NovuTrade private transport is unavailable: wgtunnel_trade.dll is "
            "missing or incompatible.");
        strip.setHost(host);
        strip.setDetail(detail);
        strip.resize(238, strip.sizeHint().height());
        QVERIFY(strip.toolTip().contains(host));
        QVERIFY(strip.toolTip().contains(detail));
    }

    void noTooltipWhenEverythingFits()
    {
        ConnectionStrip strip;
        strip.setHost(QStringLiteral("imap.novusion.io"));
        strip.setDetail(QStringLiteral("38 ms"));
        strip.resize(600, strip.sizeHint().height());
        QVERIFY(strip.toolTip().isEmpty());
    }

    void followsTheWidth()
    {
        ConnectionStrip strip;
        strip.setHost(QStringLiteral("trade.novuhub.com"));
        strip.setDetail(QStringLiteral("waiting for the tunnel to finish proving connectivity"));
        // Shown: a hidden widget is not sent resize events until it appears,
        // and the rail's strip is always on screen.
        strip.show();
        strip.resize(900, strip.sizeHint().height());
        QVERIFY(strip.toolTip().isEmpty());
        strip.resize(160, strip.sizeHint().height());
        QVERIFY(!strip.toolTip().isEmpty());
        strip.resize(900, strip.sizeHint().height());
        QVERIFY(strip.toolTip().isEmpty());
    }

    void aCompactStripHasNoTextTooltip()
    {
        ConnectionStrip strip;
        strip.setHost(QStringLiteral("a-very-long-host-name.example.novuhub.com"));
        strip.setCompact(true);
        strip.resize(40, 40);
        QVERIFY(strip.toolTip().isEmpty());
    }
};

QTEST_MAIN(ConnectionStripTooltipTest)
#include "tst_connection_strip_tooltip.moc"
