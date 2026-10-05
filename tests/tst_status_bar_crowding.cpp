// The strip's two sets of readings grow towards each other, and nothing used
// to stop them meeting.
//
// Leading entries are placed rightwards from the end of the embedded leading
// widget; trailing entries leftwards from the right edge. Both are PAINTED,
// so neither clips the other: when a leading host grew - a model picker, then
// Load and Unload beside it, then a progress bar - the branch reading and the
// bridge state were simply drawn on top of one another.

#include "novu/ui/StatusBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QTest>

using namespace novu::ui;

namespace {

// A leading host that asks for far more width than any strip will have.
QWidget* greedyHost(QWidget* parent, int width)
{
    auto* host = new QWidget(parent);
    auto* row = new QHBoxLayout(host);
    row->setContentsMargins(0, 0, 0, 0);
    auto* filler = new QLabel(host);
    filler->setFixedWidth(width);
    row->addWidget(filler);
    return host;
}

} // namespace

class StatusBarCrowding : public QObject
{
    Q_OBJECT

private slots:
    void leadingHostIsBoundedByTheStrip()
    {
        StatusBar bar;
        bar.resize(700, 26);
        bar.setLeadingWidget(greedyHost(&bar, 2000));
        bar.show();
        QCoreApplication::processEvents();

        QVERIFY2(bar.findChildren<QWidget*>().size() > 0, "no leading host");
        // Whatever it asked for, it may not run past the strip it is in.
        const QWidget* host = bar.findChild<QLabel*>()->parentWidget();
        QVERIFY2(host->geometry().right() <= bar.width(),
                 "the leading host still takes its full size hint");
    }

    void readingsNeverOverlap()
    {
        StatusBar bar;
        bar.setLeadingWidget(greedyHost(&bar, 420));
        bar.addMono(QStringLiteral("Chat RAM 0 / 2048 MB"), StatusBar::Slot::Leading);
        bar.addSeparator();
        bar.addMono(QStringLiteral("main  3up 1down 3d"), StatusBar::Slot::Leading);
        bar.addText(QStringLiteral("Push"), StatusBar::Slot::Trailing);
        bar.addText(QStringLiteral("Pull"), StatusBar::Slot::Trailing);
        bar.addStatus(QStringLiteral("Core Bridge - Connected"),
                      StatusDot::Tone::Success, StatusBar::Slot::Trailing);
        bar.show();

        for (int width : {1400, 1100, 900, 760, 640, 520, 420}) {
            bar.resize(width, 26);
            QCoreApplication::processEvents();
            // Painting places the entries, so ask for it before measuring.
            QPixmap canvas(bar.size());
            bar.render(&canvas);

            QVector<QRect> placed;
            for (int i = 0; i < bar.itemCount(); ++i) {
                const QRect box = bar.itemGeometry(i);
                if (box.isNull() || box.width() <= 0) {
                    continue;   // dropped for want of room, which is the fix
                }
                placed.append(box);
            }
            for (int a = 0; a < placed.size(); ++a) {
                for (int b = a + 1; b < placed.size(); ++b) {
                    QVERIFY2(!placed.at(a).intersects(placed.at(b)),
                             qPrintable(QStringLiteral("two readings overlap at %1px")
                                            .arg(width)));
                }
            }
        }
    }
};

QTEST_MAIN(StatusBarCrowding)
#include "tst_status_bar_crowding.moc"
