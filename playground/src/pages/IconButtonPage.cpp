#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Card.h"
#include "novu/ui/IconButton.h"
#include "novu/ui/Separator.h"
#include "novu/ui/Text.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

#include <iterator>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

struct VariantRow
{
    const char* name;
    IconButton::Variant variant;
    const char* use;
};

const VariantRow kVariants[] = {
    {"Outlined", IconButton::Variant::Outlined, "an action among other controls"},
    {"Ghost", IconButton::Variant::Ghost, "chrome: a title bar, a panel header, a row"},
};

IconButton* make(QWidget* parent, Glyph glyph, IconButton::Variant variant,
                 const QString& tooltip = {})
{
    auto* button = new IconButton(glyph, variant, parent);
    if (!tooltip.isEmpty()) {
        button->setToolTip(tooltip);
    }
    return button;
}

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);
    auto* favourite=new IconButton(Glyph::Star,IconButton::Variant::Ghost,page);favourite->setCheckable(true);favourite->setChecked(true);
    favourite->setCheckedGlyphStyle(statusToken(SemanticRole::Warning,ColorRole::StatusText));column->addWidget(favourite);
    const bool disabledScenario = ctx.scenario == Scenario::Disabled;

    // ── the matrix ───────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Variant matrix"),
                                 QStringLiteral("Square at every size — a row of these must "
                                                "not have a ragged edge."));
        QVBoxLayout* body = card->body();

        QStringList rows;
        for (const VariantRow& row : kVariants) {
            rows.append(QString::fromLatin1(row.name));
        }
        QGridLayout* grid = matrix(body, card,
                                   {QStringLiteral("Normal"), QStringLiteral("Checked"),
                                    QStringLiteral("Focused"), QStringLiteral("Disabled"),
                                    QStringLiteral("Small"), QStringLiteral("Destructive"),
                                    QStringLiteral("Used for")},
                                   rows);

        for (int r = 0; r < static_cast<int>(std::size(kVariants)); ++r) {
            const VariantRow& row = kVariants[r];

            matrixCell(grid, r, 0, make(card, Glyph::Refresh, row.variant));

            // The `.on` state is a real checked button, not a painted lookalike.
            auto* checked = make(card, Glyph::Sidebar, row.variant);
            checked->setCheckable(true);
            checked->setChecked(true);
            matrixCell(grid, r, 1, checked);

            auto* focused = make(card, Glyph::Search, row.variant);
            focused->setFocus(Qt::TabFocusReason);
            matrixCell(grid, r, 2, focused);

            auto* disabled = make(card, Glyph::Download, row.variant);
            disabled->setEnabled(false);
            matrixCell(grid, r, 3, disabled);

            auto* small = make(card, Glyph::Edit, row.variant);
            small->setControlSize(ControlSize::Small);
            matrixCell(grid, r, 4, small);

            auto* destructive = make(card, Glyph::Trash, row.variant);
            destructive->setDestructive(true);
            matrixCell(grid, r, 5, destructive);

            matrixCell(grid, r, 6, label(QString::fromLatin1(row.use), card,
                                         TypeRole::Caption, Ink::Muted));
        }

        note(body, card,
             QStringLiteral("Destructive colours on hover, not at rest — a row of actions that "
                            "is permanently red reads as a row of warnings. Hover the Trash "
                            "cells to see it."));
        column->addWidget(card);
    }

    // ── in a toolbar ─────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("In a toolbar"),
                                 QStringLiteral("Grouped by what they do, divided by a rule."));
        QVBoxLayout* body = card->body();
        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Panel header"));

        row->addWidget(make(card, Glyph::Refresh, IconButton::Variant::Ghost,
                            QStringLiteral("Refresh")));
        row->addWidget(make(card, Glyph::Filter, IconButton::Variant::Ghost,
                            QStringLiteral("Filter")));
        row->addWidget(make(card, Glyph::Sliders, IconButton::Variant::Ghost,
                            QStringLiteral("Columns")));
        toolbarDivider(row, card);
        auto* detached = make(card, Glyph::External, IconButton::Variant::Ghost,
                              QStringLiteral("Detach this panel"));
        detached->setCheckable(true);
        detached->setChecked(true);
        row->addWidget(detached);
        toolbarDivider(row, card);
        auto* close = make(card, Glyph::Close, IconButton::Variant::Ghost,
                           QStringLiteral("Close panel"));
        close->setDestructive(true);
        row->addWidget(close);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Detach stays lit for as long as its window is open. That is the "
                            "same accent an active module and a selected row use, because it "
                            "is the same statement."));
        column->addWidget(card);
    }

    // ── row actions ──────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Row actions"),
                                 QStringLiteral("rowAction(): small, ghost, one glyph."));
        QVBoxLayout* body = card->body();
        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Inside a table row"));
        const struct { Glyph glyph; const char* tip; } actions[] = {
            {Glyph::Eye, "Open"}, {Glyph::Edit, "Edit"}, {Glyph::Copy, "Duplicate"},
            {Glyph::Download, "Export"}, {Glyph::Trash, "Delete"},
        };
        for (const auto& action : actions) {
            auto* button = rowAction(action.glyph, card, QString::fromLatin1(action.tip));
            button->setDestructive(action.glyph == Glyph::Trash);
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

ComponentSpec iconButtonComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("icon-button");
    spec.name = QStringLiteral("IconButton");
    spec.category = Category::Input;
    spec.description = QStringLiteral(
        "A button that is a glyph and nothing else. Square at every control size, "
        "self-painted, with the product accent as its checked state.");
    spec.headerPath = QStringLiteral("include/novu/ui/IconButton.h");
    spec.sourcePath = QStringLiteral("src/IconButton.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Outlined"), QStringLiteral("Ghost")};
    spec.states = {QStringLiteral("Normal"), QStringLiteral("Hover"), QStringLiteral("Pressed"),
                   QStringLiteral("Checked"), QStringLiteral("Focused"),
                   QStringLiteral("Disabled")};
    spec.keywords = {QStringLiteral("icon"), QStringLiteral("glyph"), QStringLiteral("toolbar"),
                     QStringLiteral("action"), QStringLiteral("row action"),
                     QStringLiteral("square")};
    spec.properties = {
        {QStringLiteral("glyph"), QStringLiteral("enum"), {}, QStringLiteral("None"),
         QStringLiteral("Which outline the button carries.")},
        {QStringLiteral("variant"), QStringLiteral("enum"),
         {QStringLiteral("Outlined"), QStringLiteral("Ghost")}, QStringLiteral("Outlined"),
         QStringLiteral("Whether it has a plate at rest.")},
        {QStringLiteral("checkable"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("Checked is the accent: something that stays on.")},
        {QStringLiteral("destructive"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("Colours on hover rather than at rest.")},
    };
    spec.scenarios = {Scenario::Normal, Scenario::Disabled};
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
