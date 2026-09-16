#pragma once

#include "novu/playground/ComponentSpec.h"

namespace novu::playground::pages {

// One function per previewable component. Each returns a fully described
// ComponentSpec — metadata and a builder — and knows nothing about the
// gallery that will display it.
//
// Adding a NovuUI component to the gallery is four lines: a new
// pages/FooPage.cpp, a declaration here, a call in Components.cpp, and the
// file in playground/CMakeLists.txt. No navigation, search or theme code
// changes.

// Foundation
ComponentSpec typographyComponent();
ComponentSpec motionComponent();
ComponentSpec dropTargetComponent();
ComponentSpec iconComponent();
ComponentSpec separatorComponent();
ComponentSpec settingsShellComponent();
ComponentSpec themeRolesComponent();
ComponentSpec themeStylingComponent();
ComponentSpec layoutComponent();
ComponentSpec flowLayoutComponent();
ComponentSpec windowShellComponent();
ComponentSpec floatingPanelComponent();
ComponentSpec splitterComponent();
ComponentSpec gateCardComponent();
ComponentSpec activePanelComponent();

// Input
ComponentSpec bottomPanelComponent();
ComponentSpec buttonComponent();
ComponentSpec iconButtonComponent();
ComponentSpec toggleComponent();
ComponentSpec segmentedComponent();
ComponentSpec sliderComponent();
ComponentSpec swatchComponent();
ComponentSpec fieldComponent();
ComponentSpec fieldsComponent();
ComponentSpec chipComponent();
ComponentSpec settingRowComponent();
ComponentSpec formSubmissionComponent();
ComponentSpec formRowComponent();
ComponentSpec searchBarComponent();

// Navigation
ComponentSpec tabStripComponent();
ComponentSpec tileComponent();
ComponentSpec navListComponent();
ComponentSpec sidebarComponent();
ComponentSpec moduleBarComponent();
ComponentSpec disclosureComponent();
ComponentSpec popoverComponent();
ComponentSpec statusBarComponent();
ComponentSpec pageHeaderComponent();
ComponentSpec treeViewComponent();
ComponentSpec tabBarComponent();
ComponentSpec drawerComponent();
ComponentSpec commandPaletteComponent();
ComponentSpec selectionBarComponent();

// Data display
ComponentSpec avatarComponent();
ComponentSpec badgeComponent();
ComponentSpec cardComponent();
ComponentSpec statCardComponent();
ComponentSpec statusDotComponent();
ComponentSpec propertyRowComponent();
ComponentSpec tableViewComponent();
ComponentSpec appMarkComponent();
ComponentSpec productMarkComponent();
ComponentSpec keyCapComponent();
ComponentSpec shortcutListComponent();
ComponentSpec logViewComponent();
ComponentSpec diffViewComponent();

// Feedback
ComponentSpec emptyStateComponent();
ComponentSpec infoBannerComponent();
ComponentSpec progressBarComponent();
ComponentSpec helpOverlayComponent();
ComponentSpec feedbackFormComponent();
ComponentSpec spinnerComponent();
ComponentSpec skeletonComponent();
ComponentSpec statusLineComponent();
ComponentSpec stepsComponent();
ComponentSpec tooltipComponent();
ComponentSpec toastComponent();
ComponentSpec dialogComponent();

} // namespace novu::playground::pages
