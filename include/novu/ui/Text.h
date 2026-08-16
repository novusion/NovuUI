#pragma once

#include "novu/ui/ThemeProfile.h"

#include <QColor>
#include <QFont>
#include <QString>

class QLabel;
class QWidget;

namespace novu::ui {

// Text by role, never by number.
//
// A page asks for TypeRole::CardTitle or Ink::Muted; the profile decides the
// pixel size and the colour. This is the difference between a design system and
// a habit: the same label is 13px graphite in one product and 13px slate in
// another without a single conditional at the call site.
//
// Colour is applied through the widget palette rather than a stylesheet, so a
// label can be recoloured live (connected/disconnected, profit/loss) without
// re-parsing a sheet and without competing with the scope's composed sheet.

// Semantic colour roles for text.
enum class Ink {
    Default,     // primary body text
    Secondary,
    Muted,
    Accent,      // the profile's accent
    Success,
    Warning,
    Danger,
    Info,
};

QColor inkColor(const QWidget* context, Ink ink);

// Font for a type role, resolved against the widget's scope.
QFont roleFont(const QWidget* context, TypeRole role, bool bold = false);

// Monospace at the same role size, for ids, prices and paths.
QFont roleMonoFont(const QWidget* context, TypeRole role, bool bold = false);

// Icon font. Windows 11 ships Segoe Fluent Icons; Segoe MDL2 Assets is the
// fallback on older systems and shares the code points, so a glyph looks the
// same on both.
QFont iconFont(int pixelSize);

// A label carrying a type role and an ink role. Creates no container.
QLabel* label(const QString& text, QWidget* parent,
              TypeRole role = TypeRole::Body, Ink ink = Ink::Default,
              bool bold = false);

// Uppercase heading above a group of controls.
QLabel* caption(const QString& text, QWidget* parent);

// Applies a type role and an ink to a label that already exists.
//
// label() covers creation; this covers everything else - a title that changes
// weight, a status line that changes meaning, a label the caller did not build.
// Without it every consumer writes the same setFont/setInk pair, which is how
// the roles start drifting apart again.
void applyText(QLabel* label, TypeRole role, Ink ink = Ink::Default, bool bold = false);

// Recolours a live label by role.
void setInk(QLabel* label, Ink ink);
void setInk(QLabel* label, const QColor& color);

} // namespace novu::ui
