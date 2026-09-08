#include "Pages.h"

#include "novu/playground/ExampleLayout.h"
#include "novu/playground/SampleData.h"

#include "novu/ui/Badge.h"
#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/Feedback.h"
#include "novu/ui/Text.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

// Three components share a header and a source file; they get three registry
// entries because a developer looking for "banner" is not looking for
// "progress", and one page called "Feedback" would hide two of the three from
// the search that is supposed to find them.

struct ToneRow
{
    const char* name;
    Badge::Tone tone;
};

const ToneRow kTones[] = {
    {"Neutral", Badge::Tone::Neutral},
    {"Success", Badge::Tone::Success},
    {"Warning", Badge::Tone::Warning},
    {"Danger", Badge::Tone::Danger},
    {"Info", Badge::Tone::Info},
};

QWidget* buildEmptyState(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    const QString longText = ctx.data->text(QStringLiteral("long_text"));

    {
        Card* card = exampleCard(page, QStringLiteral("The three lines"),
                                 QStringLiteral("Glyph, what is missing, what would make it "
                                                "appear."));
        QVBoxLayout* body = card->body();
        body->addWidget(new EmptyState(QString(QChar(0xE9D9)),
                                       QStringLiteral("No open trades"),
                                       QStringLiteral("Positions appear here once the bot places "
                                                      "its first order."),
                                       card));
        note(body, card,
             QStringLiteral("The third line matters: \"No open trades\" alone reads as a fault, "
                            "and users report it as one."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("Wording lengths"),
                                 QStringLiteral("Centred, so a long subtitle must not push the "
                                                "glyph off axis."));
        QVBoxLayout* body = card->body();
        const QString subtitle = ctx.scenario == Scenario::LongText
            ? longText
            : QStringLiteral("Connect an exchange to start.");
        const QString title = ctx.scenario == Scenario::LongIdentifier
            ? ctx.data->text(QStringLiteral("long_identifier"))
            : QStringLiteral("Nothing to show");
        body->addWidget(new EmptyState(QString(QChar(0xE7C3)), title, subtitle, card));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

QWidget* buildInfoBanner(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    const QString text = ctx.scenario == Scenario::LongText
        ? ctx.data->text(QStringLiteral("long_text"))
        : ctx.scenario == Scenario::LongIdentifier
            ? ctx.data->text(QStringLiteral("long_identifier"))
            : QStringLiteral("Live data is delayed by up to fifteen seconds on this plan.");

    {
        Card* card = exampleCard(page, QStringLiteral("Tones"),
                                 QStringLiteral("One sentence of context, tinted by meaning."));
        QVBoxLayout* body = card->body();
        for (const ToneRow& row : kTones) {
            body->addWidget(caption(QString::fromLatin1(row.name), card));
            body->addWidget(new InfoBanner(text, row.tone, card));
        }
        note(body, card,
             QStringLiteral("Painted rather than styled: a tint per tone would need one "
                            "stylesheet rule each, and a QFrame rule cascades into every QLabel "
                            "inside it — the defect this codebase has hit most often."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("With an action"),
                                 QStringLiteral("The trailing widget is the caller's."));
        QVBoxLayout* body = card->body();
        auto* action = ghostButton(QStringLiteral("Upgrade"), card);
        action->setEnabled(ctx.scenario != Scenario::Disabled);
        body->addWidget(new InfoBanner(QStringLiteral("Your trial ends in three days."),
                                       Badge::Tone::Warning, card, action));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

QWidget* buildProgressBar(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    Card* card = exampleCard(page, QStringLiteral("Determinate progress"),
                             QStringLiteral("For long local work: a backup, a retention sweep, "
                                            "an export."));
    QVBoxLayout* body = card->body();

    for (const int value : {0, 12, 50, 88, 100}) {
        body->addWidget(caption(QStringLiteral("%1%").arg(value), card));
        body->addWidget(new ProgressBar(value, card));
    }

    if (ctx.scenario == Scenario::Normal) {
        // A live bar, because a static screenshot of a progress strip proves
        // nothing about how it repaints.
        body->addWidget(caption(QStringLiteral("Running"), card));
        auto* live = new ProgressBar(0, card);
        body->addWidget(live);
        auto* timer = new QTimer(live);
        timer->setInterval(120);
        QObject::connect(timer, &QTimer::timeout, live, [live]() {
            live->setValue(live->value() >= 100 ? 0 : live->value() + 2);
        });
        timer->start();
    }

    if (ctx.scenario == Scenario::ErrorState) {
        body->addWidget(new InfoBanner(QStringLiteral("Export failed at 43%."),
                                       Badge::Tone::Danger, card));
        body->addWidget(new ProgressBar(43, card));
    }

    note(body, card,
         QStringLiteral("setValue() clamps to 0..100 and only repaints on a real change."));
    column->addWidget(card);

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec emptyStateComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("empty-state");
    spec.name = QStringLiteral("EmptyState");
    spec.category = Category::Feedback;
    spec.description = QStringLiteral(
        "What a panel shows instead of a table when there is nothing to show: a glyph, what is "
        "missing, and what would make it appear.");
    spec.headerPath = QStringLiteral("include/novu/ui/Feedback.h");
    spec.sourcePath = QStringLiteral("src/Feedback.cpp");
    spec.usedBy = {QStringLiteral("novuspace"),
                   QStringLiteral("novuai"),
                   QStringLiteral("novuexplorer"),
                   QStringLiteral("novutrade")};
    spec.variants = {QStringLiteral("Glyph + title + subtitle")};
    spec.states = {QStringLiteral("Normal")};
    spec.keywords = {QStringLiteral("empty"), QStringLiteral("placeholder"),
                     QStringLiteral("nothing"), QStringLiteral("zero state"),
                     QStringLiteral("feedback")};
    spec.properties = {
        {QStringLiteral("glyph"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("Segoe Fluent / MDL2 code point.")},
        {QStringLiteral("title"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("What is missing.")},
        {QStringLiteral("subtitle"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("What would make it appear. Not optional in practice.")},
    };
    spec.scenarios = {Scenario::Normal, Scenario::LongText, Scenario::LongIdentifier};
    spec.build = buildEmptyState;
    return spec;
}

ComponentSpec infoBannerComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("info-banner");
    spec.name = QStringLiteral("InfoBanner");
    spec.category = Category::Feedback;
    spec.description = QStringLiteral(
        "A tinted strip carrying one sentence of context, optionally with an action. Painted "
        "rather than styled, so a tone costs no stylesheet rule.");
    spec.headerPath = QStringLiteral("include/novu/ui/Feedback.h");
    spec.sourcePath = QStringLiteral("src/Feedback.cpp");
    spec.usedBy = {QStringLiteral("novuspace"),
                   QStringLiteral("novuai"),
                   QStringLiteral("novuexplorer"),
                   QStringLiteral("novutrade")};
    spec.variants = {QStringLiteral("Neutral"), QStringLiteral("Success"),
                     QStringLiteral("Warning"), QStringLiteral("Danger"), QStringLiteral("Info"),
                     QStringLiteral("With trailing action")};
    spec.states = {QStringLiteral("Normal"), QStringLiteral("Disabled action")};
    spec.keywords = {QStringLiteral("banner"), QStringLiteral("notice"), QStringLiteral("alert"),
                     QStringLiteral("message"), QStringLiteral("callout"),
                     QStringLiteral("feedback")};
    spec.properties = {
        {QStringLiteral("text"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("One sentence.")},
        {QStringLiteral("tone"), QStringLiteral("enum"),
         {QStringLiteral("Neutral"), QStringLiteral("Success"), QStringLiteral("Warning"),
          QStringLiteral("Danger"), QStringLiteral("Info")},
         QStringLiteral("Info"), QStringLiteral("Tint and text colour.")},
        {QStringLiteral("trailing"), QStringLiteral("QWidget*"), {}, QString(),
         QStringLiteral("Optional action, owned by the caller.")},
    };
    spec.scenarios = {Scenario::Normal, Scenario::LongText, Scenario::LongIdentifier,
                      Scenario::Disabled};
    spec.build = buildInfoBanner;
    return spec;
}

ComponentSpec progressBarComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("progress-bar");
    spec.name = QStringLiteral("ProgressBar");
    spec.category = Category::Feedback;
    spec.description = QStringLiteral(
        "A determinate progress strip for long local work. Six pixels tall, painted from the "
        "profile.");
    spec.headerPath = QStringLiteral("include/novu/ui/Feedback.h");
    spec.sourcePath = QStringLiteral("src/Feedback.cpp");
    spec.usedBy = {QStringLiteral("novuspace"),
                   QStringLiteral("novuexplorer"),
                   QStringLiteral("novuhub"),
                   QStringLiteral("novutrade")};
    spec.variants = {QStringLiteral("Determinate")};
    spec.states = {QStringLiteral("0%"), QStringLiteral("Partial"), QStringLiteral("100%"),
                   QStringLiteral("Running"), QStringLiteral("Error")};
    spec.keywords = {QStringLiteral("progress"), QStringLiteral("loading"),
                     QStringLiteral("busy"), QStringLiteral("percent"),
                     QStringLiteral("feedback")};
    spec.properties = {
        {QStringLiteral("value"), QStringLiteral("int"), {}, QStringLiteral("0"),
         QStringLiteral("0..100. Clamped, and only repaints on a real change.")},
    };
    spec.scenarios = {Scenario::Normal, Scenario::ErrorState};
    spec.build = buildProgressBar;
    return spec;
}

} // namespace novu::playground::pages
