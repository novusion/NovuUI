#pragma once

#include <QString>

class QWidget;

namespace novu::ui {

// Marks a widget subtree as belonging to a theme profile.
//
// Not a widget: it is a property written onto an existing widget. Adding a
// container just to carry a theme would be exactly the kind of layer this
// design system exists to remove — a module's root widget already exists, so
// the scope rides on that.
//
//     ThemeScope::apply(novuTradeWorkspace, "novutrade");
//
// Everything constructed under that root then resolves to the NovuTrade
// profile, while the NovuSpace shell around it keeps its own.
class ThemeScope
{
public:
    // Idempotent. Applying a different profile to a live subtree repaints it.
    static void apply(QWidget* root, const QString& profileId);

    // The profile id set directly on this widget, or empty if it is not a
    // scope root itself.
    static QString of(const QWidget* widget);

    // The profile id in effect for this widget: its own scope, or the nearest
    // ancestor's, or empty when none is set.
    static QString resolve(const QWidget* widget);

    static void clear(QWidget* root);
};

} // namespace novu::ui
