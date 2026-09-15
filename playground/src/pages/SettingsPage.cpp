#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/Field.h"
#include "novu/ui/Fields.h"
#include "novu/ui/NavList.h"
#include "novu/ui/PageHeader.h"
#include "novu/ui/Segmented.h"
#include "novu/ui/SettingRow.h"
#include "novu/ui/Settings.h"
#include "novu/ui/Slider.h"
#include "novu/ui/Swatch.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/Toggle.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

// Every page below is built the way an application would build it: out of the
// library's own controls, handed to the shell, with no settings model anywhere
// near the shell itself.
void fillGeneral(SettingsPage* page, QWidget* owner)
{
    SettingsSection* startup = page->addSection(
        QStringLiteral("Startup"),
        QStringLiteral("What happens when you sign in to this machine."));
    startup->addRow(QStringLiteral("Start with the system"),
                    QStringLiteral("Waits in the tray until you need it."))
        ->setControl(new Toggle(owner));
    startup->addRow(QStringLiteral("Restore the last workspace"),
                    QStringLiteral("Reopens the modules and panels you left open."))
        ->setControl([owner] { auto* t = new Toggle(owner); t->setChecked(true); return t; }());
    startup->addRow(QStringLiteral("Check for updates"))
        ->setControl(new Segmented({QStringLiteral("Never"), QStringLiteral("Weekly"),
                                    QStringLiteral("Daily")}, owner, 1));

    SettingsSection* appearance = page->addSection(QStringLiteral("Appearance"));
    appearance->addRow(QStringLiteral("Interface scale"),
                       QStringLiteral("Everything in the window, including the title bar."))
        ->setControl([owner] {
            auto* s = new Slider(80, 150, owner);
            s->setValue(100);
            s->setFixedWidth(200);
            s->setFormatter([](int v) { return QStringLiteral("%1 %").arg(v); });
            return s;
        }());
    appearance->addRow(QStringLiteral("Accent"),
                       QStringLiteral("Marks the active module and the current row."))
        ->setControl([owner] {
            auto* sw = new Swatch(owner);
            sw->setColors(tagPalette(owner).mid(0, 6), false, true);
            sw->setCurrent(tagPalette(owner).value(0));
            return sw;
        }());
}

void fillAccount(SettingsPage* page, QWidget* owner)
{
    SettingsSection* who = page->addSection(
        QStringLiteral("Signed in"),
        QStringLiteral("This account is what workspaces and vault entries belong to."));
    who->addRow(QStringLiteral("Account"))->setValue(QStringLiteral("cg.p.dostal@gmail.com"));
    who->addRow(QStringLiteral("Plan"))->setValue(QStringLiteral("Workstation · seat 1 of 3"));
    SettingRow* token = who->addRow(QStringLiteral("Session token"),
                                    QStringLiteral("Rotated every thirty days."));
    token->setValue(QStringLiteral("nv_7f3c1e9ab84d2f60c5a17e93"));
    token->setMasked(true);

    SettingsSection* danger = page->addSection(
        QStringLiteral("Leaving"),
        QStringLiteral("Both of these take effect on this machine only."));
    danger->addRow(QStringLiteral("Sign out"),
                   QStringLiteral("Keeps everything on disk; asks for the password again."))
        ->setControl(secondaryButton(QStringLiteral("Sign out"), owner));
    danger->addRow(QStringLiteral("Remove local data"),
                   QStringLiteral("Deletes cached workspaces, thumbnails and the vault "
                                  "index. Nothing on the server is touched."))
        ->setControl(dangerButton(QStringLiteral("Remove…"), owner));
}

