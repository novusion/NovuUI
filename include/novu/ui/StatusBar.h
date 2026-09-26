#pragma once

#include "novu/ui/Icon.h"
#include "novu/ui/StatusDot.h"
#include "novu/ui/StyleSpec.h"

#include <QString>
#include <QVector>
#include <QWidget>

namespace novu::ui {

// The strip along the bottom of a window: session, context and transport.
//
// The design states what belongs here and it is a rule rather than a
// suggestion — *host-owned. Shows session, context and transport, never
// module-private state.* A status bar that grows a per-panel message becomes a
// second, worse notification system, and every application that has one has
// learnt this the same way.
//
// ── one widget ───────────────────────────────────────────────────────────
//
// Every item is painted. A status bar is six to ten inert strings that change
// on a timer; as widgets that is a QLabel per string plus separators, each one
// re-polished on every theme change, for content nobody can select or click.
//
// Items are addressed by the handle `addText()` and friends return, so updating
// the latency figure four times a second is a string assignment and one
// repaint - not a widget lookup by name, which is how the previous status bars
// ended up holding pointers to their own children.
//
// `setTrailingWidget()` is the one seam for something genuinely interactive - a
// progress strip, a sync button. It is a single slot on purpose: a status bar
// that accepts arbitrary widgets in arbitrary places is a toolbar.
class StatusBar : public QWidget
{
    Q_OBJECT

public:
    // Which end an item is anchored to. Everything Leading is packed from the
    // left, everything Trailing from the right, and the gap between them is
    // the design's `.spacer`.
    enum class Slot { Leading, Trailing };

    // How one item is set. The design's four: plain text (`--text3`), an
    // emphasised value (`b`, `--text2`), a monospace figure (`.net`) and a
    // transport light (`.sdot`).
    enum class Item { Text, Value, Mono, Status, Separator };

    explicit StatusBar(QWidget* parent = nullptr);

    int addText(const QString& text, Slot slot = Slot::Leading);
    int addValue(const QString& text, Slot slot = Slot::Leading);
    int addMono(const QString& text, Slot slot = Slot::Trailing);
    // The dot and its word. Tone is the shared semantic one, so a disconnected
    // status bar and a disconnected badge are the same colour.
    int addStatus(const QString& text, StatusDot::Tone tone, Slot slot = Slot::Leading);
    int addSeparator(Slot slot = Slot::Leading);

    // A leading mark on an item. The approved strips name what a reading is
    // about before they give it -- a branch in front of the branch, a cog in
    // front of the memory figure, an arrow in front of Push -- and an item
    // that could not carry one left every caller to fake it with a character
    // in the string.
    void setItemGlyph(int id, Glyph glyph);

    // By handle. Setting the text of a separator does nothing rather than
    // asserting: a caller that stores handles in an array and updates them in a
    // loop should not have to special-case the dividers.
    void setItemText(int handle, const QString& text);
    QString itemText(int handle) const;
    void setItemTone(int handle, StatusDot::Tone tone);
    void setItemVisible(int handle, bool visible);
    bool isItemVisible(int handle) const;
    void clear();

    int itemCount() const { return m_items.size(); }
    // Where an item ended up, for a test that wants to assert on the order
    // without measuring text.
    QRect itemRect(int handle) const;

    // The whole bar goes to the offline treatment: the design's
    // `.statusbar.offline`. One call rather than re-toning each dot, because
    // losing the transport is one event and not six.
    bool isOffline() const { return m_offline; }
    void setOffline(bool offline);

    void setTrailingWidget(QWidget* widget);
    QWidget* trailingWidget() const { return m_trailing; }

    // The mirror of the trailing slot. A status bar that starts with a
    // control - NovuAI opens with a model picker and two buttons - cannot
    // say that with text items.
    void setLeadingWidget(QWidget* widget);
    QWidget* leadingWidget() const { return m_leading; }

    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool event(QEvent* event) override;

private:
    struct Entry
    {
        int handle = 0;
        Item kind = Item::Text;
        Slot slot = Slot::Leading;
        QString text;
        StatusDot::Tone tone = StatusDot::Tone::Neutral;
        Glyph glyph = Glyph::None;
        bool visible = true;
        // Filled during paint, so itemRect() answers about what is on screen
        // rather than about what a second measuring pass would compute.
        QRect placed;
    };

    const Entry* find(int handle) const;
    Entry* find(int handle);
    int entryWidth(const Entry& entry) const;
    QRect paintEntryGlyph(QPainter* painter, const Entry& entry,
                          const QRect& box, const QColor& ink) const;
    void layoutTrailing();
    void layoutLeading();

    QVector<Entry> m_items;
    int m_nextHandle = 1;
    bool m_offline = false;
    QWidget* m_leading = nullptr;
    QWidget* m_trailing = nullptr;
};

} // namespace novu::ui
