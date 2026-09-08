#include "Pages.h"

#include "novu/playground/ExampleLayout.h"
#include "novu/playground/SampleData.h"

#include "novu/ui/Card.h"
#include "novu/ui/Chip.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

QStringList itemsFor(const PreviewContext& ctx)
{
    QStringList items = ctx.data->values(QStringLiteral("columns"));
    switch (ctx.scenario) {
    case Scenario::Empty:
    case Scenario::ErrorState:
        return {};
    case Scenario::SingleItem:
        return items.isEmpty() ? items : QStringList{items.first()};
    case Scenario::ManyItems: {
        QStringList many;
        for (int i = 0; i < 24 && !items.isEmpty(); ++i) {
            many.append(QStringLiteral("%1 %2").arg(items.at(i % items.size())).arg(i + 1));
        }
        return many;
    }
    case Scenario::LongText:
        if (!items.isEmpty()) {
            items[0] = ctx.data->text(QStringLiteral("long_text")).left(60);
        }
        return items;
    case Scenario::LongIdentifier:
        if (!items.isEmpty()) {
            items[0] = ctx.data->text(QStringLiteral("long_identifier"));
        }
        return items;
    default:
        return items;
    }
}

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);
    const QStringList items = itemsFor(ctx);
    const bool enabled = ctx.scenario != Scenario::Disabled;

    // ── the chip itself ──────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Chip"),
                                 QStringLiteral("On is the accent, off is muted — legible at a "
                                                "glance across a row of twenty."));
        QVBoxLayout* body = card->body();
        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("States"));
        auto* on = new Chip(QStringLiteral("On"), card, true);
        auto* off = new Chip(QStringLiteral("Off"), card, false);
        auto* disabledOn = new Chip(QStringLiteral("On, disabled"), card, true);
        auto* disabledOff = new Chip(QStringLiteral("Off, disabled"), card, false);
        disabledOn->setEnabled(false);
        disabledOff->setEnabled(false);
        for (Chip* chip : {on, off, disabledOn, disabledOff}) {
            row->addWidget(chip);
        }
        row->addStretch(1);
        column->addWidget(card);
    }

    // ── the bar ──────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("ChipBar"),
                                 QStringLiteral("A wrapping strip — narrow the window and it "
                                                "wraps rather than overflowing."));
        QVBoxLayout* body = card->body();

        if (ctx.scenario == Scenario::ErrorState) {
            body->addWidget(scenarioErrorState(card, QStringLiteral("Columns")));
        } else if (items.isEmpty()) {
            body->addWidget(scenarioEmptyState(card, QStringLiteral("columns")));
        } else {
            const QStringList on = items.mid(0, qMax(1, items.size() / 2));
            auto* bar = new ChipBar(items, card, on, QStringLiteral("Columns"));
            bar->setEnabled(enabled);
            body->addWidget(bar);

            auto* noCaption = new ChipBar(items, card, on);
            noCaption->setEnabled(enabled);
            body->addWidget(caption(QStringLiteral("Without a caption"), card));
            body->addWidget(noCaption);

            note(body, card,
                 QStringLiteral("The caption is laid out as the first item so it wraps with the "
                                "chips, instead of sitting in a row of its own that drifts out "
                                "of alignment once the strip wraps."));
        }
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec chipComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("chip");
    spec.name = QStringLiteral("Chip / ChipBar");
    spec.category = Category::Input;
    spec.description = QStringLiteral(
        "A small on/off label, and the wrapping strip of them used to choose which columns a "
        "table shows. The strip carries no background of its own.");
    spec.headerPath = QStringLiteral("include/novu/ui/Chip.h");
    spec.sourcePath = QStringLiteral("src/Chip.cpp");
    spec.usedBy = {QStringLiteral("novuspace"),
                   QStringLiteral("novutrade")};
    spec.variants = {QStringLiteral("On"), QStringLiteral("Off"),
                     QStringLiteral("Bar with caption"), QStringLiteral("Bar without caption")};
    spec.states = {QStringLiteral("On"), QStringLiteral("Off"), QStringLiteral("Disabled"),
                   QStringLiteral("Hover")};
    spec.keywords = {QStringLiteral("toggle"), QStringLiteral("filter"), QStringLiteral("pill"),
                     QStringLiteral("column chooser"), QStringLiteral("tag")};
    spec.properties = {
        {QStringLiteral("text"), QStringLiteral("QString"), {}, QString(), QStringLiteral("Chip caption.")},
        {QStringLiteral("on"), QStringLiteral("bool"), {}, QStringLiteral("true"),
         QStringLiteral("Initial checked state.")},
        {QStringLiteral("items"), QStringLiteral("QStringList"), {}, QString(),
         QStringLiteral("ChipBar contents, in declared order.")},
        {QStringLiteral("caption"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("Laid out as the first item so it wraps with the chips.")},
    };
    spec.scenarios = {Scenario::Normal,    Scenario::Empty,     Scenario::SingleItem,
                      Scenario::ManyItems, Scenario::LongText,  Scenario::LongIdentifier,
                      Scenario::ErrorState, Scenario::Disabled};
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
