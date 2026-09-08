// Tests for the shared Novu window chrome.
//
// What is worth asserting here is the part the applications got wrong on their
// own: that the three controls are one size, that hiding a control moves
// nothing, that a dialog and a main window wear the same bar, and that the
// Close button is neutral until it is hovered. Those are the four defects this
// component exists to remove, and each of them is measurable.
//
// The hover colour is asserted by rendering the button and reading a pixel
// rather than by re-deriving the token. A test that recomputes what the paint
// code computes would keep passing after the paint code stopped using it.

#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/ThemeProfile.h"
#include "novu/ui/ThemeScope.h"
#include "novu/ui/TitleBar.h"
#include "novu/ui/WindowShell.h"

#include <QEnterEvent>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QTest>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

class WindowChromeTest : public QObject
{
    Q_OBJECT

private slots:
    // tokens
    void everyProfileDefinesWindowGeometry();
    void controlGeometryIsIdenticalAcrossProfiles();
    void windowColoursAreDerivedFromTheProfilePalette();
    void radiusRolesResolveForWindowsAndDialogs();

    // control button
    void allControlsShareOneBoundingBox();
    void controlSizeFollowsTheToken();
    void closeIsNeutralUntilHovered();
    void closeHoverUsesTheDangerToken();
    void neutralControlsDoNotUseTheDangerToken();

    // title bar
    void titleBarHeightIsTheToken();
    void hidingControlsLeavesTheOthersUntouched();
    void maximizeBecomesRestore();
    void aLongCaptionElidesInsteadOfWidening();
    void captionDoesNotSetAFloorUnderTheWidth();

    // shell
    void dialogHasNoMinimizeOrMaximize();
    void dialogAndWindowShareTheSameBar();
    void toolWindowResizesButDoesNotMaximize();
    void capabilitiesFollowTheWindowFlags();
    void surfaceRoleFollowsTheWindowKind();
    void aMaximizedWindowHasNoRadius();
    void contentSizeAccountsForTheFrame();
    void shellIsDiscoverableFromItsWindow();
    void contentIsTheOnlySlotAPageFills();

private:
    // The background a control paints, sampled away from the glyph.
    static QColor plateColour(QWidget* button);
    static void hover(QWidget* widget, bool entering);
};

QColor WindowChromeTest::plateColour(QWidget* button)
{
    // Pre-filled with a colour no token uses, so "painted nothing" is
    // distinguishable from "painted the neutral surface".
    QImage canvas(button->size(), QImage::Format_ARGB32);
    canvas.fill(QColor(255, 0, 255));
    QPainter painter(&canvas);
    button->render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
    painter.end();
    // Two pixels in from the corner: inside the plate, well clear of the
    // centred glyph box.
    return canvas.pixelColor(2, 2);
}

void WindowChromeTest::hover(QWidget* widget, bool entering)
{
    const QPointF centre(widget->width() / 2.0, widget->height() / 2.0);
    if (entering) {
        QEnterEvent enter(centre, centre, widget->mapToGlobal(centre));
        QCoreApplication::sendEvent(widget, &enter);
    } else {
        QEvent leave(QEvent::Leave);
        QCoreApplication::sendEvent(widget, &leave);
    }
}

// -- tokens ------------------------------------------------------------------

void WindowChromeTest::everyProfileDefinesWindowGeometry()
{
    const QStringList ids = ThemeManager::instance().profileIds();
    QVERIFY(!ids.isEmpty());
    for (const QString& id : ids) {
        const WindowTokens& w = ThemeManager::instance().profile(id).window;
        QVERIFY2(w.titleBarHeight > 0, qPrintable(id));
        QVERIFY2(w.controlWidth > 0, qPrintable(id));
        QVERIFY2(w.controlHeight > 0, qPrintable(id));
        QVERIFY2(w.glyphSize > 0, qPrintable(id));
        QVERIFY2(w.resizeMargin > 0, qPrintable(id));
        // A control taller than its bar would be clipped by the layout.
        QVERIFY2(w.controlHeight <= w.titleBarHeight, qPrintable(id));
    }
}

void WindowChromeTest::controlGeometryIsIdenticalAcrossProfiles()
{
    // The task that created these tokens exists because three applications
    // each chose their own control size. A profile may restyle the bar; it may
    // not resize it, or the products drift apart again through the theme
    // instead of through the code.
    const QStringList ids = ThemeManager::instance().profileIds();
    const WindowTokens& first = ThemeManager::instance().profile(ids.first()).window;
    for (const QString& id : ids) {
        const WindowTokens& w = ThemeManager::instance().profile(id).window;
        QCOMPARE(w.titleBarHeight, first.titleBarHeight);
        QCOMPARE(w.controlWidth, first.controlWidth);
        QCOMPARE(w.controlHeight, first.controlHeight);
        QCOMPARE(w.titleBarPadding, first.titleBarPadding);
        QCOMPARE(w.glyphSize, first.glyphSize);
    }
}

