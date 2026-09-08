#include "Pages.h"

#include "novu/playground/ExampleLayout.h"
#include "novu/playground/SampleData.h"

#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/Fields.h"
#include "novu/ui/Form.h"
#include "novu/ui/Layout.h"
#include "novu/ui/Text.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

QWidget* buildFormRow(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    const bool enabled = ctx.scenario != Scenario::Disabled;
    const QString longCaption = ctx.scenario == Scenario::LongText
        ? ctx.data->text(QStringLiteral("long_text")).left(40)
        : QStringLiteral("Retention period");

    Card* card = exampleCard(page, QStringLiteral("formRow()"),
                             QStringLiteral("Fixed label column, control, optional hint. "
                                            "Returns a layout — no wrapper widget."));
    QVBoxLayout* body = card->body();

    auto* days = comboBox({QStringLiteral("7 days"), QStringLiteral("30 days"),
                           QStringLiteral("90 days"), QStringLiteral("Forever")},
                          card, 160);
    days->setEnabled(enabled);
    formRow(body, card, longCaption, days, QStringLiteral("applies to closed trades only"));

    auto* endpoint = lineEdit(QStringLiteral("https://…"), card, 280,
                              ctx.scenario == Scenario::LongIdentifier
                                  ? ctx.data->text(QStringLiteral("long_path"))
                                  : QStringLiteral("https://futures.kraken.com"));
    endpoint->setEnabled(enabled);
    formRow(body, card, QStringLiteral("API endpoint"), endpoint);

    auto* verify = checkBox(QStringLiteral("Verify TLS certificate"), card, true);
    verify->setEnabled(enabled);
    formRow(body, card, QStringLiteral("Security"), verify);

    formRow(body, card, QStringLiteral("Nothing attached"), nullptr,
            QStringLiteral("a row with no control still keeps the column"));

    note(body, card,
         QStringLiteral("The label column is fixed at one width for every row. A measured "
                        "column makes each row as wide as its own caption, which is the "
                        "staircase this replaces — and it is layout, not a width set on a "
                        "component."));
    column->addWidget(card);

    column->addStretch(1);
    return page;
}

QWidget* buildPropertyRow(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    Card* card = exampleCard(page, QStringLiteral("propertyRow()"),
                             QStringLiteral("Caption left, value right. For read-only detail "
                                            "panels."));
    QVBoxLayout* body = card->body();

    const QJsonArray rows = ctx.data->rowsFor(QStringLiteral("trades"), ctx.scenario);

    if (ctx.scenario == Scenario::ErrorState) {
        body->addWidget(scenarioErrorState(card, QStringLiteral("Inspector")));
    } else if (rows.isEmpty()) {
        body->addWidget(scenarioEmptyState(card, QStringLiteral("properties")));
    } else {
        const QJsonObject first = rows.first().toObject();
        propertyRow(body, card, QStringLiteral("Symbol"),
                    first.value(QStringLiteral("symbol")).toString(QStringLiteral("—")));
        propertyRow(body, card, QStringLiteral("Side"),
                    first.value(QStringLiteral("side")).toString(QStringLiteral("—")),
                    Ink::Accent);
        const double pnl = first.value(QStringLiteral("pnl")).toDouble();
        propertyRow(body, card, QStringLiteral("Realised P&L"),
                    QString::number(pnl, 'f', 2),
                    pnl >= 0 ? Ink::Success : Ink::Danger);
        propertyRow(body, card, QStringLiteral("Trade id"),
                    ctx.data->text(QStringLiteral("long_identifier")).left(16),
                    Ink::Secondary, true);
        propertyRow(body, card, QStringLiteral("Cache path"),
                    ctx.data->text(QStringLiteral("long_path")), Ink::Muted, true);

        note(body, card,
             QStringLiteral("Monospace is not decoration: an id or a path is compared by eye "
                            "down a column, and a proportional font makes two different "
                            "hashes look the same length."));
    }
    column->addWidget(card);

    column->addStretch(1);
    return page;
}

