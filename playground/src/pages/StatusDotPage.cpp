#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/Separator.h"
#include "novu/ui/StatusDot.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/Text.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

#include <iterator>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

struct ToneRow
{
    const char* name;
    StatusDot::Tone tone;
    const char* means;
};

const ToneRow kTones[] = {
    {"Neutral", StatusDot::Tone::Neutral, "idle, stopped, not configured"},
    {"Success", StatusDot::Tone::Success, "connected, running, live"},
    {"Warning", StatusDot::Tone::Warning, "degraded, paused, reconnecting"},
    {"Danger", StatusDot::Tone::Danger, "disconnected, failed"},
    {"Info", StatusDot::Tone::Info, "informational"},
    {"Accent", StatusDot::Tone::Accent, "the product pointing at something"},
};

QWidget* dotWithLabel(QWidget* parent, StatusDot::Tone tone, bool pulsing,
                      const QString& text)
{
    auto* row = new QWidget(parent);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(themeFor(parent).spacing.sm);
    layout->addWidget(new StatusDot(tone, pulsing, row));
    layout->addWidget(label(text, row, TypeRole::Caption, Ink::Secondary));
    layout->addStretch(1);
    return row;
}

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    // ── tones ────────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Tones"),
                                 QStringLiteral("Still, and breathing."));
        QVBoxLayout* body = card->body();

        QStringList rows;
        for (const ToneRow& row : kTones) {
            rows.append(QString::fromLatin1(row.name));
        }
        QGridLayout* grid = matrix(body, card,
                                   {QStringLiteral("Still"), QStringLiteral("Pulsing"),
                                    QStringLiteral("Larger"), QStringLiteral("Means")},
                                   rows);

        for (int r = 0; r < static_cast<int>(std::size(kTones)); ++r) {
            const ToneRow& row = kTones[r];
            matrixCell(grid, r, 0, new StatusDot(row.tone, card));
            matrixCell(grid, r, 1, new StatusDot(row.tone, true, card));

            auto* big = new StatusDot(row.tone, true, card);
            big->setDiameter(11);
            matrixCell(grid, r, 2, big);

            matrixCell(grid, r, 3, label(QString::fromLatin1(row.means), card,
                                         TypeRole::Caption, Ink::Muted));
        }

        note(body, card,
             QStringLiteral("The pulsing column breathes on the shared clock, so every dot in "
                            "the window is in step — the same thing CSS keyframes do, and the "
                            "reason two dots side by side do not shimmer against each other."));
        column->addWidget(card);
    }

    // ── changing state ───────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Changing state"),
                                 QStringLiteral("The behaviour worth keeping: a status "
                                                "cross-fades."));
        QVBoxLayout* body = card->body();

        auto* dot = new StatusDot(StatusDot::Tone::Success, true, card);
        dot->setDiameter(11);

        auto* status = label(QStringLiteral("connected"), card, TypeRole::Mono, Ink::Secondary);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("A connection"));
        row->addWidget(dot);
        row->addSpacing(4);
        row->addWidget(status);
        row->addSpacing(16);

        const struct { const char* caption; StatusDot::Tone tone; const char* word; } steps[] = {
            {"Connect", StatusDot::Tone::Success, "connected"},
            {"Degrade", StatusDot::Tone::Warning, "reconnecting"},
            {"Drop", StatusDot::Tone::Danger, "disconnected"},
            {"Stop", StatusDot::Tone::Neutral, "stopped"},
        };
        for (const auto& step : steps) {
            auto* button = new Button(QString::fromLatin1(step.caption),
                                      Button::Variant::Secondary, card);
            button->setControlSize(ControlSize::Small);
            const StatusDot::Tone tone = step.tone;
            const QString word = QString::fromLatin1(step.word);
            QObject::connect(button, &Button::clicked, dot, [dot, status, tone, word] {
                dot->setTone(tone);
                dot->setPulsing(tone == StatusDot::Tone::Success
                                || tone == StatusDot::Tone::Warning);
                status->setText(word);
            });
            row->addWidget(button);
        }
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Press them in turn. The colour crosses over rather than "
                            "switching: a connection dropping is something the eye should "
                            "catch, and a dot that changes between two frames is something "
                            "the eye misses. Ported from NovusionUI's Dot, which got this "
                            "right first."));
        column->addWidget(card);
    }

    // ── where it goes ────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("In place"),
                                 QStringLiteral("What it looks like doing its job."));
        QVBoxLayout* body = card->body();

        body->addWidget(dotWithLabel(card, StatusDot::Tone::Success, true,
                                     QStringLiteral("Kraken  ·  live  ·  14ms")));
        separator(body, card);
        body->addWidget(dotWithLabel(card, StatusDot::Tone::Warning, true,
                                     QStringLiteral("Binance  ·  reconnecting  ·  attempt 3")));
        separator(body, card);
        body->addWidget(dotWithLabel(card, StatusDot::Tone::Danger, false,
                                     QStringLiteral("Coinbase  ·  disconnected")));
        separator(body, card);
        body->addWidget(dotWithLabel(card, StatusDot::Tone::Neutral, false,
                                     QStringLiteral("Bitstamp  ·  not configured")));

        note(body, card,
             QStringLiteral("Only the live ones breathe. A dot that pulses while nothing is "
                            "happening is a dot nobody reads any more, so the pulse is a "
                            "property of the state rather than of the component."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec statusDotComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("status-dot");
    spec.name = QStringLiteral("StatusDot");
    spec.category = Category::DataDisplay;
    spec.description = QStringLiteral(
        "The dot that says something is live. Breathes on the shared clock, cross-fades "
        "between states, and takes a tone rather than a colour.");
    spec.headerPath = QStringLiteral("include/novu/ui/StatusDot.h");
    spec.sourcePath = QStringLiteral("src/StatusDot.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Neutral"), QStringLiteral("Success"),
                     QStringLiteral("Warning"), QStringLiteral("Danger"),
                     QStringLiteral("Info"), QStringLiteral("Accent")};
    spec.states = {QStringLiteral("Still"), QStringLiteral("Pulsing")};
    spec.keywords = {QStringLiteral("dot"), QStringLiteral("livedot"), QStringLiteral("led"),
                     QStringLiteral("indicator"), QStringLiteral("online"),
                     QStringLiteral("connection"), QStringLiteral("pulse")};
    spec.properties = {
        {QStringLiteral("tone"), QStringLiteral("enum"),
         {QStringLiteral("Neutral"), QStringLiteral("Success"), QStringLiteral("Warning"),
          QStringLiteral("Danger"), QStringLiteral("Info"), QStringLiteral("Accent")},
         QStringLiteral("Neutral"), QStringLiteral("Meaning. Changing it cross-fades.")},
        {QStringLiteral("pulsing"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("Whether this one is reporting something live.")},
        {QStringLiteral("diameter"), QStringLiteral("int"), {}, QStringLiteral("7"),
         QStringLiteral("The dot; the halo sits outside it, so layout does not move.")},
    };
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
