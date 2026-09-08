#include "novu/ui/StatCard.h"

#include "novu/ui/ThemeManager.h"

#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace novu::ui {

namespace {

// A signed metric is tinted by its own sign.
//
// A leading + or - on a number is never decoration - it is the whole point of
// the figure, and a realised P&L printed in the same ink whether it made money
// or lost it is a dashboard the reader has to parse instead of glance at.
//
// Applied only when the caller asked for no particular ink. A caller that
// names a tone means it: an accent metric stays accent even when it happens to
// carry a minus.
//
// The sign must be followed by a digit, so an em-dash or a bare "-" standing in
// for "no data yet" is not painted as a loss.
Ink signedInk(const QString& value, Ink requested)
{
    if (requested != Ink::Default) {
        return requested;
    }
    const QString text = value.trimmed();
    if (text.size() < 2 || !text.at(1).isDigit()) {
        return requested;
    }
    const QChar sign = text.at(0);
    if (sign == QLatin1Char('+')) {
        return Ink::Success;
    }
    // U+2212 as well as the ASCII hyphen: a number formatted for display often
    // carries the typographic minus, and it means exactly the same thing.
    if (sign == QLatin1Char('-') || sign == QChar(0x2212)) {
        return Ink::Danger;
    }
    return requested;
}

} // namespace

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
    m_value = label(value, this, TypeRole::Metric, signedInk(value, tone), true);
    body()->addWidget(m_caption);
    body()->addWidget(m_value);
}

void StatCard::setValue(const QString& value, Ink tone)
{
    m_value->setText(value);
    // Re-resolved on every update: a position that crosses zero changes colour
    // without the page having to notice.
    setInk(m_value, signedInk(value, tone));
}

void StatCard::setCaption(const QString& captionText)
{
    m_caption->setText(captionText);
}

MiniStat::MiniStat(const QString& captionText, const QString& value,
                   QWidget* parent, Ink tone)
    : QWidget(parent)
{
    // As tall as a caption over a value and no taller. Left to expand, a metric
    // handed spare height by its grid pushes the two lines apart until the
    // caption no longer reads as belonging to the number under it.
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto* column = new QVBoxLayout(this);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(2);
    column->addWidget(label(captionText, this, TypeRole::Caption, Ink::Muted));
    m_value = label(value, this, TypeRole::SectionTitle, signedInk(value, tone), true);
    column->addWidget(m_value);
}

void MiniStat::setValue(const QString& value, Ink tone)
{
    m_value->setText(value);
    setInk(m_value, signedInk(value, tone));
}

QGridLayout* statGrid(QWidget* parent, const QVector<StatSpec>& stats, int columns,
                      QVector<MiniStat*>* created)
{
    const ThemeProfile& profile = themeFor(parent);

    auto* grid = new QGridLayout();
    // Wider apart than they are tall: two metrics side by side read as one
    // value with a stray caption when the columns sit at the card gap.
    grid->setHorizontalSpacing(profile.spacing.xl);
    grid->setVerticalSpacing(profile.spacing.md);
    for (int i = 0; i < stats.size(); ++i) {
        const StatSpec& spec = stats.at(i);
        auto* cell = new MiniStat(spec.caption, spec.value, parent, spec.tone);
        grid->addWidget(cell, i / columns, i % columns);
        if (created) {
            created->append(cell);
        }
    }
    return grid;
}

} // namespace novu::ui
