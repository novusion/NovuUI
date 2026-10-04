#pragma once

#include "novu/ui/Icon.h"
#include <QAbstractButton>
#include "novu/ui/StatusDot.h"
#include "novu/ui/StyleSpec.h"

#include <QFont>
#include <QPointer>
#include <QString>
#include <QVector>
#include <QWidget>

namespace novu::ui {

class Popover;

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
// The control that opens one, showing the choice in force.
//
// `.select` in every approved design: a pill carrying the current option, an
// optional status dot or glyph in front of it, and a chevron that turns over
// while the menu is up. Pressing it opens a Popover of the options; choosing
// one closes the menu and changes the label.
//
// It is a button rather than a QComboBox, because that is what the design is:
// a combo brings a system popup list, a focus frame and a line-edit shape that
// no amount of stylesheet makes into a pill, and its list cannot carry the
// groups, separators, meta column and checked accent a Popover already draws.
// Every shell had one of these and each was reaching for QComboBox.
class SelectButton : public QAbstractButton
{
    Q_OBJECT

public:
    explicit SelectButton(QWidget* parent = nullptr);

    // The simple case: labels are their own keys.
    void setOptions(const QStringList& options);
    int addOption(const QString& key, const QString& label, Glyph glyph = Glyph::None);
    void clearOptions();

    QString currentKey() const { return m_currentKey; }
    void setCurrentKey(const QString& key);
    QString currentText() const;
    void setMetaText(const QString& text){m_metaText=text;updateGeometry();update();}
    QString metaText()const{return m_metaText;}

    // `.select .dot` -- a status the choice itself carries: a provider that is
    // reachable, an engine that is running. Cleared by passing no tone.
    void setStatus(StatusDot::Tone tone, bool live = false);
    void clearStatus();

    // `.select` with a leading mark, as the agent picker has.
    void setGlyph(Glyph glyph);
    Glyph glyph() const { return m_glyph; }

    // `.select` is 26px and `.select.xs` is 23px; the profile states both.
    void setControlSize(ControlSize size);
    ControlSize controlSize() const { return m_size; }

    // The face the label is set in. `.rt-model` is the same pill in the mono
    // face at a step smaller, which is a property of that picker rather than
    // of every picker.
    void setLabelRole(TypeRole role);

    // The menu is up. `.select.open` takes the accent border and turns its
    // chevron over.
    bool isOpen() const { return m_open; }

    // The menu itself, for a caller that wants to group or annotate it.
    Popover* menu() const { return m_menu; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

signals:
    void currentChanged(const QString& key);

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    bool event(QEvent* event) override;

private:
    void openMenu();
    void applyGeometry();
    QFont labelFont() const;

    Popover* m_menu = nullptr;
    QString m_currentKey;
    QString m_metaText;
    Glyph m_glyph = Glyph::None;
    StatusDot* m_dot = nullptr;
    ControlSize m_size = ControlSize::Medium;
    TypeRole m_labelRole = TypeRole::Body;
    bool m_open = false;
};

class Popover : public QWidget
{
    Q_OBJECT

public:
    explicit Popover(QWidget* parent = nullptr);

    // `key` is what `triggered` carries. Never shown.
    int addItem(const QString& key, const QString& label, Glyph glyph = Glyph::None);
    // `.pgrp` — a tracked caption over a run of items. Not selectable.
    int addGroup(const QString& title);
    Popover* addSubmenu(const QString& key,const QString& label,Glyph glyph=Glyph::None);
    // `.psep` — a hairline between runs.
    int addSeparator();
    void clear();

    int count() const { return m_items.size(); }
    QString itemLabel(const QString& key) const;

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
    // Opens under one PART of an anchor, given in the anchor's own
    // coordinates. A painted strip - a module bar, a tab bar - has items that
    // are not widgets, so anchoring a menu to the whole strip drops it under
    // the strip's left edge rather than under the item that was clicked. The
    // flipping and the screen clamping are the same, and stay in one place.
    void popupUnder(QWidget* anchor, const QRect& anchorRect);
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
    // Emitted for every dismissal path, including Qt::Popup's automatic
    // outside-click close. Anchors use it to clear their open visual state.
    void closed();

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
        QPointer<Popover> submenu;
        bool dangerous = false;
        bool enabled = true;
    };

    int entryHeight(const Entry& entry) const;
    int neighbour(int from, int step) const;
    void trigger(int index);
    void openSubmenu(int index,bool keyboard=false);
    // A menu and its submenus dismiss as one surface: a click or a focus
    // change that lands outside the whole chain closes all of it, while moving
    // between a parent and its submenu closes nothing.
    Popover* chainRoot();
    bool chainContains(const QWidget* widget) const;
    void closeChain();
    // Closes whatever unrelated popup is currently open, so opening a second
    // menu always leaves exactly one. Does not depend on the platform grab.
    void dismissForeignPopup(QWidget* anchor = nullptr);

    QPointer<Popover> m_parentMenu;
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
