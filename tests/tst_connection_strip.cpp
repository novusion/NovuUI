// ConnectionStrip: nothing may be painted to the left of the status dot.
//
// The bug this pins: the detail text was drawn at its natural width at
// `rightEdge - width`, with nothing clamping it to the space available. A detail
// longer than the strip - "server replied: Not Found" on a 238px rail - started
// at a negative x, so its tail was painted over and to the left of the dot while
// its head was clipped by the widget edge. What the eye saw was a cut-off word
// fragment before the status light. The host then got a width of zero and
// disappeared, which is why the strip showed a transport error where the
// hostname belongs.
//
// It is a shared component: Trade, Web, Mail and Vault all draw the same strip,
// so the same wrong detail would have done the same thing in each.
//
// The checks render the widget and read pixels, because the defect was in what
// was painted rather than in what was stored. Every case runs at several widths,
// with and without the action button, at 1x and 2x, because the failure only
// appeared once the text no longer fitted.

#include "novu/ui/Sidebar.h"
#include "novu/ui/Theme.h"

#include <QImage>
#include <QPainter>
#include <QTest>

using namespace novu::ui;

namespace {

// The dot sits kConnPaddingH (10) from the left and is kConnDot (7) across, so
// everything before x = 10 belongs to the strip's own padding and must stay the
// strip's own background. A glyph or a letter there is the bug.
constexpr int kDotLeft = 10;

QImage renderStrip(ConnectionStrip* strip, int width, qreal dpr)
{
    strip->resize(width, strip->sizeHint().height());
    QImage image(strip->size() * dpr, QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(dpr);
    image.fill(Qt::transparent);
    strip->render(&image);
    return image;
}

// The colours the strip legitimately paints in its left padding: nothing, or the
// well's own background and border. Text is a different thing, and the way to
// tell is that text is drawn in a foreground ink with high alpha over a band
// that should be uniform.
//
// Rather than guess inks, the check compares the padding band against the same
// band rendered with an empty host and detail - the known-good background. Any
// pixel that differs is something that was painted where nothing should be.
int paddingPixelsDiffering(const QImage& actual, const QImage& blank, qreal dpr)
{
    const int band = static_cast<int>(kDotLeft * dpr);
    const int height = qMin(actual.height(), blank.height());
    int differing = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < qMin(band, actual.width()); ++x) {
            if (actual.pixel(x, y) != blank.pixel(x, y)) {
                ++differing;
            }
        }
    }
    return differing;
}

} // namespace

class ConnectionStripTest : public QObject
{
    Q_OBJECT

private slots:
    void nothingIsPaintedLeftOfTheDot_data();
    void nothingIsPaintedLeftOfTheDot();
    void theHostSurvivesALongDetail();
    void theDetailNeverEatsTheWholeBand();
    void theCompactStripDrawsOnlyTheDot();
};

void ConnectionStripTest::nothingIsPaintedLeftOfTheDot_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<QString>("detail");
    QTest::addColumn<bool>("action");
    QTest::addColumn<int>("width");
    QTest::addColumn<qreal>("dpr");

    // The exact pair that produced the fragment.
    const QString longDetail = QStringLiteral("server replied: Not Found");
    const QString host = QStringLiteral("trade.novuhub.com");

    for (qreal dpr : {qreal(1.0), qreal(2.0)}) {
        for (int width : {120, 180, 238, 320}) {
            const QString tag = QStringLiteral("w%1@%2x").arg(width).arg(dpr);
            QTest::newRow(qPrintable(tag + QStringLiteral(" long detail, action"))
                          ) << host << longDetail << true << width << dpr;
            QTest::newRow(qPrintable(tag + QStringLiteral(" long detail, no action"))
                          ) << host << longDetail << false << width << dpr;
            QTest::newRow(qPrintable(tag + QStringLiteral(" long host"))
                          ) << QStringLiteral("a-very-long-hostname.example.invalid")
                            << QStringLiteral("42 ms") << true << width << dpr;
            QTest::newRow(qPrintable(tag + QStringLiteral(" both long"))
                          ) << QStringLiteral("a-very-long-hostname.example.invalid")
                            << longDetail << true << width << dpr;
            QTest::newRow(qPrintable(tag + QStringLiteral(" signed out")))
                << QStringLiteral("Not signed in")
                << QStringLiteral("sign in to continue") << true << width << dpr;
        }
    }
}

