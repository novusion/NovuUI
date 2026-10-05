// A filled segmented strip divides the SURPLUS, not the content.
//
// The bug this pins: fill divided the tray evenly between the segments
// regardless of how wide each one's label was. Four segments in a narrow rail
// - Work, Project, Code, Assets - were each handed less than their own text
// needed, and a segment paints its label rather than laying it out, so the
// labels ran into each other.

#include "novu/ui/Segmented.h"

#include <QFontMetrics>
#include <QTest>

using namespace novu::ui;

class SegmentedFill : public QObject
{
    Q_OBJECT

private slots:
    void segmentsNeverNarrowerThanTheirText()
    {
        Segmented strip({QStringLiteral("Work"), QStringLiteral("Project"),
                         QStringLiteral("Code"), QStringLiteral("Assets")});
        strip.setFill(true);
        strip.show();

        // The width a rail actually gives it, and narrower still.
        for (int width : {340, 300, 260, 220, 180}) {
            strip.resize(width, strip.sizeHint().height());
            QCoreApplication::processEvents();

            QRect previous;
            for (int i = 0; i < 4; ++i) {
                const QRect box = strip.itemRect(i);
                QVERIFY2(box.width() > 0, "a segment was given no width at all");
                if (!previous.isNull()) {
                    QVERIFY2(box.left() >= previous.right(),
                             qPrintable(QStringLiteral("segments %1 and %2 overlap at %3px")
                                            .arg(i - 1).arg(i).arg(width)));
                }
                previous = box;
            }
        }
    }

    void surplusIsStillShared()
    {
        // Wide enough for every label with room to spare: fill should still
        // stretch the segments to the tray rather than leaving a gap.
        Segmented strip({QStringLiteral("A"), QStringLiteral("B")});
        strip.setFill(true);
        strip.resize(600, strip.sizeHint().height());
        strip.show();
        QCoreApplication::processEvents();

        const QRect first = strip.itemRect(0);
        const QRect second = strip.itemRect(1);
        QVERIFY(first.width() > 100);
        QVERIFY(second.width() > 100);
        QVERIFY2(second.right() > 500, "fill stopped filling");
    }
};

QTEST_MAIN(SegmentedFill)
#include "tst_segmented_fill.moc"
