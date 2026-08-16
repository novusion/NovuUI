#include "novu/ui/Card.h"

#include "novu/ui/StyleSheet.h"
#include "novu/ui/Text.h"

#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>

namespace novu::ui {

namespace {

void applyTypeRole(QLabel* label, const ThemeProfile& profile, TypeRole role, const QColor& colour)
{
    // Through roleFont(), not by editing the label's inherited font. Editing it
    // never named a family, so the label kept whatever the application default
    // carried and only took size and weight from the profile - which is how a
    // card title ended up lighter than the same role elsewhere on the page.
    // One font-building path, or the two drift.
    const bool mono = profile.text(role).monospace;
    label->setFont(mono ? roleMonoFont(label, role) : roleFont(label, role));
    setInk(label, colour);
}

} // namespace

Card::Card(QWidget* parent)
    : Card(QString(), parent)
{
}

Card::Card(const QString& title, QWidget* parent)
    : QFrame(parent)
{
    setFrameShape(QFrame::NoFrame);
    // The card paints its own surface; letting Qt fill the background as well
    // would draw a square corner behind the rounded one.
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAutoFillBackground(false);

    m_body = new QVBoxLayout(this);

    m_headerRow = new QHBoxLayout();

    // Title and subtitle share one column so the trailing widgets stay
    // vertically centred against the pair, not against the title alone.
    auto* titleColumn = new QVBoxLayout();
    titleColumn->setSpacing(1);
    m_title = new QLabel(title, this);
    m_title->setVisible(!title.isEmpty());
    titleColumn->addWidget(m_title);
    m_subtitle = new QLabel(this);
    m_subtitle->hide();
    titleColumn->addWidget(m_subtitle);
    m_headerRow->addLayout(titleColumn);

    m_headerRow->addStretch(1);
    m_body->addLayout(m_headerRow);

    applyTheme();
}

void Card::setTitle(const QString& title)
{
    m_title->setText(title);
    m_title->setVisible(!title.isEmpty());
}

QString Card::title() const
{
    return m_title->text();
}

void Card::setSubtitle(const QString& subtitle)
{
    m_subtitle->setText(subtitle);
    m_subtitle->setVisible(!subtitle.isEmpty());
    applyTheme();
}

QString Card::subtitle() const
{
    return m_subtitle->text();
}

void Card::setHeaderTrailing(QWidget* widget)
{
    setHeaderTrailing(widget ? QList<QWidget*>{widget} : QList<QWidget*>{});
}

void Card::setHeaderTrailing(const QList<QWidget*>& widgets)
{
    if (m_headerTrailing == widgets) {
        return;
    }
    for (QWidget* previous : m_headerTrailing) {
        if (!previous) {
            continue;
        }
        m_headerRow->removeWidget(previous);
        // Reparented away rather than deleted: the caller owns these.
        previous->setParent(nullptr);
    }
    m_headerTrailing.clear();

    for (QWidget* widget : widgets) {
        if (!widget) {
            continue;
        }
        widget->setParent(this);
        m_headerRow->addWidget(widget);
        m_headerTrailing.append(widget);
    }
}

QVBoxLayout* Card::body() const
{
    return m_body;
}

void Card::setHeaderVisible(bool visible)
{
    m_title->setVisible(visible && !m_title->text().isEmpty());
    for (QWidget* trailing : m_headerTrailing) {
        if (trailing) {
            trailing->setVisible(visible);
        }
    }
}

void Card::applyTheme()
{
    const ThemeProfile& profile = themeFor(this);
    const int padding = profile.space(SpacingRole::CardPadding);
    m_body->setContentsMargins(padding, padding, padding, padding);
    m_body->setSpacing(profile.space(SpacingRole::SectionGap));
    m_headerRow->setSpacing(profile.space(SpacingRole::ToolbarGap));
    // The approved design gives a card title the primary ink; secondary made it
    // recede into the surface and the card read as unlabelled.
    applyTypeRole(m_title, profile, TypeRole::CardTitle, profile.colors.textPrimary);
    applyTypeRole(m_subtitle, profile, TypeRole::Caption, profile.colors.textDisabled);
    updateGeometry();
}

bool Card::event(QEvent* event)
{
    // Live retheming: the profile may have been edited or swapped under a
    // running window, so geometry and fonts are recomputed, not just repainted.
    //
    // PolishRequest is in here for a second reason. Applying the scope sheet to
    // the page re-polishes its widgets, and that drops the font this card set
    // on its title in the constructor - the title came out at the default size
    // and weight while every other label on the page kept its role. Re-applying
    // after the polish is what makes the title survive it.
    if (event->type() == ThemeChangeEvent::type()
        || event->type() == QEvent::PolishRequest
        || event->type() == QEvent::ParentChange) {
        applyTheme();
        update();
    }
    return QFrame::event(event);
}

void Card::paintEvent(QPaintEvent* event)
{
    const ThemeProfile& profile = themeFor(this);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF surface = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const int radius = qMin(profile.radius(SurfaceRole::Card),
                            static_cast<int>(surface.height() / 2));

    QPainterPath path;
    path.addRoundedRect(surface, radius, radius);
    painter.fillPath(path, profile.colors.panelBg);

    const int borderWidth = profile.border(BorderRole::Card);
    if (borderWidth > 0) {
        painter.setPen(QPen(profile.colors.border, borderWidth));
        painter.drawPath(path);
    }

    QFrame::paintEvent(event);
}

QLabel* section(QVBoxLayout* into, const QString& title, QWidget* parent)
{
    auto* label = new QLabel(title, parent);
    const ThemeProfile& profile = themeFor(parent);
    applyTypeRole(label, profile, TypeRole::SectionTitle, profile.colors.textPrimary);
    if (into) {
        into->addWidget(label);
    }
    return label;
}

} // namespace novu::ui
