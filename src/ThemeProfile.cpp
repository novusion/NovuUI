#include "novu/ui/ThemeProfile.h"

namespace novu::ui {

namespace {

// Role defaults shared by both profiles. A profile overrides only what it
// actually differs on — that way adding a role later cannot leave a profile
// silently unset.
void applyCommonRoles(ThemeProfile& p)
{
    p.setRoleSpacing(SpacingRole::CardPadding, p.spacing.lg);
    p.setRoleSpacing(SpacingRole::CardGap, p.spacing.md);
    p.setRoleSpacing(SpacingRole::SectionGap, p.spacing.md);
    p.setRoleSpacing(SpacingRole::ToolbarGap, p.spacing.sm);
    p.setRoleSpacing(SpacingRole::FormRowGap, p.spacing.sm);
    p.setRoleSpacing(SpacingRole::TableCellPadding, p.spacing.sm);
    p.setRoleSpacing(SpacingRole::PagePadding, p.spacing.lg);

    p.setRoleType(TypeRole::PageTitle,    TypeStyle{p.type.xl, 700, false});
    p.setRoleType(TypeRole::SectionTitle, TypeStyle{p.type.lg, 700, false});
    // Bold, not demibold: at 13px the two are nearly indistinguishable on
    // screen, and the approved mockups draw a card title at full weight.
    p.setRoleType(TypeRole::CardTitle,    TypeStyle{p.type.md, 700, false});
    p.setRoleType(TypeRole::Body,         TypeStyle{p.type.md, 400, false});
    p.setRoleType(TypeRole::Caption,      TypeStyle{p.type.sm, 400, false});
    p.setRoleType(TypeRole::Label,        TypeStyle{p.type.sm, 600, false});
    p.setRoleType(TypeRole::Metric,       TypeStyle{p.type.xl, 700, false});
    p.setRoleType(TypeRole::Mono,         TypeStyle{p.type.sm, 400, true});

    p.setRoleBorder(BorderRole::Card,    p.borders.hairline);
    p.setRoleBorder(BorderRole::Input,   p.borders.hairline);
    p.setRoleBorder(BorderRole::Table,   p.borders.hairline);
    p.setRoleBorder(BorderRole::Divider, p.borders.hairline);
    p.setRoleBorder(BorderRole::Focus,   p.borders.strong);
}

} // namespace

ThemeProfile novuSpaceProfile()
{
    ThemeProfile p;
    p.id = QStringLiteral("novuspace");
    p.displayName = QStringLiteral("NovuSpace");
    // Colours are ThemeColors' defaults — the canonical graphite theme from
    // the design system, unchanged.
    p.radii = RadiusScale{0, 2, 4, 6, 8, 10, 9999};
    p.spacing = SpacingScale{4, 8, 12, 16, 24};
    p.type = TypeScale{11, 12, 13, 14, 16};
    p.controls = ControlMetrics{28, 24, 32};
    // Near-flat: the shell's language. "Flat" is a choice this profile makes,
    // not a property of the system.
    p.setRoleRadius(SurfaceRole::Card, RadiusStep::Small);
    p.setRoleRadius(SurfaceRole::Dialog, RadiusStep::Small);
    p.setRoleRadius(SurfaceRole::Button, RadiusStep::Small);
    p.setRoleRadius(SurfaceRole::Input, RadiusStep::XS);
    p.setRoleRadius(SurfaceRole::Badge, RadiusStep::Pill);
    p.setRoleRadius(SurfaceRole::Toolbar, RadiusStep::None);
    p.setRoleRadius(SurfaceRole::Table, RadiusStep::Small);
    p.setRoleRadius(SurfaceRole::Overlay, RadiusStep::Small);

    p.table.headerBg = p.colors.headerBg;
    p.table.gridLine = p.colors.border;
    p.table.selectionBg = p.colors.surfaceActive;
    p.table.hoverBg = p.colors.surfaceHover;
    p.table.headerHeight = 34;
    p.table.rowHeight = p.controls.rowHeight;

    p.chart.grid = p.colors.border;
    p.chart.axis = p.colors.textDisabled;
    p.chart.bullish = p.colors.success;
    p.chart.bearish = p.colors.danger;
    p.chart.volume = p.colors.accentLight;
    p.chart.crosshair = p.colors.textSecondary;
    p.chart.series = {p.colors.accent, p.colors.accentLight, p.colors.warning,
                      p.colors.success, p.colors.danger};

    applyCommonRoles(p);
    return p;
}

ThemeProfile novuTradeProfile()
{
    ThemeProfile p;
    p.id = QStringLiteral("novutrade");
    p.displayName = QStringLiteral("NovuTrade");

    // Mirrors frontend/tailwind.config.ts of the trading web app, so the
    // desktop reads as the same product rather than a different one.
    p.colors.windowBg = QColor(QStringLiteral("#0f172a"));   // background
    p.colors.panelBg = QColor(QStringLiteral("#111827"));    // surface
    p.colors.panelBgAlt = QColor(QStringLiteral("#0b1220"));
    p.colors.headerBg = QColor(QStringLiteral("#0b1220"));
    p.colors.surfaceHover = QColor(QStringLiteral("#1b2432"));
    p.colors.surfaceActive = QColor(QStringLiteral("#222e40"));

    p.colors.border = QColor(148, 163, 184, 46);
    p.colors.borderSubtle = QColor(148, 163, 184, 30);
    p.colors.borderHover = QColor(16, 185, 129, 90);

    p.colors.textPrimary = QColor(QStringLiteral("#E3E8F0"));
    p.colors.textSecondary = QColor(QStringLiteral("#9ca3af"));
    p.colors.textDisabled = QColor(QStringLiteral("#6b7280"));   // muted
    p.colors.textOnAccent = QColor(QStringLiteral("#062018"));

    p.colors.accent = QColor(QStringLiteral("#10b981"));        // primary
    p.colors.accentLight = QColor(QStringLiteral("#0ea5e9"));   // secondary
    // green-600, not green-500: the lighter one read as a grassy yellow-green
    // against the slate surfaces.
    p.colors.success = QColor(QStringLiteral("#16a34a"));
    p.colors.warning = QColor(QStringLiteral("#f59e0b"));       // accent
    p.colors.danger = QColor(QStringLiteral("#ef4444"));

    // The Tailwind scale the web actually uses: p-4/px-4 = 16, py-3 = 12,
    // gap-2 = 8, p-6 = 24.
    p.spacing = SpacingScale{4, 8, 12, 16, 24};
    p.type = TypeScale{11, 12, 13, 16, 20};
    p.controls = ControlMetrics{32, 26, 34};
    // rounded-lg = 8, rounded-xl = 12, rounded-2xl = 16.
    p.radii = RadiusScale{0, 4, 6, 8, 12, 16, 9999};

    p.setRoleRadius(SurfaceRole::Card, RadiusStep::Large);      // R_CARD  = 12
    p.setRoleRadius(SurfaceRole::Dialog, RadiusStep::XL);       // R_DIALOG = 16
    p.setRoleRadius(SurfaceRole::Button, RadiusStep::Small);    // R_BTN   = 6
    p.setRoleRadius(SurfaceRole::Input, RadiusStep::XS);        // R_INPUT = 4
    p.setRoleRadius(SurfaceRole::Badge, RadiusStep::Pill);
    p.setRoleRadius(SurfaceRole::Toolbar, RadiusStep::None);
    p.setRoleRadius(SurfaceRole::Table, RadiusStep::Medium);
    p.setRoleRadius(SurfaceRole::Overlay, RadiusStep::Large);

    p.table.headerBg = p.colors.headerBg;
    p.table.gridLine = p.colors.borderSubtle;
    p.table.selectionBg = QColor(16, 185, 129, 56);
    p.table.hoverBg = p.colors.surfaceHover;
    // 34, matching the row height: the approved mockup draws the header as
    // one more row of the grid, not as a taller band above it.
    p.table.headerHeight = 34;
    p.table.rowHeight = p.controls.rowHeight;

    p.chart.grid = p.colors.borderSubtle;
    p.chart.axis = p.colors.textDisabled;
    p.chart.bullish = p.colors.success;   // #22c55e, as the web candles
    p.chart.bearish = p.colors.danger;    // #ef4444
    p.chart.volume = p.colors.accentLight;
    p.chart.crosshair = p.colors.textSecondary;
    p.chart.series = {p.colors.accent, p.colors.accentLight, p.colors.warning,
                      p.colors.success, p.colors.danger};

    applyCommonRoles(p);
    // The trading UI is denser than the shell: cards sit closer together and
    // the toolbar gap follows the web's gap-2.
    p.setRoleSpacing(SpacingRole::CardPadding, p.spacing.lg);   // p-4
    p.setRoleSpacing(SpacingRole::PagePadding, p.spacing.xl);   // p-6
    p.setRoleSpacing(SpacingRole::CardGap, p.spacing.lg);       // space-y-4
    // A card title is fully bold here, not semibold. Scoped to this profile
    // rather than changed in applyCommonRoles(), because the approved mockup
    // speaks for NovuTrade only and NovuSpace keeps its lighter heading.
    p.setRoleType(TypeRole::CardTitle, TypeStyle{p.type.md, 700, false});
    return p;
}

} // namespace novu::ui
