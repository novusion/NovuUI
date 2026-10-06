#pragma once

#include "novu/ui/Export.h"

// A transient surface that holds a widget.
//
// Popover is a menu - items, separators, submenus - and FloatingPanel is
// deliberately persistent. Between them sat a gap: a panel of arbitrary
// content that still behaves like a menu, closing on an outside click, on
// Escape, and when another popup opens. Anything needing that was writing its
// own Qt::Popup widget and, with it, its own half of the dismissal rules.
//
// The dismissal here is the same as Popover's, and for the same reason: a
// popup with a mouse grab is handed presses that land anywhere, so a press
// outside must be left unaccepted rather than swallowed, and window
// deactivation is the only signal that still arrives when a native child
// surface such as a web view takes the click.

#include <QWidget>

namespace novu::ui {

class NOVUUI_API PopupSurface : public QWidget
{
    Q_OBJECT

public:
    explicit PopupSurface(QWidget* parent = nullptr);

    // Takes ownership and sizes the surface to it.
    void setContent(QWidget* content);
    QWidget* content() const { return m_content; }

    // Below an anchor widget, flipped above it when there is no room, and
    // kept on screen either way.
    void popupUnder(QWidget* anchor);
    void popupAt(const QPoint& globalPosition);

signals:
    void closed();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    bool event(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void dismissForeignPopup();
    void placeAt(const QPoint& topLeft, const QSize& size);

    QWidget* m_content = nullptr;
};

} // namespace novu::ui
