#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/IconButton.h"
#include "novu/ui/Separator.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/Toggle.h"
#include "novu/ui/Tooltip.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    // ── LABEL ─────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("LABEL"),
                                 QStringLiteral("Identification only — a tab, a path, a "
                                                "row. Nothing is promised, because "
                                                "these are not activated."));
        QVBoxLayout* body = card->body();

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Hover these"));
        const struct { const char* caption; const char* name; const char* status; } labels[] = {
            {"General", "General", "2 categories"},
            {"Novusion", "Novusion", "20 categories"},
            {"Archive 2024", "Archive 2024", ""},
        };
        for (const auto& entry : labels) {
            auto* button = new Button(QString::fromLatin1(entry.caption),
                                      Button::Variant::Ghost, card);
            button->setControlSize(ControlSize::Small);
            Tooltip::install(button, TipContent::label(QString::fromLatin1(entry.name),
                                                      QString::fromLatin1(entry.status)));
            row->addWidget(button);
        }
        row->addStretch(1);

        note(body, card,
             QStringLiteral("The one-string install() is this level: the string is the "
                            "control's name. It used to fill the description instead, which "
                            "produced a card with no title and matched none of the three "
                            "levels."));
        column->addWidget(card);
    }

    // ── ACTION ──────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("ACTION"),
                                 QStringLiteral("The default for anything actionable. The "
                                                "sentence says what happens when you "
                                                "activate it."));
        QVBoxLayout* body = card->body();

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Hover these"));
        const struct {
            const char* caption; const char* title; const char* brief; const char* key;
        } actions[] = {
            {"New workspace", "New workspace",
             "Create a workspace for organizing chats, tasks and context.", "Ctrl N"},
            {"Send / Share", "Send / Share",
             "Send files, links or content to another destination.", ""},
            {"Console", "Console",
             "Open the runtime console and inspect current activity.", "F12"},
        };
        for (const auto& entry : actions) {
            auto* button = new Button(QString::fromLatin1(entry.caption),
                                      Button::Variant::Secondary, card);
            button->setControlSize(ControlSize::Small);
            Tooltip::install(button, TipContent::action(QString::fromLatin1(entry.title),
                                                       QString::fromLatin1(entry.brief),
                                                       QString::fromLatin1(entry.key)));
            row->addWidget(button);
        }
        row->addStretch(1);

        note(body, card,
             QStringLiteral("The sentence must explain the result, never repeat the label. "
                            "\"Create a workspace for organizing chats, tasks and context\" "
                            "— not \"New workspace\" again. A brief that restates the "
                            "title is the one way this level fails."));
        column->addWidget(card);
    }

    // ── the full form ────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("EXTENDED"),
                                 QStringLiteral("Exceptional, not the default. Only where "
                                                "the extra paragraph earns its place."));
        QVBoxLayout* body = card->body();

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Toolbar"));

        const struct {
            Glyph glyph;
            const char* title;
            const char* shortcut;
            const char* brief;
            const char* status;
            const char* detail;
        } rich[] = {
            {Glyph::Refresh, "Full rebuild", "Ctrl+Shift+R",
             "Rebuilds the index and the knowledge base from scratch.",
             "last run 4 minutes ago",
             "Every document in every workspace is re-read and re-embedded. On a large "
             "workspace this takes several minutes and the index is unavailable while it "
             "runs, so it is not what you want for a single changed file."},
            {Glyph::Plug, "Reconnect", "Ctrl+R",
             "Drops the transport and dials the hub again.",
             "connected · 52 ms",
             "Open sessions are resumed rather than recreated, so nothing in flight is "
             "lost. A reconnect that fails falls back to the last known endpoint."},
            {Glyph::Trash, "Delete workspace", "",
             "Removes the workspace and everything indexed from it.",
             "3 workspaces",
             "The files themselves are untouched — only the index, the embeddings and the "
             "conversation history for this workspace are removed. There is no undo."},
        };
        for (const auto& entry : rich) {
            auto* button = new IconButton(entry.glyph, card);
            Tooltip::install(button, TipContent::extended(
                QString::fromLatin1(entry.title), QString::fromLatin1(entry.brief),
                QString::fromLatin1(entry.detail), QString::fromLatin1(entry.shortcut),
                QString::fromLatin1(entry.status)));
            row->addWidget(button);
        }
        row->addStretch(1);

        separator(body, card);

        QHBoxLayout* modeRow = exampleRow(body, card, QStringLiteral("Tooltip help"));
        auto* toggle = new Toggle(card);
        toggle->setChecked(Tooltip::helpMode());
        auto* state = label(Tooltip::helpMode() ? QStringLiteral("on")
                                                : QStringLiteral("off"),
                            card, TypeRole::Mono, Ink::Secondary);
        QObject::connect(toggle, &Toggle::toggled, state, [state](bool on) {
            Tooltip::setHelpMode(on);
            state->setText(on ? QStringLiteral("on") : QStringLiteral("off"));
        });
        modeRow->addWidget(toggle);
        modeRow->addWidget(state);
        modeRow->addStretch(1);

        note(body, card,
             QStringLiteral("Switch help on and hover the same three buttons. Off, a tip is "
                            "the name and the key. On, it also explains the control and says "
                            "what it is currently doing — and if the pointer stays, the long "
                            "form arrives under a rule. Help is a mode rather than an overlay "
                            "on purpose: nothing is highlighted in advance and nothing is "
                            "covered, so the explanation appears where you were already "
                            "looking."));
        column->addWidget(card);
    }

    // ── what this replaced ───────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("What this replaced"),
                                 QStringLiteral("Three implementations of one "
                                                "mechanism."));
        QVBoxLayout* body = card->body();

        note(body, card,
             QStringLiteral("QToolTip — an OS tooltip: its own delay, its own type, a square "
                            "surface, unstyleable. NovusionUI's Tooltip — the branded "
                            "replacement, which got the positioning right. NovusionUI's "
                            "HelpTip — which got the content model right. This is the last two "
                            "merged, with one change: the colours are tokens rather than the "
                            "Token::Panel / Token::Hair2 constants either original read, so a "
                            "tip inside a NovuTrade scope is a NovuTrade tip. Neither original "
                            "could be."));
        note(body, card,
             QStringLiteral("The tip resolves its palette against the widget it is about, not "
                            "against itself. A tooltip is a top-level window and therefore "
                            "inside no ThemeScope — asking for its own profile would give "
                            "every tip in the process the default product's colours, which is "
                            "exactly the defect the token layer exists to prevent. Switch the "
                            "gallery's theme and hover again."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec tooltipComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("tooltip");
    spec.name = QStringLiteral("Tooltip");
    spec.category = Category::Help;
    spec.description = QStringLiteral(
        "The branded hover tip, in two modes. Off it gives the name and the key; on it "
        "explains the control, says what it is doing, and offers the long form.");
    spec.headerPath = QStringLiteral("include/novu/ui/Tooltip.h");
    spec.sourcePath = QStringLiteral("src/Tooltip.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("LABEL"), QStringLiteral("ACTION"),
                     QStringLiteral("EXTENDED")};
    spec.states = {QStringLiteral("Help off"), QStringLiteral("Help on"),
                   QStringLiteral("Detail shown")};
    spec.keywords = {QStringLiteral("tip"), QStringLiteral("hint"),
                     QStringLiteral("help"), QStringLiteral("hover"),
                     QStringLiteral("qtooltip"), QStringLiteral("helptip"),
                     QStringLiteral("shortcut")};
    spec.properties = {
        {QStringLiteral("title"), QStringLiteral("QString"), {}, QStringLiteral(""),
         QStringLiteral("The control's name — .tip-t.")},
        {QStringLiteral("shortcut"), QStringLiteral("QString"), {}, QStringLiteral(""),
         QStringLiteral("The key that reaches it, in a chip — .tip-t .sc.")},
        {QStringLiteral("brief"), QStringLiteral("QString"), {}, QStringLiteral(""),
         QStringLiteral("One line on what it does — .tip-s.")},
        {QStringLiteral("status"), QStringLiteral("QString"), {}, QStringLiteral(""),
         QStringLiteral("What it is doing now, in the accent — .tip-st.")},
        {QStringLiteral("detail"), QStringLiteral("QString"), {}, QStringLiteral(""),
         QStringLiteral("The long form, in help mode after a second delay — .tip-d.")},
    };
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
