#include "novu/ui/Separator.h"

#include "novu/ui/ThemeManager.h"

#include <QBoxLayout>
#include <QEvent>
#include <QPainter>

namespace novu::ui {

namespace {

// The design's `.tdiv`: 18px of rule between two groups of title-bar controls.
// A length, so it belongs to the component rather than to whatever toolbar the
// divider is dropped into.
constexpr int kDividerLength = 18;

} // namespace

Separator::Separator(Qt::Orientation orientation, QWidget* parent)
    : QWidget(parent)
    , m_orientation(orientation)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setFocusPolicy(Qt::NoFocus);
    applyPolicy();
}

Separator::Separator(Qt::Orientation orientation, Weight weight, QWidget* parent)
    : Separator(orientation, parent)
{
    m_weight = weight;
}

void Separator::setOrientation(Qt::Orientation orientation)
{
    if (m_orientation == orientation) {
        return;
    }
    m_orientation = orientation;
    applyPolicy();
    updateGeometry();
    update();
}

void Separator::setWeight(Weight weight)
{
    if (m_weight == weight) {
        return;
    }
    m_weight = weight;
    update();
}

void Separator::setInset(int px)
{
    px = qMax(0, px);
    if (m_inset == px) {
        return;
    }
    m_inset = px;
    updateGeometry();
    update();
}

void Separator::setLength(int px)
{
    px = qMax(0, px);
    if (m_length == px) {
        return;
    }
    m_length = px;
    applyPolicy();
    updateGeometry();
    update();
}

void Separator::applyPolicy()
{
    // Thin in its own direction and never compressible; free along the other
    // unless a length was asked for.
    const QSizePolicy::Policy along = m_length > 0 ? QSizePolicy::Fixed : QSizePolicy::Expanding;
    if (m_orientation == Qt::Horizontal) {
        setSizePolicy(along, QSizePolicy::Fixed);
    } else {
        setSizePolicy(QSizePolicy::Fixed, along);
    }
}

QSize Separator::sizeHint() const
{
    const ThemeProfile& profile = themeFor(this);
    const int thickness = qMax(1, profile.border(BorderRole::Divider));
    // Along the rule: the requested length, else enough to be laid out at all.
    // The layout gives it the rest.
    const int run = m_length > 0 ? m_length + 2 * m_inset : thickness;
    return m_orientation == Qt::Horizontal ? QSize(run, thickness) : QSize(thickness, run);
}

bool Separator::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type()
        || event->type() == QEvent::ParentChange) {
        updateGeometry();
        update();
    }
    return QWidget::event(event);
}

void Separator::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const QColor color = m_weight == Weight::Strong ? profile.colors.borderHover
                                                    : profile.colors.border;

    QPainter painter(this);
    QRect line = rect();
    if (m_orientation == Qt::Horizontal) {
        line.adjust(m_inset, 0, -m_inset, 0);
        line.setHeight(qMax(1, profile.border(BorderRole::Divider)));
        // A fixed length is centred in whatever the layout handed over, so a
        // divider in a taller toolbar does not sit against its top edge.
        if (m_length > 0 && width() > line.width()) {
            line.moveLeft((width() - line.width()) / 2);
        }
        line.moveTop((height() - line.height()) / 2);
    } else {
        line.adjust(0, m_inset, 0, -m_inset);
        line.setWidth(qMax(1, profile.border(BorderRole::Divider)));
        if (m_length > 0 && height() > line.height()) {
            line.moveTop((height() - line.height()) / 2);
        }
        line.moveLeft((width() - line.width()) / 2);
    }
    painter.fillRect(line, color);
}

Separator* separator(QBoxLayout* into, QWidget* parent, Qt::Orientation orientation)
{
    auto* rule = new Separator(orientation, parent);
    if (into) {
        into->addWidget(rule);
    }
    return rule;
}

Separator* toolbarDivider(QBoxLayout* into, QWidget* parent)
{
    auto* rule = new Separator(Qt::Vertical, Separator::Weight::Strong, parent);
    rule->setLength(kDividerLength);
    if (into) {
        into->addWidget(rule);
    }
    return rule;
}

} // namespace novu::ui
