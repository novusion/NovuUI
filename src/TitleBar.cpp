#include "novu/ui/TitleBar.h"

#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>

namespace novu::ui {

namespace {

// The glyph box, centred in the control and snapped to whole pixels.
//
// Snapped rather than centred in floating point because every glyph here is
// built from axis-aligned bars: a half-pixel origin turns a one-pixel line
// into two grey ones, which at 100% scaling is the difference between a
// window control and a smudge.
QRect glyphBox(const QRect& bounds, int size)
{
    const int x = bounds.x() + (bounds.width() - size) / 2;
    const int y = bounds.y() + (bounds.height() - size) / 2;
    return QRect(x, y, size, size);
}

// A rectangle outline drawn as four filled bars.
//
// Not drawRect() with a pen: a pen straddles the path, so a one-pixel border
// lands on half-pixels and antialiases into two rows of grey. Filled bars are
// exactly the pixels asked for.
void strokeRect(QPainter& painter, const QRect& box, int stroke, const QColor& colour)
{
    painter.fillRect(QRect(box.left(), box.top(), box.width(), stroke), colour);
    painter.fillRect(QRect(box.left(), box.bottom() - stroke + 1, box.width(), stroke), colour);
    painter.fillRect(QRect(box.left(), box.top(), stroke, box.height()), colour);
    painter.fillRect(QRect(box.right() - stroke + 1, box.top(), stroke, box.height()), colour);
}

} // namespace

// -- WindowControlButton ----------------------------------------------------

WindowControlButton::WindowControlButton(Control control, QWidget* parent)
    : QAbstractButton(parent)
    , m_control(control)
{
    // A window control never takes focus. Tabbing into Close and closing the
    // window with Space is not a gesture any of these applications wants.
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setAttribute(Qt::WA_Hover, true);
    // Deliberately the arrow, not a pointing hand: these read as window
    // furniture rather than as content, and every other frame on the desktop
    // agrees.
    setCursor(Qt::ArrowCursor);
    applyTheme();
}

void WindowControlButton::setControl(Control control)
{
    if (m_control == control) {
        return;
    }
    m_control = control;
    update();
}

void WindowControlButton::setWindowActive(bool active)
{
    if (m_windowActive == active) {
        return;
    }
    m_windowActive = active;
    update();
}

QSize WindowControlButton::sizeHint() const
{
    const WindowTokens& w = themeFor(this).window;
    return QSize(w.controlWidth, w.controlHeight);
}

void WindowControlButton::enterEvent(QEnterEvent* event)
{
    m_hovered = true;
    update();
    QAbstractButton::enterEvent(event);
}

void WindowControlButton::leaveEvent(QEvent* event)
{
    m_hovered = false;
    update();
    QAbstractButton::leaveEvent(event);
}

bool WindowControlButton::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type()
        || event->type() == QEvent::PolishRequest
        || event->type() == QEvent::ParentChange) {
        applyTheme();
    }
    return QAbstractButton::event(event);
}

void WindowControlButton::applyTheme()
{
    // Geometry only - the colours are read at paint time, so a live profile
    // change is a repaint rather than a rebuild.
    updateGeometry();
    update();
}

void WindowControlButton::paintEvent(QPaintEvent*)
{
    const WindowTokens& w = themeFor(this).window;

    QPainter painter(this);

    const bool pressed = isDown();
    const bool hovered = m_hovered || pressed;
    const bool danger = m_control == Control::Close;

    // Close is neutral until the pointer is on it. Permanently red is what
    // several of the applications did before, and it made every window read as
    // if it were asking to be closed.
    QColor background = Qt::transparent;
    if (pressed) {
        background = danger ? w.closePressedBg : w.controlPressedBg;
    } else if (hovered) {
        background = danger ? w.closeHoverBg : w.controlHoverBg;
    }
    if (background.alpha() > 0) {
        // Square, edge to edge. A rounded control would leave a wedge of
        // window background in the corner of a maximised window, exactly
        // where the pointer is thrown to hit Close.
        painter.fillRect(rect(), background);
    }

    QColor ink = m_windowActive ? w.controlGlyph : w.controlGlyphInactive;
    if (danger && hovered) {
        ink = w.closeHoverGlyph;
    } else if (hovered) {
        // A neutral control brightens on hover; without this the glyph sits at
        // the same weight on a lighter plate and reads as disabled.
        ink = w.titleText;
    }
    if (!isEnabled()) {
        ink = w.controlGlyphInactive;
    }

    const int stroke = qMax(1, w.glyphStroke);
    const QRect box = glyphBox(rect(), qMax(6, w.glyphSize));

    switch (m_control) {
    case Control::Minimize: {
        // One bar on the glyph box's own centre line, not on the button's: the
        // three glyphs then share an optical centre even though only this one
        // is a single stroke.
        const int y = box.top() + (box.height() - stroke) / 2;
        painter.fillRect(QRect(box.left(), y, box.width(), stroke), ink);
        break;
    }
    case Control::Maximize:
        strokeRect(painter, box, stroke, ink);
        break;
    case Control::Restore: {
        // Two sheets: the front one down-left, the back one showing only its
        // top and right edges. Drawing the back as a full square and painting
        // the front over it would need the button's background colour, which
        // is transparent when the control is not hovered.
        const int offset = qMax(2, stroke * 2);
        const QRect front(box.left(), box.top() + offset,
                          box.width() - offset, box.height() - offset);
        strokeRect(painter, front, stroke, ink);
        painter.fillRect(QRect(front.left() + offset, box.top(),
                               front.width(), stroke), ink);
        painter.fillRect(QRect(box.right() - stroke + 1, box.top(),
                               stroke, front.height()), ink);
        break;
    }
    case Control::Close: {
        // The only glyph that is not axis-aligned, so it is the only one drawn
        // with a pen and antialiasing. Without antialiasing a diagonal at this
        // size comes out as a staircase.
        painter.setRenderHint(QPainter::Antialiasing, true);
        QPen pen(ink);
        pen.setWidthF(stroke);
        pen.setCapStyle(Qt::FlatCap);
        painter.setPen(pen);
        const QRectF cross = QRectF(box).adjusted(0.5, 0.5, -0.5, -0.5);
        painter.drawLine(cross.topLeft(), cross.bottomRight());
        painter.drawLine(cross.topRight(), cross.bottomLeft());
        break;
    }
    }
}

