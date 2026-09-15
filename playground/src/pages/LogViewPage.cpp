#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/Form.h"
#include "novu/ui/LogView.h"
#include "novu/ui/Segmented.h"
#include "novu/ui/Separator.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QElapsedTimer>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

const struct { LogView::Level level; const char* text; } kSample[] = {
    {LogView::Level::Info, "hub.session: opening transport to hub.novusion.net:51820"},
    {LogView::Level::Debug, "wg.identity: loaded device key from DPAPI blob (32 bytes)"},
    {LogView::Level::Debug, "wg.tunnel: state Disconnected -> Connecting"},
    {LogView::Level::Info, "wg.tunnel: handshake complete, rtt 52ms"},
    {LogView::Level::Debug, "wg.tunnel: state Connecting -> Connected"},
    {LogView::Level::Info, "index.sweep: 4 of 12 workspaces re-read"},
    {LogView::Level::Warning, "index.sweep: workspace 'Archive 2024' is read-only, skipping"},
    {LogView::Level::Debug, "api.client: GET /api/v1/netpeers?limit=100 -> 200 in 41ms"},
    {LogView::Level::Error, "api.client: POST /api/v1/netpeers/revoke -> 409 conflict "
                            "(peer already revoked at 2026-09-14T22:11:03Z)"},
    {LogView::Level::Info, "audit: recorded 1 administrative action"},
    {LogView::Level::Debug, "pulse: 0 subscribers, clock stopped"},
    {LogView::Level::Warning, "hub.session: heartbeat late by 4.2s, will retry"},
    {LogView::Level::Error, "wg.tunnel: handshake did not complete within 15s"},
    {LogView::Level::Debug, "wg.tunnel: backoff 1s -> 2s"},
    {LogView::Level::Info, "wg.tunnel: reconnected, rtt 61ms"},
};

