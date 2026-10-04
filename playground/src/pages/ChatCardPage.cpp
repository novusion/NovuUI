#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/ChatCard.h"
#include "novu/ui/Text.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

// A turn's content is the application's, so the preview says something plain
// and lets the shell be what is on show.
QLabel* said(QWidget* parent, const QString& text)
{
    QLabel* line = label(text, parent, TypeRole::Body, Ink::Secondary);
    line->setWordWrap(true);
    return line;
}

ChatCard* turn(QWidget* parent, ChatCard::Author who, const QString& name,
               const QString& model, const QString& when, const QString& text)
{
    auto* card = new ChatCard(who, name, parent);
    card->setModel(model);
    card->setTime(when);
    card->addContent(said(card, text));
    return card;
}

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    {
        Card* card = exampleCard(page, QStringLiteral("User Chat"),
                                 QStringLiteral("A person's turn: a card against the right "
                                                "edge, as wide as what they said."));
        card->body()->addWidget(
            turn(card, ChatCard::Author::User, QStringLiteral("Pavel"), QString(),
                 QStringLiteral("13:46"),
                 QStringLiteral("Apply it and stage nothing automatically — I want to see "
                                "the list first.")));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("Agent Chat"),
                                 QStringLiteral("The agent's turn: the product's mark beside "
                                                "it, the full width of the column, no plate."));
        card->body()->addWidget(
            turn(card, ChatCard::Author::Agent, QStringLiteral("Coding Agent"),
                 QStringLiteral("Claude Opus 5"), QStringLiteral("13:47"),
                 QStringLiteral("Wiring the panel to task_completed now. Staging stays "
                                "manual — the panel lists the paths with a checkbox each, "
                                "and Commit is disabled until at least one is ticked.")));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(
            page, QStringLiteral("An exchange"),
            QStringLiteral("Why the two are shaped differently: alternating turns stay "
                           "skimmable because only one side carries a plate."));
        card->body()->addWidget(
            turn(card, ChatCard::Author::User, QStringLiteral("Pavel"), QString(),
                 QStringLiteral("13:44"), QStringLiteral("Where did the rail width come from?")));
        card->body()->addWidget(
            turn(card, ChatCard::Author::Agent, QStringLiteral("Coding Agent"),
                 QStringLiteral("Claude Opus 5"), QStringLiteral("13:44"),
                 QStringLiteral("From the design tokens — 340px. The port had 304, which is "
                                "why everything inside it looked loose.")));
        card->body()->addWidget(
            turn(card, ChatCard::Author::User, QStringLiteral("Pavel"), QString(),
                 QStringLiteral("13:45"), QStringLiteral("Fix it globally, not per module.")));
        column->addWidget(card);
    }

    note(column, page,
         QStringLiteral("Streaming, tool calls, fenced code and retries are the "
                        "application's and stay there. What repeats across products is "
                        "the shell around a turn, which is this."));

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec chatCardComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("chat-card");
    spec.name = QStringLiteral("Chat Card");
    spec.category = Category::DataDisplay;
    spec.description = QStringLiteral(
        "One turn in a conversation: who spoke, on what model, when, and a body the "
        "application fills. A person's turn is a card against the right edge; an agent's "
        "runs the column width with the product's mark beside it.");
    spec.headerPath = QStringLiteral("include/novu/ui/ChatCard.h");
    spec.sourcePath = QStringLiteral("src/ChatCard.cpp");
    spec.designedFor = {QStringLiteral("novuai")};
    spec.variants = {QStringLiteral("User Chat"), QStringLiteral("Agent Chat")};
    spec.states = {QStringLiteral("Normal")};
    // What someone actually types when they are looking for this and do not
    // know it is called a card.
    spec.keywords = {QStringLiteral("chat"),    QStringLiteral("bubble"),
                     QStringLiteral("bubbles"), QStringLiteral("message"),
                     QStringLiteral("turn"),    QStringLiteral("conversation"),
                     QStringLiteral("agent"),   QStringLiteral("user"),
                     QStringLiteral("transcript")};
    spec.properties = {
        {QStringLiteral("author"), QStringLiteral("enum"),
         {QStringLiteral("User"), QStringLiteral("Agent")}, QStringLiteral("User"),
         QStringLiteral("Decides the whole shape, so it is a constructor argument.")},
        {QStringLiteral("name"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("Who spoke.")},
        {QStringLiteral("model"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("Empty hides the badge — a person's turn has no model.")},
        {QStringLiteral("time"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("Empty hides it rather than leaving a gap.")},
    };
    spec.scenarios = {Scenario::Normal, Scenario::LongText};
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
