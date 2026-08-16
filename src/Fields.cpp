#include "novu/ui/Fields.h"

#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QCheckBox>
#include <QLineEdit>
#include <QPainter>
#include <QPolygonF>

namespace novu::ui {

ComboBox::ComboBox(QWidget* parent)
    : QComboBox(parent)
{
}

void ComboBox::paintEvent(QPaintEvent* event)
{
    QComboBox::paintEvent(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const qreal cx = width() - 14.0;
    const qreal cy = height() / 2.0;
    p.setPen(Qt::NoPen);
    p.setBrush(themeFor(this).colors.textSecondary);
    p.drawPolygon(QPolygonF({QPointF(cx - 4.5, cy - 2.0),
                             QPointF(cx + 4.5, cy - 2.0),
                             QPointF(cx, cy + 3.5)}));
}

ComboBox* comboBox(const QStringList& items, QWidget* parent, int width)
{
    auto* c = new ComboBox(parent);
    c->addItems(items);
    c->setFont(roleFont(parent, TypeRole::Body));
    c->setFixedHeight(themeFor(parent).controlHeight(ControlSize::Medium));
    if (width > 0) {
        c->setFixedWidth(width);
    }
    return c;
}

QLineEdit* lineEdit(const QString& placeholder, QWidget* parent, int width,
                    const QString& value)
{
    auto* e = new QLineEdit(parent);
    e->setPlaceholderText(placeholder);
    if (!value.isEmpty()) {
        e->setText(value);
    }
    e->setFont(roleFont(parent, TypeRole::Body));
    // Through the palette, because Qt has no ::placeholder pseudo-element —
    // a QSS rule for it is silently ignored, and the hint then renders in the
    // primary ink and reads as a value the user already typed.
    QPalette palette = e->palette();
    palette.setColor(QPalette::PlaceholderText, themeFor(parent).colors.textDisabled);
    e->setPalette(palette);
    e->setFixedHeight(themeFor(parent).controlHeight(ControlSize::Medium));
    if (width > 0) {
        e->setFixedWidth(width);
    }
    return e;
}

QCheckBox* checkBox(const QString& text, QWidget* parent, bool checked)
{
    auto* c = new QCheckBox(text, parent);
    c->setFont(roleFont(parent, TypeRole::Body));
    c->setChecked(checked);
    c->setCursor(Qt::PointingHandCursor);
    return c;
}

} // namespace novu::ui
