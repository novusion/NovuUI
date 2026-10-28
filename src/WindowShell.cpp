#include "novu/ui/WindowShell.h"
#include "novu/ui/NativeFrame.h"

#include "novu/ui/Button.h"
#include "novu/ui/Feedback.h"
#include <QKeyEvent>
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QRegion>
#include <QTextOption>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>
#include <QOperatingSystemVersion>

#ifdef Q_OS_WIN
#include <QAbstractNativeEventFilter>
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>

// Declared rather than pulled from a newer SDK, so the library builds against
// whatever Windows SDK a machine happens to have. The values are stable API.
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif
#ifndef SM_CXPADDEDBORDERWIDTH
#define SM_CXPADDEDBORDERWIDTH 92
#endif
#endif // Q_OS_WIN

namespace novu::ui {

namespace {

// The surface the whole window is drawn on.
//
// A widget rather than a paintEvent on the window itself, and it satisfies
// justification 1 of the container rule - it is a visible surface with a
// background and a border. Painting the window directly would have to happen
// from an event filter, where any stylesheet the application later puts on its
// own window would paint straight over it.
//
// It also carries the resize band on the platforms that have no native one.
class WindowFrame : public QWidget
{
public:
    WindowFrame(WindowShell* shell, QWidget* parent)
        : QWidget(parent)
        , m_shell(shell)
    {
#ifndef Q_OS_WIN
        // Only the fallback path needs to see the pointer crossing the border.
        // On Windows the band is outside the client area and the OS owns it.
        setMouseTracking(true);
#endif
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        const ThemeProfile& profile = themeFor(this);

        QPainter painter(this);
        QWidget* top = window();
        const bool maximised = top && top->isMaximized();

        // A maximised window has no border and no curve: the frame is against
        // the screen edge, and a rounded corner or a hairline there reads as a
        // rendering fault rather than as a frame.
        if (maximised) {
            painter.fillRect(rect(), profile.colors.windowBg);
            return;
        }

#ifdef Q_OS_WIN
        // On Windows the compositor owns the outline: DWM clips the corners
        // and draws the hairline in the border token (see applyNativeFrame),
        // exactly as NovuHub does. Painting a second, Qt-side curve here is
        // what used to fight it - three radii (mask, DWM preset, this path)
        // that never agreed, so the border vanished at every corner.
        painter.fillRect(rect(), profile.colors.windowBg);
        return;
#endif

        // The background is clipped to the same rounded path the border draws
        // - not a plain fillRect() - because DWM (Windows) clips the real
        // window to its own curve at a radius it picks from three fixed
        // presets, never the exact token this profile states. A flat
        // rectangle filled to the corner painted content DWM was about to cut
        // away, and a *square-cornered* 1px border on top of that curve is
        // what a "missing" or "wrong" corner actually was: not the wrong
        // radius, content that never agreed with any radius at all.
        painter.setRenderHint(QPainter::Antialiasing, true);
        const qreal radius = qMax(0, m_shell->cornerRadius());
        QPainterPath shape;
        shape.addRoundedRect(QRectF(rect()), radius, radius);
        painter.fillPath(shape, profile.colors.windowBg);

        const int width = profile.window.borderWidth;
        if (width <= 0) {
            return;
        }
        // A stroke on the same path, not four filled bars: those were exactly
        // right for a square frame and exactly wrong for this one, since a
        // flat bar has no notion of the curve its own corner sits on.
        const QColor colour = profile.window.border;
        QPainterPath strokePath;
        strokePath.addRoundedRect(QRectF(rect()).adjusted(width / 2.0, width / 2.0,
                                                          -width / 2.0, -width / 2.0),
                                  qMax(0.0, radius - width / 2.0),
                                  qMax(0.0, radius - width / 2.0));
        painter.setPen(QPen(colour, width));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(strokePath);
    }

    bool event(QEvent* event) override
    {
        if (event->type() == ThemeChangeEvent::type()) {
            update();
        }
        return QWidget::event(event);
    }

#ifndef Q_OS_WIN
    void mouseMoveEvent(QMouseEvent* event) override
    {
        const Qt::Edges edges = edgesAt(event->position().toPoint());
        setCursor(cursorFor(edges));
        QWidget::mouseMoveEvent(event);
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() != Qt::LeftButton) {
            QWidget::mousePressEvent(event);
            return;
        }
        const Qt::Edges edges = edgesAt(event->position().toPoint());
        QWidget* top = window();
        if (edges && top && top->windowHandle()) {
            // The compositor performs the resize. Doing it by hand with
            // setGeometry() loses snapping on every platform and is simply not
            // permitted on Wayland.
            top->windowHandle()->startSystemResize(edges);
            event->accept();
            return;
        }
        QWidget::mousePressEvent(event);
    }

    void leaveEvent(QEvent* event) override
    {
        unsetCursor();
        QWidget::leaveEvent(event);
    }

private:
    Qt::Edges edgesAt(const QPoint& pos) const
    {
        QWidget* top = window();
        if (!m_shell->isResizable() || (top && top->isMaximized())) {
            return {};
        }
        const int band = qMax(1, themeFor(this).window.resizeMargin);
        Qt::Edges edges;
        if (pos.x() < band)                  edges |= Qt::LeftEdge;
        if (pos.x() > width() - band - 1)    edges |= Qt::RightEdge;
        if (pos.y() < band)                  edges |= Qt::TopEdge;
        if (pos.y() > height() - band - 1)   edges |= Qt::BottomEdge;
        return edges;
    }

