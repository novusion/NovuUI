#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/StatusDot.h"
#include "novu/ui/CommandPalette.h"
#include "novu/ui/Popover.h"

#include <QKeySequence>
#include "novu/ui/Fields.h"
#include "novu/ui/Form.h"
#include "novu/ui/GateCard.h"
#include "novu/ui/Separator.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

// ── CommandPalette ───────────────────────────────────────────────────────

QVector<Command> sampleCommands()
{
    const struct {
        const char* name;
        const char* group;
        const char* shortcut;
        Glyph glyph;
    } sample[] = {
        {"New device peer", "Network", "Ctrl+N", Glyph::Plus},
        {"Revoke device peer", "Network", "", Glyph::Trash},
        {"Reconnect", "Network", "Ctrl+R", Glyph::Plug},
        {"Connection settings…", "Network", "", Glyph::Sliders},
        {"New workspace", "File", "Ctrl+Shift+N", Glyph::Folder},
        {"Open workspace…", "File", "Ctrl+O", Glyph::Folder},
        {"Close workspace", "File", "Ctrl+W", Glyph::Close},
        {"Export selection…", "File", "", Glyph::Download},
        {"Toggle sidebar", "View", "Ctrl+B", Glyph::Sidebar},
        {"Full screen", "View", "F11", Glyph::External},
        {"Developer console", "View", "Ctrl+Shift+I", Glyph::Diagnostics},
        {"Tooltip help", "View", "F1", Glyph::HelpTip},
        {"Rebuild the index", "Maintenance", "Ctrl+Shift+R", Glyph::Refresh},
        {"Run diagnostics", "Maintenance", "", Glyph::Diagnostics},
        {"Rotate API keys", "Maintenance", "", Glyph::Keys},
    };
    QVector<Command> commands;
    for (const auto& entry : sample) {
        Command command;
        command.name = QString::fromLatin1(entry.name);
        command.group = QString::fromLatin1(entry.group);
        command.shortcut = QString::fromLatin1(entry.shortcut);
        command.glyph = entry.glyph;
        command.data = command.name;
        commands.append(command);
    }
    return commands;
}

