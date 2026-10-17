#include "novu/ui/Card.h"
#include "novu/ui/Badge.h"
#include "novu/ui/Button.h"
#include "novu/ui/Splitter.h"
#include "novu/ui/Segmented.h"
#include "novu/ui/NavList.h"
#include "novu/ui/TreeView.h"
#include "novu/ui/Keys.h"
#include "novu/ui/CommandPalette.h"
#include "novu/ui/IconButton.h"
#include "novu/ui/Chip.h"
#include <QShortcut>
#include <QScrollArea>
#include <QScrollBar>
#include <QPainterPath>
#include "novu/ui/Feedback.h"
#include "novu/ui/Fields.h"
#include "novu/ui/Popover.h"
#include "novu/ui/ThemeManager.h"
#include <QPushButton>
#include <QGraphicsOpacityEffect>
#include <QMouseEvent>
#include "novu/ui/Text.h"
#include "novu/ui/ThemeScope.h"
#include <QLabel>
#include <QLineEdit>
#include "novu/ui/StyleSheet.h"
#include <QSignalSpy>
#include <QTest>
#include <QVBoxLayout>
using novu::ui::Card;
class CardInteractionTest : public QObject {
    Q_OBJECT
private slots:
    void compactTagsAndLibraryRailOptionsStayExplicit(){
        using namespace novu::ui;
        Badge tag("indexed",Badge::Tone::Success);QCOMPARE(tag.sizeHint().height(),19);
        Badge header("running",Badge::Tone::Info);header.setSize(Badge::Size::Large);QVERIFY(header.sizeHint().height()>tag.sizeHint().height());
        NavList nav;nav.setPlainCounts(true);nav.setAccentCurrentGlyph(true);QVERIFY(nav.hasPlainCounts());QVERIFY(nav.accentsCurrentGlyph());
        Segmented counts({"Tasks  8","Changes  3"});QCOMPARE(counts.segmentText(0),QString("Tasks  8"));QVERIFY(counts.sizeHint().width()>0);
    }
    void glyphSetsStayScopedAndNormalizeTheirViewBox(){
        using namespace novu::ui;const auto base=glyphPixmap(Glyph::Book,24,Qt::white).toImage();
        registerGlyphSet("test-doc-icons",{{Glyph::Book,"book","<rect x=\"2\" y=\"2\" width=\"12\" height=\"12\"/>"},{Glyph::None,"triangle","<polygon points=\"2,2 14,8 2,14\"/>"}},16,1.7);
        QWidget host;auto p=novusionBaseProfile();p.id="doc-icon-profile";p.icons.family="test-doc-icons";ThemeManager::instance().registerProfile(p);applyStyleSheet(&host,p.id);
        QCOMPARE(glyphPath(Glyph::Book,&host).boundingRect(),QRectF(3,3,18,18));QCOMPARE(allGlyphs(&host).size(),2);
        const auto triangle=glyphFromName("triangle",&host);QVERIFY(triangle!=Glyph::None);QCOMPARE(glyphPath(triangle,&host).boundingRect(),QRectF(3,3,18,18));
        QVERIFY(glyphPixmap(Glyph::Book,24,Qt::white,1,0,&host).toImage()!=base);QCOMPARE(glyphPixmap(Glyph::Book,24,Qt::white).toImage(),base);
    }

    void accentButtonAndEmptyStateMeasureAreExplicit(){
        using namespace novu::ui;Button accent("Continue",Button::Variant::Accent);const auto spec=accent.styleSpec();
        QCOMPARE(spec.background.describe(),QString("accent@8"));QCOMPARE(spec.foreground.describe(),QString("accent"));QCOMPARE(spec.border.describe(),QString("accent@50"));QCOMPARE(spec.hoverBackground.describe(),QString("accent@14"));
        EmptyState empty(Glyph::Chat,"Nothing here","A wrapped explanation");empty.setTextMaximumWidth(330);QCOMPARE(empty.textMaximumWidth(),330);
    }