// -- TitleBar ---------------------------------------------------------------

TitleBar::TitleBar(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    m_row = new QHBoxLayout(this);
    m_row->setSpacing(0);

    m_iconLabel = new QLabel(this);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->hide();
    m_row->addWidget(m_iconLabel);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    // Ignored, so a long caption shortens instead of widening the window.
    // updateElidedTitle() then decides what "shortened" looks like.
    m_titleLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    // The label must not swallow the press: dragging a window by its caption
    // is the gesture users actually use, and a label that accepted the event
    // would make exactly the readable part of the bar dead.
    m_titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_row->addWidget(m_titleLabel, 1);

    m_minimize = new WindowControlButton(WindowControlButton::Control::Minimize, this);
    m_maximize = new WindowControlButton(WindowControlButton::Control::Maximize, this);
    m_close = new WindowControlButton(WindowControlButton::Control::Close, this);
    m_row->addWidget(m_minimize);
    m_row->addWidget(m_maximize);
    m_row->addWidget(m_close);

    connect(m_minimize, &QAbstractButton::clicked, this, &TitleBar::minimizeRequested);
    connect(m_maximize, &QAbstractButton::clicked, this, &TitleBar::maximizeRestoreRequested);
    connect(m_close, &QAbstractButton::clicked, this, &TitleBar::closeRequested);

    applyTheme();
}

void TitleBar::setTitle(const QString& title)
{
    if (m_titleText == title) {
        return;
    }
    m_titleText = title;
    updateElidedTitle();
}

QString TitleBar::title() const
{
    return m_titleText;
}

void TitleBar::setIcon(const QIcon& icon)
{
    m_iconSource = icon;
    updateIconPixmap();
    applyTheme();
}

void TitleBar::setMinimizable(bool enabled)
{
    if (m_minimizable == enabled) {
        return;
    }
    m_minimizable = enabled;
    m_minimize->setVisible(enabled);
    updateGeometry();
}

void TitleBar::setMaximizable(bool enabled)
{
    if (m_maximizable == enabled) {
        return;
    }
    m_maximizable = enabled;
    m_maximize->setVisible(enabled);
    updateGeometry();
}

void TitleBar::setClosable(bool enabled)
{
    if (m_closable == enabled) {
        return;
    }
    m_closable = enabled;
    m_close->setVisible(enabled);
    updateGeometry();
}

void TitleBar::setMaximized(bool maximized)
{
    if (m_maximized == maximized) {
        return;
    }
    m_maximized = maximized;
    m_maximize->setControl(maximized ? WindowControlButton::Control::Restore
                                     : WindowControlButton::Control::Maximize);
}

void TitleBar::setWindowActive(bool active)
{
    if (m_windowActive == active) {
        return;
    }
    m_windowActive = active;
    m_minimize->setWindowActive(active);
    m_maximize->setWindowActive(active);
    m_close->setWindowActive(active);
    applyTheme();
    update();
}

void TitleBar::setLeading(QWidget* widget)
{
    if (m_leading == widget) {
        return;
    }
    if (m_leading) {
        m_row->removeWidget(m_leading);
        // Reparented away rather than deleted: the caller owns it.
        m_leading->setParent(nullptr);
    }
    m_leading = widget;
    if (m_leading) {
        m_leading->setParent(this);
        // After the icon, before the caption: index 1 is where the icon ends,
        // and a menu bar reads as belonging to the application mark next to it.
        m_row->insertWidget(1, m_leading);
    }
    applyTheme();
}

