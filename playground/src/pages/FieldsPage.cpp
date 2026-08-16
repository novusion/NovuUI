#include "Pages.h"

#include "novu/playground/ExampleLayout.h"
#include "novu/playground/SampleData.h"

#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/Fields.h"
#include "novu/ui/Layout.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    const bool enabled = ctx.scenario != Scenario::Disabled;
    QStringList options = ctx.data->values(QStringLiteral("plans"));
    if (ctx.scenario == Scenario::Empty || ctx.scenario == Scenario::ErrorState) {
        options.clear();
    } else if (ctx.scenario == Scenario::SingleItem && !options.isEmpty()) {
        options = QStringList{options.first()};
    } else if (ctx.scenario == Scenario::LongText && !options.isEmpty()) {
        options[0] = ctx.data->text(QStringLiteral("long_text")).left(80);
    } else if (ctx.scenario == Scenario::LongIdentifier && !options.isEmpty()) {
        options[0] = ctx.data->text(QStringLiteral("long_identifier"));
    } else if (ctx.scenario == Scenario::ManyItems) {
        QStringList many;
        for (int i = 0; i < 50 && !options.isEmpty(); ++i) {
            many.append(QStringLiteral("%1 %2").arg(options.at(i % options.size())).arg(i + 1));
        }
        options = many;
    }

    // ── the three factories ──────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Controls"),
                                 QStringLiteral("Height, font and — only where the caller needs "
                                                "it — width. Appearance comes from the scope "
                                                "stylesheet."));
        QVBoxLayout* body = card->body();

        QGridLayout* grid = matrix(body, card,
                                   {QStringLiteral("Enabled"), QStringLiteral("Disabled"),
                                    QStringLiteral("Fixed width")},
                                   {QStringLiteral("ComboBox"), QStringLiteral("LineEdit"),
                                    QStringLiteral("CheckBox")});

        auto* combo = comboBox(options, card);
        auto* comboDisabled = comboBox(options, card);
        comboDisabled->setEnabled(false);
        matrixCell(grid, 0, 0, combo);
        matrixCell(grid, 0, 1, comboDisabled);
        matrixCell(grid, 0, 2, comboBox(options, card, 140));

        matrixCell(grid, 1, 0, lineEdit(QStringLiteral("Search users"), card));
        auto* editDisabled = lineEdit(QStringLiteral("Search users"), card);
        editDisabled->setEnabled(false);
        matrixCell(grid, 1, 1, editDisabled);
        matrixCell(grid, 1, 2,
                   lineEdit(QStringLiteral("Symbol"), card, 140, QStringLiteral("BTCUSDT")));

        matrixCell(grid, 2, 0, checkBox(QStringLiteral("Include archived"), card, true));
        auto* checkDisabled = checkBox(QStringLiteral("Include archived"), card, true);
        checkDisabled->setEnabled(false);
        matrixCell(grid, 2, 1, checkDisabled);
        matrixCell(grid, 2, 2, checkBox(QStringLiteral("Off"), card, false));

        note(body, card,
             QStringLiteral("The combo box draws its own arrow: Qt renders a CSS border "
                            "triangle as a filled rectangle that reads as a dash, and the "
                            "control then looks exactly like a line edit."));

        if (options.isEmpty()) {
            note(body, card,
                 QStringLiteral("This scenario supplies no options — an empty combo box must "
                                "still be the same height as a populated one."));
        }
        column->addWidget(card);
    }

    // ── a real toolbar ───────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("In a toolbar"),
                                 QStringLiteral("Controls must not stagger. Any difference in "
                                                "height here is a bug in a control, not in the "
                                                "row."));
        QVBoxLayout* body = card->body();
        QHBoxLayout* row = toolbar(body);
        auto* search = lineEdit(QStringLiteral("Filter"), card, 200);
        auto* plan = comboBox(options, card, 160);
        auto* archived = checkBox(QStringLiteral("Archived"), card);
        auto* apply = primaryButton(QStringLiteral("Apply"), card);
        for (QWidget* widget : {static_cast<QWidget*>(search), static_cast<QWidget*>(plan),
                                static_cast<QWidget*>(archived), static_cast<QWidget*>(apply)}) {
            widget->setEnabled(enabled);
            row->addWidget(widget);
        }
        row->addStretch(1);
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec fieldsComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("fields");
    spec.name = QStringLiteral("Fields");
    spec.category = Category::Input;
    spec.description = QStringLiteral(
        "ComboBox, and the lineEdit() / checkBox() factories. They set what a stylesheet "
        "cannot — height, font and an optional fixed width — and nothing else.");
    spec.headerPath = QStringLiteral("include/novu/ui/Fields.h");
    spec.sourcePath = QStringLiteral("src/Fields.cpp");
    spec.variants = {QStringLiteral("ComboBox"), QStringLiteral("LineEdit"),
                     QStringLiteral("CheckBox")};
    spec.states = {QStringLiteral("Normal"), QStringLiteral("Focused"), QStringLiteral("Disabled"),
                   QStringLiteral("Empty"), QStringLiteral("Populated")};
    spec.keywords = {QStringLiteral("input"), QStringLiteral("form"), QStringLiteral("combobox"),
                     QStringLiteral("dropdown"), QStringLiteral("text field"),
                     QStringLiteral("checkbox"), QStringLiteral("search")};
    spec.properties = {
        {QStringLiteral("items"), QStringLiteral("QStringList"), {}, QString(),
         QStringLiteral("ComboBox contents.")},
        {QStringLiteral("placeholder"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("LineEdit placeholder.")},
        {QStringLiteral("width"), QStringLiteral("int"), {}, QStringLiteral("0"),
         QStringLiteral("Fixed width; 0 means the control sizes itself.")},
        {QStringLiteral("checked"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("CheckBox initial state.")},
    };
    spec.scenarios = {Scenario::Normal,    Scenario::Empty,      Scenario::SingleItem,
                      Scenario::ManyItems, Scenario::LongText,   Scenario::LongIdentifier,
                      Scenario::ErrorState, Scenario::Disabled};
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