QWidget* buildCommandPalette(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    {
        Card* card = exampleCard(page, QStringLiteral("Type what you want"),
                                 QStringLiteral("Ranked, not merely filtered."));
        QVBoxLayout* body = card->body();

        auto* palette = new CommandPalette(card);
        palette->setCommands(sampleCommands());

        auto* chosen = label(QStringLiteral("—"), card, TypeRole::Mono, Ink::Secondary);
        QObject::connect(palette, &CommandPalette::chosen, chosen,
                         [chosen](const QVariant& data) { chosen->setText(data.toString()); });

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Open it"));
        auto* open = new Button(QStringLiteral("Command palette"), Button::Variant::Primary,
                                card);
        open->setControlSize(ControlSize::Small);
        QObject::connect(open, &Button::clicked, palette,
                         [palette, card] { palette->present(card); });
        row->addWidget(open);
        row->addSpacing(16);
        row->addWidget(chosen);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Open it and type \"ndp\". \"New device peer\" is the top match — "
                            "the initials of a command find it, which is most of why a palette "
                            "is faster than a menu and the thing a substring filter cannot do. "
                            "Then type \"set\" and watch the order: a prefix beats a word start "
                            "beats a scattered subsequence, and a shorter name wins at the same "
                            "rank."));
        note(body, card,
             QStringLiteral("Without a ranking the top result — the one Return runs — is "
                            "whichever command happened to be registered first, which makes "
                            "pressing Return a gamble. That is the change from NovusionUI's "
                            "implementation, which this is otherwise a port of."));
        column->addWidget(card);
    }

    // ── the field that opens it ──────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("The field in the titlebar"),
                                 QStringLiteral("A door, not a field."));
        QVBoxLayout* body = card->body();

        auto* field = new CommandField(
            QStringLiteral("Search this workspace, or run a command"), card);
        field->setShortcut(QKeySequence(QStringLiteral("Ctrl+K")));
        field->setMaximumWidth(460);

        auto* opened = label(QStringLiteral("not opened"), card, TypeRole::Mono,
                             Ink::Muted);
        auto* palette2 = new CommandPalette(card);
        palette2->setCommands(sampleCommands());
        QObject::connect(field, &CommandField::clicked, card, [palette2, opened, card] {
            opened->setText(QStringLiteral("opened"));
            palette2->present(card);
        });

        body->addWidget(field);
        body->addWidget(opened);

        auto* bare = new CommandField(QStringLiteral("Application-owned search shortcut"), card);
        bare->setShortcutHint(QKeySequence(QStringLiteral("Ctrl+F")));
        bare->setMaximumWidth(220);
        body->addWidget(label(QStringLiteral("Without a shortcut"), card,
                              TypeRole::Label, Ink::Muted));
        body->addWidget(bare);

        note(body, card,
             QStringLiteral("Every approved shell draws this same `.omni`, and each was "
                            "building it from a read-only QLineEdit: a caret, a focus ring "
                            "and a selection on a control that accepts no text, and nowhere "
                            "to put the cap, since a line edit's actions are icons only. "
                            "Nothing is typed here — the palette is where typing happens."));
        note(body, card,
             QStringLiteral("The cap and the shortcut are one statement: setShortcut() both "
                            "draws the keys and binds them, so the key that opens the "
                            "palette cannot drift from the one the field advertises."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("The ranking, as data"),
                                 QStringLiteral("Assertable without a pixel."));
        QVBoxLayout* body = card->body();

        // A real palette, filtered from the real ranking - the top three for
        // each needle, read back through the component's own public surface.
        const QVector<Command> commands = sampleCommands();
        auto* probe = new CommandPalette(card);
        probe->setCommands(commands);
        probe->hide();

        for (const char* needle : {"ndp", "set", "wo", "view", "zzz"}) {
            probe->setFilter(QString::fromLatin1(needle));
            const QVector<int> matches = probe->matches();
            QStringList names;
            for (int i = 0; i < qMin(3, matches.size()); ++i) {
                names.append(commands.at(matches.at(i)).name);
            }
            propertyRow(body, card, QStringLiteral("\"%1\"").arg(QLatin1String(needle)),
                        names.isEmpty() ? QStringLiteral("no match")
                                        : names.join(QStringLiteral("  ·  ")),
                        Ink::Secondary, true);
        }

        note(body, card,
             QStringLiteral("matches() and matchScore() are public because the ranking is the "
                            "interesting part of this component, and a test that had to read "
                            "pixels to check it would be checking the font instead."));
        column->addWidget(card);
    }

    // ── the select ───────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("SelectButton"),
                                 QStringLiteral("The control that opens one, "
                                                "showing the choice in force."));
        QVBoxLayout* body = card->body();

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Engine"));
        auto* provider = new SelectButton(card);
        provider->setControlSize(ControlSize::Small);
        provider->setOptions({QStringLiteral("Anthropic"), QStringLiteral("OpenAI"),
                              QStringLiteral("Google"), QStringLiteral("Local runtime")});
        provider->setStatus(StatusDot::Tone::Success, true);

        auto* model = new SelectButton(card);
        model->setControlSize(ControlSize::Small);
        model->setOptions({QStringLiteral("Claude Opus 5"), QStringLiteral("Claude Sonnet 5"),
                           QStringLiteral("GPT-4.1"), QStringLiteral("Gemini 1.5 Pro")});

        auto* agent = new SelectButton(card);
        agent->setControlSize(ControlSize::Small);
        agent->setGlyph(Glyph::Agent);
        agent->setOptions({QStringLiteral("Coding Agent"), QStringLiteral("Analysis Agent"),
                           QStringLiteral("Review Agent")});
        for (SelectButton* one : {provider, model, agent}) {
            row->addWidget(one);
        }
        row->addStretch(1);

        QHBoxLayout* big = exampleRow(body, card, QStringLiteral("Regular size"));
        auto* plain = new SelectButton(card);
        plain->setOptions({QStringLiteral("Today"), QStringLiteral("This week"),
                           QStringLiteral("All time")});
        big->addWidget(plain);
        auto* mono = new SelectButton(card);
        mono->setControlSize(ControlSize::Small);
        mono->setLabelRole(TypeRole::Mono);
        mono->setStatus(StatusDot::Tone::Neutral, false);
        mono->setOptions({QStringLiteral("qwen2-1_5b-instruct"),
                          QStringLiteral("mistral-7b-instruct")});
        big->addWidget(mono);
        big->addStretch(1);

        auto* said = label(QStringLiteral("nothing chosen yet"), card, TypeRole::Mono,
                           Ink::Muted);
        for (SelectButton* one : {provider, model, agent, plain, mono}) {
            QObject::connect(one, &SelectButton::currentChanged, said,
                             [said](const QString& key) { said->setText(key); });
        }
        body->addWidget(said);

        note(body, card,
             QStringLiteral("Every shell draws this and each was reaching for a "
                            "QComboBox: a system popup list, a focus frame and a "
                            "line-edit shape no stylesheet makes into a pill, and a "
                            "list that cannot carry the groups, separators, meta "
                            "column and checked accent a Popover already draws."));
        note(body, card,
             QStringLiteral("Setting the current option is not choosing it: "
                            "currentChanged fires for the menu, not for "
                            "setCurrentKey(), so restoring a saved choice at startup "
                            "does not read as the user making one."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

// ── GateCard ─────────────────────────────────────────────────────────────

QWidget* buildGateCard(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    {
        Card* card = exampleCard(page, QStringLiteral("Signing in"),
                                 QStringLiteral("The shape is shared; the fields "
                                                "are not."));
        QVBoxLayout* body = card->body();

        auto* gate = new GateCard(QStringLiteral("Sign in to NovuAdmin"), card);
        gate->setGlyph(Glyph::Shield);
        gate->setSubtitle(QStringLiteral("Administrative access is audited. Every action "
                                         "you take is recorded against this account."));
        gate->setHost(QStringLiteral("admin.novusion.net"));
        gate->setFootnote(QStringLiteral("v0.67.11 · production"));

        auto* email = lineEdit(QStringLiteral("you@novusion.net"), gate->formWidget());
        gate->form()->addWidget(email);
        auto* password = lineEdit(QStringLiteral("Password"), gate->formWidget());
        password->setEchoMode(QLineEdit::Password);
        gate->form()->addWidget(password);

        // Into the action row rather than straight onto the form: left under
        // the password box the primary action looked unbalanced, and where a
        // form's action sits is a property of the card, not of this page.
        auto* submit = new Button(QStringLiteral("Sign in"), Button::Variant::Primary,
                                  gate->formWidget());
        gate->actions()->addWidget(submit);

        QObject::connect(submit, &Button::clicked, gate, [gate] {
            gate->setError(QString());
            gate->setBusy(true);
            QTimer::singleShot(1200, gate, [gate] {
                gate->setError(QObject::tr("That email and password do not match an "
                                           "account with administrative access."));
            });
        });

        QHBoxLayout* holder = exampleRow(body, card, QStringLiteral("The card"));
        holder->addWidget(gate, 0, Qt::AlignTop);
        holder->addStretch(1);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("States"));
        auto* clear = new Button(QStringLiteral("Clear"), Button::Variant::Ghost, card);
        clear->setControlSize(ControlSize::Small);
        QObject::connect(clear, &Button::clicked, gate, [gate] {
            gate->setError(QString());
            gate->setBusy(false);
        });
        row->addWidget(clear);

        auto* staging = new Button(QStringLiteral("Point it at staging"),
                                   Button::Variant::Secondary, card);
        staging->setControlSize(ControlSize::Small);
        QObject::connect(staging, &Button::clicked, gate, [gate] {
            gate->setHost(QStringLiteral("admin.staging.novusion.net"));
            gate->setFootnote(QObject::tr("v0.68.0-rc1 · staging · not production data"));
        });
        row->addWidget(staging);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Press Sign in. The whole form is disabled while the request is "
                            "out — not only the button — because a second press creates a "
                            "second session; and the failure clears busy, because the failure "
                            "is the answer to the attempt."));
        note(body, card,
             QStringLiteral("The host in the footer is a security property rather than a "
                            "layout choice: the one thing a person must be able to check "
                            "before typing a credential is where it is going. It is set in the "
                            "monospace face for the same reason — a hostname in a proportional "
                            "font is one nobody reads character by character. Press \"Point it "
                            "at staging\"."));
        note(body, card,
             QStringLiteral("There is no field in this component. NovuAdmin signs in with an "
                            "email and a password, NovuSpace with a workspace and a key, and a "
                            "future one will use a device code — a component with fields would "
                            "have to grow one every time a product grows an authentication "
                            "method. form() is a layout the caller fills."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec commandPaletteComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("command-palette");
    spec.name = QStringLiteral("CommandPalette");
    spec.category = Category::Navigation;
    spec.description = QStringLiteral(
        "Every action the application has, reached by name. Subsequence matching with a "
        "ranking, so the command Return runs is the one you meant.");
    spec.headerPath = QStringLiteral("include/novu/ui/CommandPalette.h");
    spec.sourcePath = QStringLiteral("src/CommandPalette.cpp");
    spec.designedFor = {QStringLiteral("novuexplorer"), QStringLiteral("novuvoice")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Grouped"), QStringLiteral("Ranked")};
    spec.states = {QStringLiteral("Empty filter"), QStringLiteral("Filtered"),
                   QStringLiteral("No match")};
    spec.keywords = {QStringLiteral("palette"), QStringLiteral("pcmd"),
                     QStringLiteral("commands"), QStringLiteral("ctrl+k"),
                     QStringLiteral("quick open"), QStringLiteral("fuzzy"),
                     QStringLiteral("search")};
    spec.properties = {
        {QStringLiteral("commands"), QStringLiteral("QVector<Command>"), {},
         QStringLiteral("{}"),
         QStringLiteral("Given, not discovered: the application knows what it can do.")},
        {QStringLiteral("filter"), QStringLiteral("QString"), {}, QStringLiteral(""),
         QStringLiteral("Matches a subsequence of the name, or the group.")},
    };
    spec.build = buildCommandPalette;
    return spec;
}

ComponentSpec gateCardComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("gate-card");
    spec.name = QStringLiteral("GateCard");
    spec.category = Category::Foundation;
    spec.description = QStringLiteral(
        "The card a person signs in on: a mark, a title, the form the caller fills, and a "
        "footer naming the host the credential is about to go to.");
    spec.headerPath = QStringLiteral("include/novu/ui/GateCard.h");
    spec.sourcePath = QStringLiteral("src/GateCard.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("With a footnote"), QStringLiteral("Host only")};
    spec.states = {QStringLiteral("Idle"), QStringLiteral("Busy"),
                   QStringLiteral("Failed")};
    spec.keywords = {QStringLiteral("login"), QStringLiteral("gate"),
                     QStringLiteral("sign in"), QStringLiteral("auth"),
                     QStringLiteral("credential"), QStringLiteral("login-card")};
    spec.properties = {
        {QStringLiteral("host"), QStringLiteral("QString"), {}, QStringLiteral(""),
         QStringLiteral("Where the credential is going. A security property.")},
        {QStringLiteral("error"), QStringLiteral("QString"), {}, QStringLiteral(""),
         QStringLiteral("Above the form. Setting one clears busy.")},
        {QStringLiteral("busy"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("Disables the whole form, not only the button.")},
    };
    spec.build = buildGateCard;
    return spec;
}

} // namespace novu::playground::pages
