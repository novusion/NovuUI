#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/Fields.h"
#include "novu/ui/FloatingPanel.h"
#include "novu/ui/Form.h"
#include "novu/ui/LogView.h"
#include "novu/ui/Separator.h"
#include "novu/ui/SettingRow.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/Toast.h"
#include "novu/ui/Toggle.h"
#include "novu/ui/WindowShell.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

// ── FloatingPanel ────────────────────────────────────────────────────────

FloatingPanel* makePanel(FloatingLayer* layer, Glyph glyph, const QString& title,
                         const QString& subtitle)
{
    auto* panel = new FloatingPanel(glyph, title);
    panel->setSubtitle(subtitle);
    panel->addBarAction(Glyph::External, QObject::tr("Detach to a window"),
                        QStringLiteral("detach"));
    panel->addBarAction(Glyph::Copy, QObject::tr("Duplicate"), QStringLiteral("clone"));

    auto* log = new LogView(panel->bodyWidget());
    log->setShowTimestamps(false);
    log->appendAll({{LogView::Level::Info, QStringLiteral("session opened")},
                    {LogView::Level::Debug, QStringLiteral("model: novu-local-7b")},
                    {LogView::Level::Info, QStringLiteral("context: 4 documents")},
                    {LogView::Level::Warning, QStringLiteral("one document is stale")}});
    panel->body()->addWidget(log);

    panel->setTaskText(QObject::tr("Summarising 4 documents"), QObject::tr("38%"));

    // `.fw-foot` - the row the window's own buttons stand in, a sibling of
    // the body rather than the last thing inside it. The body scrolls; this
    // does not, so a form longer than the window cannot scroll its own
    // confirm button out of reach.
    auto* cancel = new Button(QObject::tr("Cancel"), Button::Variant::Secondary, panel);
    cancel->setControlSize(ControlSize::Small);
    QObject::connect(cancel, &QAbstractButton::clicked, panel, &QObject::deleteLater);
    auto* confirm = new Button(QObject::tr("Save"), Button::Variant::Primary, panel);
    confirm->setControlSize(ControlSize::Small);
    panel->footer()->addStretch(1);
    panel->footer()->addWidget(cancel);
    panel->footer()->addWidget(confirm);

    QObject::connect(panel, &FloatingPanel::closeRequested, panel, [panel] {
        panel->deleteLater();
    });
    layer->addPanel(panel);
    return panel;
}

