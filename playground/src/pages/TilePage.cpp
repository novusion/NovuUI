#include "Pages.h"

#include "novu/playground/ExampleLayout.h"
#include "novu/playground/SampleData.h"

#include "novu/ui/Card.h"
#include "novu/ui/Tile.h"

#include <QGridLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLayout>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    const QJsonArray rows = ctx.data->rowsFor(QStringLiteral("exchanges"), ctx.scenario);

    // ── states ───────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("States"),
                                 QStringLiteral("Selected is one fill and one border read from "
                                                "the profile, not a stylesheet repeating every "
                                                "colour."));
        QVBoxLayout* body = card->body();
        QGridLayout* grid = matrix(body, card,
                                   {QStringLiteral("Unchecked"), QStringLiteral("Checked"),
                                    QStringLiteral("Badged (unavailable)")},
                                   {QStringLiteral("Tile")});
        matrixCell(grid, 0, 0,
                   new Tile(QStringLiteral("Binance"), QStringLiteral("Spot and futures"), card,
                            false));
        matrixCell(grid, 0, 1,
                   new Tile(QStringLiteral("Bybit"), QStringLiteral("Perpetuals"), card, true));
        matrixCell(grid, 0, 2,
                   new Tile(QStringLiteral("Kraken"), QStringLiteral("Spot"), card, false,
                            QStringLiteral("Soon")));
        note(body, card,
             QStringLiteral("A tile carrying a badge is disabled by the component itself — the "
                            "badge exists to say why it cannot be chosen. That is behaviour, "
                            "not a state the gallery sets."));
        column->addWidget(card);
    }

    // ── from data ────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("A chooser"),
                                 QStringLiteral("What the approved design uses tiles for."));
        QVBoxLayout* body = card->body();

        if (ctx.scenario == Scenario::ErrorState) {
            body->addWidget(scenarioErrorState(card, QStringLiteral("Exchanges")));
        } else if (rows.isEmpty()) {
            body->addWidget(scenarioEmptyState(card, QStringLiteral("exchanges")));
        } else {
            QLayout* flow = wrappingRow(body, card, QString());
            const int limit = qMin(rows.size(), 40);
            for (int i = 0; i < limit; ++i) {
                const QJsonObject row = rows.at(i).toObject();
                auto* tile = new Tile(row.value(QStringLiteral("name")).toString(),
                                      row.value(QStringLiteral("markets")).toString(),
                                      card, i == 0,
                                      row.value(QStringLiteral("badge")).toString());
                flow->addWidget(tile);
            }
            if (rows.size() > limit) {
                note(body, card,
                     QStringLiteral("Showing %1 of %2.").arg(limit).arg(rows.size()));
            }
            if (ctx.scenario == Scenario::MissingOptional) {
                note(body, card,
                     QStringLiteral("With the badge field removed every tile becomes available — "
                                    "which is the point: absence of the badge is the enabled "
                                    "state, so a dropped field silently enables a choice."));
            }
        }
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec tileComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("tile");
    spec.name = QStringLiteral("Tile");
    spec.category = Category::Navigation;
    spec.description = QStringLiteral(
        "A selectable two-line tile: title, subtitle, optional badge. Self-painted, so the "
        "selected state is one fill and one border from the profile.");
    spec.headerPath = QStringLiteral("include/novu/ui/Tile.h");
    spec.sourcePath = QStringLiteral("src/Tile.cpp");
    spec.usedBy = {QStringLiteral("novuspace"),
                   QStringLiteral("novutrade")};
    spec.variants = {QStringLiteral("Plain"), QStringLiteral("Badged")};
    spec.states = {QStringLiteral("Unchecked"), QStringLiteral("Checked"),
                   QStringLiteral("Hover"), QStringLiteral("Disabled (badged)")};
    spec.keywords = {QStringLiteral("choice"), QStringLiteral("option"), QStringLiteral("card"),
                     QStringLiteral("selector"), QStringLiteral("exchange"),
                     QStringLiteral("profile")};
    spec.properties = {
        {QStringLiteral("title"), QStringLiteral("QString"), {}, QString(), QStringLiteral("First line.")},
        {QStringLiteral("subtitle"), QStringLiteral("QString"), {}, QString(), QStringLiteral("Second line, muted.")},
        {QStringLiteral("checked"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("Selected state.")},
        {QStringLiteral("badge"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("Non-empty disables the tile — the badge says why.")},
    };
    spec.scenarios = {Scenario::Normal,     Scenario::Empty,          Scenario::SingleItem,
                      Scenario::ManyItems,  Scenario::LongText,       Scenario::LongIdentifier,
                      Scenario::MissingOptional, Scenario::ErrorState};
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
