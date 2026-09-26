#include "novu/ui/PopupSurface.h"

#include "novu/ui/Popover.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"

#include <QApplication>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QVBoxLayout>

namespace novu::ui {

PopupSurface::PopupSurface(QWidget* parent)
    : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFocusPolicy(Qt::StrongFocus);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(1, 1, 1, 1);
    layout->setSpacing(0);
}

void PopupSurface::setContent(QWidget* content)
{
    if (m_content == content) {
        return;
    }
    if (m_content) {
        layout()->removeWidget(m_content);
        m_content->deleteLater();
    }
    m_content = content;
    if (m_content) {
        m_content->setParent(this);
        layout()->addWidget(m_content);
    }
}

void PopupSurface::dismissForeignPopup()
{
    QWidget* active = QApplication::activePopupWidget();
    if (!active || active == this) {
        return;
    }
    // Opening one transient surface closes another, whichever kind it is.
    if (auto* menu = qobject_cast<Popover*>(active)) {
        menu->hide();
    } else {
        active->close();
    }
}

void PopupSurface::placeAt(const QPoint& topLeft, const QSize& size)
{
    const QScreen* screen = QGuiApplication::screenAt(topLeft);
    const QRect bounds = (screen ? screen : QGuiApplication::primaryScreen())
                             ->availableGeometry();

    const int x = qBound(bounds.left() + 4, topLeft.x(),
                         qMax(bounds.left() + 4, bounds.right() - size.width() - 4));
    int y = topLeft.y();
    if (y + size.height() > bounds.bottom() - 4) {
        y = qMax(bounds.top() + 4, y - size.height());
    }

    move(x, y);
    show();
    raise();
    setFocus(Qt::PopupFocusReason);
}

void PopupSurface::popupAt(const QPoint& globalPosition)
{
    dismissForeignPopup();
    const QSize size = sizeHint();
    setFixedSize(size);
    placeAt(globalPosition, size);
}

void PopupSurface::popupUnder(QWidget* anchor)
{
    if (!anchor) {
        return;
    }
    dismissForeignPopup();

    const QSize size = sizeHint();
    setFixedSize(size);

    const QRect box(anchor->mapToGlobal(QPoint(0, 0)), anchor->size());
    // Right-aligned to the anchor: these hang off a titlebar control near the
    // window's right edge, and left-aligning would push them off screen.
    placeAt(QPoint(box.right() - size.width(), box.bottom() + 4), size);
}

void PopupSurface::mousePressEvent(QMouseEvent* event)
{
    // A grabbing popup receives presses that land anywhere. Accepting one that
    // fell outside is what would leave this surface open over the application.
    if (!rect().contains(event->position().toPoint())) {
        hide();
        event->ignore();
        return;
    }
    QWidget::mousePressEvent(event);
}

void PopupSurface::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        hide();
        return;
    }
    QWidget::keyPressEvent(event);
}

void PopupSurface::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    emit closed();
}

bool PopupSurface::event(QEvent* event)
{
    if (event->type() == QEvent::WindowDeactivate) {
        // The signal that still arrives when the platform grab does not reach
        // the click - a native child surface taking focus, another
        // application being raised.
        QWidget* active = QApplication::activePopupWidget();
        if (active == this || active == nullptr) {
            hide();
        }
    }
    return QWidget::event(event);
}

void PopupSurface::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = ThemeManager::instance().defaultProfile();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush(profile.colors.panelBg);
    painter.setPen(profile.colors.border);
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 10, 10);
}

} // namespace novu::ui
