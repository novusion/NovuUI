#include "Pages.h"

#include "novu/playground/ExampleLayout.h"
#include "novu/playground/SampleData.h"

#include "novu/ui/Badge.h"
#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/Feedback.h"
#include "novu/ui/Separator.h"
#include "novu/ui/Form.h"
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

    {
        Card* card = exampleCard(page, QStringLiteral("A message nobody budgeted for"),
                                 QStringLiteral("Width is a preference. The sentence never "
                                                "gets a vote."));
        QVBoxLayout* body = card->body();

        // The real thing, verbatim: one unbroken line of linker output. This
        // card is a regression guard, not an illustration - a banner that sized
        // itself to this text dragged the gallery window off the screen, and
        // the only way that stays fixed is if it is visible when it breaks.
        body->addWidget(new InfoBanner(
            QStringLiteral("NovuUI.lib(Button.obj) : error LNK2019: unresolved external symbol "
                           "\"public: static class novu::ui::Pulse & __cdecl "
                           "novu::ui::Pulse::instance(void)\" (?instance@Pulse@ui@novu@@SAEAAV123"
                           "@XZ) referenced in function \"private: void __cdecl "
                           "novu::ui::Button::applyVariant(void)\" (?applyVariant@Button@ui@novu"
                           "@@AEAAXXZ)"),
            Badge::Tone::Danger, card));

        auto* retry = ghostButton(QStringLiteral("Rebuild"), card);
        body->addWidget(new InfoBanner(
            QStringLiteral("C:/Users/someone/Projects/Novusion/NovuSpace/modules/novutrade/"
                           "backtesting/strategies/mean-reversion/parameters/optimisation.json "
                           "could not be written: the path exceeds the maximum length."),
            Badge::Tone::Warning, card, retry));

        note(body, card,
             QStringLiteral("Both elide to whatever they are given, and both keep the whole "
                            "message on the tooltip. The second one elides against its button "
                            "rather than underneath it. Drag the window narrower - the banners "
                            "shrink; nothing else moves."));
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
         QStringLiteral("setValue() clamps to 0..100 and only repaints on a real change. "
                        "A caller computing a percentage from two live counters will hand "
                        "over 101 sooner or later, and a fill wider than its track paints "
                        "over the card it is in."));
    column->addWidget(card);

    // ── capacity, which colours itself ───────────────────────────────────
    {
        Card* usage = exampleCard(page, QStringLiteral("How much is used"),
                                  QStringLiteral("A capacity bar picks its own "
                                                 "colour; a progress bar must not."));
        QVBoxLayout* usageBody = usage->body();

        const struct { int percent; const char* used; const char* left; } capacities[] = {
            {34, "34%", "312 GB free"},
            {78, "78%", "104 GB free"},
            {94, "94%", "28 GB free"},
        };
        for (const auto& entry : capacities) {
            QHBoxLayout* row = exampleRow(
                usageBody, usage, QStringLiteral("At %1%").arg(entry.percent));
            auto* bar = new ProgressBar(entry.percent, ProgressBar::Tone::Threshold, usage);
            bar->setLegend(QString::fromLatin1(entry.used),
                           QString::fromLatin1(entry.left));
            bar->setFixedWidth(280);
            row->addWidget(bar);
            row->addWidget(label(QStringLiteral("effectiveRole() = %1")
                                     .arg(semanticRoleName(bar->effectiveRole())),
                                 usage, TypeRole::Mono, Ink::Muted));
            row->addStretch(1);
        }

        note(usageBody, usage,
             QStringLiteral("Amber past 75%, red past 90% — a full disk has to read as full "
                            "without anyone reading the number. The progress bars above do "
                            "not do this, or every download would turn red as it finished, "
                            "so it is a tone the caller chooses rather than something the "
                            "component decides for itself. Ported from NovuAdmin's UsageBar."));
        column->addWidget(usage);
    }

    // ── not measured ─────────────────────────────────────────────────────
    {
        Card* unknown = exampleCard(page, QStringLiteral("Not measured"),
                                    QStringLiteral("Which is not the same as zero."));
        QVBoxLayout* unknownBody = unknown->body();

        QHBoxLayout* none = exampleRow(unknownBody, unknown, QStringLiteral("Unknown"));
        auto* nothing = new ProgressBar(0, ProgressBar::Tone::Threshold, unknown);
        nothing->setUnknown(true);
        nothing->setLegend(QStringLiteral("—"), QStringLiteral("no reading"));
        nothing->setFixedWidth(280);
        none->addWidget(nothing);
        none->addStretch(1);

        QHBoxLayout* zero = exampleRow(unknownBody, unknown,
                                       QStringLiteral("Measured zero"));
        auto* empty = new ProgressBar(0, ProgressBar::Tone::Threshold, unknown);
        empty->setLegend(QStringLiteral("0%"), QStringLiteral("1.2 TB free"));
        empty->setFixedWidth(280);
        zero->addWidget(empty);
        zero->addStretch(1);

        note(unknownBody, unknown,
             QStringLiteral("Drawn differently on purpose. A telemetry panel that cannot reach "
                            "a host shows the first; an idle disk shows the second. A value of "
                            "0 cannot say which one it is, which is the whole reason "
                            "setUnknown() exists."));
        column->addWidget(unknown);
    }

    // ── a metric row ─────────────────────────────────────────────────────
    {
        Card* metrics = exampleCard(page, QStringLiteral("A metric row"),
                                    QStringLiteral("Composed, rather than a fourth "
                                                   "row component."));
        QVBoxLayout* metricsBody = metrics->body();
        metricsBody->setSpacing(4);

        const struct { const char* label; const char* value; int percent; } readings[] = {
            {"Memory", "6.1 GB / 16 GB", 38},
            {"Disk", "912 GB / 1 TB", 91},
            {"Sessions", "144 / 200", 72},
        };
        for (const auto& reading : readings) {
            propertyRow(metricsBody, metrics, QString::fromLatin1(reading.label),
                        QString::fromLatin1(reading.value), Ink::Secondary, true);
            metricsBody->addWidget(
                new ProgressBar(reading.percent, ProgressBar::Tone::Threshold, metrics));
            metricsBody->addSpacing(8);
        }

        separator(metricsBody, metrics);
        QHBoxLayout* bytes = exampleRow(metricsBody, metrics,
                                        QStringLiteral("formatBytes()"));
        for (const qint64 size : {qint64(0), qint64(812), qint64(1024) * 1024 * 3 / 2,
                                  qint64(1024) * 1024 * 1024 * 14,
                                  qint64(1024) * 1024 * 1024 * 1024 * 3}) {
            bytes->addWidget(new Badge(formatBytes(size), Badge::Tone::Neutral, metrics));
        }
        bytes->addStretch(1);

        note(metricsBody, metrics,
             QStringLiteral("A labelled reading with a bar under it is propertyRow() plus a "
                            "ProgressBar, not a fourth row component. The library already has "
                            "three kinds of row, and a fourth that is one of them with a bar "
                            "is how a component set stops being learnable."));
        note(metricsBody, metrics,
             QStringLiteral("formatBytes() is shared for the same reason a size in a table and "
                            "a size under a bar must agree. NovuAdmin had two of these and "
                            "they disagreed about whether a kilobyte was 1000 or 1024, which "
                            "showed as a file that was 1.0 MB in the table and 0.98 MB in its "
                            "own properties."));
        column->addWidget(metrics);
    }

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
    spec.designedFor = {QStringLiteral("novuspace"),
                   QStringLiteral("novuai"),
                   QStringLiteral("novuexplorer"),
                   QStringLiteral("novutrade")};
    spec.usedBy = {QStringLiteral("novuspace")};
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
    spec.designedFor = {QStringLiteral("novuspace"),
                   QStringLiteral("novuai"),
                   QStringLiteral("novuexplorer"),
                   QStringLiteral("novutrade")};
    spec.usedBy = {QStringLiteral("novuspace")};
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
        "A proportion: how much is done, or how much is used. Capacity bars colour "
        "themselves by threshold; an unmeasured reading is drawn differently from zero.");
    spec.headerPath = QStringLiteral("include/novu/ui/Feedback.h");
    spec.sourcePath = QStringLiteral("src/Feedback.cpp");
    spec.designedFor = {QStringLiteral("novuspace"),
                   QStringLiteral("novuexplorer"),
                   QStringLiteral("novuhub"),
                   QStringLiteral("novutrade")};
    spec.usedBy = {QStringLiteral("novuspace")};
    spec.variants = {QStringLiteral("Accent"), QStringLiteral("Threshold"),
                     QStringLiteral("Success"), QStringLiteral("Warning"),
                     QStringLiteral("Danger")};
    spec.states = {QStringLiteral("0%"), QStringLiteral("Partial"), QStringLiteral("100%"),
                   QStringLiteral("Running"), QStringLiteral("Unknown"),
                   QStringLiteral("Error")};
    spec.keywords = {QStringLiteral("progress"), QStringLiteral("loading"),
                     QStringLiteral("busy"), QStringLiteral("percent"),
                     QStringLiteral("feedback"), QStringLiteral("meter"),
                     QStringLiteral("quota"), QStringLiteral("usage"),
                     QStringLiteral("capacity"), QStringLiteral("formatBytes")};
    spec.properties = {
        {QStringLiteral("value"), QStringLiteral("int"), {}, QStringLiteral("0"),
         QStringLiteral("0..100. Clamped, and only repaints on a real change.")},
        {QStringLiteral("tone"), QStringLiteral("enum"),
         {QStringLiteral("Accent"), QStringLiteral("Threshold"), QStringLiteral("Success"),
          QStringLiteral("Warning"), QStringLiteral("Danger")},
         QStringLiteral("Accent"),
         QStringLiteral("Threshold goes amber past 75% and red past 90%.")},
        {QStringLiteral("unknown"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("An empty track and no fill: not measured, which is not idle.")},
        {QStringLiteral("legend"), QStringLiteral("QString"), {}, QStringLiteral(""),
         QStringLiteral("Used on the left, left on the right. Adds a line to the height.")},
    };
    spec.scenarios = {Scenario::Normal, Scenario::ErrorState};
    spec.build = buildProgressBar;
    return spec;
}

} // namespace novu::playground::pages
