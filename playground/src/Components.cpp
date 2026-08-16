#include "novu/playground/ComponentRegistry.h"

#include "pages/Pages.h"

#include <QDebug>

namespace novu::playground {

void registerBuiltinComponents(ComponentRegistry& registry)
{
    // The complete list, in one place. Explicit rather than self-registering:
    // a static registrar in each page's translation unit can be dropped by
    // the linker from a static library, and the component would then be
    // missing from exactly the build nobody checks.
    const QVector<ComponentSpec (*)()> factories = {
        // Foundation
        &pages::typographyComponent,
        &pages::themeRolesComponent,
        &pages::layoutComponent,
        &pages::flowLayoutComponent,
        // Input
        &pages::buttonComponent,
        &pages::fieldsComponent,
        &pages::chipComponent,
        &pages::formRowComponent,
        &pages::searchBarComponent,
        // Navigation
        &pages::tabStripComponent,
        &pages::tileComponent,
        // Data display
        &pages::badgeComponent,
        &pages::cardComponent,
        &pages::statCardComponent,
        &pages::propertyRowComponent,
        &pages::tableViewComponent,
        // Feedback
        &pages::emptyStateComponent,
        &pages::infoBannerComponent,
        &pages::progressBarComponent,
    };

    for (const auto factory : factories) {
        QString error;
        if (!registry.add(factory(), &error)) {
            // Loud on purpose. A component that fails to register is a hole
            // in the visual test surface, and a silent one is worse than a
            // missing entry.
            qWarning() << "NovuUI gallery:" << error;
        }
    }
}

} // namespace novu::playground
