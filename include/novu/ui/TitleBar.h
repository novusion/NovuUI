#pragma once

#include "novu/ui/ThemeProfile.h"

#include <QAbstractButton>
#include <QIcon>
#include <QList>
#include <QString>
#include <QWidget>

class QHBoxLayout;
class QLabel;

namespace novu::ui {

// One window control. Minimize, Maximize/Restore and Close are this class with
// a different glyph — not three buttons that happen to look alike.
//
// This is the whole point of the component. Before it existed NovuExplorer
// drew 46x32 flat QPushButtons, NovuHub drew stylesheet buttons with no size
// at all, and NovuExplorer's popup drew a 22x22 close button with a lowercase
// "x" in it. Three implementations, three sizes, three hover treatments, and
// a Close button that was permanently tinted in one of them.
//
// Geometry comes from WindowTokens and is not settable. A caller chooses which
// control this is and nothing else; there is deliberately no setFixedSize()
// path for an application to reach, because "the application picked its own
// size" is the defect being removed.
//
// Self-painted rather than styled: the glyphs are straight-line drawings that
// must stay crisp at 125% and 150% scaling, and a font glyph ("✕", "□") is at
// the mercy of whatever font the system substitutes.
class WindowControlButton : public QAbstractButton
{
    Q_OBJECT

public:
    enum class Control {
        Minimize,
        Maximize,
        Restore,   // what Maximize becomes once the window is maximized
        Close,
    };

    explicit WindowControlButton(Control control, QWidget* parent = nullptr);

    Control control() const { return m_control; }
    void setControl(Control control);

    // An inactive window dims its glyphs, the same way its title dims. Set by
    // the shell from the window's activation state; nothing else touches it.
    bool isWindowActive() const { return m_windowActive; }
    void setWindowActive(bool active);

    // Both are the token box, identically, for all four controls.
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    bool event(QEvent* event) override;

private:
    void applyTheme();

    Control m_control;
    bool m_hovered = false;
    bool m_windowActive = true;
};

// The title bar every Novu window and every Novu dialog wears.
//
// It owns the icon, the caption, the three controls and the drag gesture. A
// page never builds one of these and never puts a title of its own above its
// content — if a window needs a title, the shell already drew it.
//
// Applications may add their own widgets to two slots: `leading`, for a menu
// bar, and `trailing`, for a status chip or a search field. Both sit inside
// the bar's own geometry; neither can move, resize or restyle the controls.
//
// The bar reports gestures rather than performing them. It does not call
// showMinimized() on its window: WindowShell connects the signals, because the
// shell is what knows whether this window may maximize and what "close" means
// for a modal dialog.
class TitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(QWidget* parent = nullptr);

    void setTitle(const QString& title);
    QString title() const;

    void setIcon(const QIcon& icon);
    QIcon icon() const { return m_iconSource; }

    // Capabilities hide a control while leaving every other measurement
    // untouched: a dialog with only Close has the same bar height, the same
    // caption baseline and the same Close box as the main window behind it.
    void setMinimizable(bool enabled);
    bool isMinimizable() const { return m_minimizable; }
    void setMaximizable(bool enabled);
    bool isMaximizable() const { return m_maximizable; }
    void setClosable(bool enabled);
    bool isClosable() const { return m_closable; }

    // Swaps the middle control between Maximize and Restore. Driven by the
    // shell from the real window state, never guessed from the last click.
    void setMaximized(bool maximized);
    bool isMaximized() const { return m_maximized; }

    void setWindowActive(bool active);
    bool isWindowActive() const { return m_windowActive; }

    // The frame's own corner radius, so the bar's background can round its
    // own top corners to match rather than squaring them off over what
    // WindowFrame already painted underneath it. 0 for a maximised window,
    // which has no curve at all. The shell is the one thing that knows this
    // number - see WindowShell::cornerRadius() - so it is pushed in rather
    // than guessed here from a token the bar has no way to pick correctly on
    // its own (Window and Dialog do not share one radius).
    void setCornerRadius(int radius);
    int cornerRadius() const { return m_cornerRadius; }

    // A menu bar, typically. Reparented into the bar; the caller keeps
    // ownership of what it hands over.
    void setLeading(QWidget* widget);
    QWidget* leading() const { return m_leading; }

    // A widget centred in the bar rather than packed against an edge. The
    // caption keeps the leftover space when there is none; when there is one,
    // the centre takes it and the caption stays at its natural width, so the
    // centred thing is centred on the WINDOW and not on what is left over.
    // NovuAI's command field is the case this exists for.
    void setCentre(QWidget* widget);
    // Fraction of the spare space before the centre slot (0..1).
    // Negative restores the default of centring on the whole window.
    void setCentrePosition(qreal proportion);
    QWidget* centre() const { return m_centre; }

    // Widgets between the caption and the controls. A list rather than one
    // slot for the same reason Card::setHeaderTrailing takes a list: one slot
    // does not survive a window that wants both a sync indicator and a
    // profile chip, and wrapping them creates a container that exists only to
    // hold two things in a row.
    void setTrailing(const QList<QWidget*>& widgets);

    WindowControlButton* minimizeButton() const { return m_minimize; }
    WindowControlButton* maximizeButton() const { return m_maximize; }
    WindowControlButton* closeButton() const { return m_close; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void minimizeRequested();
    void maximizeRestoreRequested();
    void closeRequested();
    // A press landed on draggable bar surface. The shell turns this into a
    // native window move; the bar does not move its own window, because a
    // manual move loses snap on Windows and is not permitted at all on
    // Wayland.
    void dragRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool event(QEvent* event) override;

private:
    void applyTheme();
    void layoutCentre();
    void updateIconPixmap();
    void updateBrandVisibility();
    // The caption is elided by the bar, not by the label: a window title is
    // whatever the application named the document, and a path that does not
    // fit must shorten rather than push the controls off the frame.
    void updateElidedTitle();

    QHBoxLayout* m_row = nullptr;
    // The mark and the leading widget, in a row of their own so the gap
    // between them can differ from the bar's. See WindowTokens::brandGap.
    QWidget* m_brand = nullptr;
    QHBoxLayout* m_brandRow = nullptr;
    QLabel* m_iconLabel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QWidget* m_leading = nullptr;
    QWidget* m_centre = nullptr;
    qreal m_centrePosition = -1;
    QList<QWidget*> m_trailing;

    WindowControlButton* m_minimize = nullptr;
    WindowControlButton* m_maximize = nullptr;
    WindowControlButton* m_close = nullptr;

    QIcon m_iconSource;
    QString m_titleText;
    bool m_minimizable = true;
    bool m_maximizable = true;
    bool m_closable = true;
    bool m_maximized = false;
    bool m_windowActive = true;
    int m_cornerRadius = 0;
};

} // namespace novu::ui
