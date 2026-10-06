#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/ThemeProfile.h"
#include "novu/ui/StyleSpec.h"

#include <QFrame>
#include <QMargins>
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
class NOVUUI_API Card : public QFrame
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

    // The card's own padding and the gap between the things stacked in it.
    //
    // Both come from the profile, and a caller that states them here is
    // saying this card's content is not the usual stack of sections — a
    // metric, a list of rows that carry their own padding, a table that must
    // meet the card's edge. It has to be said through the card rather than by
    // reaching into body(): applyTheme() runs again on every theme change and
    // every polish, and what it re-applies is exactly these two values, so a
    // margin set directly on the layout survives until the first re-polish
    // and then silently reverts.
    void setBodyMetrics(const QMargins& margins, int spacing);
    // Back to whatever the profile says.
    void clearBodyMetrics();

    // The rule under the header row - `.card-head{border-bottom:1px solid
    // var(--hair)}`. On by default, because every approved design draws it;
    // a card whose header and body are one continuous block turns it off.
    bool hasHeaderDivider() const { return m_headerDivider; }
    void setHeaderDivider(bool divider);

    // Surface and border are independent. Raised uses the panel shade and
    // retains the border; borderless rows request setBorderVisible(false).
    enum class Surface { Plain, Raised, Window, Elevated };
    Surface surface() const { return m_surface; }
    void setSurface(Surface surface);
    // Optional token-based hover surface; follows live theme changes.
    void setHoverSurface(Surface surface) { m_hoverSurface = surface; m_ownHover = true; update(); }
    void setHoverAccentBorder(bool enabled) { m_hoverAccentBorder = enabled; update(); }
    // Dashed notes and strong outlines share the same surface primitive.
    void setBorderStyle(Qt::PenStyle style,bool emphasized=false) { m_borderStyle=style;m_borderEmphasized=emphasized;update(); }
    void setBorderVisible(bool visible);
    void setRadiusStep(RadiusStep step) { m_radiusStep = step; m_ownRadius = true; update(); }

    // Cards used as a plain surface (no title) can drop the header entirely -
    // the row, not only the words in it, so the card does not pay a gap for a
    // header that is not there.
    void setHeaderMetrics(const QMargins& margins, int minimumContentHeight = 0);
    void setHeaderVisible(bool visible);
    bool isHeaderVisible() const { return m_headerVisible; }

    // Optional action surface. Child buttons retain their own interaction.
    void setInteractive(bool interactive);
    bool isInteractive() const { return m_interactive; }
    void setSelected(bool selected);
    void setSelectionOpacity(qreal fill,qreal border){m_selectionFill=qBound(0.0,fill,1.0);m_selectionBorder=qBound(0.0,border,1.0);update();}
    void setLeadingAccent(ColorToken color,int width=2){m_leadingAccent=color;m_leadingWidth=qMax(0,width);update();}
    bool isSelected() const { return m_selected; }

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    bool event(QEvent* event) override;

private:
    void applyTheme();
    void updateHeaderVisibility();

    QLabel* m_title = nullptr;
    QLabel* m_subtitle = nullptr;
    QHBoxLayout* m_headerRow = nullptr;
    // The header row lives in a widget of its own so hiding it takes its gap
    // with it: a layout skips a hidden widget entirely, but keeps the spacing
    // around a layout whose items merely have nothing to show.
    QWidget* m_header = nullptr;
    QVBoxLayout* m_outer = nullptr;
    QVBoxLayout* m_body = nullptr;
    QList<QWidget*> m_headerTrailing;
    Surface m_surface = Surface::Plain;
    Surface m_hoverSurface = Surface::Raised;
    bool m_ownHover = false;
    bool m_hoverAccentBorder = false;
    bool m_borderVisible = true;
    Qt::PenStyle m_borderStyle=Qt::SolidLine;
    bool m_borderEmphasized=false;
    bool m_ownRadius = false;
    RadiusStep m_radiusStep = RadiusStep::Large;
    bool m_headerVisible = true;
    bool m_ownHeaderMetrics = false;
    QMargins m_headerMargins;
    int m_headerMinimumContentHeight = 0;
    bool m_headerDivider = true;
    bool m_ownMetrics = false;
    bool m_interactive = false;
    bool m_pressed = false;
    bool m_selected = false;
    qreal m_selectionFill=.13,m_selectionBorder=1.0;
    ColorToken m_leadingAccent;
    int m_leadingWidth=0;
    QMargins m_bodyMargins;
    int m_bodySpacing = 0;
};

// A heading inside a card or page. Not a widget wrapper — it adds a styled
// label to the layout you already have, because grouping is what the parent
// layout does.
QLabel* section(QVBoxLayout* into, const QString& title, QWidget* parent = nullptr);

} // namespace novu::ui
