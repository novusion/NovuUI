#include "Pages.h"

#include "novu/playground/ExampleLayout.h"
#include "novu/playground/SampleData.h"

#include "novu/ui/Button.h"
#include "novu/ui/Icon.h"
#include "novu/ui/Card.h"
#include "novu/ui/StyleSheet.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/ThemeScope.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>

#include <iterator>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

struct VariantRow
{
    const char* name;
    Button::Variant variant;
};

const VariantRow kVariants[] = {
    {"Primary", Button::Variant::Primary},
    {"Secondary", Button::Variant::Secondary},
    // `.btn` itself: a hairline and a transparent plate until the pointer
    // arrives. Between Secondary (filled at rest, to read against a card) and
    // Ghost (no border at rest, for chrome).
    {"Outlined", Button::Variant::Outlined},
    {"Ghost", Button::Variant::Ghost},
    {"Success", Button::Variant::Success},
    {"Warning", Button::Variant::Warning},
    {"Danger", Button::Variant::Danger},
};

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    // The base profile carries the shared semantic palette every profile
    // outside novu-light is required to agree on (see
    // everyProfileSharesTheSemanticPalette); only the accent - what Primary's
    // fill and its disabled dimming actually read - is taken from the
    // gallery's active theme, so a disabled Primary here is still *that*
    // theme's Primary, never a colour borrowed from novusionBaseProfile's own
    // unrelated palette.
    auto compactProfile=novusionBaseProfile();compactProfile.id=ctx.profileId+QStringLiteral("-button-opacity-example");
    const ThemeProfile& active=ThemeManager::instance().profile(ctx.profileId);
    compactProfile.colors.accent=active.colors.accent;
    compactProfile.colors.accentLight=active.colors.accentLight;
    compactProfile.colors.textOnAccent=active.colors.textOnAccent;
    compactProfile.controls.buttonPaddingSmall=11;compactProfile.controls.buttonPaddingIncludesBorder=false;compactProfile.controls.buttonDisabledOpacity=.42;
    ThemeManager::instance().registerProfile(compactProfile);
    auto* opacityExample=exampleCard(page,QStringLiteral("Disabled surface opacity"),QStringLiteral("A disabled primary retains its filled surface at 42 percent opacity."));
    ThemeScope::apply(opacityExample,compactProfile.id);
    auto* dimmed=new Button(QStringLiteral("Send"),Button::Variant::Primary,opacityExample);dimmed->setControlSize(ControlSize::Small);dimmed->setEnabled(false);opacityExample->body()->addWidget(dimmed,0,Qt::AlignLeft);column->addWidget(opacityExample);
    const bool disabledScenario = ctx.scenario == Scenario::Disabled;
    const QString overrideText = ctx.scenario == Scenario::LongText
        ? ctx.data->text(QStringLiteral("long_text")).left(70)
        : ctx.scenario == Scenario::LongIdentifier
            ? ctx.data->text(QStringLiteral("long_identifier"))
            : QString();

    // ── the matrix ───────────────────────────────────────────────────────
    // Variant down, interaction state across. A Ghost button that stops
    // matching a Primary button's height shows up here as a bent column,
    // which is the whole reason this presentation exists.
    {
        Card* card = exampleCard(page, QStringLiteral("Variant matrix"),
                                 QStringLiteral("Every variant has identical geometry — "
                                                "only the colour differs."));
        QVBoxLayout* body = card->body();

        QStringList rows;
        for (const VariantRow& row : kVariants) {
            rows.append(QString::fromLatin1(row.name));
        }
        QGridLayout* grid = matrix(body, card,
                                   {QStringLiteral("Normal"), QStringLiteral("Focused"),
                                    QStringLiteral("Disabled"), QStringLiteral("Small")},
                                   rows);

        for (int r = 0; r < static_cast<int>(std::size(kVariants)); ++r) {
            const VariantRow& row = kVariants[r];
            const QString text = overrideText.isEmpty() ? QString::fromLatin1(row.name)
                                                        : overrideText;

            auto* normal = new Button(text, row.variant, card);
            matrixCell(grid, r, 0, normal);

            // Focus is a real state, set the way a page would set it, not
            // painted to look like one.
            auto* focused = new Button(text, row.variant, card);
            focused->setFocus(Qt::TabFocusReason);
            matrixCell(grid, r, 1, focused);

            auto* disabled = new Button(text, row.variant, card);
            disabled->setEnabled(false);
            matrixCell(grid, r, 2, disabled);

            // The one geometry variant: row actions. Set through the
            // component's own API — Button composes its stylesheet from the
            // profile, so a bare property on the widget reaches the scope sheet
            // and is then overwritten by the button's own.
            auto* small = new Button(text, row.variant, card);
            small->setControlSize(ControlSize::Small);
            matrixCell(grid, r, 3, small);
        }

        note(body, card,
             QStringLiteral("Hover and pressed are drawn by Qt from the scope stylesheet and "
                            "cannot be forced from outside the widget; hover a cell to see "
                            "them. Everything else here is a real widget state."));
        column->addWidget(card);
    }

    // ── the named factories ──────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Factories"),
                                 QStringLiteral("How the design system names them at a call site."));
        QVBoxLayout* body = card->body();
        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Helpers"));
        row->addWidget(primaryButton(QStringLiteral("Save"), card));
        row->addWidget(secondaryButton(QStringLiteral("Cancel"), card));
        row->addWidget(ghostButton(QStringLiteral("Reset"), card));
        row->addWidget(successButton(QStringLiteral("Start"), card));
        row->addWidget(warningButton(QStringLiteral("Go live"), card));
        row->addWidget(dangerButton(QStringLiteral("Delete"), card));
        row->addWidget(detachButton(card));
        row->addStretch(1);
        note(body, card,
             QStringLiteral("detachButton() is a named factory rather than a ghost button so "
                            "the gesture reads the same in every panel that offers it."));
        column->addWidget(card);
    }

    // ── with a glyph ─────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("With a glyph"),
                                 QStringLiteral("Named from the icon set, so it is redrawn in "
                                                "the variant's own ink."));
        QVBoxLayout* body = card->body();

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Leading icon"));
        const struct { const char* text; Glyph glyph; Button::Variant variant; } withIcons[] = {
            {"New bot", Glyph::Plus, Button::Variant::Primary},
            {"Refresh", Glyph::Refresh, Button::Variant::Secondary},
            {"Export", Glyph::Download, Button::Variant::Ghost},
            {"Start", Glyph::Play, Button::Variant::Success},
            {"Go live", Glyph::Warning, Button::Variant::Warning},
            {"Delete", Glyph::Trash, Button::Variant::Danger},
        };
        for (const auto& entry : withIcons) {
            auto* button = new Button(QString::fromLatin1(entry.text), entry.variant, card);
            button->setGlyph(entry.glyph);
            button->setEnabled(!disabledScenario);
            row->addWidget(button);
        }
        row->addStretch(1);

        QHBoxLayout* smallRow = exampleRow(body, card, QStringLiteral("Small"));
        for (const auto& entry : withIcons) {
            auto* button = new Button(QString::fromLatin1(entry.text), entry.variant, card);
            button->setGlyph(entry.glyph);
            button->setControlSize(ControlSize::Small);
            smallRow->addWidget(button);
        }
        smallRow->addStretch(1);

        note(body, card,
             QStringLiteral("A Glyph rather than a QIcon: an icon handed in by a page is a "
                            "drawing at a colour the page chose, and it stops following the "
                            "theme the moment the profile changes."));
        column->addWidget(card);
    }

    // ── in a row ─────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("In a toolbar"),
                                 QStringLiteral("Mixed captions, one row: heights must not stagger."));
        QVBoxLayout* body = card->body();
        QHBoxLayout* row = exampleRow(body, card, QString());
        const QStringList captions = {QStringLiteral("OK"), QStringLiteral("Apply"),
                                      QStringLiteral("Save and close"),
                                      QStringLiteral("Reset to defaults")};
        for (int i = 0; i < captions.size(); ++i) {
            auto* button = new Button(captions.at(i),
                                      kVariants[i % static_cast<int>(std::size(kVariants))].variant,
                                      card);
            button->setEnabled(!disabledScenario);
            row->addWidget(button);
        }
        row->addStretch(1);
        column->addWidget(card);
    }

    auto* compact=new Card(QStringLiteral("Compact pill profile"),page);
    auto profile=themeFor(page);
    profile.id=QStringLiteral("gallery-compact-pill");
    profile.controls.smallButtonRadius=RadiusStep::Pill;
    ThemeManager::instance().registerProfile(profile);
    ThemeScope::apply(compact,profile.id);
    auto* compactAction=new Button(QStringLiteral("Resume work"),Button::Variant::Primary,compact);
    compactAction->setControlSize(ControlSize::Small);
    compact->body()->addWidget(compactAction);
    column->addWidget(compact);
    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec buttonComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("button");
    spec.name = QStringLiteral("Button");
    spec.category = Category::Input;
    spec.description = QStringLiteral(
        "The single button used across all Novu applications. Variant selects colour only "
        "— geometry is identical, and computed rather than styled.");
    spec.headerPath = QStringLiteral("include/novu/ui/Button.h");
    spec.sourcePath = QStringLiteral("src/Button.cpp");
    spec.designedFor = {QStringLiteral("novuspace"),
                   QStringLiteral("novuai"),
                   QStringLiteral("novuexplorer"),
                   QStringLiteral("novuhub"),
                   QStringLiteral("novuvoice"),
                   QStringLiteral("novutrade")};
    spec.usedBy = {QStringLiteral("novuadmin"), QStringLiteral("novuspace")};
    spec.variants = {QStringLiteral("Primary"), QStringLiteral("Secondary"),
                     QStringLiteral("Outlined"), QStringLiteral("Ghost"),
                     QStringLiteral("Success"), QStringLiteral("Warning"),
                     QStringLiteral("Danger")};
    spec.states = {QStringLiteral("Normal"), QStringLiteral("Hover"), QStringLiteral("Pressed"),
                   QStringLiteral("Focused"), QStringLiteral("Disabled")};
    spec.keywords = {QStringLiteral("action"), QStringLiteral("push"), QStringLiteral("cta"),
                     QStringLiteral("primary"), QStringLiteral("danger")};
    spec.properties = {
        {QStringLiteral("text"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("Caption. sizeHint() is measured from it.")},
        {QStringLiteral("variant"), QStringLiteral("enum"),
         {QStringLiteral("Primary"), QStringLiteral("Secondary"), QStringLiteral("Ghost"),
          QStringLiteral("Success"), QStringLiteral("Warning"), QStringLiteral("Danger")},
         QStringLiteral("Secondary"), QStringLiteral("Colour role. Never changes geometry.")},
        {QStringLiteral("glyph"), QStringLiteral("enum"), {}, QStringLiteral("None"),
         QStringLiteral("A leading icon from the Novu set, redrawn per variant and theme.")},
        {QStringLiteral("enabled"), QStringLiteral("bool"), {}, QStringLiteral("true"),
         QStringLiteral("Disabled buttons drop to the muted palette.")},
        {QStringLiteral("novuSize"), QStringLiteral("enum"), {QStringLiteral("small")},
         QString(), QStringLiteral("Geometry variant for row actions; composes with any variant.")},
    };
    spec.scenarios = {Scenario::Normal, Scenario::LongText, Scenario::LongIdentifier,
                      Scenario::Disabled};
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
