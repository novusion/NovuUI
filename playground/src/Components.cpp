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
        &pages::motionComponent,
        &pages::iconComponent,
        &pages::separatorComponent,
        &pages::themeStylingComponent,
        &pages::themeRolesComponent,
        &pages::layoutComponent,
        &pages::flowLayoutComponent,
        &pages::windowShellComponent,
        &pages::floatingPanelComponent,
        &pages::splitterComponent,
        &pages::gateCardComponent,
        &pages::activePanelComponent,
        // Input
        &pages::bottomPanelComponent,
        &pages::buttonComponent,
        &pages::iconButtonComponent,
        &pages::toggleComponent,
        &pages::segmentedComponent,
        &pages::sliderComponent,
        &pages::swatchComponent,
        &pages::fieldComponent,
        &pages::fieldsComponent,
        &pages::chipComponent,
        &pages::settingRowComponent,
        &pages::formSubmissionComponent,
        &pages::formRowComponent,
        &pages::searchBarComponent,
        // Navigation
        &pages::tabStripComponent,
        &pages::tileComponent,
        &pages::navListComponent,
        &pages::settingsShellComponent,
        &pages::sidebarComponent,
        &pages::moduleBarComponent,
        &pages::disclosureComponent,
        &pages::popoverComponent,
        &pages::statusBarComponent,
        &pages::pageHeaderComponent,
        &pages::treeViewComponent,
        &pages::tabBarComponent,
        &pages::drawerComponent,
        &pages::commandPaletteComponent,
        &pages::selectionBarComponent,
        // Data display
        &pages::avatarComponent,
        &pages::badgeComponent,
        &pages::cardComponent,
        &pages::statCardComponent,
        &pages::statusDotComponent,
        &pages::propertyRowComponent,
        &pages::tableViewComponent,
        &pages::appMarkComponent,
        &pages::keyCapComponent,
        &pages::shortcutListComponent,
        &pages::logViewComponent,
        &pages::diffViewComponent,
        // Feedback
        &pages::emptyStateComponent,
        &pages::infoBannerComponent,
        &pages::progressBarComponent,
        &pages::helpOverlayComponent,
        &pages::feedbackFormComponent,
        &pages::spinnerComponent,
        &pages::skeletonComponent,
        &pages::statusLineComponent,
        &pages::stepsComponent,
        &pages::tooltipComponent,
        &pages::toastComponent,
        &pages::dialogComponent,
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
