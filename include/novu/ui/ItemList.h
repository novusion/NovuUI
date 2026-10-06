#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/Text.h"

#include <QString>
#include <QVector>
#include <QWidget>

class QVBoxLayout;

namespace novu::ui {

class Separator;

// The design's `.hlist` — rows divided by hairlines, filling a card.
//
// `.hrow{display:flex;align-items:center;gap:10px;padding:9px 16px;
// border-bottom:1px solid var(--hair)}` with `.hrow:last-child{border-bottom:
// none}`, a `.hn` that takes the width (12.5px, secondary ink) with an
// optional `.s` under it (10.5px, muted), and a `.hv` at the trailing edge
// (monospace, 11px, muted).
//
// ── why it is a component ─────────────────────────────────────────────────
//
// It is the second most repeated shape in the approved designs after the
// table, and the most reinvented: Overview's running bots, Performance's best
// and worst, Performance's per-bot split, History's archived years, Markets'
// ticker, recordings and stored sessions are all this list. Every one of them
// was a hand-rolled loop over QHBoxLayouts with its own padding, its own type
// and its own idea of whether the last row gets a rule — which is how six
// lists in one product came out at six different row heights.
//
// A row is a widget rather than painted data, unlike NavList: the rows here
// carry badges, buttons, status lights and two-line cells, and the variety is
// the point. What this owns is the *list* — the hairlines, the metrics, the
// hover, and that the last row has no rule under it.
class NOVUUI_API ItemList : public QWidget
{
    Q_OBJECT

public:
    explicit ItemList(QWidget* parent = nullptr);

    // The common row: a name, an optional line under it, and a value at the
    // trailing edge. Returns the row so a caller can put something else in it.
    QWidget* addRow(const QString& title, const QString& subtitle = QString(),
                    const QString& value = QString(), Ink valueInk = Ink::Muted);

    // A row the caller builds itself, for the shapes that are not name/value —
    // a feed item with a glyph plate, a recording with a status badge.
    int addRow(QWidget* content);

    // Something at the trailing edge of a row that is not text: a badge, an
    // icon button. Added after the value, which is where the design puts it.
    void addTrailing(int index, QWidget* widget);
    // Something before the name — the tinted glyph plate a feed row carries.
    void setLeading(int index, QWidget* widget);

    // `.hrow.clickable{cursor:pointer}` with `:hover{background:var(--panel)}`.
    void setRowClickable(int index, bool clickable);
    bool isRowClickable(int index) const;

    int count() const { return m_rows.size(); }
    QWidget* rowAt(int index) const;
    void clear();

signals:
    // A clickable row was pressed and released inside itself.
    void activated(int index);

protected:
    bool event(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    struct Row
    {
        QWidget* widget = nullptr;
        Separator* rule = nullptr;
        bool clickable = false;
    };

    void applyTheme();
    // The last row has no rule under it, and which row is last changes as
    // rows are added — so it is decided here rather than at each addRow().
    void updateRules();

    QVBoxLayout* m_column = nullptr;
    QVector<Row> m_rows;
    int m_hovered = -1;
    int m_pressed = -1;
};

} // namespace novu::ui
