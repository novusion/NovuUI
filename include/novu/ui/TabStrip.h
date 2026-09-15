#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <QWidget>

class QPushButton;

namespace novu::ui {

// A row of buttons where exactly one is active.
//
// Every product hand-rolls this and every hand-rolled copy differs: settings
// sections, market data tabs, a period selector, a preset switcher. One
// component so they cannot drift.
//
// Not QTabWidget: that owns the pages as well as the tabs, which forces the
// content into a stacked widget it controls. A strip only reports which item
// is current and leaves the layout to the page.
//
// ── overlap, stated rather than merged ───────────────────────────────────
//
// `Variant::Segment` and `Segmented` are the same control. Segmented is the
// newer one: it is painted, it derives from ItemStrip, and it is what the
// approved design's `.chipset` is. This class keeps a QPushButton per item.
//
// They were not merged during the extraction freeze because NovuSpace consumes
// this header today, and rewriting the internals of a live consumer's
// component is not a freeze. **New code should use Segmented**; this stays for
// `Variant::Pill`, which is a section switcher and a different thing.
//
// See docs/EXTRACTION_STATUS.md §6.
class TabStrip : public QWidget
{
    Q_OBJECT

public:
    // Pill    - standalone control height, used as a section switcher.
    // Segment - compact, used above a table as a view switcher.
    enum class Variant { Pill, Segment };

    // Accent is the profile's accent. Warning is for a switcher that sits
    // directly above accent-coloured chips, so the two never read as one row.
    enum class Tone { Accent, Warning };

    explicit TabStrip(const QStringList& items, QWidget* parent = nullptr,
                      int active = 0, Variant variant = Variant::Pill,
                      Tone tone = Tone::Accent);

    int current() const { return m_current; }
    void setCurrent(int index);
    QPushButton* buttonAt(int index) const;

signals:
    void selected(int index);

private:
    void refreshState();

    QList<QPushButton*> m_buttons;
    int m_current = 0;
    Tone m_tone = Tone::Accent;
};

} // namespace novu::ui