QWidget* buildFloatingPanel(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    {
        Card* card = exampleCard(page, QStringLiteral("Panels inside a window"),
                                 QStringLiteral("Draggable, resizable, and "
                                                "clipped to what they belong to."));
        QVBoxLayout* body = card->body();

        // The host is a real widget in this page: a floating panel cannot leave
        // its layer, and the point of the component is that this is a boundary
        // rather than a suggestion.
        auto* host = new QWidget(card);
        host->setMinimumSize(560, 340);
        host->setAutoFillBackground(true);
        QPalette ground = host->palette();
        ground.setColor(QPalette::Window, themeFor(card).colors.panelBg);
        host->setPalette(ground);

        FloatingLayer* layer = FloatingLayer::forHost(host);
        FloatingPanel* first = makePanel(layer, Glyph::Ai, QStringLiteral("Assistant"),
                                         QStringLiteral("session 4f2b · novu-local-7b"));
        first->placeAt(QPoint(24, 20), QSize(400, 230));

        body->addWidget(host);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Panels"));
        auto* add = new Button(QStringLiteral("Open another"), Button::Variant::Secondary,
                               card);
        add->setControlSize(ControlSize::Small);
        int* counter = new int(0);
        QObject::connect(add, &Button::clicked, layer, [layer, counter] {
            const int index = ++(*counter);
            FloatingPanel* panel = makePanel(
                layer, Glyph::Chart, QObject::tr("Inspector %1").arg(index),
                QObject::tr("detached view"));
            panel->placeAt(QPoint(40 + index * 26, 30 + index * 26), QSize(380, 210));
        });
        QObject::connect(layer, &QObject::destroyed, add, [counter] { delete counter; });
        row->addWidget(add);

        auto* cascade = new Button(QStringLiteral("Cascade"), Button::Variant::Secondary,
                                   card);
        cascade->setControlSize(ControlSize::Small);
        QObject::connect(cascade, &Button::clicked, layer, [layer] { layer->cascade(); });
        row->addWidget(cascade);

        auto* roll = new Button(QStringLiteral("Roll the front one up"),
                                Button::Variant::Ghost, card);
        roll->setControlSize(ControlSize::Small);
        QObject::connect(roll, &Button::clicked, layer, [layer] {
            if (FloatingPanel* front = layer->frontPanel()) {
                front->setRolled(!front->isRolled());
            }
        });
        row->addWidget(roll);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Drag a panel by its bar and try to drag it out of the grey area. "
                            "It cannot leave — which is the whole difference between this and "
                            "a ToolWindow. A ToolWindow is a window of the desktop: it can go "
                            "to another screen and the compositor draws its shadow. This is a "
                            "detached view of something inside this window, and it is clipped "
                            "to it."));
        note(body, card,
             QStringLiteral("Open a second and click between them. The front one gets the "
                            "accent wash down from its top edge and a brighter caption; front "
                            "is a property of exactly one panel, set by the layer rather than "
                            "decided by each panel, which is how two of them both end up "
                            "drawing the focused treatment."));
        note(body, card,
             QStringLiteral("Double-click a bar to roll it up. Rolling is not closing: the "
                            "panel keeps its contents and its height, and unrolls to exactly "
                            "what it was rather than to a size hint."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

// ── Toast ────────────────────────────────────────────────────────────────

QWidget* buildToast(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    {
        Card* card = exampleCard(page, QStringLiteral("Confirmations"),
                                 QStringLiteral("An outcome the user already "
                                                "expected."));
        QVBoxLayout* body = card->body();

        auto* host = new QWidget(card);
        host->setMinimumSize(520, 220);
        host->setAutoFillBackground(true);
        QPalette ground = host->palette();
        ground.setColor(QPalette::Window, themeFor(card).colors.panelBg);
        host->setPalette(ground);
        body->addWidget(host);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Post one"));

        const struct { const char* caption; const char* message; Toast::Tone tone; } samples[] = {
            {"Plain", "Copied", Toast::Tone::Neutral},
            {"Success", "Workspace archived", Toast::Tone::Success},
            {"Warning", "Two documents were skipped because they are locked",
             Toast::Tone::Warning},
            {"Danger", "Could not reach the hub", Toast::Tone::Danger},
        };
        for (const auto& sample : samples) {
            auto* button = new Button(QString::fromLatin1(sample.caption),
                                      Button::Variant::Secondary, card);
            button->setControlSize(ControlSize::Small);
            const QString message = QString::fromLatin1(sample.message);
            const Toast::Tone tone = sample.tone;
            QObject::connect(button, &Button::clicked, host, [host, message, tone] {
                Toast::post(host, message, tone);
            });
            row->addWidget(button);
        }

        auto* undo = new Button(QStringLiteral("With an action"), Button::Variant::Primary,
                                card);
        undo->setControlSize(ControlSize::Small);
        QObject::connect(undo, &Button::clicked, host, [host] {
            Toast* toast = Toast::postWithAction(host, QObject::tr("3 files moved to Archive"),
                                                 QObject::tr("Undo"), Toast::Tone::Success);
            QObject::connect(toast, &Toast::actionTriggered, host, [host] {
                Toast::post(host, QObject::tr("Move undone"), Toast::Tone::Neutral);
            });
        });
        row->addWidget(undo);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Hover a toast while it is on screen: the timer stops. A message "
                            "with an Undo in it is unusable otherwise — the user reaches for "
                            "the button and it leaves while the pointer is on its way."));
        note(body, card,
             QStringLiteral("How long each stays is scaled to how long it takes to read. "
                            "\"Copied\" and the two-line warning above do not need the same "
                            "time on screen, and a single fixed duration is wrong for one of "
                            "them."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("Stacking, and the cap"),
                                 QStringLiteral("Not a notification centre."));
        QVBoxLayout* body = card->body();

        auto* host = new QWidget(card);
        host->setMinimumSize(520, 200);
        host->setAutoFillBackground(true);
        QPalette ground = host->palette();
        ground.setColor(QPalette::Window, themeFor(card).colors.panelBg);
        host->setPalette(ground);
        body->addWidget(host);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Flood it"));
        auto* burst = new Button(QStringLiteral("Post six at once"),
                                 Button::Variant::Secondary, card);
        burst->setControlSize(ControlSize::Small);
        QObject::connect(burst, &Button::clicked, host, [host] {
            for (int i = 1; i <= 6; ++i) {
                Toast::post(host, QObject::tr("Message %1").arg(i),
                            i % 2 ? Toast::Tone::Neutral : Toast::Tone::Info, 6000);
            }
        });
        row->addWidget(burst);

        auto* clear = new Button(QStringLiteral("Dismiss all"), Button::Variant::Ghost, card);
        clear->setControlSize(ControlSize::Small);
        QObject::connect(clear, &Button::clicked, host, [host] { Toast::dismissAll(host); });
        row->addWidget(clear);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Six were posted; three are on screen. A fourth replaces the "
                            "oldest rather than growing the stack, because a column of six "
                            "toasts is a notification centre nobody asked for — and one that "
                            "cannot be scrolled, dismissed individually or read later."));
        note(body, card,
             QStringLiteral("There is deliberately no modal toast, no close button and no "
                            "stay-until-dismissed default. Anything a user must act on is a "
                            "dialog; anything that persists is an InfoBanner in the panel it "
                            "belongs to. A toast that has to be read will be missed."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

// ── Dialog ───────────────────────────────────────────────────────────────

QWidget* buildDialog(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    {
        Card* card = exampleCard(page, QStringLiteral("The head"),
                                 QStringLiteral("A tone-marked question, above the "
                                                "body."));
        QVBoxLayout* body = card->body();

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Open one"));

        const struct {
            const char* caption;
            Dialog::Tone tone;
            const char* heading;
            const char* note;
        } samples[] = {
            {"Neutral", Dialog::Tone::Neutral, "Rename workspace",
             "The name is shown in the sidebar and in every link to this workspace."},
            {"Accent", Dialog::Tone::Accent, "Connect a device",
             "The device will be issued a WireGuard identity and an address on the "
             "platform network. It can be revoked at any time."},
            {"Warning", Dialog::Tone::Warning, "Rebuild the index",
             "Every document in every workspace is re-read. The index is unavailable "
             "while this runs, which on a large workspace is several minutes."},
            {"Danger", Dialog::Tone::Danger, "Delete 12 peers",
             "Their authorisation is removed immediately. Devices currently connected "
             "are disconnected on the reconciler's next pass. This cannot be undone."},
        };

        for (const auto& sample : samples) {
            auto* button = new Button(QString::fromLatin1(sample.caption),
                                      sample.tone == Dialog::Tone::Danger
                                          ? Button::Variant::Danger
                                          : Button::Variant::Secondary,
                                      card);
            button->setControlSize(ControlSize::Small);
            const Dialog::Tone tone = sample.tone;
            const QString heading = QString::fromLatin1(sample.heading);
            const QString hint = QString::fromLatin1(sample.note);

            QObject::connect(button, &Button::clicked, card, [card, tone, heading, hint] {
                auto* dialog = new Dialog(heading, card->window());
                const ThemeProfile& profile = themeFor(dialog);
                const int padding = profile.space(SpacingRole::CardPadding);
                dialog->contentLayout()->setContentsMargins(padding, padding, padding,
                                                            padding);
                dialog->contentLayout()->setSpacing(profile.space(SpacingRole::SectionGap));

                auto* field = lineEdit(QObject::tr("Novusion"), dialog->content(), 0);
                dialog->contentLayout()->addWidget(field);
                dialog->contentLayout()->addStretch(1);

                // Called after the body is filled, which is the common order and
                // has to work: the head goes in at the top regardless.
                dialog->setHeading(heading, hint, tone);

                Button* cancel = secondaryButton(QObject::tr("Cancel"), dialog->content());
                Button* accept = tone == Dialog::Tone::Danger
                    ? dangerButton(QObject::tr("Delete"), dialog->content())
                    : primaryButton(QObject::tr("Continue"), dialog->content());
                dialog->buttonRow()->addWidget(cancel);
                dialog->buttonRow()->addWidget(accept);
                QObject::connect(cancel, &QAbstractButton::clicked, dialog, &QDialog::reject);
                QObject::connect(accept, &QAbstractButton::clicked, dialog, &QDialog::accept);
                if (tone == Dialog::Tone::Danger) {
                    cancel->setDefault(true);
                } else {
                    accept->setDefault(true);
                }

                dialog->setModal(true);
                dialog->shell()->setContentSize(QSize(460, 250));
                dialog->exec();
                dialog->deleteLater();
            });
            row->addWidget(button);
        }
        row->addStretch(1);

        note(body, card,
             QStringLiteral("The head is not the title bar's caption, and the two say different "
                            "things. The caption names the window — it is what appears in the "
                            "task bar and what you see when the dialog is behind something. "
                            "The head is the question, and it has room for a sentence the "
                            "caption does not."));
        note(body, card,
             QStringLiteral("Open the Danger one and press Return. Nothing is deleted: a "
                            "destructive confirmation never makes its dangerous option the "
                            "default. A user who hits Enter to dismiss a dialog they have not "
                            "read would otherwise delete the thing it was asking about, which "
                            "makes the confirmation worse than useless."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("The short forms"),
                                 QStringLiteral("What replaced QMessageBox."));
        QVBoxLayout* body = card->body();

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("One call each"));

        auto* confirm = new Button(QStringLiteral("confirm()"), Button::Variant::Secondary,
                                   card);
        confirm->setControlSize(ControlSize::Small);
        QObject::connect(confirm, &Button::clicked, card, [card] {
            const bool yes = Dialog::confirm(
                card->window(), QObject::tr("Discard changes?"),
                QObject::tr("The three edits you have made to this profile will be lost."),
                QObject::tr("Discard"), QObject::tr("Keep editing"), true);
            Toast::post(card->window(),
                        yes ? QObject::tr("Discarded") : QObject::tr("Still editing"),
                        yes ? Toast::Tone::Warning : Toast::Tone::Neutral);
        });
        row->addWidget(confirm);

        auto* inform = new Button(QStringLiteral("inform()"), Button::Variant::Secondary,
                                  card);
        inform->setControlSize(ControlSize::Small);
        QObject::connect(inform, &Button::clicked, card, [card] {
            Dialog::inform(card->window(), QObject::tr("Export finished"),
                           QObject::tr("2,481 rows were written to orders-2026-03.csv."));
        });
        row->addWidget(inform);

        auto* error = new Button(QStringLiteral("reportError()"), Button::Variant::Secondary,
                                 card);
        error->setControlSize(ControlSize::Small);
        QObject::connect(error, &Button::clicked, card, [card] {
            Dialog::reportError(
                card->window(), QObject::tr("Could not save"),
                QObject::tr("The profile directory is read-only: "
                            "C:\\ProgramData\\Novusion\\profiles"));
        });
        row->addWidget(error);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Same shell, same chrome, same button geometry as every other "
                            "window in the application — which a QMessageBox cannot be, "
                            "because it is drawn by the platform. reportError puts the danger "
                            "ink on the sentence rather than on the frame: the window is not "
                            "the error, the sentence is."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec floatingPanelComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("floating-panel");
    spec.name = QStringLiteral("FloatingPanel");
    spec.category = Category::Foundation;
    spec.description = QStringLiteral(
        "A panel that floats inside the application window: a grab bar, a body, a task "
        "strip, a resize grip and a roll-up. Clipped to its host, which is the point.");
    spec.headerPath = QStringLiteral("include/novu/ui/FloatingPanel.h");
    spec.sourcePath = QStringLiteral("src/FloatingPanel.cpp");
    spec.designedFor = {QStringLiteral("novuai")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("With a task strip"), QStringLiteral("Rolled")};
    spec.states = {QStringLiteral("Front"), QStringLiteral("Behind"),
                   QStringLiteral("Dragging"), QStringLiteral("Resizing")};
    spec.keywords = {QStringLiteral("floatwin"), QStringLiteral("floatlayer"),
                     QStringLiteral("panel"), QStringLiteral("detach"),
                     QStringLiteral("mdi"), QStringLiteral("dock")};
    spec.properties = {
        {QStringLiteral("rolled"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("Only the bar. Keeps its contents and its height.")},
        {QStringLiteral("front"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("Set by the layer, never by the panel.")},
        {QStringLiteral("resizable"), QStringLiteral("bool"), {}, QStringLiteral("true"),
         QStringLiteral("Hides the grip as well as refusing the drag.")},
    };
    spec.build = buildFloatingPanel;
    return spec;
}

ComponentSpec toastComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("toast");
    spec.name = QStringLiteral("Toast");
    spec.category = Category::Feedback;
    spec.description = QStringLiteral(
        "The line that says something happened and then goes away. Confirms an outcome "
        "the user expected; refuses to be anything a user has to act on.");
    spec.headerPath = QStringLiteral("include/novu/ui/Toast.h");
    spec.sourcePath = QStringLiteral("src/Toast.cpp");
    spec.designedFor = {QStringLiteral("novuai")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Neutral"), QStringLiteral("Success"),
                     QStringLiteral("Warning"), QStringLiteral("Danger"),
                     QStringLiteral("With an action")};
    spec.states = {QStringLiteral("Rising"), QStringLiteral("Held"),
                   QStringLiteral("Stacked")};
    spec.keywords = {QStringLiteral("toast"), QStringLiteral("notification"),
                     QStringLiteral("snackbar"), QStringLiteral("undo"),
                     QStringLiteral("confirmation"), QStringLiteral("transient")};
    spec.properties = {
        {QStringLiteral("tone"), QStringLiteral("enum"),
         {QStringLiteral("Neutral"), QStringLiteral("Success"), QStringLiteral("Warning"),
          QStringLiteral("Danger"), QStringLiteral("Info"), QStringLiteral("Accent")},
         QStringLiteral("Neutral"), QStringLiteral("The mark. Most toasts need no colour.")},
        {QStringLiteral("duration"), QStringLiteral("int"), {}, QStringLiteral("0"),
         QStringLiteral("0 scales with the length of the message.")},
        {QStringLiteral("actionText"), QStringLiteral("QString"), {}, QStringLiteral(""),
         QStringLiteral("Taking the action dismisses the toast.")},
    };
    spec.build = buildToast;
    return spec;
}

ComponentSpec dialogComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("dialog");
    spec.name = QStringLiteral("Dialog");
    spec.category = Category::Feedback;
    spec.description = QStringLiteral(
        "A question wearing the Novu frame: a tone-marked head, a body, and a footer "
        "whose button order is the component's decision rather than the page's.");
    spec.headerPath = QStringLiteral("include/novu/ui/WindowShell.h");
    spec.sourcePath = QStringLiteral("src/WindowShell.cpp");
    spec.designedFor = {QStringLiteral("novuspace"), QStringLiteral("novuai")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Neutral"), QStringLiteral("Accent"),
                     QStringLiteral("Warning"), QStringLiteral("Danger")};
    spec.states = {QStringLiteral("confirm()"), QStringLiteral("inform()"),
                   QStringLiteral("reportError()")};
    spec.keywords = {QStringLiteral("modal"), QStringLiteral("modal-head"),
                     QStringLiteral("confirm"), QStringLiteral("qmessagebox"),
                     QStringLiteral("alert"), QStringLiteral("prompt")};
    spec.properties = {
        {QStringLiteral("tone"), QStringLiteral("enum"),
         {QStringLiteral("Neutral"), QStringLiteral("Accent"), QStringLiteral("Warning"),
          QStringLiteral("Danger")},
         QStringLiteral("Neutral"),
         QStringLiteral("What kind of question this is, before the title is read.")},
        {QStringLiteral("note"), QStringLiteral("QString"), {}, QStringLiteral(""),
         QStringLiteral("Wraps, and the head grows to fit it.")},
    };
    spec.build = buildDialog;
    return spec;
}

} // namespace novu::playground::pages
