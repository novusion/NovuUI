#pragma once

#include "novu/ui/Text.h"

#include <QString>
#include <QWidget>

class QHBoxLayout;
class QLineEdit;
class QBoxLayout;

namespace novu::ui {

// Form and property rows, and the search box every list panel needs.
//
// These are the last three shapes every page was still hand-rolling. Each of
// them had drifted: a settings page with a 120px label column next to one
// with 140, a key/value line right-aligned here and left-aligned there, a
// search box that was a bare QLineEdit in one panel and a styled one in the
// next.
//
// All three follow the library's rule of returning a layout wherever a layout
// is enough — a labelled row does not need a widget to live in.

// Label on the left at a fixed column width, control on the right, optional
// muted hint after it.
//
// The label column is fixed rather than measured on purpose: rows in a form
// must line up with each other, and a measured column makes every row as wide
// as its own caption, which is the staircase this replaces.
QHBoxLayout* formRow(QBoxLayout* into, QWidget* parent, const QString& caption,
                     QWidget* control, const QString& hint = {});

// Caption left, value right, pushed apart. For read-only detail panels —
// connection state, versions, counts.
//
// `mono` puts the value in the profile's monospace family, which is what ids,
// hashes and paths need in order to be comparable by eye down a column.
QHBoxLayout* propertyRow(QBoxLayout* into, QWidget* parent, const QString& caption,
                         const QString& value, Ink tone = Ink::Default,
                         bool mono = false);

// A search field with the profile's control geometry and a clear button.
//
// A named factory rather than lineEdit() with a placeholder, because search is
// the one input whose behaviour a user expects to be identical everywhere:
// the clear affordance, the height and the placeholder voice are decided once
// here rather than per panel.
QLineEdit* searchBar(QWidget* parent, const QString& placeholder = {}, int width = 0);

} // namespace novu::ui
