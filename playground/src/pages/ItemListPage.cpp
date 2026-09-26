#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Badge.h"
#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/IconButton.h"
#include "novu/ui/ItemList.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    if (ctx.scenario == Scenario::Empty) {
        column->addWidget(scenarioEmptyState(page, QStringLiteral("rows")));
        column->addStretch(1);
        return page;
    }

    // ── name, sub-line, value ────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Running bots"),
                                 QStringLiteral("A name, what it is, and what "
                                                "it is making."));
        // `.hlist` fills the card: the rows carry the padding, so the body
        // must not add its own or every rule stops short of both edges.
        card->setBodyMetrics(QMargins(0, 0, 0, 0), 0);

        auto* list = new ItemList(card);
        list->addRow(QStringLiteral("BTC Grid Alpha"),
                     QStringLiteral("PF_XBTUSD · 5m · LIVE"),
                     QStringLiteral("+$1,284.42"), Ink::Success);
        list->addRow(QStringLiteral("ETH Momentum"),
                     QStringLiteral("ETHUSDT · 15m · LIVE"),
                     QStringLiteral("-$312.08"), Ink::Danger);
        list->addRow(QStringLiteral("DCA Weekly"),
                     QStringLiteral("XBT/EUR · 1d · LIVE"),
                     QStringLiteral("+$4,210.77"), Ink::Success);
        for (int i = 0; i < list->count(); ++i) {
            list->setRowClickable(i, true);
        }
        card->body()->addWidget(list);

        auto* chosen = label(QStringLiteral("nothing yet"), card, TypeRole::Mono,
                             Ink::Secondary);
        QObject::connect(list, &ItemList::activated, chosen, [chosen](int index) {
            chosen->setText(QStringLiteral("row %1").arg(index));
        });

        note(card->body(), card,
             QStringLiteral("The rule belongs *between* rows, so the last one has "
                            "none — a list that ends in a hairline draws a line "
                            "across the bottom of its card, a pixel above the card's "
                            "own border."));
        card->body()->addWidget(chosen);
        column->addWidget(card);
    }

    // ── a value that is not a number ─────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Recording"),
                                 QStringLiteral("A badge and a figure at the "
                                                "same edge."));
        card->setBodyMetrics(QMargins(0, 0, 0, 0), 0);

        auto* list = new ItemList(card);
        list->addRow(QStringLiteral("PF_XBTUSD"),
                     QStringLiteral("candles · trades · book · since 2026-09-13 06:02"),
                     QStringLiteral("184,200"));
        list->addTrailing(0, new Badge(QStringLiteral("recording"), Badge::Tone::Success));
        list->addRow(QStringLiteral("ETHUSDT"),
                     QStringLiteral("candles · since 2026-09-12 22:14"),
                     QStringLiteral("96,410"));
        list->addTrailing(1, new Badge(QStringLiteral("idle"), Badge::Tone::Neutral));
        list->addRow(QStringLiteral("trades_2025.json"),
                     QStringLiteral("1,284 trades · archived 2026-01-08"));
        list->addTrailing(2, new IconButton(Glyph::Download));
        card->body()->addWidget(list);

        note(card->body(), card,
             QStringLiteral("Trailing widgets are appended after the value rather "
                            "than replacing it, which is the order the design puts "
                            "them in: what it is, then how much of it there is."));
        column->addWidget(card);
    }

    // ── a row the caller builds ──────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("A row of your own"),
                                 QStringLiteral("For the shapes that are not "
                                                "name and value."));
        card->setBodyMetrics(QMargins(0, 0, 0, 0), 0);

        auto* list = new ItemList(card);
        for (const char* text : {"Last price", "24h open", "24h high"}) {
            auto* row = new QWidget(card);
            auto* line = new QHBoxLayout(row);
            line->setContentsMargins(16, 9, 16, 9);
            line->setSpacing(10);
            line->addWidget(label(QString::fromLatin1(text), row, TypeRole::Body,
                                  Ink::Secondary), 1);
            line->addWidget(label(QStringLiteral("64,212.50"), row, TypeRole::Mono,
                                  Ink::Muted));
            list->addRow(row);
        }
        card->body()->addWidget(list);

        note(card->body(), card,
             QStringLiteral("What the list owns is the list — the hairlines, the "
                            "metrics, the hover, and that the last row has no rule. "
                            "What a row carries is the caller's, because the variety "
                            "is the point: a feed item, a ticker line and a bot row "
                            "are the same list."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec itemListComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("item-list");
    spec.name = QStringLiteral("ItemList");
    spec.category = Category::DataDisplay;
    spec.description = QStringLiteral(
        "The design's `.hlist` — rows divided by hairlines, filling a card. The second most "
        "repeated shape after the table, and the most reinvented.");
    spec.headerPath = QStringLiteral("include/novu/ui/ItemList.h");
    spec.sourcePath = QStringLiteral("src/ItemList.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {QStringLiteral("novuspace")};
    spec.variants = {QStringLiteral("Name and value"), QStringLiteral("Trailing widget"),
                     QStringLiteral("Caller-built row")};
    spec.states = {QStringLiteral("Normal"), QStringLiteral("Hovered"),
                   QStringLiteral("Clickable")};
    spec.keywords = {QStringLiteral("list"), QStringLiteral("rows"), QStringLiteral("hlist"),
                     QStringLiteral("hairline"), QStringLiteral("feed")};
    spec.scenarios = {Scenario::Normal, Scenario::Empty, Scenario::LongText};
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
