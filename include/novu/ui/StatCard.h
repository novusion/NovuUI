#pragma once

#include "novu/ui/Card.h"
#include "novu/ui/Text.h"

#include <QString>
#include <QVector>

class QGridLayout;

namespace novu::ui {

// A caption and one large number.
//
// setValue() updates in place. A metric card is never rebuilt because its
// number changed — rebuilding a widget to change its text is what makes a
// dashboard flicker and lose focus on every refresh.
class StatCard : public Card
{
    Q_OBJECT

public:
    StatCard(const QString& caption, const QString& value,
             QWidget* parent = nullptr, Ink tone = Ink::Default);

    void setValue(const QString& value, Ink tone = Ink::Default);
    void setCaption(const QString& caption);

private:
    QLabel* m_caption = nullptr;
    QLabel* m_value = nullptr;
};

// One cell of a metric grid inside a card. Same contract as StatCard, smaller,
// and with no surface of its own because the card already is one.
class MiniStat : public QWidget
{
    Q_OBJECT

public:
    MiniStat(const QString& caption, const QString& value,
             QWidget* parent = nullptr, Ink tone = Ink::Default);

    void setValue(const QString& value, Ink tone = Ink::Default);

private:
    QLabel* m_value = nullptr;
};

struct StatSpec
{
    QString caption;
    QString value;
    Ink tone = Ink::Default;
};

// A grid of MiniStats. Returns a layout — it creates no container of its own.
//
// `created` receives the cells in the order given, for a page that has to
// update them later. Without it the caller has to build the grid by hand just
// to keep the pointers, and then the grid's geometry stops being ours.
QGridLayout* statGrid(QWidget* parent, const QVector<StatSpec>& stats,
                      int columns = 2, QVector<MiniStat*>* created = nullptr);

} // namespace novu::ui
