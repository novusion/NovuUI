#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/Icon.h"
#include "novu/ui/ThemeProfile.h"
#include "novu/ui/TitleBar.h"

#include <QDialog>
#include <QIcon>
#include <QObject>
#include <QSize>
#include <QString>
#include <QWidget>

class QHBoxLayout;
class QVBoxLayout;

namespace novu::ui {

// What a top-level window is, semantically. The shell turns this into flags,
// capabilities and a radius role; an application never spells any of those out.
enum class WindowKind {
    Main,     // the application's own window: resizable, minimizable, maximizable
    Tool,     // a detached panel or utility window: resizable, no minimize/maximize
    Dialog,   // Settings, Confirm, Import, Error, a picker: Close only
};

// The Novu window frame, installed onto a top-level widget.
//
// Not a widget itself, and not a base class, on purpose: a dialog has to be a
// QDialog (exec(), accept(), modality, default button handling) and a main
// window has to be a QWidget or QMainWindow. C++ gives one base, so a shared
// base class would force one of the two into the wrong hierarchy - which is
// precisely how the applications ended up with three separate title bars.
// WindowShell is composed instead, so AppWindow, ToolWindow and Dialog are
// thin classes that all wear the identical frame.
//
// The shell owns, and an application may not draw for itself:
//
//   - the window background, border and corner radius
//   - the title bar, its caption and its icon
//   - the three window controls and their geometry
//   - dragging by the title bar, double-click maximize/restore
//   - resize behaviour and the frame's hit-testing
//   - the active/inactive treatment of caption and glyphs
//
// An application says what the window *is* - title, icon, resizable,
// minimizable, maximizable, modal - and fills content(). There is deliberately
// no accessor that hands out a mutable control, and no way to set a control's
// size: "the application picked its own" is the defect being removed.
//
// Platform note. On Windows the shell restores the native thick frame under
// its own chrome and answers WM_NCCALCSIZE / WM_NCHITTEST itself, so resizing,
// Aero Snap, the drop shadow and the taskbar all keep working while nothing
// native is drawn. Corner radius and border colour are handed to DWM, which
// is what makes a Novu window round on Windows 11 without a translucent
// backing store - translucency would cost sub-pixel text antialiasing on every
// label in the window. Elsewhere the frame falls back to Qt's own
// startSystemResize(); the appearance is identical, and the grab band sits
// inside the frame instead of outside it.
class NOVUUI_API WindowShell : public QObject
{
    Q_OBJECT

public:
    // `window` must be a top-level widget. The shell parents itself to it and
    // lives exactly as long as it does.
    WindowShell(QWidget* window, WindowKind kind);
    ~WindowShell() override;

    QWidget* window() const { return m_window; }
    WindowKind kind() const { return m_kind; }
    TitleBar* titleBar() const { return m_titleBar; }

    // The one slot an application fills. Everything above and around it is the
    // shell's; a page placed in here must not draw a title bar of its own.
    QWidget* content() const { return m_content; }
    QVBoxLayout* contentLayout() const { return m_contentLayout; }
    // Places a single widget as the whole content. Replacing what is there
    // deletes the previous one, which is the behaviour a window wants when it
    // swaps pages.
    void setContentWidget(QWidget* widget);

    void setTitle(const QString& title);
    QString title() const;
    void setIcon(const QIcon& icon);

    // Semantic capabilities. Each hides its control and updates the window's
    // flags together, so the system menu and the taskbar agree with the bar.
    void setResizable(bool resizable);
    bool isResizable() const { return m_resizable; }
    void setMinimizable(bool minimizable);
    bool isMinimizable() const { return m_minimizable; }
    void setMaximizable(bool maximizable);
    bool isMaximizable() const { return m_maximizable; }

    // Sizes the content, not the frame: a caller asking for 900x600 means its
    // page, and should not have to know how tall this profile's title bar is.
    void setContentSize(const QSize& size);
    // What the frame adds around the content - title bar, border and, on the
    // platforms that need it, the resize band.
    QSize frameOverhead() const;

    // Window for Main and Tool, Dialog for Dialog. Both come from the profile,
    // so a product changes the shape of every window it has by setting one
    // role.
    SurfaceRole surfaceRole() const;
    int cornerRadius() const;

    // The shell installed on this window, or nullptr. Lets a page ask whether
    // it is inside Novu chrome without an application-wide singleton.
    static WindowShell* of(const QWidget* window);

public slots:
    void toggleMaximizeRestore();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void applyWindowFlags();
    void applyTheme();
    void applyNativeFrame();
    void applyWindowMask();
    void syncStateFromWindow();

    QWidget* m_window = nullptr;
    WindowKind m_kind = WindowKind::Main;
    QWidget* m_frame = nullptr;          // paints the surface; see the .cpp
    TitleBar* m_titleBar = nullptr;
    QWidget* m_content = nullptr;
    QVBoxLayout* m_contentLayout = nullptr;

    bool m_resizable = true;
    bool m_minimizable = true;
    bool m_maximizable = true;

    class NativeFrame;                   // Windows only; empty elsewhere
    NativeFrame* m_native = nullptr;
};

// A main application window wearing the Novu frame.
//
//     AppWindow window;
//     window.setTitle(tr("NovuExplorer"));
//     window.setIcon(QIcon(":/icons/NovuExplorer.png"));
//     window.setContentWidget(new ExplorerWorkspace);
//     window.show();
//
// That is the entire integration. An application that also wants a menu bar
// puts it in the title bar's leading slot rather than under it:
//
//     window.titleBar()->setLeading(menuBar);
class NOVUUI_API AppWindow : public QWidget
{
    Q_OBJECT

public:
    explicit AppWindow(QWidget* parent = nullptr);

