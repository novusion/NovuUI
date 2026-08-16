#include "novu/ui/StatCard.h"

#include "novu/ui/ThemeManager.h"

#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace novu::ui {

StatCard::StatCard(const QString& captionText, const QString& value,
                   QWidget* parent, Ink tone)
    : Card(parent)
{
    const ThemeProfile& profile = themeFor(this);
    const int padding = profile.space(SpacingRole::CardPadding);

    setHeaderVisible(false);
    // Tighter vertically than a content card: a metric is two lines, and the
    // standard card padding would leave it floating in its own surface.
    body()->setContentsMargins(padding, padding * 3 / 4, padding, padding * 3 / 4);
    body()->setSpacing(2);

    m_caption = label(captionText, this, TypeRole::Caption, Ink::Muted);
    m_value = label(value, this, TypeRole::Metric, tone, true);
    body()->addWidget(m_caption);
    body()->addWidget(m_value);
}

void StatCard::setValue(const QString& value, Ink tone)
{
    m_value->setText(value);
    setInk(m_value, tone);
}

void StatCard::setCaption(const QString& captionText)
{
    m_caption->setText(captionText);
}

MiniStat::MiniStat(const QString& captionText, const QString& value,
                   QWidget* parent, Ink tone)
    : QWidget(parent)
{
    auto* column = new QVBoxLayout(this);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(2);
    column->addWidget(label(captionText, this, TypeRole::Caption, Ink::Muted));
    m_value = label(value, this, TypeRole::SectionTitle, tone, true);
    column->addWidget(m_value);
}

void MiniStat::setValue(const QString& value, Ink tone)
{
    m_value->setText(value);
    setInk(m_value, tone);
}

QGridLayout* statGrid(QWidget* parent, const QVector<StatSpec>& stats, int columns)
{
    const ThemeProfile& profile = themeFor(parent);

    auto* grid = new QGridLayout();
    // Wider apart than they are tall: two metrics side by side read as one
    // value with a stray caption when the columns sit at the card gap.
    grid->setHorizontalSpacing(profile.spacing.xl);
    grid->setVerticalSpacing(profile.spacing.md);
    for (int i = 0; i < stats.size(); ++i) {
        const StatSpec& spec = stats.at(i);
        grid->addWidget(new MiniStat(spec.caption, spec.value, parent, spec.tone),
                        i / columns, i % columns);
    }
    return grid;
}

} // namespace novu::ui