void WindowChromeTest::windowColoursAreDerivedFromTheProfilePalette()
{
    for (const QString& id : ThemeManager::instance().profileIds()) {
        const ThemeProfile& p = ThemeManager::instance().profile(id);
        // Derived, not the struct's literal defaults: a profile that skipped
        // the derivation would wear the graphite bar over its own palette.
        QCOMPARE(p.window.titleBarBg, p.colors.headerBg);
        QCOMPARE(p.window.titleText, p.colors.textPrimary);
        QCOMPARE(p.window.titleTextInactive, p.colors.textDisabled);
        QCOMPARE(p.window.border, p.colors.border);
        QCOMPARE(p.window.controlHoverBg, p.colors.surfaceHover);
        QCOMPARE(p.window.closeHoverBg, p.colors.danger);
        QVERIFY(p.window.closePressedBg != p.window.closeHoverBg);
    }
}

void WindowChromeTest::radiusRolesResolveForWindowsAndDialogs()
{
    for (const QString& id : ThemeManager::instance().profileIds()) {
        const ThemeProfile& p = ThemeManager::instance().profile(id);
        QVERIFY(p.radius(SurfaceRole::Window) >= 0);
        QVERIFY(p.radius(SurfaceRole::Dialog) >= 0);
        // Not the pill marker: a window rounded to half its height is not a
        // window.
        QVERIFY(p.radius(SurfaceRole::Window) < 100);
        QVERIFY(p.radius(SurfaceRole::Dialog) < 100);
    }
}

// -- control button ----------------------------------------------------------

void WindowChromeTest::allControlsShareOneBoundingBox()
{
    QWidget host;
    WindowControlButton minimize(WindowControlButton::Control::Minimize, &host);
    WindowControlButton maximize(WindowControlButton::Control::Maximize, &host);
    WindowControlButton restore(WindowControlButton::Control::Restore, &host);
    WindowControlButton close(WindowControlButton::Control::Close, &host);

    // Close is not larger than Minimize. This is the assertion the whole
    // component exists for.
    QCOMPARE(close.sizeHint(), minimize.sizeHint());
    QCOMPARE(maximize.sizeHint(), minimize.sizeHint());
    QCOMPARE(restore.sizeHint(), minimize.sizeHint());
    QCOMPARE(close.minimumSizeHint(), close.sizeHint());
}

void WindowChromeTest::controlSizeFollowsTheToken()
{
    QWidget host;
    WindowControlButton close(WindowControlButton::Control::Close, &host);
    const WindowTokens& w = themeFor(&host).window;
    QCOMPARE(close.sizeHint(), QSize(w.controlWidth, w.controlHeight));
}

void WindowChromeTest::closeIsNeutralUntilHovered()
{
    QWidget host;
    host.resize(200, 60);
    WindowControlButton close(WindowControlButton::Control::Close, &host);
    close.resize(close.sizeHint());

    const WindowTokens& w = themeFor(&host).window;
    // Nothing painted: the magenta the canvas was filled with survives, which
    // is what "the normal Novu title-bar surface shows through" means.
    QCOMPARE(plateColour(&close), QColor(255, 0, 255));
    QVERIFY(plateColour(&close) != w.closeHoverBg);
}

void WindowChromeTest::closeHoverUsesTheDangerToken()
{
    QWidget host;
    host.resize(200, 60);
    WindowControlButton close(WindowControlButton::Control::Close, &host);
    close.resize(close.sizeHint());

    hover(&close, true);
    const WindowTokens& w = themeFor(&host).window;
    QCOMPARE(plateColour(&close), w.closeHoverBg);
    QCOMPARE(w.closeHoverBg, themeFor(&host).colors.danger);

    hover(&close, false);
    QCOMPARE(plateColour(&close), QColor(255, 0, 255));
}

void WindowChromeTest::neutralControlsDoNotUseTheDangerToken()
{
    QWidget host;
    host.resize(200, 60);
    const WindowTokens& w = themeFor(&host).window;

    for (auto control : {WindowControlButton::Control::Minimize,
                         WindowControlButton::Control::Maximize,
                         WindowControlButton::Control::Restore}) {
        WindowControlButton button(control, &host);
        button.resize(button.sizeHint());
        hover(&button, true);
        QCOMPARE(plateColour(&button), w.controlHoverBg);
        QVERIFY(plateColour(&button) != w.closeHoverBg);
    }
}

// -- title bar ---------------------------------------------------------------