void seed(LogView* view, int rounds = 4)
{
    QVector<QPair<LogView::Level, QString>> lines;
    for (int round = 0; round < rounds; ++round) {
        for (const auto& entry : kSample) {
            lines.append({entry.level, QString::fromLatin1(entry.text)});
        }
    }
    view->appendAll(lines);
}

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    if (ctx.scenario == Scenario::Empty) {
        column->addWidget(scenarioEmptyState(page, QStringLiteral("log lines")));
        column->addStretch(1);
        return page;
    }

    // ── the log ──────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("A running log"),
                                 QStringLiteral("Timestamp, level, message — the "
                                                "level in a column of its own."));
        QVBoxLayout* body = card->body();

        auto* view = new LogView(card);
        view->setMinimumHeight(260);
        seed(view, ctx.scenario == Scenario::LargeDataset ? 400 : 4);

        QHBoxLayout* controls = exampleRow(body, card, QStringLiteral("Controls"));

        auto* levels = new Segmented({QStringLiteral("Debug"), QStringLiteral("Info"),
                                      QStringLiteral("Warning"), QStringLiteral("Error")},
                                     card, 0);
        QObject::connect(levels, &Segmented::currentIndexChanged, view, [view](int index) {
            view->setMinimumLevel(static_cast<LogView::Level>(index));
        });
        controls->addWidget(levels);

        auto* filter = searchBar(card, QObject::tr("Filter…"), 200);
        QObject::connect(filter, &QLineEdit::textChanged, view, &LogView::setFilter);
        controls->addWidget(filter);

        auto* status = label(QString(), card, TypeRole::Mono, Ink::Muted);
        const auto refresh = [status, view] {
            status->setText(QStringLiteral("%1 of %2 · %3")
                                .arg(view->visibleLineCount())
                                .arg(view->lineCount())
                                .arg(view->isFollowing() ? QStringLiteral("following")
                                                         : QStringLiteral("held")));
        };
        refresh();
        QObject::connect(view, &LogView::followingChanged, status,
                         [refresh](bool) { refresh(); });
        QObject::connect(filter, &QLineEdit::textChanged, status,
                         [refresh](const QString&) { refresh(); });
        QObject::connect(levels, &Segmented::currentIndexChanged, status,
                         [refresh](int) { refresh(); });
        controls->addWidget(status);
        controls->addStretch(1);

        body->addWidget(view);

        QHBoxLayout* feed = exampleRow(body, card, QStringLiteral("Feed it"));
        auto* burst = new Button(QStringLiteral("Add 200 lines"), Button::Variant::Secondary,
                                 card);
        burst->setControlSize(ControlSize::Small);
        QObject::connect(burst, &Button::clicked, view, [view, refresh] {
            seed(view, 14);
            refresh();
        });
        feed->addWidget(burst);

        auto* live = new Button(QStringLiteral("Stream"), Button::Variant::Secondary, card);
        live->setControlSize(ControlSize::Small);
        auto* timer = new QTimer(view);
        timer->setInterval(140);
        int* cursor = new int(0);
        QObject::connect(timer, &QTimer::timeout, view, [view, cursor, refresh] {
            const auto& entry = kSample[(*cursor)++ % int(std::size(kSample))];
            view->append(entry.level, QString::fromLatin1(entry.text));
            refresh();
        });
        QObject::connect(live, &Button::clicked, view, [timer, live] {
            if (timer->isActive()) {
                timer->stop();
                live->setText(QObject::tr("Stream"));
            } else {
                timer->start();
                live->setText(QObject::tr("Stop"));
            }
        });
        QObject::connect(view, &QObject::destroyed, timer, [cursor] { delete cursor; });
        feed->addWidget(live);

        auto* clear = new Button(QStringLiteral("Clear"), Button::Variant::Ghost, card);
        clear->setControlSize(ControlSize::Small);
        QObject::connect(clear, &Button::clicked, view, [view, refresh] {
            view->clear();
            refresh();
        });
        feed->addWidget(clear);
        feed->addStretch(1);

        note(body, card,
             QStringLiteral("Press Stream, then scroll up. The view stops following and the "
                            "status says \"held\" — a log that yanks itself back to the bottom "
                            "while someone is reading history is unusable, and that is the "
                            "single most complained-about behaviour these consoles had. Scroll "
                            "back to the end and it follows again. Following is a consequence "
                            "of where the user is, not a mode they set."));
        note(body, card,
             QStringLiteral("A text edit can be made to look right and cannot be made to "
                            "behave right: the level has to be a column, which means either "
                            "padding every message with spaces or a rich-text table costing a "
                            "layout pass per line — and appending to a document of ten "
                            "thousand lines re-lays out ten thousand lines. This holds lines "
                            "in a vector and paints the twenty on screen."));
        column->addWidget(card);
    }

    // ── the cap ──────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("The cap"),
                                 QStringLiteral("A log view must have one."));
        QVBoxLayout* body = card->body();

        auto* view = new LogView(card);
        view->setMaximumLines(40);
        view->setMinimumHeight(140);
        seed(view, 20);

        body->addWidget(view);
        propertyRow(body, card, QStringLiteral("maximumLines"), QString::number(40),
                    Ink::Secondary, true);
        propertyRow(body, card, QStringLiteral("held"),
                    QString::number(view->lineCount()), Ink::Secondary, true);

        note(body, card,
             QStringLiteral("Three hundred lines were appended; forty are kept. A process that "
                            "logs steadily for a day fills memory otherwise, and the oldest "
                            "lines are the ones nobody will read. The default is 5000."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec logViewComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("log-view");
    spec.name = QStringLiteral("LogView");
    spec.category = Category::DataDisplay;
    spec.description = QStringLiteral(
        "A stream of log lines with the level in its own column, filtered by level and by "
        "text. Paints only what is on screen and follows the tail unless you scroll away.");
    spec.headerPath = QStringLiteral("include/novu/ui/LogView.h");
    spec.sourcePath = QStringLiteral("src/LogView.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Debug"), QStringLiteral("Info"),
                     QStringLiteral("Warning"), QStringLiteral("Error")};
    spec.states = {QStringLiteral("Following"), QStringLiteral("Held"),
                   QStringLiteral("Filtered"), QStringLiteral("At the cap")};
    spec.scenarios = {Scenario::Normal, Scenario::Empty, Scenario::LargeDataset};
    spec.keywords = {QStringLiteral("logs"), QStringLiteral("console"),
                     QStringLiteral("output"), QStringLiteral("trace"),
                     QStringLiteral("stdout"), QStringLiteral("terminal")};
    spec.properties = {
        {QStringLiteral("minimumLevel"), QStringLiteral("enum"),
         {QStringLiteral("Debug"), QStringLiteral("Info"), QStringLiteral("Warning"),
          QStringLiteral("Error")},
         QStringLiteral("Debug"),
         QStringLiteral("Filters the view; never discards what it is not showing.")},
        {QStringLiteral("filter"), QStringLiteral("QString"), {}, QStringLiteral(""),
         QStringLiteral("Case-insensitive substring match against the message.")},
        {QStringLiteral("maximumLines"), QStringLiteral("int"), {}, QStringLiteral("5000"),
         QStringLiteral("The oldest go first.")},
        {QStringLiteral("following"), QStringLiteral("bool"), {}, QStringLiteral("true"),
         QStringLiteral("Set by where the user has scrolled, not by the caller.")},
    };
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
