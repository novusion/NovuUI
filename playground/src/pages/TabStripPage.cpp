#include "Pages.h"

#include "novu/playground/ExampleLayout.h"
#include "novu/playground/SampleData.h"

#include "novu/ui/Card.h"
#include "novu/ui/TabStrip.h"
#include "novu/ui/Text.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

QStringList itemsFor(const PreviewContext& ctx)
{
    QStringList items = ctx.data->values(QStringLiteral("sections"));
    switch (ctx.scenario) {
    case Scenario::Empty:
    case Scenario::ErrorState:
        return {};
    case Scenario::SingleItem:
        return items.isEmpty() ? items : QStringList{items.first()};
    case Scenario::ManyItems: {
        QStringList many;
        for (int i = 0; i < 14 && !items.isEmpty(); ++i) {
            many.append(QStringLiteral("%1 %2").arg(items.at(i % items.size())).arg(i + 1));
        }
        return many;
    }
    case Scenario::LongText:
        if (!items.isEmpty()) {
            items[1 % items.size()] = ctx.data->text(QStringLiteral("long_text")).left(50);
        }
        return items;
    case Scenario::LongIdentifier:
        if (!items.isEmpty()) {
            items[1 % items.size()] = ctx.data->text(QStringLiteral("long_identifier"));
        }
        return items;
    default:
        return items;
    }
}

void addStrip(QVBoxLayout* body, QWidget* parent, const QString& text,
              const QStringList& items, TabStrip::Variant variant, TabStrip::Tone tone,
              bool enabled)
{
    body->addWidget(caption(text, parent));
    auto* strip = new TabStrip(items, parent, 0, variant, tone);
    strip->setEnabled(enabled);

    // The strip reports which item is current and leaves the layout to the
    // page — so this is what a page does with it.
    auto* echo = label(QStringLiteral("current: %1").arg(items.value(0)), parent,
                       TypeRole::Caption, Ink::Accent);
    QObject::connect(strip, &TabStrip::selected, echo, [echo, items](int index) {
        echo->setText(QStringLiteral("current: %1").arg(items.value(index)));
    });

    body->addWidget(strip);
    body->addWidget(echo);
}

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    const QStringList items = itemsFor(ctx);
    const bool enabled = ctx.scenario != Scenario::Disabled;

    Card* card = exampleCard(page, QStringLiteral("Variants and tones"),
                             QStringLiteral("A row of buttons where exactly one is active."));
    QVBoxLayout* body = card->body();

    if (ctx.scenario == Scenario::ErrorState) {
        body->addWidget(scenarioErrorState(card, QStringLiteral("Sections")));
    } else if (items.isEmpty()) {
        body->addWidget(scenarioEmptyState(card, QStringLiteral("sections")));
    } else {
        addStrip(body, card, QStringLiteral("Pill / Accent"), items,
                 TabStrip::Variant::Pill, TabStrip::Tone::Accent, enabled);
        addStrip(body, card, QStringLiteral("Pill / Warning"), items,
                 TabStrip::Variant::Pill, TabStrip::Tone::Warning, enabled);
        addStrip(body, card, QStringLiteral("Segment / Accent"), items,
                 TabStrip::Variant::Segment, TabStrip::Tone::Accent, enabled);
        addStrip(body, card, QStringLiteral("Segment / Warning"), items,
                 TabStrip::Variant::Segment, TabStrip::Tone::Warning, enabled);

        note(body, card,
             QStringLiteral("Two tones exist because a preset switcher sitting directly above a "
                            "row of accent-coloured chips must not read as one of the chips. "
                            "Pill is a section switcher at control height; Segment is compact, "
                            "for above a table."));
    }
    column->addWidget(card);

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec tabStripComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("tab-strip");
    spec.name = QStringLiteral("TabStrip");
    spec.category = Category::Navigation;
    spec.description = QStringLiteral(
        "A row of buttons where exactly one is active. Not QTabWidget: a strip reports which "
        "item is current and leaves the layout to the page.");
    spec.headerPath = QStringLiteral("include/novu/ui/TabStrip.h");
    spec.sourcePath = QStringLiteral("src/TabStrip.cpp");
    spec.variants = {QStringLiteral("Pill"), QStringLiteral("Segment"),
                     QStringLiteral("Accent"), QStringLiteral("Warning")};
    spec.states = {QStringLiteral("Active"), QStringLiteral("Inactive"), QStringLiteral("Hover"),
                   QStringLiteral("Disabled")};
    spec.keywords = {QStringLiteral("tabs"), QStringLiteral("switcher"), QStringLiteral("segment"),
                     QStringLiteral("navigation"), QStringLiteral("selector")};
    spec.properties = {
        {QStringLiteral("items"), QStringLiteral("QStringList"), {}, QString(),
         QStringLiteral("Captions, in order.")},
        {QStringLiteral("variant"), QStringLiteral("enum"),
         {QStringLiteral("Pill"), QStringLiteral("Segment")}, QStringLiteral("Pill"),
         QStringLiteral("Pill is control height; Segment is compact.")},
        {QStringLiteral("tone"), QStringLiteral("enum"),
         {QStringLiteral("Accent"), QStringLiteral("Warning")}, QStringLiteral("Accent"),
         QStringLiteral("Warning is for a switcher above accent-coloured chips.")},
        {QStringLiteral("current"), QStringLiteral("int"), {}, QStringLiteral("0"),
         QStringLiteral("Active index.")},
    };
    spec.scenarios = {Scenario::Normal,    Scenario::Empty,     Scenario::SingleItem,
                      Scenario::ManyItems, Scenario::LongText,  Scenario::LongIdentifier,
                      Scenario::ErrorState, Scenario::Disabled};
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