    void controlFamiliesKeepIndependentHeights(){
        using namespace novu::ui;QWidget host;auto p=novusionBaseProfile();p.id="family-metrics";p.controls.height=28;p.controls.heightSmall=24;p.controls.selectHeight=26;p.controls.selectSmallHeight=23;p.controls.iconHeight=26;p.controls.iconSmallHeight=22;ThemeManager::instance().registerProfile(p);applyStyleSheet(&host,p.id);
        SelectButton select(&host);select.addOption("a","A");QCOMPARE(select.height(),26);QCOMPARE(select.sizeHint().height(),26);select.setControlSize(ControlSize::Small);QCOMPARE(select.height(),23);
        IconButton icon(Glyph::Star,&host);QCOMPARE(icon.sizeHint().height(),26);icon.setControlSize(ControlSize::Small);QCOMPARE(icon.sizeHint().height(),22);
        auto* action=rowAction(Glyph::Plus,&host);QCOMPARE(action->sizeHint().height(),24);
        Button ordinary("Action",Button::Variant::Outlined,&host);QCOMPARE(ordinary.sizeHint().height(),28);
    }

    void scrollThumbFollowsOwnerHoverWithAnInset(){
        using namespace novu::ui;QWidget host;auto p=novusionBaseProfile();p.id="hover-scroll-test";p.controls.scrollThumbOnAreaHover=true;p.controls.scrollThumbInset=3;ThemeManager::instance().registerProfile(p);applyStyleSheet(&host,p.id);
        host.resize(220,160);QScrollArea area(&host);area.setGeometry(0,0,180,120);auto* content=new QWidget;content->setFixedSize(140,500);area.setWidget(content);host.show();QCoreApplication::processEvents();
        auto* bar=area.verticalScrollBar();QTest::mouseMove(&host,QPoint(210,150));QCoreApplication::processEvents();QVERIFY(!bar->property("novuScrollHovered").toBool());
        const auto resting=bar->grab().toImage();QTest::mouseMove(area.viewport(),QPoint(30,30));QCoreApplication::processEvents();QVERIFY(bar->property("novuScrollHovered").toBool());
        const auto hovered=bar->grab().toImage();QCOMPARE(hovered.pixelColor(5,10),p.colors.borderHover);QCOMPARE(hovered.pixelColor(1,10),resting.pixelColor(1,10));
        QTest::mouseMove(&host,QPoint(210,150));QCoreApplication::processEvents();QVERIFY(!bar->property("novuScrollHovered").toBool());
    }

    void annotationAndFavouriteHaveIndependentSurfaces(){
        using namespace novu::ui;QWidget host;auto profile=novusionBaseProfile();profile.id="annotation-regression";ThemeManager::instance().registerProfile(profile);applyStyleSheet(&host,profile.id);
        IconButton star(Glyph::Star,IconButton::Variant::Ghost,&host);star.setCheckable(true);star.setChecked(true);star.setFixedSize(24,24);
        star.setCheckedGlyphStyle(statusToken(SemanticRole::Warning,ColorRole::StatusText));star.show();
        QCOMPARE(star.grab().toImage().pixelColor(12,12),profile.semantic.warning.text);
        Card note(&host);note.setHeaderVisible(false);note.setBorderVisible(false);note.setLeadingAccent(token(ColorRole::Accent),2);note.resize(100,60);note.show();
        QCOMPARE(note.grab().toImage().pixelColor(1,30),profile.colors.accent);
        Chip tag("workspace",&host,false);tag.setTagStyle(true);QVERIFY(!tag.isCheckable());QCOMPARE(tag.height(),22);
        QSignalSpy clicked(&tag,&Chip::clicked);tag.show();QTest::mouseClick(&tag,Qt::LeftButton);QCOMPARE(clicked.count(),1);
    }

