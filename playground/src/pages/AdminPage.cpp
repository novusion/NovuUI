#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Badge.h"
#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/Drawer.h"
#include "novu/ui/Feedback.h"
#include "novu/ui/Fields.h"
#include "novu/ui/Form.h"
#include "novu/ui/Separator.h"
#include "novu/ui/SettingRow.h"
#include "novu/ui/TableView.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/Toast.h"
#include "novu/ui/WindowShell.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

// ── the drawer ───────────────────────────────────────────────────────────

QWidget* buildDrawer(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    {
        Card* card = exampleCard(page, QStringLiteral("Detail, over the list"),
                                 QStringLiteral("The table keeps its filters and "
                                                "its scroll position."));
        QVBoxLayout* body = card->body();

        auto* host = new QWidget(card);
        host->setMinimumSize(640, 380);
        host->setAutoFillBackground(true);
        QPalette ground = host->palette();
        ground.setColor(QPalette::Window, themeFor(card).colors.panelBg);
        host->setPalette(ground);

        auto* hostColumn = new QVBoxLayout(host);
        hostColumn->setContentsMargins(14, 14, 14, 14);
        hostColumn->setSpacing(10);

        auto* table = new TableView(host);
        table->setColumns({{QStringLiteral("Application"), TableView::ColumnRole::Text},
                           {QStringLiteral("Version"), TableView::ColumnRole::Text},
                           {QStringLiteral("Status"), TableView::ColumnRole::Content}});
        const struct { const char* name; const char* version; Badge::Tone tone;
                       const char* status; } rows[] = {
            {"NovuSpace", "0.67.11", Badge::Tone::Success, "running"},
            {"NovuAI", "0.41.2", Badge::Tone::Success, "running"},
            {"NovuExplorer", "0.98.0", Badge::Tone::Warning, "degraded"},
            {"NovuHub", "0.12.4", Badge::Tone::Danger, "stopped"},
        };
        table->setRowCount(4);
        for (int i = 0; i < 4; ++i) {
            table->setText(i, 0, QString::fromLatin1(rows[i].name));
            table->setText(i, 1, QString::fromLatin1(rows[i].version));
            table->setWidget(i, 2, new Badge(QString::fromLatin1(rows[i].status),
                                             rows[i].tone, table));
        }
        hostColumn->addWidget(table);

        auto* drawer = new Drawer(host);
        drawer->setTitle(QStringLiteral("NovuExplorer"));
        drawer->setSubtitle(QStringLiteral("0.98.0 · eu-central · 4 runtimes"));
        drawer->setGlyph(Glyph::Folder);

        auto* restart = new Button(QStringLiteral("Restart"), Button::Variant::Secondary,
                                   drawer->actionsWidget());
        restart->setControlSize(ControlSize::Small);
        drawer->actions()->addWidget(restart);
        auto* logs = new Button(QStringLiteral("Logs"), Button::Variant::Secondary,
                                drawer->actionsWidget());
        logs->setControlSize(ControlSize::Small);
        drawer->actions()->addWidget(logs);
        drawer->actions()->addStretch(1);
        auto* remove = new Button(QStringLiteral("Remove"), Button::Variant::Danger,
                                  drawer->actionsWidget());
        remove->setControlSize(ControlSize::Small);
        drawer->actions()->addWidget(remove);

        QVBoxLayout* detail = drawer->body();
        for (const char* section : {"Versions", "Modules", "Runtimes"}) {
            detail->addWidget(label(QString::fromLatin1(section).toUpper(),
                                    drawer->bodyWidget(), TypeRole::Eyebrow, Ink::Muted));
            for (int i = 1; i <= 3; ++i) {
                propertyRow(detail, drawer->bodyWidget(),
                            QStringLiteral("%1 %2").arg(QString::fromLatin1(section)).arg(i),
                            QStringLiteral("0.98.%1").arg(i), Ink::Secondary, true);
            }
            detail->addSpacing(10);
        }
        detail->addStretch(1);

        body->addWidget(host);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Open it"));
        auto* open = new Button(QStringLiteral("Show detail"), Button::Variant::Primary,
                                card);
        open->setControlSize(ControlSize::Small);
        QObject::connect(open, &Button::clicked, drawer, [drawer] { drawer->open(); });
        row->addWidget(open);

        auto* leading = new Button(QStringLiteral("From the left"), Button::Variant::Ghost,
                                   card);
        leading->setControlSize(ControlSize::Small);
        QObject::connect(leading, &Button::clicked, drawer, [drawer] {
            drawer->setEdge(drawer->edge() == Drawer::Edge::Trailing
                                ? Drawer::Edge::Leading
                                : Drawer::Edge::Trailing);
            drawer->open();
        });
        row->addWidget(leading);

        auto* sticky = new Button(QStringLiteral("Guard the scrim"),
                                  Button::Variant::Secondary, card);
        sticky->setControlSize(ControlSize::Small);
        QObject::connect(sticky, &Button::clicked, drawer, [drawer, sticky] {
            const bool closes = !drawer->closesOnScrimClick();
            drawer->setClosesOnScrimClick(closes);
            sticky->setText(closes ? QObject::tr("Guard the scrim")
                                   : QObject::tr("Release the scrim"));
            drawer->open();
        });
        row->addWidget(sticky);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("It opens over the list rather than navigating away, so the table "
                            "keeps its filters and its scroll position. That is the whole "
                            "point, and it is the thing a web admin loses on every "
                            "drill-down."));
        note(body, card,
             QStringLiteral("Click the dimmed area, or press Escape. \"Guard the scrim\" turns "
                            "the outside click off, for a drawer holding an edit in progress "
                            "where a stray click would lose it — the scrim still swallows the "
                            "click, so the list behind cannot be hit by accident either way."));
        note(body, card,
             QStringLiteral("The scrim is what separates this from a FloatingPanel. A drawer "
                            "is about one row of the list behind it, and dimming that list is "
                            "how it says so; a floating panel is a detached view and leaves "
                            "the application working underneath."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

// ── submitting ───────────────────────────────────────────────────────────

QWidget* buildFormSubmission(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    {
        Card* card = exampleCard(page, QStringLiteral("Between the press and the answer"),
                                 QStringLiteral("The state seventeen forms each "
                                                "wrote by hand."));
        QVBoxLayout* body = card->body();

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Try it"));

        const struct { const char* caption; bool fails; } runs[] = {
            {"Submit, and succeed", false},
            {"Submit, and fail", true},
        };
        for (const auto& run : runs) {
            auto* button = new Button(QString::fromLatin1(run.caption),
                                      run.fails ? Button::Variant::Secondary
                                                : Button::Variant::Primary,
                                      card);
            button->setControlSize(ControlSize::Small);
            const bool fails = run.fails;

            QObject::connect(button, &Button::clicked, card, [card, fails] {
                auto* dialog = new Dialog(QObject::tr("New device peer"), card->window());
                const ThemeProfile& profile = themeFor(dialog);
                const int padding = profile.space(SpacingRole::CardPadding);
                dialog->contentLayout()->setContentsMargins(padding, padding, padding,
                                                            padding);
                dialog->contentLayout()->setSpacing(profile.space(SpacingRole::SectionGap));

                auto* name = lineEdit(QObject::tr("Device name"), dialog->content());
                name->setText(QObject::tr("pdost-laptop"));
                formRow(dialog->contentLayout(), dialog->content(), QObject::tr("Name"), name);

                auto* role = new ComboBox(dialog->content());
                role->addItems({QObject::tr("HUB"), QObject::tr("SERVICE")});
                formRow(dialog->contentLayout(), dialog->content(), QObject::tr("Role"), role);
                dialog->contentLayout()->addStretch(1);

                dialog->setHeading(QObject::tr("New device peer"),
                                   QObject::tr("The device is issued a WireGuard identity "
                                               "and an address on the platform network."),
                                   Dialog::Tone::Accent);

                Button* cancel = secondaryButton(QObject::tr("Cancel"), dialog->content());
                Button* create = primaryButton(QObject::tr("Create"), dialog->content());
                dialog->buttonRow()->addWidget(cancel);
                dialog->buttonRow()->addWidget(create);
                create->setDefault(true);
                QObject::connect(cancel, &QAbstractButton::clicked, dialog, &QDialog::reject);

                QObject::connect(create, &QAbstractButton::clicked, dialog, [dialog, fails] {
                    dialog->setBusy(true, QObject::tr("Provisioning…"));
                    QTimer::singleShot(1400, dialog, [dialog, fails] {
                        if (fails) {
                            dialog->showFormError(
                                QObject::tr("The address pool for eu-central is exhausted. "
                                            "Free a revoked peer or widen the pool."));
                        } else {
                            dialog->setBusy(false);
                            dialog->accept();
                        }
                    });
                });

                dialog->setModal(true);
                dialog->shell()->setContentSize(QSize(480, 300));
                const bool created = dialog->exec() == QDialog::Accepted;
                dialog->deleteLater();
                if (created) {
                    Toast::post(card->window(), QObject::tr("Peer provisioned"),
                                Toast::Tone::Success);
                }
            });
            row->addWidget(button);
        }
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Press Create and watch the footer. Every button is disabled — not "
                            "only the primary — because a Cancel that still works while a "
                            "request is in flight leaves the caller holding a reply for a "
                            "dialog that has gone. Escape stops working for the same reason: a "
                            "request already sent cannot be cancelled by closing the window "
                            "that sent it."));
        note(body, card,
             QStringLiteral("The spinner goes on the left of the footer, so the buttons stay "
                            "where they were. One that pushed them sideways is one the user's "
                            "hand has to chase."));
        note(body, card,
             QStringLiteral("On the failing run the error appears above the footer, which is "
                            "where the eye goes after pressing a button that did not do what "
                            "it said — and busy clears itself, because a dialog cannot be both "
                            "reporting a failure and still working. NovuAdmin wrote this by "
                            "hand seventeen times; the halves that were forgotten are the ones "
                            "that create two records on a slow network."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("Where validation goes"),
                                 QStringLiteral("Three places, and they mean "
                                                "different things."));
        QVBoxLayout* body = card->body();
        body->setSpacing(0);

        auto* field = new SettingRow(
            QStringLiteral("Address pool"),
            QStringLiteral("The CIDR range new peers are allocated from."), card);
        auto* input = lineEdit(QStringLiteral("10.42.0.0/16"), field, 200);
        input->setText(QStringLiteral("10.42.0.0/33"));
        field->setControl(input);
        field->setControlColumn(210);
        field->setError(QStringLiteral("A prefix length above 32 is not a v4 network."));
        field->setDivider(false);
        body->addWidget(field);

        body->addSpacing(12);
        body->addWidget(new InfoBanner(
            QStringLiteral("Two peers still hold addresses outside the pool. They keep "
                           "working; new allocations come from the range above."),
            Badge::Tone::Warning, card));

        note(body, card,
             QStringLiteral("SettingRow's error is about one field and sits under it. "
                            "InfoBanner is about the panel and sits in it. The dialog's form "
                            "error is about the submission and sits above the button that "
                            "caused it. Putting any of them in another's place is how a user "
                            "ends up looking for the wrong thing."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec drawerComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("drawer");
    spec.name = QStringLiteral("Drawer");
    spec.category = Category::Navigation;
    spec.description = QStringLiteral(
        "The detail view that opens over a list instead of navigating away from it, so "
        "the table keeps its filters and its scroll position.");
    spec.headerPath = QStringLiteral("include/novu/ui/Drawer.h");
    spec.sourcePath = QStringLiteral("src/Drawer.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Trailing"), QStringLiteral("Leading"),
                     QStringLiteral("With actions")};
    spec.states = {QStringLiteral("Open"), QStringLiteral("Closed"),
                   QStringLiteral("Guarded")};
    spec.keywords = {QStringLiteral("drawer"), QStringLiteral("scrim"),
                     QStringLiteral("detail"), QStringLiteral("panel"),
                     QStringLiteral("drill-down"), QStringLiteral("side"),
                     QStringLiteral("sheet")};
    spec.properties = {
        {QStringLiteral("edge"), QStringLiteral("enum"),
         {QStringLiteral("Leading"), QStringLiteral("Trailing")},
         QStringLiteral("Trailing"), QStringLiteral("Which side it is anchored to.")},
        {QStringLiteral("maximumWidth"), QStringLiteral("int"), {}, QStringLiteral("760"),
         QStringLiteral("Capped in pixels and as a fraction: both matter.")},
        {QStringLiteral("closesOnScrimClick"), QStringLiteral("bool"), {},
         QStringLiteral("true"),
         QStringLiteral("False for a drawer holding an edit a stray click would lose.")},
    };
    spec.build = buildDrawer;
    return spec;
}

ComponentSpec formSubmissionComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("form-submission");
    spec.name = QStringLiteral("Form submission");
    spec.category = Category::Input;
    spec.description = QStringLiteral(
        "What a dialog does between the press and the answer: busy state that disables "
        "the whole footer, and the server's error where the eye is already looking.");
    spec.headerPath = QStringLiteral("include/novu/ui/WindowShell.h");
    spec.sourcePath = QStringLiteral("src/WindowShell.cpp");
    spec.designedFor = {QStringLiteral("novuspace"), QStringLiteral("novuai")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Idle"), QStringLiteral("Busy"),
                     QStringLiteral("Failed")};
    spec.states = {QStringLiteral("Field error"), QStringLiteral("Panel notice"),
                   QStringLiteral("Submission error")};
    spec.keywords = {QStringLiteral("submit"), QStringLiteral("busy"),
                     QStringLiteral("validation"), QStringLiteral("error"),
                     QStringLiteral("form"), QStringLiteral("double-submit")};
    spec.properties = {
        {QStringLiteral("busy"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("Disables every footer button and stops Escape closing it.")},
        {QStringLiteral("formError"), QStringLiteral("QString"), {}, QStringLiteral(""),
         QStringLiteral("Shown above the footer. Setting one always clears busy.")},
    };
    spec.build = buildFormSubmission;
    return spec;
}

} // namespace novu::playground::pages
