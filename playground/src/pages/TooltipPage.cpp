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

    // ── the short form ───────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("The short form"),
                                 QStringLiteral("One string, and the OS tooltip "
                                                "is out of the way."));
        QVBoxLayout* body = card->body();

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Hover these"));
        const struct { const char* caption; const char* tip; } simple[] = {
            {"Refresh", "Re-reads the index from disk."},
            {"Export", "Writes the current selection to CSV."},
            {"Purge", "Removes every archived record older than the retention period. "
                      "This cannot be undone."},
        };
        for (const auto& entry : simple) {
            auto* button = new Button(QString::fromLatin1(entry.caption),
                                      Button::Variant::Secondary, card);
            button->setControlSize(ControlSize::Small);
            Tooltip::install(button, QString::fromLatin1(entry.tip));
            row->addWidget(button);
        }
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Centred over the target, eight pixels clear of it, flipped below "
                            "when there is no room above, clamped to the screen. That geometry "
                            "is NovusionUI's Tooltip, kept unchanged — it was already right. "
                            "The third one wraps at the design's 274px measure rather than "
                            "running off the edge, which is what QToolTip does with a sentence "
                            "that long."));
        column->addWidget(card);
    }

    // ── the full form ────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("The full form"),
                                 QStringLiteral("Five fields, because the design's "
                                                "tip is five things."));
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
            TipContent content;
            content.title = QString::fromLatin1(entry.title);
            content.shortcut = QString::fromLatin1(entry.shortcut);
            content.brief = QString::fromLatin1(entry.brief);
            content.status = QString::fromLatin1(entry.status);
            content.detail = QString::fromLatin1(entry.detail);
            Tooltip::install(button, content);
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

    // ── compact ──────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("About a row"),
                                 QStringLiteral("The compact card, for a list."));
        QVBoxLayout* body = card->body();

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Rows"));
        for (int i = 0; i < 3; ++i) {
            auto* button = new Button(QStringLiteral("Row %1").arg(i + 1),
                                      Button::Variant::Ghost, card);
            button->setControlSize(ControlSize::Small);
            TipContent content;
            content.compact = true;
            content.title = QStringLiteral("orders-2026-%1.csv").arg(i + 1, 2, 10, QLatin1Char('0'));
            content.brief = QStringLiteral("Imported 14 March, 2,481 rows, no errors.");
            Tooltip::install(button, content);
            row->addWidget(button);
        }
        row->addStretch(1);

        note(body, card,
             QStringLiteral("A tip about a row is the same card in a smaller hand. Without it, "
                            "a full-size tip under the pointer in a list covers the rows it is "
                            "about — which is the whole reason the flag exists rather than a "
                            "second tooltip class."));
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
    spec.category = Category::Feedback;
    spec.description = QStringLiteral(
        "The branded hover tip, in two modes. Off it gives the name and the key; on it "
        "explains the control, says what it is doing, and offers the long form.");
    spec.headerPath = QStringLiteral("include/novu/ui/Tooltip.h");
    spec.sourcePath = QStringLiteral("src/Tooltip.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Brief"), QStringLiteral("Full"),
                     QStringLiteral("Compact")};
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
        {QStringLiteral("compact"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("A tip about a row: the same card, one step smaller.")},
    };
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