void fillConnections(SettingsPage* page, QWidget* owner)
{
    SettingsSection* endpoint = page->addSection(
        QStringLiteral("Server"),
        QStringLiteral("Where this machine synchronises to."));

    auto* host = new Field(QStringLiteral("Host"),
                           lineEdit(QStringLiteral("hub.novusion.io"), owner, 0,
                                    QStringLiteral("hub.novusion.io")),
                           QStringLiteral("A host name or an address, without a scheme."),
                           owner);
    host->setRequired(true);
    endpoint->addWidget(host);

    auto* port = new Field(QStringLiteral("Port"),
                           lineEdit(QString(), owner, 0, QStringLiteral("0")), owner);
    port->setRequired(true);
    port->setError(QStringLiteral("Zero is not a port. Leave it blank for the default."));
    endpoint->addWidget(port);

    SettingsSection* behaviour = page->addSection(QStringLiteral("Behaviour"));
    behaviour->addRow(QStringLiteral("Reconnect automatically"))
        ->setControl([owner] { auto* t = new Toggle(owner); t->setChecked(true); return t; }());
    behaviour->addRow(QStringLiteral("Retry limit"))
        ->setControl([owner] {
            auto* s = new Slider(1, 20, owner);
            s->setValue(5);
            s->setFixedWidth(200);
            return s;
        }());
}

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    {
        Card* card = exampleCard(page, QStringLiteral("The shell"),
                                 QStringLiteral("Navigation, page host, headings, grouping, "
                                                "spacing — and no settings."));
        QVBoxLayout* body = card->body();

        auto* shell = new SettingsShell(card);
        shell->setSearchEnabled(true);
        shell->setMinimumHeight(460);

        shell->addGroup(QStringLiteral("This machine"));
        fillGeneral(shell->addPage(QStringLiteral("general"), QStringLiteral("General"),
                                   Glyph::Settings,
                                   QStringLiteral("How NovuSpace behaves on this computer.")),
                    card);
        fillConnections(shell->addPage(QStringLiteral("connections"),
                                       QStringLiteral("Connections"), Glyph::Plug,
                                       QStringLiteral("Where this machine synchronises to.")),
                        card);
        shell->addPage(QStringLiteral("shortcuts"), QStringLiteral("Shortcuts"), Glyph::Keys,
                       QStringLiteral("Every gesture, and what it is bound to."));

        shell->addGroup(QStringLiteral("Account"));
        fillAccount(shell->addPage(QStringLiteral("account"), QStringLiteral("Account"),
                                   Glyph::Users,
                                   QStringLiteral("Who this machine is signed in as.")),
                    card);
        shell->addPage(QStringLiteral("privacy"), QStringLiteral("Privacy"), Glyph::Shield,
                       QStringLiteral("What leaves this machine, and what never does."));

        shell->addGroup(QStringLiteral("Advanced"));
        shell->addPage(QStringLiteral("diagnostics"), QStringLiteral("Diagnostics"),
                       Glyph::Diagnostics);
        shell->addPage(QStringLiteral("experiments"), QStringLiteral("Experiments"),
                       Glyph::Zap);

        body->addWidget(shell);

        auto* showing = label(QStringLiteral("showing: general"), card, TypeRole::Mono,
                              Ink::Muted);
        QObject::connect(shell, &SettingsShell::currentChanged, showing,
                         [showing](const QString& key) {
                             showing->setText(QStringLiteral("showing: %1").arg(key));
                         });
        body->addSpacing(10);
        body->addWidget(showing);

        note(body, card,
             QStringLiteral("Click through the navigation; type in the search box to filter "
                            "it. The shell owns the shape and which page is showing — a pure "
                            "view state nothing else in the window has an opinion about. It "
                            "owns no setting, reads nothing, writes nothing and validates "
                            "nothing: the pages above are built by this page out of "
                            "SettingRow, Field, Toggle, Slider, Segmented and Swatch, exactly "
                            "as an application would build them."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("A section on its own"),
                                 QStringLiteral("The unit every page is made of."));
        QVBoxLayout* body = card->body();

        auto* section = new SettingsSection(
            QStringLiteral("Notifications"),
            QStringLiteral("Only the ones you ask for. Nothing here is on by default."),
            card);
        section->addRow(QStringLiteral("A backtest finishes"))->setControl(new Toggle(card));
        section->addRow(QStringLiteral("A bot stops on its own"),
                        QStringLiteral("Includes an error, a limit and a manual stop."))
            ->setControl([card] { auto* t = new Toggle(card); t->setChecked(true); return t; }());
        section->addRow(QStringLiteral("A transfer needs a decision"))
            ->setControl(new Toggle(card));
        body->addWidget(section);

        note(body, card,
             QStringLiteral("The heading and its sentence are painted into the section's top "
                            "margin rather than held in two QLabels — twenty sections in a "
                            "settings window is forty labels that nobody selects. The divider "
                            "between rows belongs to the section, because a caller adding "
                            "rows cannot know which one will end up last."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec settingsShellComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("settings-shell");
    spec.name = QStringLiteral("SettingsShell");
    spec.category = Category::Navigation;
    spec.description = QStringLiteral(
        "The settings window as a shape: navigation, page host, headings, grouping and "
        "spacing. Owns no setting, no value and no persistence.");
    spec.headerPath = QStringLiteral("include/novu/ui/Settings.h");
    spec.sourcePath = QStringLiteral("src/Settings.cpp");
    spec.designedFor = {QStringLiteral("novuai"), QStringLiteral("novuexplorer"),
                        QStringLiteral("novuspace"), QStringLiteral("novuadmin")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("With search"), QStringLiteral("Grouped"),
                     QStringLiteral("Section only")};
    spec.states = {QStringLiteral("Normal"), QStringLiteral("Filtered"),
                   QStringLiteral("Invalid field")};
    spec.keywords = {QStringLiteral("settings"), QStringLiteral("preferences"),
                     QStringLiteral("options"), QStringLiteral("configuration"),
                     QStringLiteral("shell"), QStringLiteral("section")};
    spec.properties = {
        {QStringLiteral("currentKey"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("Which page is showing. View state, so the shell keeps it.")},
        {QStringLiteral("searchEnabled"), QStringLiteral("bool"), {},
         QStringLiteral("false"),
         QStringLiteral("A filter over the navigation by page title.")},
        {QStringLiteral("navigationWidth"), QStringLiteral("int"), {},
         QStringLiteral("200"), QStringLiteral("One number, so nineteen pages agree.")},
    };
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