void WindowChromeTest::titleBarHeightIsTheToken()
{
    QWidget host;
    TitleBar bar(&host);
    const WindowTokens& w = themeFor(&host).window;
    QCOMPARE(bar.sizeHint().height(), w.titleBarHeight);
    QCOMPARE(bar.minimumSizeHint().height(), w.titleBarHeight);
    // setFixedHeight in applyTheme, so a layout cannot stretch it either.
    QCOMPARE(bar.maximumHeight(), w.titleBarHeight);
}

void WindowChromeTest::hidingControlsLeavesTheOthersUntouched()
{
    QWidget host;
    host.resize(600, 80);
    TitleBar full(&host);
    full.setTitle(QStringLiteral("Window"));
    full.resize(600, full.sizeHint().height());

    TitleBar closeOnly(&host);
    closeOnly.setTitle(QStringLiteral("Dialog"));
    closeOnly.setMinimizable(false);
    closeOnly.setMaximizable(false);
    closeOnly.resize(600, closeOnly.sizeHint().height());

    full.layout()->activate();
    closeOnly.layout()->activate();

    // Same bar height, same Close box, and Close still flush against the
    // trailing edge. A dialog therefore reads as the same product as the
    // window behind it.
    QCOMPARE(closeOnly.height(), full.height());
    QCOMPARE(closeOnly.closeButton()->size(), full.closeButton()->size());
    QCOMPARE(closeOnly.closeButton()->geometry().right(),
             full.closeButton()->geometry().right());
    QCOMPARE(closeOnly.closeButton()->geometry().right(), closeOnly.width() - 1);
    QVERIFY(!closeOnly.minimizeButton()->isVisibleTo(&closeOnly));
    QVERIFY(!closeOnly.maximizeButton()->isVisibleTo(&closeOnly));
    QVERIFY(closeOnly.closeButton()->isVisibleTo(&closeOnly));
}

void WindowChromeTest::maximizeBecomesRestore()
{
    QWidget host;
    TitleBar bar(&host);
    QCOMPARE(bar.maximizeButton()->control(), WindowControlButton::Control::Maximize);
    QVERIFY(!bar.isMaximized());

    bar.setMaximized(true);
    QCOMPARE(bar.maximizeButton()->control(), WindowControlButton::Control::Restore);
    QVERIFY(bar.isMaximized());
    // The glyph changed; the box did not.
    QCOMPARE(bar.maximizeButton()->sizeHint(), bar.closeButton()->sizeHint());

    bar.setMaximized(false);
    QCOMPARE(bar.maximizeButton()->control(), WindowControlButton::Control::Maximize);
}

void WindowChromeTest::aLongCaptionElidesInsteadOfWidening()
{
    QWidget host;
    host.resize(400, 80);
    TitleBar bar(&host);
    const QString caption =
        QStringLiteral("J:/Github/NovuExplorer/src/ui/dialogs/"
                       "a4f9c1e2b7d84e0fa1c3d5e7b9081234.novuproject");
    bar.setTitle(caption);
    bar.resize(320, bar.sizeHint().height());
    bar.layout()->activate();

    // title() still reports what the caller set - the eliding is presentation.
    QCOMPARE(bar.title(), caption);
    QLabel* rendered = bar.findChild<QLabel*>();
    QVERIFY(rendered);
    // Controls stay on the frame: the caption gave way, not the chrome.
    QCOMPARE(bar.closeButton()->geometry().right(), bar.width() - 1);
    QVERIFY(bar.closeButton()->geometry().left() >= 0);
}

void WindowChromeTest::captionDoesNotSetAFloorUnderTheWidth()
{
    QWidget host;
    TitleBar bar(&host);
    const int bare = bar.minimumSizeHint().width();
    bar.setTitle(QStringLiteral("An extremely long window caption that would otherwise "
                                "force the window to be wider than the user asked for"));
    QCOMPARE(bar.minimumSizeHint().width(), bare);
}

// -- shell -------------------------------------------------------------------

void WindowChromeTest::dialogHasNoMinimizeOrMaximize()
{
    Dialog dialog(QStringLiteral("Confirm"));
    QVERIFY(!dialog.shell()->isMinimizable());
    QVERIFY(!dialog.shell()->isMaximizable());
    QVERIFY(!dialog.shell()->isResizable());
    QVERIFY(!dialog.titleBar()->isMinimizable());
    QVERIFY(!dialog.titleBar()->isMaximizable());
    QVERIFY(dialog.titleBar()->isClosable());
}

