#include "Pages.h"

#include "novu/playground/ExampleLayout.h"
#include "novu/playground/SampleData.h"

#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/StyleSheet.h"
#include "novu/ui/Text.h"

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
    {"Ghost", Button::Variant::Ghost},
    {"Danger", Button::Variant::Danger},
};

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

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

            // The one geometry variant the stylesheet offers: row actions.
            auto* small = new Button(text, row.variant, card);
            small->setProperty(prop::size, QStringLiteral("small"));
            repolish(small);
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
        row->addWidget(dangerButton(QStringLiteral("Delete"), card));
        row->addWidget(detachButton(card));
        row->addStretch(1);
        note(body, card,
             QStringLiteral("detachButton() is a named factory rather than a ghost button so "
                            "the gesture reads the same in every panel that offers it."));
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
            auto* button = new Button(captions.at(i), kVariants[i % 4].variant, card);
            button->setEnabled(!disabledScenario);
            row->addWidget(button);
        }
        row->addStretch(1);
        column->addWidget(card);
    }

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
    spec.variants = {QStringLiteral("Primary"), QStringLiteral("Secondary"),
                     QStringLiteral("Ghost"), QStringLiteral("Danger")};
    spec.states = {QStringLiteral("Normal"), QStringLiteral("Hover"), QStringLiteral("Pressed"),
                   QStringLiteral("Focused"), QStringLiteral("Disabled")};
    spec.keywords = {QStringLiteral("action"), QStringLiteral("push"), QStringLiteral("cta"),
                     QStringLiteral("primary"), QStringLiteral("danger")};
    spec.properties = {
        {QStringLiteral("text"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("Caption. sizeHint() is measured from it.")},
        {QStringLiteral("variant"), QStringLiteral("enum"),
         {QStringLiteral("Primary"), QStringLiteral("Secondary"), QStringLiteral("Ghost"),
          QStringLiteral("Danger")},
         QStringLiteral("Secondary"), QStringLiteral("Colour role. Never changes geometry.")},
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