    static Qt::CursorShape cursorFor(Qt::Edges edges)
    {
        if ((edges & Qt::TopEdge && edges & Qt::LeftEdge)
            || (edges & Qt::BottomEdge && edges & Qt::RightEdge)) {
            return Qt::SizeFDiagCursor;
        }
        if ((edges & Qt::TopEdge && edges & Qt::RightEdge)
            || (edges & Qt::BottomEdge && edges & Qt::LeftEdge)) {
            return Qt::SizeBDiagCursor;
        }
        if (edges & (Qt::LeftEdge | Qt::RightEdge)) {
            return Qt::SizeHorCursor;
        }
        if (edges & (Qt::TopEdge | Qt::BottomEdge)) {
            return Qt::SizeVerCursor;
        }
        return Qt::ArrowCursor;
    }
#endif // !Q_OS_WIN

private:
    WindowShell* m_shell = nullptr;
};

} // namespace

// -- native frame ------------------------------------------------------------

#ifdef Q_OS_WIN

// Windows keeps its own frame under our chrome.
//
// The alternative - a plain Qt::FramelessWindowHint window - loses the drop
// shadow, Aero Snap, the resize borders and the DWM corner rounding all at
// once, and every application that tried it ended up reimplementing those four
// things slightly differently. Restoring WS_THICKFRAME and answering
// WM_NCCALCSIZE with a zero-height caption keeps the whole native window
// manager working while nothing native is drawn.
class WindowShell::NativeFrame : public QAbstractNativeEventFilter
{
public:
    explicit NativeFrame(WindowShell* shell)
        : m_shell(shell)
    {
        qApp->installNativeEventFilter(this);
    }

    ~NativeFrame() override
    {
        if (qApp) {
            qApp->removeNativeEventFilter(this);
        }
    }

    void setHwnd(HWND hwnd) { m_hwnd = hwnd; }

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override
    {
        if (!m_hwnd
            || (eventType != "windows_generic_MSG" && eventType != "windows_dispatcher_MSG")) {
            return false;
        }
        auto* msg = static_cast<MSG*>(message);

        // A native child window near the frame edge would answer the hit test
        // itself and swallow the resize. Qt 6 makes children native only on
        // demand, so this is rare - and exactly the kind of rare that is
        // impossible to diagnose from a bug report.
        if (msg->hwnd != m_hwnd) {
            if (msg->message == WM_NCHITTEST && IsChild(m_hwnd, msg->hwnd)
                && hitTest(msg->lParam) != HTCLIENT) {
                *result = HTTRANSPARENT;
                return true;
            }
            return false;
        }

        switch (msg->message) {
        case WM_NCCALCSIZE:
            if (msg->wParam == TRUE) {
                // Zero-height caption: the client area becomes the whole
                // window, which is what puts our title bar where the native one
                // was. A maximised window still needs the frame compensation
                // Windows adds, or its edges are cropped off-screen.
                if (IsZoomed(m_hwnd)) {
                    auto* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam);
                    const int frame = GetSystemMetrics(SM_CXSIZEFRAME)
                                      + GetSystemMetrics(SM_CXPADDEDBORDERWIDTH);
                    params->rgrc[0].left += frame;
                    params->rgrc[0].top += frame;
                    params->rgrc[0].right -= frame;
                    params->rgrc[0].bottom -= frame;
                }
                *result = 0;
                return true;
            }
            return false;

        case WM_NCHITTEST: {
            const LRESULT hit = hitTest(msg->lParam);
            if (hit == HTCLIENT) {
                return false;
            }
            *result = hit;
            return true;
        }

        case WM_GETMINMAXINFO: {
            // Without this a frameless thick-framed window can be dragged
            // smaller than its own layout minimum, and the content is clipped
            // rather than refusing to shrink.
            QWidget* window = m_shell->window();
            if (!window) {
                return false;
            }
            const QSize minimum = window->minimumSizeHint()
                                      .expandedTo(window->minimumSize());
            const qreal ratio = window->devicePixelRatioF();
            auto* info = reinterpret_cast<MINMAXINFO*>(msg->lParam);
            info->ptMinTrackSize.x = qRound(minimum.width() * ratio);
            info->ptMinTrackSize.y = qRound(minimum.height() * ratio);
            *result = 0;
            return true;
        }

        default:
            return false;
        }
    }

private:
    // HTCLIENT means "not on the frame" - the caller then leaves the message
    // alone.
    LRESULT hitTest(LPARAM lParam) const
    {
        QWidget* window = m_shell->window();
        if (!m_shell->isResizable() || !window || IsZoomed(m_hwnd)) {
            return HTCLIENT;
        }

        RECT frame;
        GetWindowRect(m_hwnd, &frame);
        const long x = GET_X_LPARAM(lParam);
        const long y = GET_Y_LPARAM(lParam);

        // The token band in device pixels, never narrower than the metric the
        // system itself uses - a 6px band on a 150% display is 4 logical pixels
        // and users miss it.
        const int band = qMax(qRound(themeFor(window).window.resizeMargin
                                     * window->devicePixelRatioF()),
                              GetSystemMetrics(SM_CXSIZEFRAME)
                                  + GetSystemMetrics(SM_CXPADDEDBORDERWIDTH));

        const bool left = x < frame.left + band;
        const bool right = x >= frame.right - band;
        const bool top = y < frame.top + band;
        const bool bottom = y >= frame.bottom - band;

        if (top && left)     return HTTOPLEFT;
        if (top && right)    return HTTOPRIGHT;
        if (bottom && left)  return HTBOTTOMLEFT;
        if (bottom && right) return HTBOTTOMRIGHT;
        if (left)            return HTLEFT;
        if (right)           return HTRIGHT;
        if (top)             return HTTOP;
        if (bottom)          return HTBOTTOM;
        return HTCLIENT;
    }

    WindowShell* m_shell = nullptr;
    HWND m_hwnd = nullptr;
};

