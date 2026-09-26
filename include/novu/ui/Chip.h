#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include "novu/ui/Icon.h"

#include <QPushButton>
#include <QRect>
#include <QWidget>

namespace novu::ui {

// A small on/off label. On is the accent, off is muted — legible at a glance
// across a row of twenty.
//
// One QPushButton per chip, which predates ItemStrip. A row of twenty is
// therefore twenty widgets; Segmented and ModuleBar paint their items instead.
// Left alone during the extraction freeze because NovuSpace consumes this
// header today — see docs/EXTRACTION_STATUS.md §6.
class Chip : public QPushButton
{
    Q_OBJECT

public:
    explicit Chip(const QString& text, QWidget* parent = nullptr, bool on = true);

    // ── the token variant ────────────────────────────────────────────────
    //
    // A chip that names a thing rather than a state: an attached file, a
    // recipient, a filter you can take off again. It carries a leading glyph,
    // a remove affordance, or both — and once it carries either it stops
    // being a toggle, because a token is present or gone, not on or off.
    //
    // A chip with neither is untouched: same geometry, same stylesheet, same
    // checkable behaviour as before.
    // Compact neutral document/category tag; still emits clicked().
    void setTagStyle(bool enabled){m_tagStyle=enabled;setCheckable(!enabled);applyGeometry();update();}
    void setGlyph(Glyph glyph);
    Glyph glyph() const { return m_glyph; }

    void setRemovable(bool removable);
    bool isRemovable() const { return m_removable; }

    // A token in the product accent rather than the neutral ink.
    //
    // `.sesschip` against `.sesschip.anon`: the same pill, and the accent is
    // what says the thing it names is *yours* - a signed-in session against an
    // anonymous one. Off by default, because a token names a thing and most
    // things a token names are not the user's identity.
    void setAccented(bool accented);
    bool isAccented() const { return m_accented; }

    // A token whose text is an identifier - a session name, a hash, a key -
    // set in the monospace face. `.sesschip{font-family:'JetBrains Mono'}`.
    //
    // Said here rather than with setFont() because a chip's font is the
    // stylesheet's: the sheet states `font-weight` on the pill selector, and
    // Qt lets a sheet's font property overrule one set on the widget. The
    // token paint path draws itself, so the face it draws with has to be part
    // of what the component knows.
    void setMonospaced(bool monospaced);
    bool isMonospaced() const { return m_monospaced; }

    bool isToken() const { return m_glyph != Glyph::None || m_removable; }

    // The x's hit area, in local coordinates. Empty when the chip has none.
    QRect removeRect() const;

    // Overridden because QPushButton's own sizeHint() computes from the
    // widget's QFont property, which stays the pill's proportional face even
    // when setMonospaced(true) has the *paint* path measuring and drawing in
    // the monospace one - applyGeometry() already resized the chip correctly
    // for whichever face is active, so any caller that lays this out by its
    // sizeHint() rather than its actual size() (AddressBar's trailing slot
    // does exactly that) must see the same box the chip already committed to,
    // not a narrower one guessed from the wrong font.
    QSize sizeHint() const override { return size(); }

signals:
    // The x was pressed. The chip does not remove itself: what a token
    // belongs to is the caller's, and a list that deletes its own rows
    // cannot be undone by the thing that owns it.
    void removed();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    bool event(QEvent* event) override;

private:
    void applyGeometry();
    // The face the token path measures and paints with.
    QFont tokenFont() const;

    Glyph m_glyph = Glyph::None;
    bool m_accented = false;
    bool m_monospaced = false;
    bool m_removable = false;
    bool m_removeHovered = false;
    bool m_removePressed = false;
    bool m_tagStyle=false;
};

// A wrapping strip of chips used to choose which columns a table shows.
//
// The wrapper exists because FlowLayout needs a host widget, and for no other
// reason: it carries no background and no border. An earlier version gave it a
// tinted band with rules above and below, which fought with the card it sits
// in.
class ChipBar : public QWidget
{
    Q_OBJECT

public:
    // The caption, when given, is laid out as the first item so it wraps with
    // the chips instead of sitting in a row of its own that drifts out of
    // alignment once the strip wraps.
    explicit ChipBar(const QStringList& items, QWidget* parent = nullptr,
                     const QStringList& initiallyOn = {},
                     const QString& caption = {});

    // Items whose chip is currently on, in declared order.
    QStringList selected() const;
    void setSelected(const QStringList& items);

signals:
    void changed();

private:
    QStringList m_items;
    QList<Chip*> m_chips;
};

} // namespace novu::ui