void ConnectionStripTest::nothingIsPaintedLeftOfTheDot()
{
    QFETCH(QString, host);
    QFETCH(QString, detail);
    QFETCH(bool, action);
    QFETCH(int, width);
    QFETCH(qreal, dpr);

    ConnectionStrip strip;
    strip.setState(ConnectionStrip::State::Degraded);
    if (action) {
        strip.setAction(Glyph::Key, QStringLiteral("Sign in"));
    }

    // The same strip with no text: whatever it paints in the left padding is the
    // background, the border and the dot, and is allowed.
    strip.setHost(QString());
    strip.setDetail(QString());
    const QImage blank = renderStrip(&strip, width, dpr);

    strip.setHost(host);
    strip.setDetail(detail);
    const QImage actual = renderStrip(&strip, width, dpr);

    const int differing = paddingPixelsDiffering(actual, blank, dpr);
    QVERIFY2(differing == 0,
             qPrintable(QStringLiteral("%1 pixel(s) painted left of the status dot "
                                      "at width %2, dpr %3")
                            .arg(differing).arg(width).arg(dpr)));
}

// The host is the identity. A long detail may be shortened, but it may not take
// the host's place - that is how a transport error came to be displayed where
// the hostname belongs.
void ConnectionStripTest::theHostSurvivesALongDetail()
{
    ConnectionStrip strip;
    strip.setAction(Glyph::Key, QStringLiteral("Sign in"));
    strip.setHost(QStringLiteral("trade.novuhub.com"));

    strip.setDetail(QString());
    const QImage withoutDetail = renderStrip(&strip, 238, 1.0);

    strip.setDetail(QStringLiteral("server replied: Not Found"));
    const QImage withDetail = renderStrip(&strip, 238, 1.0);

    // The host band is the region just right of the dot. Something has to be
    // drawn there in both renders; if the long detail had erased the host, the
    // band would have gone back to being uniform background.
    const int bandLeft = kDotLeft + 7 + 8;
    const int bandRight = qMin(withDetail.width(), bandLeft + 40);
    const auto hasInk = [&](const QImage& image) {
        QRgb first = image.pixel(bandLeft, image.height() / 2);
        for (int x = bandLeft; x < bandRight; ++x) {
            for (int y = 0; y < image.height(); ++y) {
                if (image.pixel(x, y) != first) {
                    return true;
                }
            }
        }
        return false;
    };
    QVERIFY(hasInk(withoutDetail));
    QVERIFY2(hasInk(withDetail),
             "the host disappeared once a long detail was set");
}

// A detail is an aside. It may take at most half the band, so that there is
// always somewhere for the host to go.
void ConnectionStripTest::theDetailNeverEatsTheWholeBand()
{
    ConnectionStrip strip;
    strip.setHost(QStringLiteral("trade.novuhub.com"));
    strip.setDetail(QStringLiteral(
        "an extremely long detail string that could never fit in a sidebar strip"));
    const QImage image = renderStrip(&strip, 238, 1.0);

    // Nothing left of the dot, whatever the length.
    strip.setHost(QString());
    strip.setDetail(QString());
    const QImage blank = renderStrip(&strip, 238, 1.0);
    QCOMPARE(paddingPixelsDiffering(image, blank, 1.0), 0);
}

// Collapsed rail: the dot is centred and neither text is drawn at all.
void ConnectionStripTest::theCompactStripDrawsOnlyTheDot()
{
    ConnectionStrip strip;
    strip.setHost(QStringLiteral("trade.novuhub.com"));
    strip.setDetail(QStringLiteral("server replied: Not Found"));
    strip.setAction(Glyph::Key, QStringLiteral("Sign in"));
    strip.setCompact(true);

    const QImage withText = renderStrip(&strip, 48, 1.0);
    strip.setHost(QString());
    strip.setDetail(QString());
    const QImage withoutText = renderStrip(&strip, 48, 1.0);

    // Identical: a compact strip paints the dot and stops, so the text cannot
    // leak out of the collapsed rail.
    QCOMPARE(withText, withoutText);
}

QTEST_MAIN(ConnectionStripTest)
#include "tst_connection_strip.moc"