QWidget* buildSearchBar(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    const bool enabled = ctx.scenario != Scenario::Disabled;

    Card* card = exampleCard(page, QStringLiteral("searchBar()"),
                             QStringLiteral("One search field for every panel — same height, "
                                            "same clear affordance, same voice."));
    QVBoxLayout* body = card->body();

    body->addWidget(caption(QStringLiteral("Default"), card));
    auto* plain = searchBar(card);
    plain->setEnabled(enabled);
    body->addWidget(plain);

    body->addWidget(caption(QStringLiteral("With its own placeholder, fixed width"), card));
    auto* scoped = searchBar(card, QStringLiteral("Search trade id, symbol…"), 320);
    scoped->setEnabled(enabled);
    body->addWidget(scoped);

    body->addWidget(caption(QStringLiteral("Populated — the clear button appears"), card));
    auto* filled = searchBar(card, QStringLiteral("Search"), 320);
    filled->setText(ctx.scenario == Scenario::LongIdentifier
                        ? ctx.data->text(QStringLiteral("long_identifier"))
                        : QStringLiteral("PF_XBTUSD"));
    filled->setEnabled(enabled);
    body->addWidget(filled);

    body->addWidget(caption(QStringLiteral("In a toolbar, beside the other controls"), card));
    QHBoxLayout* row = toolbar(body);
    auto* search = searchBar(card, QStringLiteral("Filter"), 220);
    search->setEnabled(enabled);
    row->addWidget(search);
    auto* status = comboBox({QStringLiteral("Status: All"), QStringLiteral("OPEN"),
                             QStringLiteral("CLOSED")},
                            card, 140);
    status->setEnabled(enabled);
    row->addWidget(status);
    auto* clear = ghostButton(QStringLiteral("Clear"), card);
    clear->setEnabled(enabled);
    row->addWidget(clear);
    row->addStretch(1);

    note(body, card,
         QStringLiteral("A search box a user cannot clear in one gesture is one they clear by "
                        "selecting the text, and half of them do not. The affordance belongs "
                        "to the component."));
    column->addWidget(card);

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec formRowComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("form-row");
    spec.name = QStringLiteral("FormRow");
    spec.category = Category::Input;
    spec.description = QStringLiteral(
        "Label on the left at a fixed column width, control on the right, optional muted "
        "hint. Returns a layout and creates no wrapper.");
    spec.headerPath = QStringLiteral("include/novu/ui/Form.h");
    spec.sourcePath = QStringLiteral("src/Form.cpp");
    spec.usedBy = {QStringLiteral("novuspace"),
                   QStringLiteral("novuai"),
                   QStringLiteral("novuexplorer"),
                   QStringLiteral("novuhub"),
                   QStringLiteral("novutrade")};
    spec.variants = {QStringLiteral("With control"), QStringLiteral("With hint"),
                     QStringLiteral("Without control")};
    spec.states = {QStringLiteral("Normal"), QStringLiteral("Disabled")};
    spec.keywords = {QStringLiteral("form"), QStringLiteral("settings"), QStringLiteral("label"),
                     QStringLiteral("field"), QStringLiteral("row")};
    spec.properties = {
        {QStringLiteral("caption"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("Left column text.")},
        {QStringLiteral("control"), QStringLiteral("QWidget*"), {}, QString(),
         QStringLiteral("Any control; reparented to the row's owner.")},
        {QStringLiteral("hint"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("Muted note after the control.")},
    };
    spec.scenarios = {Scenario::Normal, Scenario::LongText, Scenario::LongIdentifier,
                      Scenario::Disabled};
    spec.build = buildFormRow;
    return spec;
}

ComponentSpec propertyRowComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("property-row");
    spec.name = QStringLiteral("PropertyRow");
    spec.category = Category::DataDisplay;
    spec.description = QStringLiteral(
        "Caption left, value right, pushed apart. For read-only detail panels — connection "
        "state, versions, ids and paths.");
    spec.headerPath = QStringLiteral("include/novu/ui/Form.h");
    spec.sourcePath = QStringLiteral("src/Form.cpp");
    spec.usedBy = {QStringLiteral("novuspace"),
                   QStringLiteral("novuai"),
                   QStringLiteral("novuexplorer"),
                   QStringLiteral("novutrade")};
    spec.variants = {QStringLiteral("Default"), QStringLiteral("Toned"),
                     QStringLiteral("Monospace")};
    spec.states = {QStringLiteral("Populated"), QStringLiteral("Empty"), QStringLiteral("Error")};
    spec.keywords = {QStringLiteral("property"), QStringLiteral("key value"),
                     QStringLiteral("detail"), QStringLiteral("inspector"),
                     QStringLiteral("kv")};
    spec.properties = {
        {QStringLiteral("caption"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("What the value is.")},
        {QStringLiteral("value"), QStringLiteral("QString"), {}, QString(), QStringLiteral("The value.")},
        {QStringLiteral("tone"), QStringLiteral("enum"),
         {QStringLiteral("Default"), QStringLiteral("Secondary"), QStringLiteral("Muted"),
          QStringLiteral("Accent"), QStringLiteral("Success"), QStringLiteral("Warning"),
          QStringLiteral("Danger"), QStringLiteral("Info")},
         QStringLiteral("Default"), QStringLiteral("Ink role for the value.")},
        {QStringLiteral("mono"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("Monospace, for ids, hashes and paths.")},
    };
    spec.scenarios = {Scenario::Normal, Scenario::Empty, Scenario::SingleItem,
                      Scenario::LongIdentifier, Scenario::ErrorState};
    spec.build = buildPropertyRow;
    return spec;
}

ComponentSpec searchBarComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("search-bar");
    spec.name = QStringLiteral("SearchBar");
    spec.category = Category::Input;
    spec.description = QStringLiteral(
        "The search field every list panel needs, with the profile's control geometry and a "
        "clear button decided once rather than per panel.");
    spec.headerPath = QStringLiteral("include/novu/ui/Form.h");
    spec.sourcePath = QStringLiteral("src/Form.cpp");
    spec.usedBy = {QStringLiteral("novuspace"),
                   QStringLiteral("novuai"),
                   QStringLiteral("novuexplorer"),
                   QStringLiteral("novutrade")};
    spec.variants = {QStringLiteral("Default placeholder"), QStringLiteral("Custom placeholder"),
                     QStringLiteral("Fixed width")};
    spec.states = {QStringLiteral("Empty"), QStringLiteral("Populated"),
                   QStringLiteral("Focused"), QStringLiteral("Disabled")};
    spec.keywords = {QStringLiteral("search"), QStringLiteral("filter"), QStringLiteral("find"),
                     QStringLiteral("query"), QStringLiteral("input")};
    spec.properties = {
        {QStringLiteral("placeholder"), QStringLiteral("QString"), {}, QStringLiteral("Search…"),
         QStringLiteral("Empty takes the shared default.")},
        {QStringLiteral("width"), QStringLiteral("int"), {}, QStringLiteral("0"),
         QStringLiteral("0 lets the layout size it.")},
    };
    spec.scenarios = {Scenario::Normal, Scenario::LongIdentifier, Scenario::Disabled};
    spec.build = buildSearchBar;
    return spec;
}

} // namespace novu::playground::pages
