#include "Pages.h"

#include "novu/playground/ExampleLayout.h"
#include "novu/playground/SampleData.h"

#include "novu/ui/Card.h"
#include "novu/ui/StatCard.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

Ink toneFromName(const QString& name)
{
    if (name == QLatin1String("success")) return Ink::Success;
    if (name == QLatin1String("warning")) return Ink::Warning;
    if (name == QLatin1String("danger"))  return Ink::Danger;
    if (name == QLatin1String("accent"))  return Ink::Accent;
    if (name == QLatin1String("info"))    return Ink::Info;
    if (name == QLatin1String("muted"))   return Ink::Muted;
    return Ink::Default;
}

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    const QJsonArray rows = ctx.data->rowsFor(QStringLiteral("metrics"), ctx.scenario);

    // ── the cards ────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("StatCard"),
                                 QStringLiteral("A caption and one large number, updated in "
                                                "place."));
        QVBoxLayout* body = card->body();

        if (ctx.scenario == Scenario::ErrorState) {
            body->addWidget(scenarioErrorState(card, QStringLiteral("Metrics")));
        } else if (rows.isEmpty()) {
            body->addWidget(scenarioEmptyState(card, QStringLiteral("metrics")));
        } else {
            QHBoxLayout* row = exampleRow(body, card, QString());
            const int limit = qMin(rows.size(), 6);
            for (int i = 0; i < limit; ++i) {
                const QJsonObject object = rows.at(i).toObject();
                row->addWidget(new StatCard(
                    object.value(QStringLiteral("caption")).toString(),
                    object.value(QStringLiteral("value")).toString(QStringLiteral("—")),
                    card,
                    toneFromName(object.value(QStringLiteral("tone")).toString())));
            }
            row->addStretch(1);
            note(body, card,
                 QStringLiteral("setValue() updates in place. A metric card is never rebuilt "
                                "because its number changed — rebuilding a widget to change its "
                                "text is what makes a dashboard flicker and lose focus on every "
                                "refresh."));
        }
        column->addWidget(card);
    }

    // ── the grid ─────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("statGrid() / MiniStat"),
                                 QStringLiteral("Metric cells inside a card that already is a "
                                                "surface, so they have none of their own."));
        QVBoxLayout* body = card->body();

        if (rows.isEmpty()) {
            body->addWidget(scenarioEmptyState(card, QStringLiteral("metrics")));
        } else {
            QVector<StatSpec> stats;
            const int limit = qMin(rows.size(), 8);
            for (int i = 0; i < limit; ++i) {
                const QJsonObject object = rows.at(i).toObject();
                stats.append({object.value(QStringLiteral("caption")).toString(),
                              object.value(QStringLiteral("value")).toString(QStringLiteral("—")),
                              toneFromName(object.value(QStringLiteral("tone")).toString())});
            }
            body->addLayout(statGrid(card, stats, 3));
            note(body, card,
                 QStringLiteral("statGrid() returns a layout — it creates no container of its "
                                "own."));
        }
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec statCardComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("stat-card");
    spec.name = QStringLiteral("StatCard");
    spec.category = Category::DataDisplay;
    spec.description = QStringLiteral(
        "A caption and one large number, plus MiniStat and statGrid() for a metric grid inside "
        "a card that is already a surface.");
    spec.headerPath = QStringLiteral("include/novu/ui/StatCard.h");
    spec.sourcePath = QStringLiteral("src/StatCard.cpp");
    spec.usedBy = {QStringLiteral("novuspace"),
                   QStringLiteral("novuai"),
                   QStringLiteral("novutrade")};
    spec.variants = {QStringLiteral("StatCard"), QStringLiteral("MiniStat"),
                     QStringLiteral("statGrid()")};
    spec.states = {QStringLiteral("Default"), QStringLiteral("Success"), QStringLiteral("Warning"),
                   QStringLiteral("Danger"), QStringLiteral("Empty")};
    spec.keywords = {QStringLiteral("metric"), QStringLiteral("kpi"), QStringLiteral("number"),
                     QStringLiteral("dashboard"), QStringLiteral("summary")};
    spec.properties = {
        {QStringLiteral("caption"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("What the number is.")},
        {QStringLiteral("value"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("The number. setValue() updates in place.")},
        {QStringLiteral("tone"), QStringLiteral("enum"),
         {QStringLiteral("Default"), QStringLiteral("Secondary"), QStringLiteral("Muted"),
          QStringLiteral("Accent"), QStringLiteral("Success"), QStringLiteral("Warning"),
          QStringLiteral("Danger"), QStringLiteral("Info")},
         QStringLiteral("Default"), QStringLiteral("Ink role for the value.")},
        {QStringLiteral("columns"), QStringLiteral("int"), {}, QStringLiteral("2"),
         QStringLiteral("statGrid() column count.")},
    };
    spec.scenarios = {Scenario::Normal,     Scenario::Empty,     Scenario::SingleItem,
                      Scenario::ManyItems,  Scenario::LongText,  Scenario::LongIdentifier,
                      Scenario::MissingOptional, Scenario::ErrorState};
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
