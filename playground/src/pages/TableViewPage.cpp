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

#include <iterator>

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

QColor familyHueNamed(const QString& name)
{
    for (const FamilyHue& hue : familyHues()) {
        if (hue.name == name) {
            return hue.color;
        }
    }
    return QColor();
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

    column->addWidget(card);

    // â”€â”€ identity cells â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    {
        Card* models = exampleCard(
            page, QStringLiteral("Leading type in a Local model row"),
            QStringLiteral("The Model Hub classification stays visible in Local."));
        QVBoxLayout* body = models->body();
        models->setBodyMetrics(QMargins(0, 0, 0, 0), 0);

        auto* table = new TableView(models);
        using Role = TableView::ColumnRole;
        table->setColumns({{QStringLiteral("Name / file"), Role::Text},
                           {QStringLiteral("Size"), Role::Numeric},
                           {QStringLiteral("Status"), Role::Content},
                           {QStringLiteral("Last used"), Role::Mono}});
        const struct {
            const char* code;
            const char* hue;
            const char* title;
            const char* file;
            const char* size;
        } rows[] = {
            {"LAN", "Blue", "Qwen2.5 Coder 14b Instruct", "unsloth.Q8_0.gguf", "14.6 GB"},
            {"IMA", "Purple", "Gemma 4 Vision", "gemma-4-31b-it-q4_k_m.gguf", "17.4 GB"},
            {"VID", "Cyan", "Wan 2.2 I2V", "wan2.2-i2v-fp8.safetensors", "18.0 GB"},
            {"AUD", "Green", "Whisper large-v3", "whisper-large-v3.onnx", "3.1 GB"},
        };
        table->setRowCount(static_cast<int>(std::size(rows)));
        for (int r = 0; r < static_cast<int>(std::size(rows)); ++r) {
            auto* kind = new Badge(QString::fromLatin1(rows[r].code),
                                   Badge::Tone::Neutral, table);
            kind->setClassificationHue(familyHueNamed(QString::fromLatin1(rows[r].hue)));
            table->setIdentity(r, 0, QString::fromLatin1(rows[r].title),
                               QString::fromLatin1(rows[r].file), kind);
            table->setText(r, 1, QString::fromLatin1(rows[r].size));
            table->setWidget(r, 2, statusBadge(QStringLiteral("Ready"),
                                                Badge::Tone::Success, table));
            table->setText(r, 3, QStringLiteral("2026-09-21"));
        }
        table->relayoutColumns();
        body->addWidget(table);
        column->addWidget(models);
    }

    // ── the sheet's own column classes ───────────────────────────────────
    {
        Card* roles = exampleCard(page, QStringLiteral("What a column IS"),
                                  QStringLiteral("Six roles, and every one of "
                                                 "them a class the design "
                                                 "actually writes."));
        QVBoxLayout* body = roles->body();
        roles->setBodyMetrics(QMargins(0, 0, 0, 0), 0);

        auto* table = new TableView(roles);
        using Role = TableView::ColumnRole;
        table->setColumns({{QStringLiteral("Bot"), Role::Text},
                           {QStringLiteral("State"), Role::Content},
                           {QStringLiteral("Why"), Role::Flexible},
                           {QStringLiteral("P&L"), Role::Numeric},
                           {QStringLiteral("Since"), Role::Mono},
                           {QString(), Role::Action}});
        const struct { const char* bot; const char* why; const char* pnl;
                       const char* since; } sample[] = {
            {"Breakout Hunter",
             "Exchange rejected order: insufficient margin (code -2019). "
             "Retried 3 times, bot halted.",
             "-77.50", "18 min ago"},
            {"Kraken Spot",
             "Exchange returned EAPI:Invalid key — the key was revoked or its "
             "permissions changed.",
             "—", "2026-09-13 06:01"},
        };
        table->setRowCount(2);
        for (int r = 0; r < 2; ++r) {
            table->setText(r, 0, QString::fromUtf8(sample[r].bot));
            table->setWidget(r, 1, new Badge(QStringLiteral("error"),
                                             Badge::Tone::Danger, table));
            table->setText(r, 2, QString::fromUtf8(sample[r].why));
            table->setText(r, 3, QString::fromUtf8(sample[r].pnl));
            table->setText(r, 4, QString::fromUtf8(sample[r].since));
            table->setWidgets(r, 5, {ghostButton(QStringLiteral("Restart"), table)});
        }
        table->relayoutColumns();
        body->addWidget(table);

        note(body, roles,
             QStringLiteral("Narrow the window. `Bot`, `Why` and `Since` all give "
                            "width back, together and in proportion to what each was "
                            "holding — so the sentence keeps the width the timestamp "
                            "has no use for, which is what `table-layout:auto` does. "
                            "What they do with text that no longer fits on one line "
                            "is wrap it: `td` states no `white-space`, so only `.num` "
                            "and `.act` stay on one line. The row grows to hold the "
                            "wrapped sentence; the price never moves."));
        note(body, roles,
             QStringLiteral("They differ in how far each may be taken, not in whether "
                            "it is asked. A name column stops while it is still a "
                            "name; a sentence column may go further; and no column is "
                            "ever compressed past its own caption or past the minimum "
                            "of a widget laid out inside it."));
        note(body, roles,
             QStringLiteral("`Since` is `td.mono`: the monospace face a step down in "
                            "the muted ink, stated by the column rather than by a "
                            "styled label the page puts in every cell. That is the "
                            "difference — a label cannot wrap to a width the table "
                            "has not decided yet."));
        column->addWidget(roles);
    }

    column->addStretch(1);
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
    spec.designedFor = {QStringLiteral("novuspace"),
                   QStringLiteral("novuai"),
                   QStringLiteral("novuexplorer"),
                   QStringLiteral("novuhub"),
                   QStringLiteral("novutrade")};
    spec.usedBy = {QStringLiteral("novuadmin"), QStringLiteral("novuspace")};
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