#else

// Nothing to filter: the fallback resize lives in WindowFrame, and the corner
// radius is whatever the platform's compositor draws.
class WindowShell::NativeFrame
{
};

#endif // Q_OS_WIN

// -- WindowShell -------------------------------------------------------------

WindowShell::WindowShell(QWidget* window, WindowKind kind)
    : QObject(window)
    , m_window(window)
    , m_kind(kind)
{
    Q_ASSERT_X(window, "WindowShell", "a shell needs a window");
    Q_ASSERT_X(!window->layout(), "WindowShell",
               "the shell owns the window's layout; build it before anything else");

    // Semantics per kind. An application overrides these by capability, never
    // by reaching for a flag.
    m_minimizable = kind == WindowKind::Main;
    m_maximizable = kind == WindowKind::Main;
    m_resizable = kind != WindowKind::Dialog;

    auto* root = new QVBoxLayout(m_window);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_frame = new WindowFrame(this, m_window);
    root->addWidget(m_frame);

    auto* frameLayout = new QVBoxLayout(m_frame);
    frameLayout->setSpacing(0);

    m_titleBar = new TitleBar(m_frame);
    frameLayout->addWidget(m_titleBar);

    // The content slot. A component boundary, justification 4: it is what an
    // application fills, and what the shell hands back from content().
    m_content = new QWidget(m_frame);
    m_contentLayout = new QVBoxLayout(m_content);
    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    m_contentLayout->setSpacing(0);
    frameLayout->addWidget(m_content, 1);

    connect(m_titleBar, &TitleBar::minimizeRequested, this, [this] {
        if (m_minimizable) {
            m_window->showMinimized();
        }
    });
    connect(m_titleBar, &TitleBar::maximizeRestoreRequested,
            this, &WindowShell::toggleMaximizeRestore);
    connect(m_titleBar, &TitleBar::closeRequested, this, [this] {
        m_window->close();
    });
    connect(m_titleBar, &TitleBar::dragRequested, this, [this] {
        if (m_window->isMaximized()) {
            // Windows and most Linux shells restore a maximised window when it
            // is dragged. Leaving it maximised and moving the frame instead is
            // the behaviour nobody expects.
            m_window->showNormal();
        }
        if (QWindow* handle = m_window->windowHandle()) {
            handle->startSystemMove();
        }
    });

    m_window->installEventFilter(this);

    applyWindowFlags();
    applyTheme();
    syncStateFromWindow();
}

WindowShell::~WindowShell()
{
    delete m_native;
}

WindowShell* WindowShell::of(const QWidget* window)
{
    if (!window) {
        return nullptr;
    }
    return window->findChild<WindowShell*>(QString(), Qt::FindDirectChildrenOnly);
}

