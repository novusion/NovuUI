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
ComponentSpec themeRolesComponent();
ComponentSpec layoutComponent();
ComponentSpec flowLayoutComponent();
ComponentSpec windowShellComponent();

// Input
ComponentSpec buttonComponent();
ComponentSpec fieldsComponent();
ComponentSpec chipComponent();
ComponentSpec formRowComponent();
ComponentSpec searchBarComponent();

// Navigation
ComponentSpec tabStripComponent();
ComponentSpec tileComponent();

// Data display
ComponentSpec badgeComponent();
ComponentSpec cardComponent();
ComponentSpec statCardComponent();
ComponentSpec propertyRowComponent();
ComponentSpec tableViewComponent();

// Feedback
ComponentSpec emptyStateComponent();
ComponentSpec infoBannerComponent();
ComponentSpec progressBarComponent();
ComponentSpec helpOverlayComponent();
ComponentSpec feedbackFormComponent();

} // namespace novu::playground::pages
