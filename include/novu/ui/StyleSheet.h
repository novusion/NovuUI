#pragma once

#include "novu/ui/ThemeProfile.h"

#include <QString>

class QWidget;

namespace novu::ui {

// One composed stylesheet per theme profile, applied to one subtree.
//
// Why this exists at all: without it, every component ends up carrying its own
// setStyleSheet() call, and each of those calls holds its own copy of the
// palette. Twenty widgets each holding "#10b981" is twenty places to miss when
// the colour changes, and it is how a design system quietly stops being one.
//
// A component therefore declares *what it is* through a dynamic property and
// says nothing about how it looks:
//
//     button->setProperty(prop::kind, "primary");
//
// This file owns the mapping from that declaration to pixels, for every
// profile. Adding a second product means adding a ThemeProfile — not touching
// a single component.
//
// Scope, not application: apply() puts the sheet on a subtree root, never on
// qApp. A host process can run several products at once (NovuSpace hosts
// modules in-process), and one product must never restyle the shell around it.
// This is the stylesheet half of what ThemeScope already does for lookups.
//
// Widgets that paint themselves — Card, Badge, tiles — take no part in this.
// They draw from the profile directly, so there is nothing to cascade and
// nothing that a page could override.

// Property names: the vocabulary a component uses to describe itself.
namespace prop {

// Buttons: primary | secondary | ghost | danger | success | tab | pill
inline constexpr const char* kind = "novuKind";
// Tone for badges, tabs and text: neutral | primary | success | warning |
// danger | info | accent
inline constexpr const char* tone = "novuTone";
// Marks the active item of a tab strip.
inline constexpr const char* active = "novuActive";
// Geometry variant that composes with any kind: "small" for row actions.
inline constexpr const char* size = "novuSize";

} // namespace prop

// Object names, for the few containers that must be addressed without a bare
// type selector. A bare QFrame{} or QWidget{} rule would cascade into every
// QLabel inside and give each one a background and a border — the single most
// common styling defect in this codebase's history.
namespace name {

inline constexpr const char* divider = "novuDivider";
inline constexpr const char* table = "novuTable";
inline constexpr const char* scrollArea = "novuScrollArea";

} // namespace name

// Composes the sheet for a profile. Pure function of the profile.
QString styleSheetFor(const ThemeProfile& profile);

// Turns a radius token into a radius a painter can use.
//
// RadiusStep::Pill is a marker, not a measurement — the profile stores it as a
// deliberately huge number that means "half of whatever this widget's height
// turns out to be". Handing that number straight to addRoundedRect() does not
// clamp: Qt draws a degenerate shape, and a banner comes out as a tapered
// wedge instead of a rounded strip. Every self-painted component resolves its
// radius through this.
int resolveRadius(int radius, int height);

// Applies the profile's sheet to a subtree root, and marks the subtree as
// belonging to that profile so themeFor() inside it resolves correctly.
void applyStyleSheet(QWidget* root, const QString& profileId);

// Qt evaluates property selectors only when a widget is polished. After
// changing a property on a live widget its style must be recomputed, or the
// old appearance sticks — a switched tab that still looks unselected.
void repolish(QWidget* widget);

} // namespace novu::ui