    WindowShell* shell() const { return m_shell; }
    TitleBar* titleBar() const;
    QWidget* content() const;
    QVBoxLayout* contentLayout() const;
    void setContentWidget(QWidget* widget);

    // Sets the caption in the title bar and the window title the OS knows
    // about, which is what the taskbar and Alt-Tab show.
    void setTitle(const QString& title);
    void setIcon(const QIcon& icon);

protected:
    // For ToolWindow, which is the same window with a different semantic.
    AppWindow(WindowKind kind, QWidget* parent);

private:
    WindowShell* m_shell = nullptr;
};

// A detached panel or utility window: resizable, but neither minimized nor
// maximized on its own. Same bar height, same control size as the main window.
class NOVUUI_API ToolWindow : public AppWindow
{
    Q_OBJECT

public:
    explicit ToolWindow(QWidget* parent = nullptr);
    // Collapse to the title bar while retaining the content and restore size.
    void setRolled(bool rolled);
    bool isRolled() const { return m_rolled; }
protected:
    void keyPressEvent(QKeyEvent* event) override;
private:
    bool m_rolled=false;
    QSize m_unrolledSize,m_unrolledMinimum,m_unrolledMaximum;

};

// Every Novu dialog: Settings, Confirm, Import, Error, Create/Edit,
// Authentication, the file picker's window.
//
// A QDialog, so exec(), accept(), reject() and modality work exactly as
// callers expect - what changes is that it is never drawn by the platform.
// Minimize and Maximize are hidden by default while the bar keeps the same
// height and the same Close box, so a dialog and the window behind it read as
// the same product.
class NOVUUI_API Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget* parent = nullptr);
    Dialog(const QString& title, QWidget* parent = nullptr);

    // What kind of question this is, before the title is read. The approved
    // designs' `.modal-head .mi` and its `.acc` / `.warn` / `.bad` variants.
    enum class Tone { Neutral, Accent, Warning, Danger };

    WindowShell* shell() const { return m_shell; }
    TitleBar* titleBar() const;
    QWidget* content() const;
    QVBoxLayout* contentLayout() const;
    void setContentWidget(QWidget* widget);

    void setTitle(const QString& title);
    void setIcon(const QIcon& icon);

    // The head the approved designs put above every dialog body: a tone-marked
    // icon, a heading, and a sentence under it saying what the question means.
    //
    // Separate from the title bar's caption, and deliberately so. The caption
    // names the *window* — it is what appears in the task bar and what a user
    // sees when the dialog is behind something. The head is the *question*, and
    // it has room for a sentence the caption does not.
    //
    // Both NovuSpace and NovuAI draw this and NovuAdmin drew a third copy of
    // it, which is why it is here: the three disagreed on the icon plate, on
    // whether the note wrapped, and on the gap between the head and the body.
    //
    // Created on first call and placed at the top of the content, above
    // whatever is already there. Called again, it updates rather than adding a
    // second head.
    void setHeading(const QString& heading, const QString& note = {},
                    Tone tone = Tone::Neutral, Glyph glyph = Glyph::None);
    QString heading() const { return m_heading; }
    QString note() const { return m_note; }
    Tone tone() const { return m_tone; }

    // The footer every dialog ends with, created on first use and always in
    // the same place: right-aligned, below the content, with the profile's
    // toolbar gap between buttons. Without it each dialog invents its own
    // button row and they stop lining up.
    QHBoxLayout* buttonRow();

    // ── submitting ───────────────────────────────────────────────────────
    //
    // The state a form dialog is in between the user pressing the button and
    // the server answering. NovuAdmin had to write this by hand for every one
    // of its seventeen forms, and the halves that were forgotten are the ones
    // that cost: a dialog that does not disable its own submit is a dialog that
    // creates two records when the network is slow.
    //
    // Busy disables every button in the footer, shows a spinner beside the
    // primary one, and stops Escape closing the window — a request already in
    // flight cannot be cancelled by closing the thing that sent it.
    bool isBusy() const { return m_busy; }
    void setBusy(bool busy, const QString& what = {});

    // The error the server sent back, shown above the footer in the danger
    // tone. Empty clears it. Setting one always clears busy, because a dialog
    // that is both reporting a failure and still working is reporting about
    // something that already finished.
    void showFormError(const QString& message);
    void clearFormError();
    QString formError() const { return m_formError; }
    bool hasFormError() const { return !m_formError.isEmpty(); }

protected:
    void keyPressEvent(QKeyEvent* event) override;
    // Never taller than the window it belongs to, nor than the screen: a long
    // body scrolls inside the dialog, the dialog does not run off the window.
    void showEvent(QShowEvent* event) override;

public:

    // Drop-in replacements for the QMessageBox calls that are the last native
    // windows left in most applications. Same shell, same chrome.
    //
    // `destructive` styles the affirmative button as the danger variant, for
    // "Delete 12 files" rather than "Save".
    static bool confirm(QWidget* parent, const QString& title, const QString& message,
                        const QString& confirmText = QString(),
                        const QString& cancelText = QString(),
                        bool destructive = false);
    static void inform(QWidget* parent, const QString& title, const QString& message);
    static void reportError(QWidget* parent, const QString& title, const QString& message);

private:
    WindowShell* m_shell = nullptr;
    QHBoxLayout* m_buttonRow = nullptr;
    QWidget* m_head = nullptr;
    QString m_heading;
    QString m_note;
    Tone m_tone = Tone::Neutral;
    Glyph m_glyph = Glyph::None;

    QWidget* m_errorBanner = nullptr;
    QWidget* m_busyMark = nullptr;
    QString m_formError;
    bool m_busy = false;
};

} // namespace novu::ui