    void selectShowsTheLabelInsteadOfItsKey(){
        novu::ui::SelectButton field;field.addOption("all","All articles");field.addOption("guides","Guides");
        QCOMPARE(field.currentKey(),QString("all"));QCOMPARE(field.currentText(),QString("All articles"));
        const int bareWidth=field.sizeHint().width();field.setMetaText("123");QVERIFY(field.sizeHint().width()>bareWidth);QCOMPARE(field.metaText(),QString("123"));
        field.setCurrentKey("guides");QCOMPARE(field.currentText(),QString("Guides"));
        field.clearOptions();field.addOption("all","Everything");QCOMPARE(field.currentText(),QString("Everything"));
    }
    void selectClearsOpenStateWheneverItsPopoverCloses(){
        using namespace novu::ui;
        QWidget host;host.resize(320,180);SelectButton field(&host);field.setOptions({"One","Two"});field.move(20,20);
        host.show();QCoreApplication::processEvents();
        QTest::mouseClick(&field,Qt::LeftButton);QCoreApplication::processEvents();
        QVERIFY(field.isOpen());QVERIFY(field.menu()->isVisible());
        field.menu()->hide();QCoreApplication::processEvents();
        QVERIFY(!field.menu()->isVisible());QVERIFY(!field.isOpen());
        QTest::mouseClick(&field,Qt::LeftButton);QCoreApplication::processEvents();
        QVERIFY(field.isOpen());
        QTest::keyClick(field.menu(),Qt::Key_Escape);QCoreApplication::processEvents();
        QVERIFY(!field.isOpen());
    }
    void enablingWrapAfterRoleLineHeightReleasesTheOneLineMaximum(){
        using namespace novu::ui;QLabel prose("A sentence long enough to wrap over several lines at a narrow measure.");
        QFont font=prose.font();font.setPixelSize(12);prose.setFont(font);setLineHeight(&prose,1.55);
        const int oneLineMaximum=prose.maximumHeight();QVERIFY(oneLineMaximum<QWIDGETSIZE_MAX);
        prose.setWordWrap(true);prose.setMaximumWidth(120);setLineHeight(&prose,1.55);
        QCOMPARE(prose.maximumHeight(),QWIDGETSIZE_MAX);QVERIFY(prose.sizeHint().height()>oneLineMaximum);
    }

    void richProseRetainsMarkupAndPlainTextIsEscaped(){
        using namespace novu::ui;QWidget host;auto* prose=paragraph("",&host);
        const QString html="<b>Heading</b><ul><li>One</li><li>Two</li></ul>";
        setLabelRichText(prose,html);setLineHeight(prose,1.65);
        QVERIFY(prose->text().contains("<li>One</li>"));QVERIFY(!prose->text().contains("&lt;li&gt;"));
        auto font=prose->font();font.setPointSizeF(12.5);prose->setFont(font);QCoreApplication::processEvents();
        QVERIFY(prose->text().contains("<b>Heading</b>"));QCOMPARE(labelText(prose),html);
        setLabelText(prose,"<b>literal</b>");QVERIFY(prose->text().contains("&lt;b&gt;literal&lt;/b&gt;"));
    }

    void commandHintDoesNotCompeteWithApplicationBinding(){
        using namespace novu::ui;QWidget host;CommandField field("Search",&host);field.setShortcutHint(QKeySequence("Ctrl+K"));
        QShortcut shortcut(QKeySequence("Ctrl+K"),&host);QSignalSpy invoked(&shortcut,&QShortcut::activated);QSignalSpy clicked(&field,&CommandField::clicked);
        host.show();host.activateWindow();QCoreApplication::processEvents();QVERIFY(field.shortcut().isEmpty());QVERIFY(field.findChild<KeyCap*>());
        QTest::keyClick(&host,Qt::Key_K,Qt::ControlModifier);QCOMPARE(invoked.count(),1);QCOMPARE(clicked.count(),0);
        QTest::mouseClick(&field,Qt::LeftButton);QCOMPARE(clicked.count(),1);
    }

