#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <QPushButton>
#include <QWidget>

namespace novu::ui {

// A small on/off label. On is the accent, off is muted — legible at a glance
// across a row of twenty.
class Chip : public QPushButton
{
    Q_OBJECT

public:
    explicit Chip(const QString& text, QWidget* parent = nullptr, bool on = true);
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
