#include "novu/ui/ThemeProfile.h"

namespace novu::ui {

// The palettes the products already had.
//
// Every profile here was found in a shipping application, and every one of them
// turned out to be the same visual language with a different palette. They are
// in the library so that a product's identity is a profile in the design system
// rather than a header in its own repository - which is what NovuVoice's
// "frozen palette" and NovuHub's inline hex strings had become.
//
// Kept out of ThemeProfile.cpp on purpose: that file describes how a profile is
// built, this one is data. Two of these palettes will change when a product
// rebrands, and neither change should be a diff against the file that defines
// what a profile *is*.
//
// NovuExplorer's own "NovuSpace Dark" is not among the palettes below, and it
// never will be: it was byte-for-byte novuSpaceProfile() to begin with. But the
// application is registered as an identity all the same - see
// novuExplorerProfile(). An earlier pass here concluded "a consumer, not a
// profile" and left it out entirely, which was wrong for a reason that only
// became visible once every application shared one palette: a profile is no
// longer a look, it is a name. An application without one cannot be scoped to,
// cannot say which product a window belongs to, and cannot be asked which
// components it uses.

namespace {

// One palette, the shell's geometry.
//
// The point of the exercise, stated as code: a product picks colours. It does
// not pick a control height, a radius, a type scale or a title-bar height.
// Anything a variant wanted to change beyond the palette would be a finding to
// argue about, not a parameter to add here.
// An application's identity: the family palette, and its own accent.
//
// One palette, every application - *except* the accent. That is the line the
// approved material draws and it is the only line: the shared design system
// fixes the surfaces, the text ramp and the four status colours for the whole
// family, and each product is then identified by one colour on top of them.
//
// `accent` invalid means "this product has no approved accent of its own yet",
// and it keeps the family's. That is a real state, not a placeholder to be
// filled in with something that looks nice - see docs/THEMING.md.
//
// applicationProfilesShareOnePalette() in the tests holds the rest of the
// palette identical across products, so a profile that starts stating surfaces
// here is still re-forking and is still caught.
ThemeProfile applicationProfile(const QString& id, const QString& displayName,
                                const QColor& accent = QColor(),
                                const QColor& accentLight = QColor(),
                                const QColor& inkOnAccent = QColor())
{
    ThemeProfile p = novusionBaseProfile();
    p.id = id;
    p.displayName = displayName;
    if (accent.isValid()) {
        p.colors.accent = accent;
        p.colors.accentLight = accentLight.isValid() ? accentLight : accent.lighter(115);
        if (inkOnAccent.isValid()) {
            p.colors.textOnAccent = inkOnAccent;
        }
        // The chart series and the window chrome are derived from the palette,
        // so they have to be re-derived after the accent moves or a product
        // gets the family's accent in its charts and its own in its buttons.
        applyDerivedTokens(p);
    }
    return p;
}

// A selectable *theme*, which is a different thing from an application.
//
// NovuExplorer offers these in its settings; they are a user's choice of
// appearance, not a product's identity, so they are allowed a palette of their
// own where an application is not.
ThemeProfile graphiteVariant(const QString& id, const QString& displayName,
                             const ThemeColors& colors)
{
    ThemeProfile p = novuSpaceProfile();
    p.id = id;
    p.displayName = displayName;
    p.colors = colors;
    // Re-derives table chrome, chart series and window chrome against the new
    // palette. Without it the variant would wear the graphite title bar.
    applyDerivedTokens(p);
    return p;
}

QColor hex(const char* value)
{
    return QColor(QLatin1String(value));
}

} // namespace

ThemeProfile novuVoiceProfile()
{
    // NovuVoice-brand-mockup.html states the product identity explicitly:
    // --acc:#C47B85; --acc-l:#DA98A1; --acc-d:#A85F6A;
    // --ink-on-accent:#1B1014. Status hues stay on the shared semantic palette.
    ThemeProfile p = applicationProfile(QStringLiteral("novuvoice"),
                                        QStringLiteral("NovuVoice"),
                                        hex("#C47B85"), hex("#DA98A1"), hex("#1B1014"));

    // ── geometry ──────────────────────────────────────────────────────────
    //
    // The palette above is the family's and stays the family's. The *scale* is
    // not: the same approved design states a radius scale and a control height
    // of its own, and it states them as tokens rather than as one-off numbers -
    //
    //     --r-sm:6px; --r-md:8px; --r-lg:11px; --r-xl:14px;
    //     .btn{height:28px} .btn.sm{height:24px} .titlebar{height:44px}
    //
    // - which is exactly the case ThemeProfile documents: a product that
    // genuinely needs different geometry overrides the roles it needs. A
    // communication client is a denser surface than a workspace: NovuSpace's
    // 32px control and 10px button corner put four fewer conversations on
    // screen and round every chip in a message row a step too far.
    p.radii = RadiusScale{0, 4, 6, 8, 11, 14, 9999};
    p.controls.height = 28;        // .btn
    p.controls.heightSmall = 24;   // .btn.sm
    // .btn svg 13, .btn.sm svg 11, and a page-header mark 17.
    p.icons = IconMetrics{11, 13, 17};
    // `.titlebar{height:44px}` - the shared default, restated because
    // novuSpaceProfile(), which this is built from, sets its own 42.
    p.window.titleBarHeight = 44;

    p.setRoleRadius(SurfaceRole::Card, RadiusStep::Large);     // .card 11
    p.setRoleRadius(SurfaceRole::Button, RadiusStep::Small);   // .btn --r-sm
    p.setRoleRadius(SurfaceRole::Input, RadiusStep::Small);    // .inp --r-sm
    p.setRoleRadius(SurfaceRole::Table, RadiusStep::Large);
    p.setRoleRadius(SurfaceRole::Window, RadiusStep::XL);      // .win 14
    p.setRoleRadiusPx(SurfaceRole::Dialog, 14);                // .dlg --r-xl
    p.setRoleRadiusPx(SurfaceRole::FloatingWindow, 11);        // .float --r-lg
    p.setRoleRadiusPx(SurfaceRole::Overlay, 11);               // .pop --r-lg
    p.setRoleRadiusPx(SurfaceRole::IconButton, 6);             // .icobtn --r-sm
    p.setRoleRadiusPx(SurfaceRole::ActionIcon, 6);
    p.setRoleRadiusPx(SurfaceRole::Segment, 4);                // .seg button

    // The navigation rail, row by row, as `.sb-item` and `.sb-head` state it:
    //
    //     .sb-item{height:28px;color:var(--text2)}
    //     .sb-item.on{color:var(--text)}   .sb-item.on svg.i{color:var(--acc)}
    //     .sb-head{padding:0 6px 6px}      .sb-sec{margin-bottom:13px}
    //
    // The current row takes the primary ink rather than the accent, because
    // this rail spends its accent twice already - on the glyph and on the 2px
    // edge beside the row. NovuAI's rail reached the same answer for the same
    // reason; NovuSpace's did not, because its rail is the only accented thing
    // on its page.
    p.controls.navRowHeight = 28;
    p.controls.navRestInk = NavInk::Secondary;
    p.controls.navCurrentInk = NavInk::Primary;
    // `.cnt.un` and `.cnt.miss` are both filled pills - the only counts this
    // rail draws.
    p.controls.navCountStyle = NavCountStyle::Filled;
    // `.cnt.un{background:var(--acc);color:var(--ink-on-accent)}` - an unread
    // count in this rail is the product's own number, not a status.
    p.controls.navCountInk = NavInk::Accent;
    p.controls.navSectionBottom = 6;
    p.setRoleSpacing(SpacingRole::SectionGap, 13);             // .sb-sec

    // -- the one status this product owns --------------------------------
    //
    // NovuVoice is the only application whose accent *is* one of the family's
    // status hues: Danger and this product's identity are both #C47B85.
    // Everywhere else the two layers never meet, so the shared palette costs
    // nothing; here they would be the same colour, and End Call, Decline, a
    // missed call and a failed message would all be drawn in the product
    // accent - which is exactly what the approved design forbids:
    //
    //     "The accent means NOVUVOICE. It is never a status colour ... a rose
    //      element on screen is always identity or selection, never state."
    //
    // So that design states a red of its own, and it is the only token in it
    // that is not the family's:
    //
    //     --red:#E5484D; --red-rgb:229,72,77;
    //
    // This is the case ThemeColors documents - a profile overrides one of the
    // status hues "only when its *surface* forces it to, never to have a look
    // of its own". The surface forces it. Success, Warning and Info stay
    // shared, and no other profile moves.
    //
    // Set before applyDerivedTokens() so semantic.danger, the close button's
    // hover plate and the chart's bearish series are all re-derived from it.
    p.colors.danger = hex("#E5484D");
    p.colors.dangerPlate = hex("#E5484D");

    applyDerivedTokens(p);
    return p;
}

ThemeProfile novuExplorerProfile()
{
    // NovuExplorer-brand-mockup-v2.html: --acc:#58B09E, --acc-l:#77CCB9,
    // --acc-d:#3F8F80, --ink-on-accent:#10171A. NovuAI's sheet names the same
    // teal as Explorer's, and NovuSpace and NovuAdmin both carry it as
    // `--explorer` for when they draw the product.
    ThemeProfile p = applicationProfile(QStringLiteral("novuexplorer"),
                                        QStringLiteral("NovuExplorer"),
                                        hex("#58B09E"), hex("#77CCB9"), hex("#10171A"));
    // `--row-h:24px`, `--side-w:216px`, and a 24px status strip: the densest
    // of the four, which is what a file manager is.
    p.controls.rowHeight = 24;
    p.window.statusBarHeight = 24;
    return p;
}

ThemeProfile novuAdminProfile()
{
    // NovuAdmin-brand-mockup.html: --acc:#3CCB7F, --acc-l:#6ADB9E,
    // --ink-on-accent:#151A20. It shares NovuHub's green, which is a decision
    // the design made and not a collision - the two are the same operator's
    // tools. The green is also the Success plate; a product is allowed to wear
    // a status hue as its accent, and the two layers stay separate because the
    // accent moves per profile while Success does not.
    return applicationProfile(QStringLiteral("novuadmin"), QStringLiteral("NovuAdmin"),
                              hex("#3CCB7F"), hex("#6ADB9E"), hex("#151A20"));
}

ThemeProfile novuHubProfile()
{
    // Resolved, and the earlier objection here was right on the principle and
    // wrong on the fact: it is not the mark colour promoted to an accent. The
    // NovuHub brand mockup states `--acc:#3CCB7F` and `--ink-on-accent:#151A20`
    // as tokens, and NovuAI's sheet independently calls Hub green. The light
    // step is NovuAdmin's, which shares the accent.
    return applicationProfile(QStringLiteral("novuhub"), QStringLiteral("NovuHub"),
                              hex("#3CCB7F"), hex("#6ADB9E"), hex("#151A20"));
}

ThemeProfile midnightBlueProfile()
{
    // NovuExplorer, user-selectable. Drawn from GitHub dark down to #0D1117 and
    // #1F6FEB, so the two status colours the source does not define are taken
    // from the same family rather than left as the shell's.
    ThemeColors c;
    c.windowBg = hex("#0D1117");
    c.panelBg = hex("#161B22");
    c.panelBgAlt = hex("#1C222D");
    c.headerBg = hex("#0D1218");
    c.surfaceHover = hex("#1C2430");
    c.surfaceActive = hex("#182230");
    c.border = hex("#212A36");
    c.borderSubtle = hex("#141C26");
    c.borderHover = hex("#303C4C");
    c.textPrimary = hex("#C9D1D9");
    c.textSecondary = hex("#7D8A98");
    c.textDisabled = hex("#48525E");
    c.textOnAccent = hex("#FFFFFF");
    c.accent = hex("#1F6FEB");
    c.accentLight = hex("#58A6FF");
    // Its own status colours were the shared ones to within a digit
    // (#F85549 against #F85149), so they are dropped rather than kept as a
    // near-miss nobody would ever notice was different.
    return graphiteVariant(QStringLiteral("midnight-blue"),
                           QStringLiteral("Midnight Blue"), c);
}

ThemeProfile warmGrayProfile()
{
    // NovuExplorer, user-selectable. Success and warning stay the shell's: the
    // source defines neither, and nothing in a neutral grey palette with a gold
    // accent says what they ought to be instead. Guessing would put a colour in
    // the system that no design ever asked for.
    ThemeColors c;
    c.windowBg = hex("#1A1A1A");
    c.panelBg = hex("#242424");
    c.panelBgAlt = hex("#2A2A2A");
    c.headerBg = hex("#1C1C1C");
    c.surfaceHover = hex("#2E2E2E");
    c.surfaceActive = hex("#2C2A26");
    c.border = hex("#3A3834");
    c.borderSubtle = hex("#262422");
    c.borderHover = hex("#4A4844");
    c.textPrimary = hex("#E0DDD5");
    c.textSecondary = hex("#9A968E");
    c.textDisabled = hex("#605C56");
    // Gold is a light accent, so the ink on top of it is the background, not
    // white.
    c.textOnAccent = hex("#1A1A1A");
    c.accent = hex("#D4A853");
    c.accentLight = hex("#E0C070");
    // The source's own #FF6655 danger is dropped with the rest: status is
    // shared, and a gold-accented theme has no reason to redefine "failed".
    return graphiteVariant(QStringLiteral("warm-gray"), QStringLiteral("Warm Gray"), c);
}

ThemeProfile darkPurpleProfile()
{
    // NovuExplorer, user-selectable. Same note as Warm Gray on success and
    // warning.
    ThemeColors c;
    c.windowBg = hex("#121018");
    c.panelBg = hex("#1C1A24");
    c.panelBgAlt = hex("#221F2C");
    c.headerBg = hex("#0F0D15");
    c.surfaceHover = hex("#2A2733");
    c.surfaceActive = hex("#302D3C");
    c.border = hex("#2A2736");
    c.borderSubtle = hex("#1E1C28");
    c.borderHover = hex("#3D3852");
    c.textPrimary = hex("#E8E0F8");
    c.textSecondary = hex("#A89EC8");
    c.textDisabled = hex("#6A6080");
    c.textOnAccent = hex("#FFFFFF");
    c.accent = hex("#A855F7");
    c.accentLight = hex("#C084FC");
    return graphiteVariant(QStringLiteral("dark-purple"), QStringLiteral("Dark Purple"), c);
}

ThemeProfile novuLightProfile()
{
    // NovuExplorer, user-selectable, and the only light palette in the system.
    //
    // It is also the proof that the token derivation is a derivation: the
    // window chrome, the table chrome and the chart series all come out light
    // without a single conditional, and the shell hands DWM a light frame
    // because it reads the background's luminance rather than a flag.
    //
    // Success and warning are GitHub light, for the same reason Midnight Blue
    // takes GitHub dark - the palette is from that family, and the shell's
    // #98C379 on white is too pale to read.
    ThemeColors c;
    c.windowBg = hex("#F5F6F8");
    c.panelBg = hex("#FFFFFF");
    c.panelBgAlt = hex("#F0F2F5");
    c.headerBg = hex("#E7EAEE");
    c.surfaceHover = hex("#DCE0E6");
    c.surfaceActive = hex("#D0D5DE");
    c.border = hex("#C8CCD4");
    c.borderSubtle = hex("#D8DCE4");
    c.borderHover = hex("#9AA0AA");
    c.textPrimary = hex("#1F2328");
    c.textSecondary = hex("#57606A");
    c.textDisabled = hex("#8C959F");
    c.textOnAccent = hex("#FFFFFF");
    c.accent = hex("#00A63E");
    c.accentLight = hex("#22C55E");
    // The one place a status override is justified, and it is about surface
    // polarity rather than product identity: the shared hues are tuned for dark
    // panels and fall to 3.37, 2.52, 3.35 and 2.53 against white. These are the
    // same four meanings darkened until they read, and they clear 4.5:1 on both
    // #FFFFFF and #F5F6F8.
    c.success = hex("#1A7F37");
    c.warning = hex("#9A6700");
    c.danger = hex("#CF222E");
    c.info = hex("#0969DA");
    return graphiteVariant(QStringLiteral("novu-light"), QStringLiteral("Novu Light"), c);
}

bool isApplicationProfile(const QString& profileId)
{
    // Listed rather than inferred: "is this a product" is a fact about the
    // company, not something a palette can be asked. The four selectable themes
    // and any development overlay are deliberately not here.
    static const QStringList applications = {
        QStringLiteral("novuspace"),
        QStringLiteral("novuai"),
        QStringLiteral("novuexplorer"),
        QStringLiteral("novuhub"),
        QStringLiteral("novuadmin"),
        QStringLiteral("novuvoice"),
        QStringLiteral("novutrade"),
    };
    return applications.contains(profileId);
}

QList<ThemeProfile> builtinProfiles()
{
    // novuspace first: ThemeManager takes the head of this list as the default,
    // and the canonical theme is what a consumer with no configuration gets.
    return {novuSpaceProfile(),
            novuTradeProfile(),
            novuAiProfile(),
            novuAiLightProfile(),
            novuExplorerProfile(),
            novuHubProfile(),
            novuAdminProfile(),
            novuVoiceProfile(),
            midnightBlueProfile(),
            warmGrayProfile(),
            darkPurpleProfile(),
            novuLightProfile()};
}

} // namespace novu::ui
