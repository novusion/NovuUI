#pragma once

#include "novu/ui/Icon.h"
#include "novu/ui/StyleSpec.h"

#include <QPointer>
#include <QString>
#include <QVector>
#include <QWidget>

namespace novu::ui {

// The menu that drops out of a button.
//
// The design's `.popover` with its `.pgrp` captions, `.pitem` rows and `.psep`
// rules. QMenu cannot be this: its item layout, its check column and its
// submenu arrow are the platform style's, its separator is a sunken line, and
// the one thing it will not do is carry a right-aligned `.meta` column — which
// is what nearly every menu in these designs uses ("Reconnect  Ctrl+R",
// "Switch profile  3 saved").
//
// So the popover is drawn, like everything else here. It is a top-level window
// with `Qt::Popup`, which is what makes clicking elsewhere close it and what
// gives it the mouse grab a menu needs.
//
// ── one widget again ──────────────────────────────────────────────────────
//
// Items are painted rows in a single widget. A menu is the clearest case for
// it: the rows exist for as long as the menu is open, which is under a second,
// and building twelve QWidgets to throw away is the cost paid every time
// someone clicks a toolbar button.
class Popover : public QWidget
{
    Q_OBJECT

public:
    explicit Popover(QWidget* parent = nullptr);

    // `key` is what `triggered` carries. Never shown.
    int addItem(const QString& key, const QString& label, Glyph glyph = Glyph::None);
    // `.pgrp` — a tracked caption over a run of items. Not selectable.
    int addGroup(const QString& title);
    // `.psep` — a hairline between runs.
    int addSeparator();
    void clear();

    int count() const { return m_items.size(); }

    // `.pitem .meta` — the right-aligned note: a shortcut, a count, a version.
    void setMeta(const QString& key, const QString& meta);
    // `.pitem.on` — the accent, for the option currently in force. Several may
    // be marked; this is not a radio group.
    void setChecked(const QString& key, bool checked);
    bool isChecked(const QString& key) const;
    // `.pitem.danger` — the ink and the hover both go red. For the one entry a
    // user must not hit by accident.
    void setDangerous(const QString& key, bool dangerous);
    void setItemEnabled(const QString& key, bool enabled);

    // Opens under `anchor`, aligned to its left edge, flipped above and clamped
    // to the screen when there is no room. The usual case in one call.
    void popupUnder(QWidget* anchor);
    // Opens at a screen position — for a context menu, where there is no anchor
    // widget, only a click.
    void popupAt(const QPoint& globalPosition);

    QRect itemRect(int index) const;
    int indexAt(const QPoint& position) const;
    int indexOfKey(const QString& key) const;

    StyleSpec styleSpec() const;
    StyleSpec itemStyleSpec(int index) const;
    ResolvedStyle resolvedStyle() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

signals:
    void triggered(const QString& key);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    bool event(QEvent* event) override;

private:
    enum class Kind { Item, Group, Separator };

    struct Entry
    {
        Kind kind = Kind::Item;
        QString key;
        QString label;
        QString meta;
        Glyph glyph = Glyph::None;
        bool checked = false;
        bool dangerous = false;
        bool enabled = true;
    };

    int entryHeight(const Entry& entry) const;
    int neighbour(int from, int step) const;
    void trigger(int index);
    Entry* find(const QString& key);
    const Entry* find(const QString& key) const;

    QVector<Entry> m_items;
    int m_hovered = -1;
    int m_pressed = -1;
    // The widget the popover was opened under, if any. Kept so the palette is
    // resolved against the caller's scope rather than against this top-level
    // window, which belongs to no scope at all.
    QPointer<QWidget> m_anchor;
};

} // namespace novu::ui