void WindowShell::setContentWidget(QWidget* widget)
{
    // The previous content is deleted, not detached: swapping the page of a
    // window is the case this exists for, and a caller that wanted to keep the
    // old one would have taken it out of the layout itself.
    while (QLayoutItem* item = m_contentLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    if (widget) {
        widget->setParent(m_content);
        m_contentLayout->addWidget(widget);
    }
}

void WindowShell::setTitle(const QString& title)
{
    // Through the window, so the taskbar, Alt-Tab and the title bar all say the
    // same thing. The bar picks it up from the WindowTitleChange event.
    m_window->setWindowTitle(title);
    m_titleBar->setTitle(title);
}

QString WindowShell::title() const
{
    return m_titleBar->title();
}

void WindowShell::setIcon(const QIcon& icon)
{
    m_window->setWindowIcon(icon);
    m_titleBar->setIcon(icon);
}

void WindowShell::setResizable(bool resizable)
{
    if (m_resizable == resizable) {
        return;
    }
    m_resizable = resizable;
    if (!m_resizable && m_maximizable) {
        // A window that cannot be resized cannot be maximised either; leaving
        // the control enabled would offer a gesture that does nothing.
        setMaximizable(false);
    }
    applyWindowFlags();
    applyNativeFrame();
}

void WindowShell::setMinimizable(bool minimizable)
{
    if (m_minimizable == minimizable) {
        return;
    }
    m_minimizable = minimizable;
    m_titleBar->setMinimizable(minimizable);
    applyWindowFlags();
}

void WindowShell::setMaximizable(bool maximizable)
{
    if (m_maximizable == maximizable) {
        return;
    }
    m_maximizable = maximizable;
    m_titleBar->setMaximizable(maximizable);
    applyWindowFlags();
}

QSize WindowShell::frameOverhead() const
{
    const WindowTokens& w = themeFor(m_window).window;
    const QMargins margins = m_frame->layout()
                                 ? m_frame->layout()->contentsMargins()
                                 : QMargins();
    return QSize(margins.left() + margins.right(),
                 margins.top() + margins.bottom() + w.titleBarHeight);
}

void WindowShell::setContentSize(const QSize& size)
{
    m_window->resize(size + frameOverhead());
}

SurfaceRole WindowShell::surfaceRole() const
{
    return m_kind == WindowKind::Dialog ? SurfaceRole::Dialog : SurfaceRole::Window;
}

int WindowShell::cornerRadius() const
{
    // A maximised window is square by definition - its corners are the screen's.
    if (m_window->isMaximized()) {
        return 0;
    }
    return themeFor(m_window).radius(surfaceRole());
}

void WindowShell::toggleMaximizeRestore()
{
    if (!m_maximizable) {
        return;
    }
    if (m_window->isMaximized()) {
        m_window->showNormal();
    } else {
        m_window->showMaximized();
    }
    syncStateFromWindow();
}

void WindowShell::applyWindowFlags()
{
    Qt::WindowFlags flags = m_kind == WindowKind::Dialog ? Qt::Dialog
                          : m_kind == WindowKind::Tool   ? Qt::Tool
                                                         : Qt::Window;
    // The system menu stays: right-clicking the taskbar button, Alt+Space and
    // the accessibility tooling all go through it, and none of that is visual.
    flags |= Qt::FramelessWindowHint | Qt::WindowSystemMenuHint
             | Qt::WindowCloseButtonHint;
    if (m_minimizable) {
        flags |= Qt::WindowMinimizeButtonHint;
    }
    if (m_maximizable) {
        flags |= Qt::WindowMaximizeButtonHint;
    }

    if (m_window->windowFlags() == flags) {
        return;
    }
    // setWindowFlags() hides a visible window, which would make a capability
    // change at runtime look like the window vanished.
    const bool wasVisible = m_window->isVisible();
    m_window->setWindowFlags(flags);
    if (wasVisible) {
        m_window->show();
    }
}

void WindowShell::applyTheme()
{
    const ThemeProfile& profile = themeFor(m_window);
    const WindowTokens& w = profile.window;

    if (auto* frameLayout = m_frame->layout()) {
#ifdef Q_OS_WIN
        // The border is the only inset: Windows keeps the resize band outside
        // the client area, where the OS hit-tests it for us.
        const int inset = w.borderWidth;
#else
        // Everywhere else the band has to live inside the frame, because a
        // frameless window has no non-client area for the pointer to find.
        const int inset = m_resizable ? qMax(w.borderWidth, w.resizeMargin)
                                      : w.borderWidth;
#endif
        frameLayout->setContentsMargins(inset, inset, inset, inset);
    }

    m_titleBar->setMinimizable(m_minimizable);
    m_titleBar->setMaximizable(m_maximizable);
    m_titleBar->setCornerRadius(cornerRadius());

    m_frame->update();
    applyNativeFrame();
    applyWindowMask();
}

void WindowShell::applyWindowMask()
{
#ifdef Q_OS_WIN
    // No mask on Windows. A QRegion mask is aliased, it cuts away the DWM
    // shadow, and its curve never matched the compositor's own; DWM already
    // rounds the window (applyNativeFrame). Any mask left from an earlier
    // build of the window is cleared.
    if (m_window)
        m_window->clearMask();
#endif
}

void WindowShell::applyNativeFrame()
{
#ifdef Q_OS_WIN
    // Before the window exists there is no HWND to configure, and forcing one
    // with winId() during construction creates a native window Qt then has to
    // recreate when the flags change.
    if (!m_window->windowHandle()) {
        return;
    }

    auto hwnd = reinterpret_cast<HWND>(m_window->winId());
    if (!hwnd) {
        return;
    }

    if (!m_native) {
        m_native = new NativeFrame(this);
    }
    m_native->setHwnd(hwnd);

    // Restore the native frame Qt removed. WS_THICKFRAME is what gives the
    // window its resize borders, its shadow and Aero Snap; WM_NCCALCSIZE above
    // is what stops it from also drawing a caption.
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    style |= WS_THICKFRAME;
    style = m_minimizable ? (style | WS_MINIMIZEBOX) : (style & ~WS_MINIMIZEBOX);
    // WS_MAXIMIZEBOX also governs Aero Snap: with it cleared, a window that
    // must not maximise cannot be snapped to the top edge either, which is the
    // behaviour a fixed dialog wants.
    style = m_maximizable ? (style | WS_MAXIMIZEBOX) : (style & ~WS_MAXIMIZEBOX);
    SetWindowLongPtr(hwnd, GWL_STYLE, style);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER
                     | SWP_FRAMECHANGED | SWP_NOACTIVATE);

    // One pixel of extended frame is what makes DWM draw the shadow. More than
    // that and the glass area starts showing through the client edges.
    MARGINS shadow = {1, 1, 1, 1};
    DwmExtendFrameIntoClientArea(hwnd, &shadow);

    const ThemeProfile& profile = themeFor(m_window);
    const ThemeColors& colors = profile.colors;

    // The immersive dark flag drives the colours DWM uses for the parts we do
    // not paint - the resize border while dragging, and the shadow.
    BOOL dark = qGray(colors.windowBg.rgb()) < 128 ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

    // The radius token, expressed in the only vocabulary DWM has. This is what
    // rounds a Novu window on Windows 11, and it rounds it properly - clipped
    // and antialiased by the compositor, with the shadow following the curve.
    // Doing it ourselves would need a translucent backing store, and that costs
    // sub-pixel antialiasing on every label in the window.
    // A maximised window is square against the screen edge, as in NovuHub.
    const bool maximised = m_window->isMaximized();
    const int radius = cornerRadius();
    int corner = (maximised || radius <= 0) ? 1   // DWMWCP_DONOTROUND
                : radius <= 5 ? 3                 // DWMWCP_ROUNDSMALL, ~4px
                              : 2;                // DWMWCP_ROUND, ~8px
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));

    // The one visible outline is DWM's, in the theme's window border token -
    // the same mechanism NovuHub uses. It follows the compositor's own curve,
    // stays antialiased at any DPI and keeps the shadow. Maximised, and with a
    // zero border width, there is no outline at all.
    constexpr COLORREF kNoDwmBorder = 0xFFFFFFFE;   // DWMWA_COLOR_NONE
    COLORREF borderColor = kNoDwmBorder;
    const QColor border = profile.window.border;
    if (!maximised && profile.window.borderWidth > 0 && border.isValid())
        borderColor = RGB(border.red(), border.green(), border.blue());
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
#endif
}