void WindowChromeTest::dialogAndWindowShareTheSameBar()
{
    AppWindow window;
    Dialog dialog;
    const WindowTokens& w = themeFor(&window).window;

    QCOMPARE(dialog.titleBar()->sizeHint().height(), w.titleBarHeight);
    QCOMPARE(window.titleBar()->sizeHint().height(), w.titleBarHeight);
    QCOMPARE(dialog.titleBar()->closeButton()->sizeHint(),
             window.titleBar()->closeButton()->sizeHint());
}

void WindowChromeTest::toolWindowResizesButDoesNotMaximize()
{
    ToolWindow tool;
    QVERIFY(tool.shell()->isResizable());
    QVERIFY(!tool.shell()->isMinimizable());
    QVERIFY(!tool.shell()->isMaximizable());
    QCOMPARE(tool.titleBar()->closeButton()->sizeHint(),
             AppWindow().titleBar()->closeButton()->sizeHint());
}

void WindowChromeTest::capabilitiesFollowTheWindowFlags()
{
    AppWindow window;
    QVERIFY(window.windowFlags() & Qt::FramelessWindowHint);
    QVERIFY(window.windowFlags() & Qt::WindowMinimizeButtonHint);
    QVERIFY(window.windowFlags() & Qt::WindowMaximizeButtonHint);

    window.shell()->setMinimizable(false);
    QVERIFY(!(window.windowFlags() & Qt::WindowMinimizeButtonHint));
    QVERIFY(!window.titleBar()->isMinimizable());

    // A window that cannot resize cannot maximize either, or the bar offers a
    // gesture the window refuses.
    window.shell()->setResizable(false);
    QVERIFY(!window.shell()->isMaximizable());
    QVERIFY(!window.titleBar()->isMaximizable());

    Dialog dialog;
    QVERIFY(dialog.windowFlags() & Qt::FramelessWindowHint);
    QVERIFY(!(dialog.windowFlags() & Qt::WindowMinimizeButtonHint));
    QVERIFY(!(dialog.windowFlags() & Qt::WindowMaximizeButtonHint));
}

void WindowChromeTest::surfaceRoleFollowsTheWindowKind()
{
    AppWindow window;
    ToolWindow tool;
    Dialog dialog;
    QCOMPARE(window.shell()->surfaceRole(), SurfaceRole::Window);
    QCOMPARE(tool.shell()->surfaceRole(), SurfaceRole::Window);
    QCOMPARE(dialog.shell()->surfaceRole(), SurfaceRole::Dialog);

    const ThemeProfile& profile = themeFor(&window);
    QCOMPARE(window.shell()->cornerRadius(), profile.radius(SurfaceRole::Window));
    QCOMPARE(dialog.shell()->cornerRadius(), profile.radius(SurfaceRole::Dialog));
}

void WindowChromeTest::aMaximizedWindowHasNoRadius()
{
    AppWindow window;
    window.shell()->setContentSize(QSize(400, 300));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    window.showMaximized();
    QTRY_VERIFY(window.isMaximized());
    // The corners of a maximized window are the screen's.
    QCOMPARE(window.shell()->cornerRadius(), 0);
    QVERIFY(window.titleBar()->isMaximized());

    window.showNormal();
    QTRY_VERIFY(!window.isMaximized());
    QVERIFY(!window.titleBar()->isMaximized());
    QCOMPARE(window.shell()->cornerRadius(),
             themeFor(&window).radius(SurfaceRole::Window));
}

void WindowChromeTest::contentSizeAccountsForTheFrame()
{
    AppWindow window;
    const QSize wanted(640, 400);
    window.shell()->setContentSize(wanted);

    const QSize overhead = window.shell()->frameOverhead();
    // The caller asked for content, not for a frame: the title bar is on top
    // of what it asked for, never carved out of it.
    QVERIFY(overhead.height() >= themeFor(&window).window.titleBarHeight);
    QCOMPARE(window.size(), wanted + overhead);
}

void WindowChromeTest::shellIsDiscoverableFromItsWindow()
{
    AppWindow window;
    QCOMPARE(WindowShell::of(&window), window.shell());

    QWidget plain;
    QVERIFY(!WindowShell::of(&plain));
    QVERIFY(!WindowShell::of(nullptr));
}

void WindowChromeTest::contentIsTheOnlySlotAPageFills()
{
    AppWindow window;
    auto* page = new QWidget;
    window.setContentWidget(page);
    QCOMPARE(page->parentWidget(), window.content());

    // Replacing the page deletes the old one, which is what a window swapping
    // views expects.
    QPointer<QWidget> first = page;
    window.setContentWidget(new QWidget);
    QTRY_VERIFY(first.isNull());

    // The bar is not reachable through the content: a page cannot find and
    // restyle the chrome above it by walking its own children.
    QVERIFY(!window.content()->findChild<TitleBar*>());
}

QTEST_MAIN(WindowChromeTest)
#include "tst_window_chrome.moc"
