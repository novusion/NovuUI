#include "Pages.h"

#include "novu/playground/ExampleLayout.h"
#include "novu/playground/SampleData.h"

#include "novu/ui/Badge.h"
#include "novu/ui/Card.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLayout>
#include <QVBoxLayout>
#include <QWidget>

#include <iterator>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

struct ToneRow
{
    const char* name;
    Badge::Tone tone;
    const char* meaning;
};

const ToneRow kTones[] = {
    {"Neutral", Badge::Tone::Neutral, "tags, plans, generic values"},
    {"Success", Badge::Tone::Success, "active, healthy, online"},
    {"Warning", Badge::Tone::Warning, "pending, expiring, trial"},
    {"Danger", Badge::Tone::Danger, "failed, revoked, offline"},
    {"Info", Badge::Tone::Info, "release channels, versions"},
};

Badge::Tone toneForStatus(const QString& status)
{
    if (status == QLatin1String("active") || status == QLatin1String("done")
        || status == QLatin1String("filled")) {
        return Badge::Tone::Success;
    }
    if (status == QLatin1String("pending") || status == QLatin1String("trial")
        || status == QLatin1String("in-progress")) {
        return Badge::Tone::Warning;
    }
    if (status == QLatin1String("failed") || status == QLatin1String("revoked")
        || status == QLatin1String("cancelled")) {
        return Badge::Tone::Danger;
    }
    if (status == QLatin1String("beta") || status == QLatin1String("preview")) {
        return Badge::Tone::Info;
    }
    return Badge::Tone::Neutral;
}

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    // ── tones ────────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Tones"),
                                 QStringLiteral("Five, and what each one means."));
        QVBoxLayout* body = card->body();

        QStringList rows;
        for (const ToneRow& row : kTones) {
            rows.append(QString::fromLatin1(row.name));
        }
        QGridLayout* grid = matrix(body, card,
                                   {QStringLiteral("Short"), QStringLiteral("Word"),
                                    QStringLiteral("Numeric"), QStringLiteral("Meaning")},
                                   rows);
        for (int r = 0; r < static_cast<int>(std::size(kTones)); ++r) {
            const ToneRow& row = kTones[r];
            matrixCell(grid, r, 0, new Badge(QStringLiteral("v2"), row.tone, card));
            matrixCell(grid, r, 1, new Badge(QString::fromLatin1(row.name), row.tone, card));
            matrixCell(grid, r, 2, new Badge(QStringLiteral("1 284.50"), row.tone, card));
            matrixCell(grid, r, 3, label(QString::fromLatin1(row.meaning), card,
                                         TypeRole::Caption, Ink::Muted));
        }
        column->addWidget(card);
    }

    // ── text lengths ─────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Text length"),
                                 QStringLiteral("A badge owns its width. Nothing outside it "
                                                "may set one."));
        QVBoxLayout* body = card->body();
        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("From one character up"));
        const QStringList texts = {
            QStringLiteral("1"), QStringLiteral("OK"), QStringLiteral("Active"),
            QStringLiteral("Awaiting confirmation"),
            ctx.data->text(QStringLiteral("long_identifier")).left(24)};
        for (const QString& text : texts) {
            row->addWidget(new Badge(text, Badge::Tone::Neutral, card));
        }
        row->addStretch(1);

        if (ctx.scenario == Scenario::LongText || ctx.scenario == Scenario::LongIdentifier) {
            const QString hostile = ctx.scenario == Scenario::LongText
                ? ctx.data->text(QStringLiteral("long_text"))
                : ctx.data->text(QStringLiteral("long_identifier"));
            QHBoxLayout* stress = exampleRow(body, card, QStringLiteral("This scenario"));
            stress->addWidget(new Badge(hostile, Badge::Tone::Warning, card));
            stress->addStretch(1);
            note(body, card,
                 QStringLiteral("A badge does not elide. If this one leaves the card, the "
                                "caller is passing a value that was never meant to be a badge."));
        }
        column->addWidget(card);
    }

    // ── from data ────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("From data"),
                                 QStringLiteral("Status values out of the sample set, mapped "
                                                "to tones."));
        QVBoxLayout* body = card->body();
        const QJsonArray rows = ctx.data->rowsFor(QStringLiteral("users"), ctx.scenario);

        if (ctx.scenario == Scenario::ErrorState) {
            body->addWidget(scenarioErrorState(card, QStringLiteral("Statuses")));
        } else if (rows.isEmpty()) {
            body->addWidget(scenarioEmptyState(card, QStringLiteral("statuses")));
        } else {
            QLayout* flow = wrappingRow(body, card, QStringLiteral("One badge per row"));
            const int limit = qMin(rows.size(), 60);
            for (int i = 0; i < limit; ++i) {
                const QJsonObject object = rows.at(i).toObject();
                const QString status = object.value(QStringLiteral("status")).toString(
                    QStringLiteral("unknown"));
                flow->addWidget(new Badge(status, toneForStatus(status), card));
            }
            if (rows.size() > limit) {
                note(body, card, QStringLiteral("Showing %1 of %2 — a badge per row past this "
                                                "point measures the flow layout, not the badge.")
                                     .arg(limit)
                                     .arg(rows.size()));
            }
        }
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec badgeComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("badge");
    spec.name = QStringLiteral("Badge");
    spec.category = Category::DataDisplay;
    spec.description = QStringLiteral(
        "A pill-shaped label for status, tag, plan, release and price. Self-painted, so "
        "sizeHint() is correct from construction rather than after polish.");
    spec.headerPath = QStringLiteral("include/novu/ui/Badge.h");
    spec.sourcePath = QStringLiteral("src/Badge.cpp");
    spec.usedBy = {QStringLiteral("novuspace"),
                   QStringLiteral("novuai"),
                   QStringLiteral("novuexplorer"),
                   QStringLiteral("novuhub"),
                   QStringLiteral("novuvoice"),
                   QStringLiteral("novutrade")};
    spec.variants = {QStringLiteral("Neutral"), QStringLiteral("Success"),
                     QStringLiteral("Warning"), QStringLiteral("Danger"), QStringLiteral("Info")};
    spec.states = {QStringLiteral("Normal")};
    spec.keywords = {QStringLiteral("tag"), QStringLiteral("status"), QStringLiteral("pill"),
                     QStringLiteral("chip"), QStringLiteral("label"), QStringLiteral("tone")};
    spec.properties = {
        {QStringLiteral("text"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("The whole width of the badge is measured from this.")},
        {QStringLiteral("tone"), QStringLiteral("enum"),
         {QStringLiteral("Neutral"), QStringLiteral("Success"), QStringLiteral("Warning"),
          QStringLiteral("Danger"), QStringLiteral("Info")},
         QStringLiteral("Neutral"), QStringLiteral("Meaning, not colour.")},
    };
    spec.scenarios = {Scenario::Normal,   Scenario::Empty,      Scenario::SingleItem,
                      Scenario::ManyItems, Scenario::LongText,  Scenario::LongIdentifier,
                      Scenario::ErrorState};
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