void WindowShell::syncStateFromWindow()
{
    m_titleBar->setMaximized(m_window->isMaximized());
    m_titleBar->setWindowActive(m_window->isActiveWindow());
}

bool WindowShell::eventFilter(QObject* watched, QEvent* event)
{
    if (watched != m_window) {
        return QObject::eventFilter(watched, event);
    }

    switch (event->type()) {
    case QEvent::WindowStateChange:
        syncStateFromWindow();
        // Margins and the corner preference both depend on whether the window
        // is maximised, so the frame is re-applied rather than only repainted.
        applyTheme();
        break;
    case QEvent::Resize:
        applyWindowMask();
        break;
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
        syncStateFromWindow();
        break;
    case QEvent::Show:
        // First point at which the platform window exists. Windows may still
        // recreate its non-client frame while completing Show, so apply the
        // DWM border and corner preference once now and once again on the next
        // event-loop turn after the final HWND style has settled.
        applyNativeFrame();
        QTimer::singleShot(0, m_window, [this] {
            applyNativeFrame();
            applyWindowMask();
            m_frame->update();
        });
        syncStateFromWindow();
        break;
    case QEvent::WindowTitleChange:
        // An application that already calls setWindowTitle() gets the caption
        // for free, which is what makes migrating a window a two-line change.
        m_titleBar->setTitle(m_window->windowTitle());
        break;
    case QEvent::WindowIconChange:
        m_titleBar->setIcon(m_window->windowIcon());
        break;
    default:
        if (event->type() == ThemeChangeEvent::type()) {
            applyTheme();
        }
        break;
    }
    return QObject::eventFilter(watched, event);
}

// -- AppWindow ---------------------------------------------------------------

AppWindow::AppWindow(QWidget* parent)
    : AppWindow(WindowKind::Main, parent)
{
}

AppWindow::AppWindow(WindowKind kind, QWidget* parent)
    : QWidget(parent)
    , m_shell(new WindowShell(this, kind))
{
}

TitleBar* AppWindow::titleBar() const
{
    return m_shell->titleBar();
}

QWidget* AppWindow::content() const
{
    return m_shell->content();
}

QVBoxLayout* AppWindow::contentLayout() const
{
    return m_shell->contentLayout();
}

void AppWindow::setContentWidget(QWidget* widget)
{
    m_shell->setContentWidget(widget);
}

void AppWindow::setTitle(const QString& title)
{
    m_shell->setTitle(title);
}

void AppWindow::setIcon(const QIcon& icon)
{
    m_shell->setIcon(icon);
}

ToolWindow::ToolWindow(QWidget* parent)
    : AppWindow(WindowKind::Tool, parent)
{
}

void ToolWindow::setRolled(bool rolled)
{
    if(m_rolled==rolled)return;
    m_rolled=rolled;
    if(rolled){
        m_unrolledSize=size();m_unrolledMinimum=minimumSize();m_unrolledMaximum=maximumSize();
        content()->hide();setMinimumSize(0,0);if(layout())layout()->activate();
        setFixedHeight(titleBar()->height()+shell()->frameOverhead().height()-themeFor(this).window.titleBarHeight);
    }else{
        setMaximumSize(m_unrolledMaximum);setMinimumSize(m_unrolledMinimum);content()->show();
        if(layout())layout()->activate();resize(m_unrolledSize);
    }
}
void ToolWindow::keyPressEvent(QKeyEvent* event)
{
    if(event->key()==Qt::Key_Escape){event->accept();close();return;}
    AppWindow::keyPressEvent(event);
}

// -- Dialog ------------------------------------------------------------------

// `.modal-body{padding:16px 18px;gap:14px}` - the modal's own metrics, and the
// one place they are stated.
//
// They used to be stated inside confirm()/inform()/reportError() and nowhere
// else, so those three had the design's padding and every hand-built Dialog had
// none: its content ran to the window frame and its buttons sat on the bottom
// edge. The shell's own content layout cannot carry this, because AppWindow
// shares it and a main window's content is not inset - so it belongs here, on
// the dialog.
//
// A caller that wants something else still calls setContentsMargins() after
// construction, as the helpers below do; this is a default, not a lock.
namespace {
constexpr int kDialogPaddingH = 18;
constexpr int kDialogPaddingV = 16;
constexpr int kDialogGap = 14;
}

Dialog::Dialog(QWidget* parent)
    : QDialog(parent)
    , m_shell(new WindowShell(this, WindowKind::Dialog))
{
    contentLayout()->setContentsMargins(kDialogPaddingH, kDialogPaddingV,
                                        kDialogPaddingH, kDialogPaddingV);
    contentLayout()->setSpacing(kDialogGap);
}

Dialog::Dialog(const QString& title, QWidget* parent)
    : Dialog(parent)
{
    setTitle(title);
}

TitleBar* Dialog::titleBar() const
{
    return m_shell->titleBar();
}

QWidget* Dialog::content() const
{
    return m_shell->content();
}

QVBoxLayout* Dialog::contentLayout() const
{
    return m_shell->contentLayout();
}

void Dialog::setContentWidget(QWidget* widget)
{
    m_shell->setContentWidget(widget);
}