    void previewPrimitiveOverridesRemainIndependent(){
        using namespace novu::ui;QWidget host;auto profile=novusionBaseProfile();profile.id="input-contract";
        profile.colors.inputBorder=profile.colors.borderHover;profile.colors.inputInk=profile.colors.textSecondary;profile.colors.inputFocusBg=profile.colors.panelBgAlt;profile.colors.inputHoverBorder=profile.colors.textDisabled;
        ThemeManager::instance().registerProfile(profile);applyStyleSheet(&host,profile.id);host.resize(200,100);host.setFocusPolicy(Qt::StrongFocus);host.show();
        auto* input=lineEdit("Search",&host);input->setGeometry(10,10,150,28);input->show();host.setFocus();QTest::mouseMove(&host,QPoint(190,90));QCoreApplication::processEvents();QCOMPARE(input->grab().toImage().pixelColor(75,0),profile.colors.inputBorder);QCOMPARE(input->palette().color(QPalette::Text),profile.colors.inputInk);
        ProgressBar bar(50,&host);bar.setTrackMetrics(5,3);bar.setTrackBackground(token(ColorRole::BorderHover));bar.resize(90,5);bar.show();QCOMPARE(bar.sizeHint().height(),5);QCOMPARE(bar.resolvedStyle().background,profile.colors.borderHover);QCOMPARE(bar.grab().toImage().pixelColor(70,2),profile.colors.borderHover);
        Segmented seg({"One","Two"},&host);seg.setCurrentIndex(-1);QCOMPARE(seg.currentIndex(),-1);seg.setCurrentIndex(1);QCOMPARE(seg.currentIndex(),1);
        ElidedLabel filename("/a/long/path/ComposerPanel.cpp",&host);filename.setElideMode(Qt::ElideLeft);QCOMPARE(filename.elideMode(),Qt::ElideLeft);QCOMPARE(filename.text(),QString("/a/long/path/ComposerPanel.cpp"));
    }

    void nestedMenusOpenNavigateAndCloseAsAChain(){
        using namespace novu::ui;QWidget host;host.show();Popover menu(&host);auto* sub=menu.addSubmenu("container","Containers");sub->addItem("row","Row");sub->addItem("col","Column");QSignalSpy chosen(&menu,&Popover::triggered);
        menu.popupAt(QPoint(100,100));QTest::keyClick(&menu,Qt::Key_Down);QTest::keyClick(&menu,Qt::Key_Right);QVERIFY(menu.isVisible());QVERIFY(sub->isVisible());
        QTest::keyClick(sub,Qt::Key_Left);QVERIFY(menu.isVisible());QVERIFY(!sub->isVisible());
        QTest::keyClick(&menu,Qt::Key_Right);QTest::keyClick(sub,Qt::Key_Down);QTest::keyClick(sub,Qt::Key_Return);QCOMPARE(chosen.count(),1);QCOMPARE(chosen.first().first().toString(),QString("col"));QVERIFY(!menu.isVisible());QVERIFY(!sub->isVisible());
        menu.popupAt(QPoint(100,100));QTest::mouseMove(&menu,menu.itemRect(0).center());QVERIFY(sub->isVisible());QTest::keyClick(sub,Qt::Key_Escape);QVERIFY(!menu.isVisible());QVERIFY(!sub->isVisible());
    }

