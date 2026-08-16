#include "novu/ui/Chip.h"

#include "novu/ui/FlowLayout.h"
#include "novu/ui/StyleSheet.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QFontMetrics>
#include <QLabel>

namespace novu::ui {

Chip::Chip(const QString& text, QWidget* parent, bool on)
    : QPushButton(text, parent)
{
    const ThemeProfile& profile = themeFor(this);

    setProperty(prop::kind, QStringLiteral("pill"));
    setCheckable(true);
    setChecked(on);
    setCursor(Qt::PointingHandCursor);
    // Caption size, bold. A chip is a compact toggle, not body text: at the
    // body size a strip of nineteen column pills was wider than the card it
    // filters.
    setFont(roleFont(this, TypeRole::Caption, true));

    // Geometry from the component, not the stylesheet: Qt does not fold QSS
    // padding into sizeHint() until a widget is polished, so anything that
    // measures a chip before then reads a width that clips the label.
    setFixedHeight(profile.controlHeight(ControlSize::Small) - 2);
    setFixedWidth(QFontMetrics(font()).horizontalAdvance(text)
                  + profile.space(SpacingRole::ToolbarGap) + 10);
}

ChipBar::ChipBar(const QStringList& items, QWidget* parent,
                 const QStringList& initiallyOn, const QString& captionText)
    : QWidget(parent)
    , m_items(items)
{
    const ThemeProfile& profile = themeFor(this);

    auto* flow = new FlowLayout(this, profile.space(SpacingRole::ToolbarGap) - 2);
    const bool allOn = initiallyOn.isEmpty();

    if (!captionText.isEmpty()) {
        flow->addWidget(caption(captionText, this));
    }

    for (const QString& item : items) {
        auto* chip = new Chip(item, this, allOn || initiallyOn.contains(item));
        connect(chip, &QPushButton::clicked, this, &ChipBar::changed);
        flow->addWidget(chip);
        m_chips.append(chip);
    }
}

QStringList ChipBar::selected() const
{
    QStringList result;
    for (Chip* chip : m_chips) {
        if (chip->isChecked()) {
            result.append(chip->text());
        }
    }
    return result;
}

void ChipBar::setSelected(const QStringList& items)
{
    for (Chip* chip : m_chips) {
        const QSignalBlocker blocker(chip);
        chip->setChecked(items.contains(chip->text()));
    }
    emit changed();
}

} // namespace novu::ui