void Dialog::setTitle(const QString& title)
{
    m_shell->setTitle(title);
}

void Dialog::setIcon(const QIcon& icon)
{
    m_shell->setIcon(icon);
}

namespace {

// The dialog head, painted.
//
// Three things that never change after they are set and that nobody can click:
// as widgets that is a framed label for the mark, a heading label and a wrapped
// note label, plus the layout to hold them — five objects and a wrapped
// QLabel's sizeHint, which is the single most common cause of a clipped dialog
// in this codebase's history.
class DialogHead : public QWidget
{
public:
    explicit DialogHead(QWidget* parent)
        : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    void setContent(const QString& heading, const QString& note, Dialog::Tone tone,
                    Glyph glyph)
    {
        m_heading = heading;
        m_note = note;
        m_tone = tone;
        m_glyph = glyph;
        updateGeometry();
        update();
    }

    int heightForWidth(int width) const override
    {
        const QFontMetrics headings(headingFont());
        int height = qMax(headings.height(), kMarkSide);
        if (!m_note.isEmpty()) {
            height += kNoteGap + noteHeight(width);
        }
        return height;
    }

    bool hasHeightForWidth() const override { return true; }

    QSize sizeHint() const override
    {
        const int width = qMax(320, QFontMetrics(headingFont()).horizontalAdvance(m_heading)
                                        + kMarkSide + kGap);
        return QSize(width, heightForWidth(width));
    }

    QSize minimumSizeHint() const override { return QSize(240, heightForWidth(240)); }

protected:
    void paintEvent(QPaintEvent*) override
    {
        const ThemeProfile& profile = themeFor(this);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        QColor ink = profile.colors.textDisabled;
        QColor plate = profile.colors.panelBgAlt;
        QColor outline = profile.colors.border;
        switch (m_tone) {
        case Dialog::Tone::Accent:
            ink = profile.colors.accentLight;
            plate = withAlpha(profile.colors.accent, 0.12);
            outline = withAlpha(profile.colors.accent, 0.45);
            break;
        case Dialog::Tone::Warning:
            ink = profile.semantic.warning.text;
            plate = profile.semantic.warning.fill;
            outline = profile.semantic.warning.border;
            break;
        case Dialog::Tone::Danger:
            ink = profile.semantic.danger.text;
            plate = profile.semantic.danger.fill;
            outline = profile.semantic.danger.border;
            break;
        case Dialog::Tone::Neutral:
            break;
        }

        const QRectF mark(0, 0, kMarkSide, kMarkSide);
        painter.setPen(QPen(outline, qMax(1, profile.border(BorderRole::Card))));
        painter.setBrush(plate);
        painter.drawRoundedRect(mark.adjusted(0.5, 0.5, -0.5, -0.5),
                                profile.radii.medium + 2, profile.radii.medium + 2);

        const Glyph glyph = m_glyph != Glyph::None ? m_glyph : defaultGlyph();
        if (glyph != Glyph::None) {
            const double inset = (kMarkSide - kMarkGlyph) / 2.0;
            paintGlyph(&painter, glyph, mark.adjusted(inset, inset, -inset, -inset), ink);
        }

        const int left = kMarkSide + kGap;
        const int available = qMax(0, width() - left);

        const QFont headings = headingFont();
        const QFontMetrics headingMetrics(headings);
        painter.setFont(headings);
        painter.setPen(profile.colors.textPrimary);
        painter.drawText(QRect(left, 0, available, qMax(headingMetrics.height(), kMarkSide)),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         headingMetrics.elidedText(m_heading, Qt::ElideRight, available));

        if (m_note.isEmpty()) {
            return;
        }
        QTextOption wrap(Qt::AlignLeft | Qt::AlignTop);
        wrap.setWrapMode(QTextOption::WordWrap);
        painter.setFont(noteFont());
        painter.setPen(profile.colors.textDisabled);
        painter.drawText(QRectF(left, qMax(headingMetrics.height(), kMarkSide) + kNoteGap,
                                available, noteHeight(width())),
                         m_note, wrap);
    }

    bool event(QEvent* event) override
    {
        if (event->type() == ThemeChangeEvent::type()
            || event->type() == QEvent::ParentChange) {
            updateGeometry();
            update();
        }
        return QWidget::event(event);
    }

private:
    // `.modal-head .mi { width:32px; height:32px }` with a 16px glyph.
    static constexpr int kMarkSide = 32;
    static constexpr int kMarkGlyph = 16;
    static constexpr int kGap = 12;
    static constexpr int kNoteGap = 6;
    static constexpr double kNoteLeading = 1.5;

    Glyph defaultGlyph() const
    {
        switch (m_tone) {
        case Dialog::Tone::Warning:
        case Dialog::Tone::Danger:  return Glyph::Warning;
        case Dialog::Tone::Accent:  return Glyph::Info;
        case Dialog::Tone::Neutral: break;
        }
        return Glyph::None;
    }

    QFont headingFont() const
    {
        const ThemeProfile& profile = themeFor(this);
        QFont font = roleFont(this, TypeRole::SectionTitle, true);
        font.setPixelSize(profile.type.lg - 1);
        return font;
    }

    QFont noteFont() const
    {
        const ThemeProfile& profile = themeFor(this);
        QFont font = roleFont(this, TypeRole::Caption);
        font.setPixelSize(profile.type.sm);
        return font;
    }