    void parentMenuStaysUsableWhileASubmenuIsOpen(){
        // The open submenu holds the mouse grab: moves over the parent arrive at the
        // submenu and must still reach the parent's other items.
        // Events are sent directly: QTest::mouseMove depends on where an earlier test left the cursor.
        using namespace novu::ui;QWidget host;host.show();Popover menu(&host);auto* sub=menu.addSubmenu("moveto","Move To");sub->addItem("ws","Workspace");menu.addItem("delete","Delete");QSignalSpy chosen(&menu,&Popover::triggered);
        const auto send=[](QWidget* to,QEvent::Type type,const QPoint& global,Qt::MouseButton button){QMouseEvent event(type,QPointF(to->mapFromGlobal(global)),QPointF(global),button,type==QEvent::MouseMove?Qt::NoButton:button,Qt::NoModifier);QApplication::sendEvent(to,&event);};
        menu.popupAt(QPoint(100,100));const QPoint same=menu.mapToGlobal(menu.itemRect(0).center()),other=menu.mapToGlobal(menu.itemRect(1).center());
        send(&menu,QEvent::MouseMove,same,Qt::NoButton);QVERIFY(sub->isVisible());
        send(sub,QEvent::MouseMove,other,Qt::NoButton);QVERIFY(!sub->isVisible());QVERIFY(menu.isVisible());
        send(&menu,QEvent::MouseButtonPress,other,Qt::LeftButton);send(&menu,QEvent::MouseButtonRelease,other,Qt::LeftButton);
        QCOMPARE(chosen.count(),1);QCOMPARE(chosen.first().first().toString(),QString("delete"));
        // A press on the item that opened the submenu leaves both open.
        menu.popupAt(QPoint(100,100));send(&menu,QEvent::MouseMove,same,Qt::NoButton);QVERIFY(sub->isVisible());
        QMouseEvent press(QEvent::MouseButtonPress,QPointF(sub->mapFromGlobal(same)),QPointF(same),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(sub,&press);
        QVERIFY(menu.isVisible());QVERIFY(sub->isVisible());
    }

    void profileControlsButtonPaddingAndDisabledSurface(){
        using namespace novu::ui;auto profile=novusionBaseProfile();profile.id="button-contract-test";
        QWidget host;ThemeManager::instance().registerProfile(profile);ThemeScope::apply(&host,profile.id);
        Button button("Send",Button::Variant::Primary,&host);button.setControlSize(ControlSize::Small);const int initial=button.sizeHint().width();
        profile.controls.buttonPaddingSmall+=3;profile.controls.buttonPaddingIncludesBorder=false;profile.controls.buttonDisabledOpacity=.42;
        ThemeManager::instance().registerProfile(profile);ThemeScope::apply(&host,profile.id);
        QCOMPARE(button.sizeHint().width(),initial+6+2*profile.border(BorderRole::Input));
        button.setEnabled(false);auto* effect=qobject_cast<QGraphicsOpacityEffect*>(button.graphicsEffect());QVERIFY(effect);QCOMPARE(effect->opacity(),.42);
        QVERIFY(!button.styleSheet().contains("background: transparent;"));
        button.setEnabled(true);QCOMPARE(effect->opacity(),1.0);
    }

    void thinSplitterTracksTheWholePointerDelta(){
        using namespace novu::ui;Splitter split(Qt::Horizontal);split.setHandleWidth(1);split.setCollapseArrows(false);split.addWidget(new QWidget);split.addWidget(new QWidget);split.resize(600,200);split.show();QCoreApplication::processEvents();split.setSizes({200,399});
        auto* handle=split.handle(1);const int before=split.widget(0)->width();const QPointF local=handle->rect().center(),global=handle->mapToGlobal(local);
        QMouseEvent down(QEvent::MouseButtonPress,local,global,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(handle,&down);
        QMouseEvent move(QEvent::MouseMove,local+QPointF(40,0),global+QPointF(40,0),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(handle,&move);
        QMouseEvent up(QEvent::MouseButtonRelease,handle->mapFromGlobal(global+QPointF(40,0)),global+QPointF(40,0),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(handle,&up);QCOMPARE(split.widget(0)->width(),before+40);
    }
    void shortcutsFilterDescriptionsAndNotes(){
        novu::ui::ShortcutList list;list.setCompact(true);list.addGroup("Windows");list.addShortcut("Preview","F3","detaches the open view");list.addShortcut("Help","F1");QCOMPARE(list.visibleRowCount(),2);const int full=list.sizeHint().height();list.setFilter("detaches");QCOMPARE(list.visibleRowCount(),1);QVERIFY(list.sizeHint().height()<full);list.setFilter("missing");list.setEmptyText("Nothing matches that.");QCOMPARE(list.visibleRowCount(),0);QVERIFY(list.sizeHint().height()>0);
    }

    void structureTreeHidesBranchesWithoutLosingRows(){
        using namespace novu::ui;TreeView tree;tree.addNode({},"root","Panel",Glyph::Layers);tree.addNode("root","child","Label",Glyph::Edit);tree.setExpanded("root",true);tree.setBranchesVisible(false);tree.setGuidesVisible(false);tree.setRowHeight(26);tree.setMeta("child","label");tree.resize(250,100);tree.show();
        QSignalSpy activation(&tree,&TreeView::activated);QVERIFY(tree.chevronRect(0).isEmpty());QCOMPARE(tree.rowRect(1).height(),26);QCOMPARE(tree.meta("child"),QString("label"));QTest::mouseClick(tree.viewport(),Qt::LeftButton,Qt::NoModifier,QPoint(9,13));QCOMPARE(activation.count(),1);QCOMPARE(tree.visibleRowCount(),2);
    }
    void segmentGeometryUsesTheScopedProfile(){
        using namespace novu::ui;QWidget host;auto profile=themeFor(&host);profile.id="segment-metric-test";profile.controls.segmentedHeight=26;profile.controls.segmentedPadding=2;ThemeManager::instance().registerProfile(profile);ThemeScope::apply(&host,profile.id);
        Segmented picker({"Properties","Screen"},&host);picker.setFill(true);picker.resize(300,picker.sizeHint().height());QCOMPARE(picker.height(),26);QCOMPARE(picker.segmentRect(0).height(),22);QCOMPARE(picker.segmentRect(0).left(),2);QCOMPARE(picker.segmentRect(1).right(),297);
    }

    void railSwitcherVariantOwnsItsGeometry(){
        using namespace novu::ui;
        Segmented picker({"Properties","Screen"});
        picker.setVariant(Segmented::Variant::RailSwitcher);
        QCOMPARE(picker.variant(),Segmented::Variant::RailSwitcher);
        QVERIFY(picker.fills());
        QCOMPARE(picker.controlSize(),ControlSize::Medium);
        QCOMPARE(picker.sizePolicy().horizontalPolicy(),QSizePolicy::Expanding);
        picker.setFill(false);
        picker.setControlSize(ControlSize::Small);
        QVERIFY(picker.fills());
        QCOMPARE(picker.controlSize(),ControlSize::Medium);
    }

    void dashedBorderContainsGaps(){
        Card card;card.setHeaderVisible(false);card.resize(240,80);card.show();QCoreApplication::processEvents();
        const QImage solid=card.grab().toImage();card.setBorderStyle(Qt::DashLine,true);const QImage dashed=card.grab().toImage();card.setBorderVisible(false);const QImage plain=card.grab().toImage();
        int changes=0,gaps=0;for(int x=24;x<216;++x){if(solid.pixel(x,0)!=dashed.pixel(x,0))++changes;if(dashed.pixel(x,1)==plain.pixel(x,1))++gaps;}
        QVERIFY(changes>20);QVERIFY(gaps>20);
    }

    void compactButtonUsesProfileRadius() {
        using namespace novu::ui;
        QWidget host;
        auto profile=themeFor(&host);
        profile.id="compact-pill-test";
        profile.controls.heightSmall=24;
        profile.controls.smallButtonRadius=RadiusStep::Pill;
        ThemeManager::instance().registerProfile(profile);
        ThemeScope::apply(&host,profile.id);
        Button button("Compact",Button::Variant::Outlined,&host);
        button.setControlSize(ControlSize::Small);
        button.resize(100,24);
        QCOMPARE(button.resolvedStyle().radius,12);
        profile.controls.smallButtonRadius=RadiusStep::Small;
        ThemeManager::instance().registerProfile(profile);
        ThemeManager::instance().notifyThemeChanged();
        QCOMPARE(button.resolvedStyle().radius,qMin(profile.radii.small,12));
    }
    void splitterHonoursExplicitHandleWidth() {
        using namespace novu::ui;
        Splitter split(Qt::Horizontal);
        split.addWidget(new QWidget);
        split.addWidget(new QWidget);
        split.setHandleWidth(1);
        split.resize(600,300);split.show();QCoreApplication::processEvents();
        QCOMPARE(split.handle(1)->sizeHint().width(),1);
        const auto a=split.widget(0)->geometry(),b=split.widget(1)->geometry();
        QCOMPARE(b.left()-a.right()-1,1);
    }
    void headerMetricsSurviveThemeApplication() {
        Card card(QStringLiteral("Header"));
        card.setHeaderMetrics(QMargins(13,12,13,11),26);
        auto* body = new QLabel(QStringLiteral("Body"), &card);
        card.body()->addWidget(body);
        card.resize(300,160);card.show();QCoreApplication::processEvents();
        QVERIFY(body->geometry().top() >= 49);
        novu::ui::ThemeManager::instance().notifyThemeChanged();
        QCoreApplication::processEvents();
        QVERIFY(body->geometry().top() >= 49);
    }

    void elisionKeepsContentWithoutForcingWidth() {
        novu::ui::ElidedLabel text(QStringLiteral("A long immutable accessible document title"));
        text.resize(80,24);
        QCOMPARE(text.minimumSizeHint().width(),0);
        QVERIFY(text.fontMetrics().horizontalAdvance(text.text()) > text.width());
        QCOMPARE(text.text(),QStringLiteral("A long immutable accessible document title"));
        QVERIFY(!text.grab().isNull());
    }

    void fractionalTypeAndTracking() {
        QWidget host;
        auto profile = novu::ui::themeFor(&host);
        profile.id = QStringLiteral("fractional-test");
        profile.type.unhintedMetrics=true;
        profile.setRoleType(novu::ui::TypeRole::Body, {12.5, 400, false, 1.6, .09});
        profile.colors.textTertiary = QColor("#8793A3");
        novu::ui::ThemeManager::instance().registerProfile(profile);
        novu::ui::ThemeScope::apply(&host, profile.id);
        QLabel label("Fractional", &host);
        QFont font = novu::ui::roleFont(&label, novu::ui::TypeRole::Body);
        QVERIFY(qAbs(font.pointSizeF() * label.logicalDpiY() / 72.0 - 12.5) < .001);
        QCOMPARE(font.letterSpacingType(), QFont::AbsoluteSpacing);
        QCOMPARE(font.hintingPreference(),QFont::PreferNoHinting);
        QVERIFY(qAbs(font.letterSpacing() - 1.125) < .01);
        QCOMPARE(novu::ui::inkColor(&label, novu::ui::Ink::Tertiary), QColor("#8793A3"));
        label.setFont(font);
        novu::ui::setLineHeight(&label, 1.6);
        QCOMPARE(label.height(), 20);
        label.setContentsMargins(0,7,0,3);
        QCoreApplication::processEvents();
        QCOMPARE(label.height(), 30);
    }
    void raisedRetainsBorder() {
        Card card; card.setHeaderVisible(false); card.resize(180,100);
        card.setSurface(Card::Surface::Raised); card.show();
        QCoreApplication::processEvents();
        auto bordered = card.grab().toImage();
        card.setBorderVisible(false);
        QVERIFY(bordered != card.grab().toImage());
    }

    void surfaceAndSelectionAreExplicit() {
        Card card;
        card.setHeaderVisible(false);
        card.resize(180, 100);
        card.show();
        QCoreApplication::processEvents();
        const auto profile = novu::ui::themeFor(&card);
        QCOMPARE(card.grab().toImage().pixelColor(90, 50), profile.colors.panelBg);
        card.setSurface(Card::Surface::Elevated);
        QCOMPARE(card.grab().toImage().pixelColor(90, 50), profile.colors.surfaceHover);
        card.setSurface(Card::Surface::Window);
        QCOMPARE(card.grab().toImage().pixelColor(90, 50), profile.colors.windowBg);
        card.setSelected(true);
        QVERIFY(card.grab().toImage().pixelColor(90, 50) != profile.colors.windowBg);
        card.setSelected(false);
        QCOMPARE(card.grab().toImage().pixelColor(90, 50), profile.colors.windowBg);
    }
    void activationAndCancellation() {
        Card card; card.resize(260,180); card.show();
        QSignalSpy spy(&card, &Card::clicked);
        QTest::mouseClick(&card, Qt::LeftButton);
        QCOMPARE(spy.count(), 0);
        card.setInteractive(true);
        QTest::mouseClick(&card, Qt::LeftButton);
        QCOMPARE(spy.count(), 1);
        QTest::keyClick(&card, Qt::Key_Space);
        QTest::keyClick(&card, Qt::Key_Return);
        QCOMPARE(spy.count(), 3);
        QTest::mousePress(&card, Qt::LeftButton, {}, QPoint(10,10));
        QTest::mouseRelease(&card, Qt::LeftButton, {}, QPoint(-5,-5));
        QCOMPARE(spy.count(), 3);
        card.setEnabled(false);
        QTest::mouseClick(&card, Qt::LeftButton);
        QTest::keyClick(&card, Qt::Key_Space);
        QCOMPARE(spy.count(), 3);
    }
    void childActionDoesNotActivateCard() {
        Card card; card.setInteractive(true);
        auto* child = new QPushButton("Child", &card);
        card.body()->addWidget(child); card.show();
        QSignalSpy cardSpy(&card, &Card::clicked);
        QSignalSpy childSpy(child, &QPushButton::clicked);
        QTest::mouseClick(child, Qt::LeftButton);
        QCOMPARE(childSpy.count(), 1); QCOMPARE(cardSpy.count(), 0);
    }
};
QTEST_MAIN(CardInteractionTest)
#include "tst_card_interaction.moc"