void TitleBar::setTrailing(const QList<QWidget*>& widgets)
{
    if (m_trailing == widgets) {
        return;
    }
    for (QWidget* previous : m_trailing) {
        if (!previous) {
            continue;
        }
        m_row->removeWidget(previous);
        previous->setParent(nullptr);
    }
    m_trailing.clear();

    // Before the three controls, which are always the last items on the row.
    int index = m_row->indexOf(m_minimize);
    for (QWidget* widget : widgets) {
        if (!widget) {
            continue;
        }
        widget->setParent(this);
        m_row->insertWidget(index++, widget);
        m_trailing.append(widget);
    }
    applyTheme();
}

QSize TitleBar::sizeHint() const
{
    const WindowTokens& w = themeFor(this).window;
    return QSize(minimumSizeHint().width(), w.titleBarHeight);
}

QSize TitleBar::minimumSizeHint() const
{
    const WindowTokens& w = themeFor(this).window;
    // Padding, icon and the controls that are actually shown. The caption is
    // deliberately not in here: it elides, so it never sets a floor under the
    // window's width.
    int width = w.titleBarPadding;
    if (!m_iconSource.isNull()) {
        width += w.iconSize + w.titleBarGap;
    }
    if (m_minimizable) {
        width += w.controlWidth;
    }
    if (m_maximizable) {
        width += w.controlWidth;
    }
    if (m_closable) {
        width += w.controlWidth;
    }
    return QSize(width, w.titleBarHeight);
}

void TitleBar::applyTheme()
{
    const ThemeProfile& profile = themeFor(this);
    const WindowTokens& w = profile.window;

    setFixedHeight(w.titleBarHeight);
    // Leading padding only. The controls sit flush against the trailing edge:
    // a gap there leaves a dead strip on a maximised window exactly where the
    // pointer is thrown to hit Close.
    m_row->setContentsMargins(w.titleBarPadding, 0, 0, 0);

    m_iconLabel->setFixedSize(w.iconSize, w.titleBarHeight);

    m_titleLabel->setFont(roleFont(this, TypeRole::WindowTitle));
    m_titleLabel->setContentsMargins(m_iconSource.isNull() ? 0 : w.titleBarGap, 0,
                                     w.titleBarGap, 0);
    setInk(m_titleLabel, m_windowActive ? w.titleText : w.titleTextInactive);

    if (m_leading) {
        m_leading->setMaximumHeight(w.titleBarHeight);
    }
    for (QWidget* widget : m_trailing) {
        if (widget) {
            widget->setMaximumHeight(w.titleBarHeight);
        }
    }

    updateElidedTitle();
    updateGeometry();
}

void TitleBar::updateIconPixmap()
{
    const WindowTokens& w = themeFor(this).window;
    if (m_iconSource.isNull()) {
        m_iconLabel->hide();
        m_iconLabel->clear();
        return;
    }
    // devicePixelRatio, so the mark is not a blurred 16px bitmap on a 150%
    // display. QIcon::pixmap() picks the right source for the requested
    // device-independent size when it is given the ratio.
    m_iconLabel->setPixmap(m_iconSource.pixmap(QSize(w.iconSize, w.iconSize),
                                               devicePixelRatioF()));
    m_iconLabel->show();
}

void TitleBar::updateElidedTitle()
{
    if (!m_titleLabel) {
        return;
    }
    const QFontMetrics fm(m_titleLabel->font());
    const int available = m_titleLabel->contentsRect().width();
    m_titleLabel->setText(available > 0
                              ? fm.elidedText(m_titleText, Qt::ElideRight, available)
                              : m_titleText);
    // The tooltip only appears when something was actually cut, so a caption
    // that fits does not sprout a tooltip repeating itself.
    m_titleLabel->setToolTip(m_titleLabel->text() == m_titleText ? QString() : m_titleText);
}

void TitleBar::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateElidedTitle();
}

void TitleBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // The press reached the bar, which means it did not land on a control:
        // a QAbstractButton accepts its own press. Labels are transparent for
        // mouse events, so the caption drags too.
        emit dragRequested();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_maximizable) {
        emit maximizeRestoreRequested();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

bool TitleBar::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type()
        || event->type() == QEvent::PolishRequest
        || event->type() == QEvent::ParentChange) {
        applyTheme();
        update();
    }
    return QWidget::event(event);
}

void TitleBar::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const WindowTokens& w = profile.window;

    QPainter painter(this);
    painter.fillRect(rect(), m_windowActive ? w.titleBarBg : w.titleBarBgInactive);

    // A hairline under the bar, so the chrome and the content read as two
    // surfaces rather than one flat field. Thickness from the divider role -
    // it is the same line the rest of the system draws between two surfaces.
    const int line = profile.border(BorderRole::Divider);
    if (line > 0) {
        painter.fillRect(QRect(0, height() - line, width(), line), w.border);
    }
}

} // namespace novu::ui