    int noteHeight(int width) const
    {
        const int available = qMax(0, width - kMarkSide - kGap);
        if (m_note.isEmpty() || available <= 0) {
            return 0;
        }
        const QFontMetrics fm(noteFont());
        const QRect bounds = fm.boundingRect(QRect(0, 0, available, 10000),
                                             Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
                                             m_note);
        const int lines = qMax(1, qRound(double(bounds.height()) / qMax(1, fm.height())));
        return qRound(fm.height() * kNoteLeading) * lines;
    }

    QString m_heading;
    QString m_note;
    Dialog::Tone m_tone = Dialog::Tone::Neutral;
    Glyph m_glyph = Glyph::None;
};

} // namespace

void Dialog::setHeading(const QString& heading, const QString& note, Tone tone, Glyph glyph)
{
    m_heading = heading;
    m_note = note;
    m_tone = tone;
    m_glyph = glyph;

    if (!m_head) {
        m_head = new DialogHead(content());
        // At the top, above whatever the caller has already put in. Calling
        // this after filling the body is the common order and must work.
        contentLayout()->insertWidget(0, m_head);
    }
    static_cast<DialogHead*>(m_head)->setContent(heading, note, tone, glyph);
}

QHBoxLayout* Dialog::buttonRow()
{
    if (m_buttonRow) {
        return m_buttonRow;
    }
    const ThemeProfile& profile = themeFor(this);
    m_buttonRow = new QHBoxLayout();
    m_buttonRow->setContentsMargins(0, 0, 0, 0);
    m_buttonRow->setSpacing(profile.space(SpacingRole::ToolbarGap));
    // Right-aligned, always. A dialog whose buttons sit somewhere else is the
    // one users click wrong.
    m_buttonRow->addStretch(1);
    contentLayout()->addLayout(m_buttonRow);
    return m_buttonRow;
}

void Dialog::keyPressEvent(QKeyEvent* event)
{
    // A request already in flight cannot be cancelled by closing the window
    // that sent it. Swallowing Escape while busy is the cheapest way to stop a
    // user leaving a dialog whose reply is about to arrive for nobody.
    if (event->key() == Qt::Key_Escape && m_busy) {
        event->accept();
        return;
    }
    QDialog::keyPressEvent(event);
}

void Dialog::setBusy(bool busy, const QString& what)
{
    if (m_busy == busy && what.isEmpty()) {
        return;
    }
    m_busy = busy;

    // Every button in the footer, not only the primary one. A Cancel that still
    // works while a request is in flight is a Cancel that leaves the caller
    // holding a reply for a dialog that has gone.
    if (m_buttonRow) {
        for (int i = 0; i < m_buttonRow->count(); ++i) {
            if (QWidget* widget = m_buttonRow->itemAt(i)->widget()) {
                widget->setEnabled(!busy);
            }
        }
    }

    if (busy) {
        if (!m_busyMark) {
            auto* row = buttonRow();
            auto* holder = new QWidget(content());
            auto* line = new QHBoxLayout(holder);
            line->setContentsMargins(0, 0, 0, 0);
            line->setSpacing(themeFor(this).space(SpacingRole::ToolbarGap));
            line->addWidget(new Spinner(ControlSize::Small, holder));
            QLabel* caption = label(QString(), holder, TypeRole::Caption, Ink::Muted);
            caption->setObjectName(QStringLiteral("novuDialogBusyText"));
            line->addWidget(caption);
            // Before the stretch, so it sits on the left of the footer and the
            // buttons stay where they were - a spinner that pushes the buttons
            // sideways is one the user's hand has to chase.
            m_buttonRow->insertWidget(0, holder);
            m_busyMark = holder;
            Q_UNUSED(row);
        }
        if (auto* caption = m_busyMark->findChild<QLabel*>(
                QStringLiteral("novuDialogBusyText"))) {
            caption->setText(what);
        }
        m_busyMark->show();
        // A request in flight cannot be cancelled by closing the window that
        // sent it, so Escape stops working while it is.
        clearFormError();
    } else if (m_busyMark) {
        m_busyMark->hide();
    }
}

void Dialog::showFormError(const QString& message)
{
    if (m_formError == message) {
        return;
    }
    m_formError = message;

    if (message.isEmpty()) {
        if (m_errorBanner) {
            m_errorBanner->hide();
        }
        return;
    }

    // A dialog cannot be both reporting a failure and still working: the
    // failure is the answer to the work.
    if (m_busy) {
        setBusy(false);
    }

    if (!m_errorBanner) {
        auto* banner = new InfoBanner(message, Badge::Tone::Danger, content());
        // Directly above the footer, which is where the eye goes after pressing
        // a button that did not do what it said.
        const int before = contentLayout()->count() - (m_buttonRow ? 1 : 0);
        contentLayout()->insertWidget(qMax(0, before), banner);
        m_errorBanner = banner;
    } else {
        static_cast<InfoBanner*>(m_errorBanner)->setText(message);
    }
    m_errorBanner->show();
}

void Dialog::clearFormError()
{
    showFormError(QString());
}

namespace {

// The body every message dialog shares: one padded column with a message and
// a button row. Kept here rather than in three near-identical static methods.
Dialog* buildMessageDialog(QWidget* parent, const QString& title, const QString& message,
                           Ink tone)
{
    auto* dialog = new Dialog(title, parent);
    const ThemeProfile& profile = themeFor(dialog);
    // `.modal-body{padding:16px 18px;gap:14px}` - the modal's own metrics.
    // The gap was SectionGap, which NovuSpace states as the *rail's* 18.
    // The padding and the gap come from the constructor now. Left here as a
    // comment rather than repeated: two copies of one measurement is how the
    // three helpers ended up being the only dialogs that had it.

    QLabel* body = label(message, dialog->content(), TypeRole::Body, tone);
    body->setWordWrap(true);
    dialog->contentLayout()->addWidget(body);
    dialog->contentLayout()->addStretch(1);
    return dialog;
}

} // namespace

