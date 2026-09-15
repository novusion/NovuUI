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
    {"Accent", Badge::Tone::Accent, "the product pointing at something, never a status"},
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
                                 QStringLiteral("Six, and what each one means."));
        QVBoxLayout* body = card->body();

        QStringList rows;
        for (const ToneRow& row : kTones) {
            rows.append(QString::fromLatin1(row.name));
        }
        QGridLayout* grid = matrix(body, card,
                                   {QStringLiteral("Short"), QStringLiteral("Word"),
                                    QStringLiteral("With a dot"), QStringLiteral("Large"),
                                    QStringLiteral("Uppercase"), QStringLiteral("Meaning")},
                                   rows);
        for (int r = 0; r < static_cast<int>(std::size(kTones)); ++r) {
            const ToneRow& row = kTones[r];
            matrixCell(grid, r, 0, new Badge(QStringLiteral("v2"), row.tone, card));
            matrixCell(grid, r, 1, new Badge(QString::fromLatin1(row.name), row.tone, card));

            auto* dotted = new Badge(QStringLiteral("running"), row.tone, card);
            dotted->setDot(true);
            matrixCell(grid, r, 2, dotted);

            auto* large = new Badge(QStringLiteral("1 284.50"), row.tone, card);
            large->setSize(Badge::Size::Large);
            matrixCell(grid, r, 3, large);

            auto* upper = new Badge(QStringLiteral("kraken"), row.tone, card);
            upper->setUppercase(true);
            matrixCell(grid, r, 4, upper);

            matrixCell(grid, r, 5, label(QString::fromLatin1(row.meaning), card,
                                         TypeRole::Caption, Ink::Muted));
        }
        note(body, card,
             QStringLiteral("The dot is not decoration: a badge with one reads as a live "
                            "status, a badge without one reads as a classification. Accent is "
                            "the product accent and is deliberately outside the semantic "
                            "palette — a status may never resolve to it."));
        column->addWidget(card);
    }

    // ── the two factories ────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Status or classification"),
                                 QStringLiteral("Two named factories, because the combination "
                                                "is what carries the meaning."));
        QVBoxLayout* body = card->body();

        QHBoxLayout* live = exampleRow(body, card,
                                       QStringLiteral("statusBadge() — tone, dot, sentence case"));
        live->addWidget(statusBadge(QStringLiteral("running"), Badge::Tone::Success, card));
        live->addWidget(statusBadge(QStringLiteral("paused"), Badge::Tone::Warning, card));
        live->addWidget(statusBadge(QStringLiteral("disconnected"), Badge::Tone::Danger, card));
        live->addWidget(statusBadge(QStringLiteral("open"), Badge::Tone::Info, card));
        live->addWidget(statusBadge(QStringLiteral("stopped"), Badge::Tone::Neutral, card));
        live->addStretch(1);

        QHBoxLayout* tags = exampleRow(body, card,
                                       QStringLiteral("pill() — uppercase, neutral, no dot"));
        for (const QString& text : {QStringLiteral("paper"), QStringLiteral("demo"),
                                    QStringLiteral("spot"), QStringLiteral("v2.4.0"),
                                    QStringLiteral("read only")}) {
            tags->addWidget(pill(text, card));
        }
        tags->addStretch(1);

        note(body, card,
             QStringLiteral("Spelling out \"neutral, uppercase, no dot\" at forty call sites "
                            "is how the distinction stops being one."));
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
    spec.designedFor = {QStringLiteral("novuspace"),
                   QStringLiteral("novuai"),
                   QStringLiteral("novuexplorer"),
                   QStringLiteral("novuhub"),
                   QStringLiteral("novuvoice"),
                   QStringLiteral("novutrade")};
    spec.usedBy = {QStringLiteral("novuadmin"), QStringLiteral("novuspace")};
    spec.variants = {QStringLiteral("Neutral"), QStringLiteral("Success"),
                     QStringLiteral("Warning"), QStringLiteral("Danger"), QStringLiteral("Info"),
                     QStringLiteral("Accent")};
    spec.states = {QStringLiteral("Normal")};
    spec.keywords = {QStringLiteral("tag"), QStringLiteral("status"), QStringLiteral("pill"),
                     QStringLiteral("chip"), QStringLiteral("label"), QStringLiteral("tone")};
    spec.properties = {
        {QStringLiteral("text"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("The whole width of the badge is measured from this.")},
        {QStringLiteral("tone"), QStringLiteral("enum"),
         {QStringLiteral("Neutral"), QStringLiteral("Success"), QStringLiteral("Warning"),
          QStringLiteral("Danger"), QStringLiteral("Info"), QStringLiteral("Accent")},
         QStringLiteral("Neutral"), QStringLiteral("Meaning, not colour.")},
        {QStringLiteral("size"), QStringLiteral("enum"),
         {QStringLiteral("Regular"), QStringLiteral("Large")}, QStringLiteral("Regular"),
         QStringLiteral("In a row, or beside a title.")},
        {QStringLiteral("dot"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("A live status carries one; a classification does not.")},
        {QStringLiteral("uppercase"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("Applied at paint time; text() still returns what was set.")},
    };
    spec.scenarios = {Scenario::Normal,   Scenario::Empty,      Scenario::SingleItem,
                      Scenario::ManyItems, Scenario::LongText,  Scenario::LongIdentifier,
                      Scenario::ErrorState};
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
