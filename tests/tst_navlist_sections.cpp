// NavList prominent captions: larger, with more room and a hairline above, and
// nothing else about the list moves. Off by default, so a rail that did not ask
// keeps its geometry. Also pins the rest of the tree the option has to live
// with: folding at every level, no guides unless asked for, and a deep row that
// still fits a narrow rail.

#include "novu/ui/NavList.h"

#include <QImage>
#include <QPainter>
#include <QSignalSpy>
#include <QTest>

using namespace novu::ui;

namespace {

constexpr int kRailContentWidth = 218;

void populate(NavList& list)
{
    list.clear();
    list.addSection(QStringLiteral("Overview"), QStringLiteral("sec:overview"));
    list.addItem(QStringLiteral("dashboard"), QStringLiteral("Dashboard"), Glyph::Folder);
    list.addSection(QStringLiteral("Catalogue"), QStringLiteral("sec:catalogue"));
    list.addItem(QStringLiteral("apps"), QStringLiteral("Apps"), Glyph::Folder);
    list.setExpandable(QStringLiteral("apps"), true);
    list.setExpanded(QStringLiteral("apps"), true);
    list.addChild(QStringLiteral("app"), QStringLiteral("NovuAI"), Glyph::Folder, 1);
    list.setExpandable(QStringLiteral("app"), true);
    list.setExpanded(QStringLiteral("app"), true);
    list.addChild(QStringLiteral("group"), QStringLiteral("Runtimes"), Glyph::Folder, 2);
    list.setExpandable(QStringLiteral("group"), true);
    list.setExpanded(QStringLiteral("group"), true);
    list.addChild(QStringLiteral("leaf"),
                  QStringLiteral("Stable Diffusion Cuda 13 with a very long name"),
                  Glyph::Folder, 3);
    list.addSection(QStringLiteral("Billing"), QStringLiteral("sec:billing"));
    list.addItem(QStringLiteral("plans"), QStringLiteral("Plans"), Glyph::Folder);
}

QImage render(NavList& list)
{
    list.resize(kRailContentWidth, list.sizeHint().height());
    QImage image(list.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    // Without the window background: what is left is what the list drew.
    list.render(&image, QPoint(), QRegion(), QWidget::DrawChildren);
    return image;
}

// How many pixels of a row band have anything painted on them in `column`.
int paintedIn(const QImage& image, int column, int top, int bottom)
{
    int painted = 0;
    for (int y = top; y < bottom; ++y) {
        painted += qAlpha(image.pixel(column, y)) > 0 ? 1 : 0;
    }
    return painted;
}

} // namespace

class NavListSectionsTest : public QObject
{
    Q_OBJECT

private slots:
    void offByDefault_keepsTheGeometry();
    void prominent_addsRoomOnlyAboveCaptionsWithSomethingOverThem();
    void prominent_leavesRowsAndTheirHorizontalPositionsAlone();
    void prominent_drawsAHairlineBetweenSections();
    void withoutGuides_nothingIsDrawnBesideADeepRow();
    void foldingWorksAtEveryLevel();
    void aDeepRowFitsARailAndItsIndentIsRespected();
};

void NavListSectionsTest::offByDefault_keepsTheGeometry()
{
    NavList plain;
    populate(plain);
    NavList toggled;
    populate(toggled);
    toggled.setProminentSections(true);
    toggled.setProminentSections(false);

    QCOMPARE(toggled.sizeHint(), plain.sizeHint());
    plain.resize(kRailContentWidth, plain.sizeHint().height());
    toggled.resize(kRailContentWidth, toggled.sizeHint().height());
    for (int i = 0; i < plain.count(); ++i) {
        QCOMPARE(toggled.rowRect(i), plain.rowRect(i));
    }
}

void NavListSectionsTest::prominent_addsRoomOnlyAboveCaptionsWithSomethingOverThem()
{
    NavList plain;
    populate(plain);
    NavList strong;
    populate(strong);
    strong.setProminentSections(true);
    plain.resize(kRailContentWidth, plain.sizeHint().height());
    strong.resize(kRailContentWidth, strong.sizeHint().height());

    const int first = strong.indexOfSection(QStringLiteral("sec:overview"));
    const int second = strong.indexOfSection(QStringLiteral("sec:catalogue"));

    const int firstGrowth = strong.rowRect(first).height() - plain.rowRect(first).height();
    const int secondGrowth = strong.rowRect(second).height() - plain.rowRect(second).height();
    QVERIFY(firstGrowth >= 0);
    // The same larger type in both, plus the gap only where there is a section
    // above to stand apart from.
    QCOMPARE(secondGrowth - firstGrowth, 4);
}

void NavListSectionsTest::prominent_leavesRowsAndTheirHorizontalPositionsAlone()
{
    NavList plain;
    populate(plain);
    NavList strong;
    populate(strong);
    strong.setProminentSections(true);
    plain.resize(kRailContentWidth, plain.sizeHint().height());
    strong.resize(kRailContentWidth, strong.sizeHint().height());

    for (const QString& key : plain.keys()) {
        const int index = plain.indexOfKey(key);
        QCOMPARE(strong.rowRect(index).height(), plain.rowRect(index).height());
        QCOMPARE(strong.twistyRect(index).left(), plain.twistyRect(index).left());
    }
}

void NavListSectionsTest::prominent_drawsAHairlineBetweenSections()
{
    NavList plain;
    populate(plain);
    NavList strong;
    populate(strong);
    strong.setProminentSections(true);

    // Near the right edge of a caption's gap: past every caption's text, so
    // the only thing that can be there is a rule across the rail.
    const int column = kRailContentWidth - 20;
    const auto gapOf = [](const NavList& list, const QString& section) {
        const int index = list.indexOfSection(section);
        return list.rowRect(index);
    };

    const QImage strongImage = render(strong);
    const QImage plainImage = render(plain);

    const QRect second = gapOf(strong, QStringLiteral("sec:catalogue"));
    QVERIFY(paintedIn(strongImage, column, second.top(), second.bottom()) > 0);

    const QRect secondPlain = gapOf(plain, QStringLiteral("sec:catalogue"));
    QCOMPARE(paintedIn(plainImage, column, secondPlain.top(), secondPlain.bottom()), 0);

    // Nothing over the first caption to divide it from.
    const QRect first = gapOf(strong, QStringLiteral("sec:overview"));
    QCOMPARE(paintedIn(strongImage, column, first.top(), first.bottom()), 0);
}

void NavListSectionsTest::withoutGuides_nothingIsDrawnBesideADeepRow()
{
    NavList list;
    populate(list);
    list.setProminentSections(true);
    const QImage bare = render(list);
    list.setTreeGuides(true);
    const QImage guided = render(list);

    const int leaf = list.indexOfKey(QStringLiteral("leaf"));
    const QRect box = list.rowRect(leaf);
    const int firstInk = list.twistyRect(list.indexOfKey(QStringLiteral("group"))).left();

    // Everything left of the deepest row's twisty slot is indent. With guides
    // it holds a rule, which is what proves this looks in the right place.
    int bareInk = 0;
    int guidedInk = 0;
    for (int x = 0; x < firstInk; ++x) {
        bareInk += paintedIn(bare, x, box.top(), box.bottom());
        guidedInk += paintedIn(guided, x, box.top(), box.bottom());
    }
    QCOMPARE(bareInk, 0);
    QVERIFY(guidedInk > 0);
}

void NavListSectionsTest::foldingWorksAtEveryLevel()
{
    NavList list;
    populate(list);
    list.setProminentSections(true);
    list.resize(kRailContentWidth, list.sizeHint().height());
    QSignalSpy toggled(&list, &NavList::expandToggled);

    // A row's twisty folds what is under it and nothing else.
    const int group = list.indexOfKey(QStringLiteral("group"));
    QTest::mouseClick(&list, Qt::LeftButton, Qt::NoModifier, list.twistyRect(group).center());
    QCOMPARE(toggled.count(), 1);
    QVERIFY(!list.isExpanded(QStringLiteral("group")));
    QVERIFY(!list.isRowVisible(QStringLiteral("leaf")));
    QVERIFY(list.isRowVisible(QStringLiteral("group")));

    // Apps shut hides the whole branch; reopening restores the shut group.
    list.resize(kRailContentWidth, list.sizeHint().height());
    const int apps = list.indexOfKey(QStringLiteral("apps"));
    QTest::mouseClick(&list, Qt::LeftButton, Qt::NoModifier, list.twistyRect(apps).center());
    QVERIFY(!list.isRowVisible(QStringLiteral("app")));
    list.resize(kRailContentWidth, list.sizeHint().height());
    QTest::mouseClick(&list, Qt::LeftButton, Qt::NoModifier, list.twistyRect(apps).center());
    QVERIFY(list.isRowVisible(QStringLiteral("group")));
    QVERIFY(!list.isRowVisible(QStringLiteral("leaf")));

    // A click on the whole caption folds its section, at its new height.
    list.resize(kRailContentWidth, list.sizeHint().height());
    const int section = list.indexOfSection(QStringLiteral("sec:catalogue"));
    QTest::mouseClick(&list, Qt::LeftButton, Qt::NoModifier, list.rowRect(section).center());
    QVERIFY(!list.isExpanded(QStringLiteral("sec:catalogue")));
    QVERIFY(!list.isRowVisible(QStringLiteral("apps")));
    QVERIFY(list.isRowVisible(QStringLiteral("plans")));

    // And a section's fold survives the rail being rebuilt.
    populate(list);
    QVERIFY(!list.isExpanded(QStringLiteral("sec:catalogue")));
    QVERIFY(!list.isRowVisible(QStringLiteral("apps")));
}

void NavListSectionsTest::aDeepRowFitsARailAndItsIndentIsRespected()
{
    NavList list;
    populate(list);
    list.setProminentSections(true);
    list.setIndentOffset(QStringLiteral("app"), 4);
    list.setIndentOffset(QStringLiteral("group"), 8);
    list.setIndentOffset(QStringLiteral("leaf"), 12);
    list.setCount(QStringLiteral("leaf"), 1234);

    // The scroll area hosting a rail follows the widest row's floor, and its
    // horizontal scrollbar is off: a floor over the rail's width is cut off.
    QVERIFY2(list.minimumSizeHint().width() <= kRailContentWidth,
             qPrintable(QStringLiteral("deepest row needs %1 px")
                            .arg(list.minimumSizeHint().width())));

    list.resize(kRailContentWidth, list.sizeHint().height());
    int previous = -1;
    for (const char* key : {"apps", "app", "group"}) {
        const int left = list.twistyRect(list.indexOfKey(QString::fromLatin1(key))).left();
        QVERIFY2(left - previous >= 12 || previous < 0, key);
        previous = left;
    }
    QCOMPARE(list.twistyRect(list.indexOfKey(QStringLiteral("group"))).left()
                 - list.twistyRect(list.indexOfKey(QStringLiteral("app"))).left(),
             16);
}

QTEST_MAIN(NavListSectionsTest)
#include "tst_navlist_sections.moc"