bool Dialog::confirm(QWidget* parent, const QString& title, const QString& message,
                     const QString& confirmText, const QString& cancelText,
                     bool destructive)
{
    Dialog* dialog = buildMessageDialog(parent, title, message, Ink::Default);
    dialog->setModal(true);

    Button* cancel = secondaryButton(
        cancelText.isEmpty() ? QObject::tr("Cancel") : cancelText, dialog->content());
    Button* accept = destructive
        ? dangerButton(confirmText.isEmpty() ? QObject::tr("Delete") : confirmText,
                       dialog->content())
        : primaryButton(confirmText.isEmpty() ? QObject::tr("OK") : confirmText,
                        dialog->content());
    dialog->buttonRow()->addWidget(cancel);
    dialog->buttonRow()->addWidget(accept);
    connect(cancel, &QAbstractButton::clicked, dialog, &QDialog::reject);
    connect(accept, &QAbstractButton::clicked, dialog, &QDialog::accept);

    // A destructive confirmation must not answer to Return. A user who hits
    // Enter to dismiss a dialog they have not read would otherwise delete the
    // thing the dialog was asking about — which makes the confirmation worse
    // than useless, because it trained them to expect one.
    if (destructive) {
        cancel->setDefault(true);
    } else {
        accept->setDefault(true);
    }

    dialog->shell()->setContentSize(QSize(360, 140));
    const bool confirmed = dialog->exec() == QDialog::Accepted;
    dialog->deleteLater();
    return confirmed;
}

void Dialog::inform(QWidget* parent, const QString& title, const QString& message)
{
    Dialog* dialog = buildMessageDialog(parent, title, message, Ink::Default);
    dialog->setModal(true);
    Button* ok = primaryButton(QObject::tr("OK"), dialog->content());
    dialog->buttonRow()->addWidget(ok);
    connect(ok, &QAbstractButton::clicked, dialog, &QDialog::accept);
    ok->setDefault(true);
    dialog->shell()->setContentSize(QSize(360, 130));
    dialog->exec();
    dialog->deleteLater();
}

void Dialog::reportError(QWidget* parent, const QString& title, const QString& message)
{
    // Danger ink on the message, not a red title bar: the window is not the
    // error, the sentence is.
    Dialog* dialog = buildMessageDialog(parent, title, message, Ink::Danger);
    dialog->setModal(true);
    Button* ok = secondaryButton(QObject::tr("Close"), dialog->content());
    dialog->buttonRow()->addWidget(ok);
    connect(ok, &QAbstractButton::clicked, dialog, &QDialog::accept);
    ok->setDefault(true);
    dialog->shell()->setContentSize(QSize(380, 140));
    dialog->exec();
    dialog->deleteLater();
}

// == one frame for every Novu window ===========================================

namespace {

// The compositor's rounded corner exists from Windows 11 on.
bool roundingCompositor()
{
#ifdef Q_OS_WIN
    static const bool available =
        QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows11;
    return available;
#else
    return false;
#endif
}

#ifdef Q_OS_WIN
// The same three settings applyNativeFrame() gives the main window.
void applyCompositorFrameTo(QWidget* popup)
{
    const QWidget* context = popup->parentWidget() ? popup->parentWidget() : popup;
    const ThemeProfile& profile = themeFor(context);
    const HWND hwnd = reinterpret_cast<HWND>(popup->winId());
    if (!hwnd)
        return;
    BOOL dark = qGray(profile.colors.windowBg.rgb()) < 128 ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    const int radius = profile.radius(SurfaceRole::Window);
    int corner = radius <= 0 ? 1 : radius <= 5 ? 3 : 2;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));
    COLORREF borderColor = 0xFFFFFFFE;   // DWMWA_COLOR_NONE
    const QColor border = profile.window.border;
    if (profile.window.borderWidth > 0 && border.isValid())
        borderColor = RGB(border.red(), border.green(), border.blue());
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
}

// Applies it whenever the popup's native window is (re)created and shown.
class CompositorFrameWatch final : public QObject
{
public:
    using QObject::QObject;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::Show || event->type() == QEvent::WinIdChange) {
            if (auto* widget = qobject_cast<QWidget*>(watched))
                applyCompositorFrameTo(widget);
        }
        return QObject::eventFilter(watched, event);
    }
};
#endif

} // namespace

bool compositorDrawsWindowFrame()
{
    return roundingCompositor();
}

int windowCornerRadius(const QWidget* context)
{
    const int token = themeFor(context).radius(SurfaceRole::Window);
    if (!roundingCompositor())
        return token;
    // What DWMWCP_ROUND / ROUNDSMALL draw, so a panel inside the window has
    // the window's own corner.
    return token <= 0 ? 0 : token <= 5 ? 4 : 8;
}

QColor windowBorderColor(const QWidget* context)
{
    const QColor border = themeFor(context).window.border;
    return border.isValid() ? border : themeFor(context).colors.border;
}

int windowBorderWidth(const QWidget* context)
{
    return qMax(1, themeFor(context).window.borderWidth);
}

bool useCompositorFrame(QWidget* popup)
{
#ifdef Q_OS_WIN
    if (!popup || !roundingCompositor())
        return false;
    popup->installEventFilter(new CompositorFrameWatch(popup));
    return true;
#else
    Q_UNUSED(popup);
    return false;
#endif
}

} // namespace novu::ui
