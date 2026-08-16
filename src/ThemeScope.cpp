#include "novu/ui/ThemeScope.h"

#include "novu/ui/ThemeManager.h"

#include <QVariant>
#include <QWidget>

namespace novu::ui {

namespace {
// Dynamic property rather than a member: any widget can become a scope root
// without deriving from a library base class, so a module keeps its own
// class hierarchy.
constexpr const char* kScopeProperty = "novuThemeProfile";
} // namespace

void ThemeScope::apply(QWidget* root, const QString& profileId)
{
    if (!root) {
        return;
    }
    if (root->property(kScopeProperty).toString() == profileId) {
        return;
    }
    root->setProperty(kScopeProperty, profileId);
    // The subtree may already be built and visible — retheming it is the
    // whole point of allowing this at runtime.
    ThemeManager::instance().notifyThemeChanged(profileId);
}

QString ThemeScope::of(const QWidget* widget)
{
    return widget ? widget->property(kScopeProperty).toString() : QString();
}

QString ThemeScope::resolve(const QWidget* widget)
{
    for (const QWidget* w = widget; w; w = w->parentWidget()) {
        const QString id = w->property(kScopeProperty).toString();
        if (!id.isEmpty()) {
            return id;
        }
    }
    return {};
}

void ThemeScope::clear(QWidget* root)
{
    if (!root) {
        return;
    }
    root->setProperty(kScopeProperty, QVariant());
    ThemeManager::instance().notifyThemeChanged();
}

} // namespace novu::ui
