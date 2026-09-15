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

ThemeProfile novuAiProfile()
{
    // UNRESOLVED accent. `design/Shared/references/Novu_App_Design.png` draws
    // NovuAI's own navigation with the family cyan, which is evidence that it
    // is at least *correct*, but it is the shared reference rather than a
    // decision about NovuAI - the sheet shows one accent for everything on it.
    //
    // So it keeps the family accent and nothing is invented. When a product
    // accent is approved, it is one argument here.
    return applicationProfile(QStringLiteral("novuai"), QStringLiteral("NovuAI"));
}

ThemeProfile novuVoiceProfile()
{
    // NovuVoice/novuvoice-client/app/src/ui/NovuSpaceTheme.h had drifted
    // furthest of any of them - a cyan accent, its own surfaces, its own status
    // colours. All of it is dropped.
    //
    // UNRESOLVED accent: it keeps the family's. The cyan it had drifted to is
    // now the family accent anyway, which makes this the one product whose old
    // colour and the shared decision happen to agree.
    return applicationProfile(QStringLiteral("novuvoice"), QStringLiteral("NovuVoice"));
}

ThemeProfile novuExplorerProfile()
{
    // UNRESOLVED accent. Its graphite surfaces were the family's all along;
    // no approved product accent has been found for it.
    return applicationProfile(QStringLiteral("novuexplorer"),
                              QStringLiteral("NovuExplorer"));
}

ThemeProfile novuAdminProfile()
{
    // UNRESOLVED accent. NovuAdmin has a brand mockup and no approved screen
    // design, so production is its reference - and production consumes this
    // library, which means it currently wears whatever this profile says.
    return applicationProfile(QStringLiteral("novuadmin"), QStringLiteral("NovuAdmin"));
}

ThemeProfile novuHubProfile()
{
    // UNRESOLVED accent. The NovuHub prototype draws its *app mark* in green,
    // but a mark colour is not a UI accent and promoting one to the other is
    // exactly the kind of guess this architecture is meant to stop.
    return applicationProfile(QStringLiteral("novuhub"), QStringLiteral("NovuHub"));
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
