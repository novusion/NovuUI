#include "novu/ui/TabStrip.h"

#include "novu/ui/StyleSheet.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QHBoxLayout>
#include <QPushButton>

namespace novu::ui {

TabStrip::TabStrip(const QStringList& items, QWidget* parent, int active,
                   Variant variant, Tone tone)
    : QWidget(parent)
    , m_current(active)
    , m_tone(tone)
{
    const ThemeProfile& profile = themeFor(this);

    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(variant == Variant::Segment
                        ? profile.space(SpacingRole::ToolbarGap) / 2
                        : profile.space(SpacingRole::ToolbarGap));

    const int height = variant == Variant::Pill
        ? profile.controlHeight(ControlSize::Medium)
        : profile.controlHeight(ControlSize::Small);

    for (int i = 0; i < items.size(); ++i) {
        auto* b = new QPushButton(items.at(i), this);
        // The look is the scope stylesheet's business; the button only says
        // what it is and whether it is current.
        b->setProperty(prop::kind, QStringLiteral("tab"));
        b->setProperty(prop::tone, m_tone == Tone::Warning ? QStringLiteral("accent")
                                                           : QStringLiteral("primary"));
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        b->setFixedHeight(height);
        connect(b, &QPushButton::clicked, this, [this, i]() { setCurrent(i); });
        row->addWidget(b);
        m_buttons.append(b);
    }
    row->addStretch(1);
    refreshState();
}

void TabStrip::setCurrent(int index)
{
    if (index < 0 || index >= m_buttons.size()) {
        return;
    }
    m_current = index;
    refreshState();
    emit selected(index);
}

QPushButton* TabStrip::buttonAt(int index) const
{
    return (index >= 0 && index < m_buttons.size()) ? m_buttons.at(index) : nullptr;
}

void TabStrip::refreshState()
{
    for (int i = 0; i < m_buttons.size(); ++i) {
        QPushButton* b = m_buttons.at(i);
        const bool on = (i == m_current);
        const QSignalBlocker blocker(b);
        b->setChecked(on);
        b->setProperty(prop::active, on ? QStringLiteral("true") : QStringLiteral("false"));
        // Caption size, bold only when current — the same weight change the
        // approved design uses to mark the active item.
        b->setFont(roleFont(this, TypeRole::Caption, on));
        // Required: Qt evaluates property selectors on polish, so without this
        // the button keeps the appearance it had before the property changed.
        repolish(b);
    }
}

} // namespace novu::ui
