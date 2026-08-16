#pragma once

#include "novu/ui/ThemeProfile.h"

#include <QFrame>
#include <QString>

class QLabel;
class QVBoxLayout;
class QHBoxLayout;

namespace novu::ui {

// The one surface primitive: background, border and radius, nothing else.
//
// Cost is deliberately a single widget. An earlier generation of this
// component (NovuTradeUiKit, AdminUiKit) created a QFrame *plus* an inner
// "body" widget whose only job was to carry padding — a container that exists
// for padding is a design error, and it multiplied by every card on screen.
// Here padding is contentsMargins on the card's own layout.
//
// It paints itself instead of carrying a stylesheet so that its radius comes
// from the theme role and changes live when the profile does. A stylesheet
// would also make the page able to override it, which the contract forbids.
class Card : public QFrame
{
    Q_OBJECT

public:
    explicit Card(QWidget* parent = nullptr);
    explicit Card(const QString& title, QWidget* parent = nullptr);

    void setTitle(const QString& title);
    QString title() const;

    // A second, muted line under the title. The approved designs use it to say
    // what a card's data *is* - "Live positions, WS-only" - which is the
    // difference between a table a user trusts and one they have to ask about.
    void setSubtitle(const QString& subtitle);
    QString subtitle() const;

    // Trailing widgets on the title row — a badge, a button, a status
    // indicator. The card owns the row; the caller owns the widgets.
    //
    // A list rather than a single widget because one trailing slot does not
    // survive contact with real headers: a live table wants both its
    // connection badge and its detach button, and forcing the caller to wrap
    // them creates a container that exists only to hold two things in a row —
    // exactly the wrapper this component was written to remove.
    void setHeaderTrailing(QWidget* widget);
    void setHeaderTrailing(const QList<QWidget*>& widgets);

    // Where content goes. There is no inner container: this is the card's
    // own layout, already carrying the profile's card padding.
    QVBoxLayout* body() const;

    // Cards used as a plain surface (no title) can drop the header entirely.
    void setHeaderVisible(bool visible);

protected:
    void paintEvent(QPaintEvent* event) override;
    bool event(QEvent* event) override;

private:
    void applyTheme();

    QLabel* m_title = nullptr;
    QLabel* m_subtitle = nullptr;
    QHBoxLayout* m_headerRow = nullptr;
    QVBoxLayout* m_body = nullptr;
    QList<QWidget*> m_headerTrailing;
};

// A heading inside a card or page. Not a widget wrapper — it adds a styled
// label to the layout you already have, because grouping is what the parent
// layout does.
QLabel* section(QVBoxLayout* into, const QString& title, QWidget* parent = nullptr);

} // namespace novu::ui
