#include "Pages.h"

#include "novu/playground/ExampleLayout.h"
#include "novu/playground/SampleData.h"

#include "novu/ui/Badge.h"
#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/TableView.h"

#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

Badge::Tone toneForStatus(const QString& status)
{
    if (status == QLatin1String("active") || status == QLatin1String("filled")) {
        return Badge::Tone::Success;
    }
    if (status == QLatin1String("pending") || status == QLatin1String("trial")) {
        return Badge::Tone::Warning;
    }
    if (status == QLatin1String("revoked") || status == QLatin1String("failed")) {
        return Badge::Tone::Danger;
    }
    return Badge::Tone::Neutral;
}

// Missing values are shown as an em dash rather than an empty cell: an empty
// cell is indistinguishable from a column that failed to populate, which is
// exactly the confusion the MissingOptional scenario exists to surface.
QString cell(const QJsonObject& row, const QString& key)
{
    const QJsonValue value = row.value(key);
    if (value.isUndefined() || value.isNull()) {
        return QString::fromUtf8("\xE2\x80\x94");
    }
    if (value.isDouble()) {
        return QString::number(value.toDouble());
    }
    return value.toString();
}

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    const QJsonArray rows = ctx.data->rowsFor(QStringLiteral("users"), ctx.scenario);

    Card* card = exampleCard(page, QStringLiteral("Users"),
                             QStringLiteral("Columns declare a role; the table decides every "
                                            "width."));
    QVBoxLayout* body = card->body();

    if (ctx.scenario == Scenario::ErrorState) {
        body->addWidget(scenarioErrorState(card, QStringLiteral("Users")));
    } else if (rows.isEmpty()) {
        // An empty TableView is still shown, with its header, because a table
        // that vanishes when a filter matches nothing is the defect this
        // scenario is looking for. The EmptyState sits beside it.
        auto* table = new TableView(card);
        table->setColumns({{QStringLiteral("Email"), TableView::ColumnRole::Text},
                           {QStringLiteral("Name"), TableView::ColumnRole::Text},
                           {QStringLiteral("Plan"), TableView::ColumnRole::Content},
                           {QStringLiteral("Status"), TableView::ColumnRole::Content}});
        table->setRowCount(0);
        table->setFixedHeight(120);
        table->relayoutColumns();
        body->addWidget(table);
        body->addWidget(scenarioEmptyState(card, QStringLiteral("users")));
    } else {
        QElapsedTimer timer;
        timer.start();

        auto* table = new TableView(card);
        table->setColumns({{QStringLiteral("Email"), TableView::ColumnRole::Text},
                           {QStringLiteral("Name"), TableView::ColumnRole::Text},
                           {QStringLiteral("Plan"), TableView::ColumnRole::Content},
                           {QStringLiteral("Status"), TableView::ColumnRole::Content},
                           {QStringLiteral("Created"), TableView::ColumnRole::Content},
                           {QStringLiteral("Actions"), TableView::ColumnRole::Content}});
        table->setRowCount(rows.size());

        const bool enabled = ctx.scenario != Scenario::Disabled;
        // Widget cells are the expensive kind: a badge and two buttons per row
        // is three widgets per row, and at a few thousand rows the cost is the
        // widget count rather than anything about the table. Past this
        // threshold the same columns are rendered as text, and the note below
        // says so — a preview that quietly changed what it was showing would
        // be worse than a slow one.
        constexpr int kWidgetCellLimit = 250;
        const bool widgetCells = rows.size() <= kWidgetCellLimit;

        for (int r = 0; r < rows.size(); ++r) {
            const QJsonObject row = rows.at(r).toObject();
            table->setText(r, 0, cell(row, QStringLiteral("email")));
            table->setText(r, 1, cell(row, QStringLiteral("name")));
            table->setText(r, 2, cell(row, QStringLiteral("plan")));

            const QString status = row.value(QStringLiteral("status")).toString();
            if (status.isEmpty() || !widgetCells) {
                table->setText(r, 3, cell(row, QStringLiteral("status")));
            } else {
                table->setWidget(r, 3, new Badge(status, toneForStatus(status), table));
            }
            table->setText(r, 4, cell(row, QStringLiteral("created")));

            if (widgetCells) {
                auto* plan = ghostButton(QStringLiteral("Plan"), table);
                auto* reset = ghostButton(QStringLiteral("Reset PW"), table);
                plan->setEnabled(enabled);
                reset->setEnabled(enabled);
                table->setWidgets(r, 5, {plan, reset});
            } else {
                table->setText(r, 5, QStringLiteral("Plan / Reset PW"));
            }
        }
        table->relayoutColumns();
        table->setMinimumHeight(320);
        body->addWidget(table);

        const qint64 elapsed = timer.elapsed();
        note(body, card,
             QStringLiteral("%1 rows populated and measured in %2 ms. Text columns absorb "
                            "leftover width; Content columns stay exactly as wide as their "
                            "widest cell, and nothing is ever elided.")
                 .arg(rows.size())
                 .arg(elapsed));
        if (!widgetCells) {
            note(body, card,
                 QStringLiteral("Above %1 rows the Status and Actions columns are rendered as "
                                "text instead of widgets — three widgets per row is the cost "
                                "being measured past that point, not the table.")
                     .arg(kWidgetCellLimit));
        }
        if (ctx.scenario == Scenario::LargeDataset) {
            note(body, card,
                 QStringLiteral("Large dataset is %1 rows. Column layout measures every cell on "
                                "every resize, so this number is the honest cost of the "
                                "never-elide guarantee.")
                     .arg(SampleData::largeDatasetSize()));
        }
        if (ctx.scenario == Scenario::MissingOptional) {
            note(body, card,
                 QStringLiteral("Optional fields removed from every row. A missing value is an "
                                "em dash, not an empty cell — an empty cell reads as a column "
                                "that failed to populate."));
        }
    }

    column->addWidget(card, 1);
    return page;
}

} // namespace

ComponentSpec tableViewComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("table-view");
    spec.name = QStringLiteral("TableView");
    spec.category = Category::DataDisplay;
    spec.description = QStringLiteral(
        "The table used across all Novu applications. It owns column layout entirely: a page "
        "declares what each column is, never how wide it becomes.");
    spec.headerPath = QStringLiteral("include/novu/ui/TableView.h");
    spec.sourcePath = QStringLiteral("src/TableView.cpp");
    spec.variants = {QStringLiteral("Text column"), QStringLiteral("Content column")};
    spec.states = {QStringLiteral("Empty"), QStringLiteral("Populated"), QStringLiteral("Error"),
                   QStringLiteral("Disabled row actions")};
    spec.keywords = {QStringLiteral("table"), QStringLiteral("grid"), QStringLiteral("rows"),
                     QStringLiteral("columns"), QStringLiteral("list"), QStringLiteral("data")};
    spec.properties = {
        {QStringLiteral("columns"), QStringLiteral("QVector<QPair<QString,ColumnRole>>"), {},
         QString(), QStringLiteral("Caption and role per column. Declared once, never inferred.")},
        {QStringLiteral("ColumnRole"), QStringLiteral("enum"),
         {QStringLiteral("Text"), QStringLiteral("Content")}, QStringLiteral("Text"),
         QStringLiteral("Text absorbs leftover width; Content is exactly as wide as its widest cell.")},
    };
    spec.scenarios = {Scenario::Normal,     Scenario::Empty,          Scenario::SingleItem,
                      Scenario::ManyItems,  Scenario::LongText,       Scenario::LongIdentifier,
                      Scenario::MissingOptional, Scenario::ErrorState, Scenario::Disabled,
                      Scenario::LargeDataset};
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
