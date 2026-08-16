#pragma once

#include "novu/ui/Card.h"
#include "novu/ui/Text.h"

#include <QString>
#include <QStringList>

class QGridLayout;
class QHBoxLayout;
class QLabel;
class QLayout;
class QVBoxLayout;
class QWidget;

namespace novu::playground {

// Arrangement for preview pages.
//
// Read the names carefully: there is no GalleryCard here, and there will not
// be one. Every visible pixel these functions produce is drawn by NovuUI —
// Card, section(), caption(), label(). What they add is placement, which is
// what a page is allowed to own. They follow NovuUI's own convention of
// returning a layout wherever a layout is enough, so they create no container
// that exists only to hold something.
//
// They exist because sixteen pages otherwise each invent their own idea of
// what a labelled example looks like, and the gallery stops being a place
// where a regression is obvious.

// A page's outer layout. Thin wrapper over pageLayout() so a page never has
// to remember which of the two it wanted.
QVBoxLayout* previewPage(QWidget* page);

// The card an example group lives in.
novu::ui::Card* exampleCard(QWidget* parent, const QString& title,
                            const QString& subtitle = QString());

// An uppercase caption followed by a row to drop widgets into. Returns the
// row; the caption is already placed.
QHBoxLayout* exampleRow(QVBoxLayout* into, QWidget* parent, const QString& caption);

// Same, but the widgets wrap when the card is narrow — for a row of twenty
// badges. Uses NovuUI's FlowLayout; the host widget it needs is the one
// FlowLayout documents as unavoidable.
QLayout* wrappingRow(QVBoxLayout* into, QWidget* parent, const QString& caption);

// A labelled grid: column captions across the top, row captions down the
// left. Cells are addressed with matrixCell(). This is the presentation that
// makes a variant regression visible at a glance — a Ghost button that stops
// matching a Primary button's height is a bent row, not a detail.
QGridLayout* matrix(QVBoxLayout* into, QWidget* parent,
                    const QStringList& columns, const QStringList& rows);

// Places a widget in a matrix built by matrix(), in 0-based cell coordinates.
void matrixCell(QGridLayout* grid, int row, int column, QWidget* widget);

// A muted explanatory line — why a variant exists, what a scenario is
// exercising.
QLabel* note(QVBoxLayout* into, QWidget* parent, const QString& text);

// The one shape every page needs and no component provides: what to show when
// the selected scenario has no data. Uses NovuUI's EmptyState.
QWidget* scenarioEmptyState(QWidget* parent, const QString& what);

// Same for the Error scenario.
QWidget* scenarioErrorState(QWidget* parent, const QString& what);

} // namespace novu::playground
