#include "Pages.h"

#include "novu/playground/ExampleLayout.h"
#include "novu/playground/SampleData.h"

#include "novu/ui/Badge.h"
#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/Chip.h"

#include <QLayout>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

QStringList itemsFor(const PreviewContext& ctx)
{
    const QStringList base = ctx.data->values(QStringLiteral("columns"));
    switch (ctx.scenario) {
    case Scenario::Empty:
        return {};
    case Scenario::SingleItem:
        return base.isEmpty() ? base : QStringList{base.first()};
    case Scenario::ManyItems:
    case Scenario::LargeDataset: {
        const int count = ctx.scenario == Scenario::LargeDataset ? 300 : 40;
        QStringList many;
        for (int i = 0; i < count && !base.isEmpty(); ++i) {
            many.append(QStringLiteral("%1 %2").arg(base.at(i % base.size())).arg(i + 1));
        }
        return many;
    }
    case Scenario::LongText: {
        QStringList items = base;
        if (!items.isEmpty()) {
            items[0] = ctx.data->text(QStringLiteral("long_text")).left(120);
        }
        return items;
    }
    case Scenario::LongIdentifier: {
        QStringList items = base;
        if (!items.isEmpty()) {
            items[0] = ctx.data->text(QStringLiteral("long_identifier"));
        }
        return items;
    }
    default:
        return base;
    }
}

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);
    const QStringList items = itemsFor(ctx);

    {
        Card* card = exampleCard(page, QStringLiteral("Wrapping"),
                                 QStringLiteral("Narrow the window: the row wraps instead of "
                                                "overflowing, and each item keeps its own "
                                                "width."));
        QVBoxLayout* body = card->body();
        if (items.isEmpty()) {
            body->addWidget(scenarioEmptyState(card, QStringLiteral("items")));
        } else {
            QLayout* chips = wrappingRow(body, card, QStringLiteral("Chips"));
            for (const QString& item : items) {
                chips->addWidget(new Chip(item, card, true));
            }
            QLayout* badges = wrappingRow(body, card, QStringLiteral("Badges"));
            for (int i = 0; i < items.size(); ++i) {
                badges->addWidget(new Badge(items.at(i),
                                            static_cast<Badge::Tone>(i % 5), card));
            }
            QLayout* buttons = wrappingRow(body, card, QStringLiteral("Mixed heights"));
            for (int i = 0; i < qMin(items.size(), 12); ++i) {
                buttons->addWidget(new Button(items.at(i),
                                              static_cast<Button::Variant>(i % 4), card));
            }
        }
        note(body, card,
             QStringLiteral("Qt ships no such layout. A QHBoxLayout overflows off the edge, and "
                            "a QGridLayout stretches every cell to the widest one, so a \"Side\" "
                            "chip ends up as wide as \"Current Price\"."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec flowLayoutComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("flow-layout");
    spec.name = QStringLiteral("FlowLayout");
    spec.category = Category::Foundation;
    spec.description = QStringLiteral(
        "A row that wraps onto the next line when it runs out of width, with every item keeping "
        "its own size. Qt ships no equivalent.");
    spec.headerPath = QStringLiteral("include/novu/ui/FlowLayout.h");
    spec.sourcePath = QStringLiteral("src/FlowLayout.cpp");
    spec.designedFor = {QStringLiteral("novuspace"),
                   QStringLiteral("novutrade")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Uniform items"), QStringLiteral("Mixed heights")};
    spec.states = {QStringLiteral("One line"), QStringLiteral("Wrapped"), QStringLiteral("Empty")};
    spec.keywords = {QStringLiteral("flow"), QStringLiteral("wrap"), QStringLiteral("layout"),
                     QStringLiteral("chips"), QStringLiteral("tags")};
    spec.properties = {
        {QStringLiteral("spacing"), QStringLiteral("int"), {}, QStringLiteral("-1"),
         QStringLiteral("-1 takes the style's default layout spacing.")},
    };
    spec.scenarios = {Scenario::Normal,    Scenario::Empty,     Scenario::SingleItem,
                      Scenario::ManyItems, Scenario::LongText,  Scenario::LongIdentifier,
                      Scenario::LargeDataset};
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
