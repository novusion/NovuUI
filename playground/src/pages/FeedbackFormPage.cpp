#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Badge.h"
#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/FeedbackForm.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/ThemeScope.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

QString stateName(FeedbackForm::State state)
{
    switch (state) {
    case FeedbackForm::State::Empty:   return QStringLiteral("Empty");
    case FeedbackForm::State::Editing: return QStringLiteral("Editing");
    case FeedbackForm::State::Invalid: return QStringLiteral("Validation error");
    case FeedbackForm::State::Sending: return QStringLiteral("Sending");
    case FeedbackForm::State::Sent:    return QStringLiteral("Success");
    case FeedbackForm::State::Failed:  return QStringLiteral("Submission error");
    }
    return QStringLiteral("Unknown");
}

Badge::Tone stateTone(FeedbackForm::State state)
{
    switch (state) {
    case FeedbackForm::State::Invalid:
    case FeedbackForm::State::Failed:  return Badge::Tone::Danger;
    case FeedbackForm::State::Sending: return Badge::Tone::Info;
    case FeedbackForm::State::Sent:    return Badge::Tone::Success;
    case FeedbackForm::State::Empty:
    case FeedbackForm::State::Editing: break;
    }
    return Badge::Tone::Neutral;
}

// The context a real consumer would pass. NovuUI never discovers any of this
// for itself - that is the boundary the component exists to hold.
FeedbackContext demoContext(bool withLogs)
{
    FeedbackContext context;
    context.application = QStringLiteral("NovuAI");
    context.version = QStringLiteral("0.89.067");
    context.workspace = QStringLiteral("Esantirion / Integration");
    if (withLogs) {
        context.diagnostics = [] {
            QStringList lines;
            for (int i = 0; i < 42; ++i) {
                lines.append(QStringLiteral("12:0%1:33  core.bridge  heartbeat ok")
                                 .arg(i % 10));
            }
            return lines.join(QLatin1Char('\n'));
        };
    }
    return context;
}

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);
    const QString profileId = ctx.profileId;

    // ── the form, live ────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("The form"),
                                 QStringLiteral("The production component, driven by a mock "
                                                "handler. No backend is involved."));
        QVBoxLayout* body = card->body();

        auto* form = new FeedbackForm(card);
        form->setContext(demoContext(true));
        body->addWidget(form);

        auto* stateRow = new QHBoxLayout();
        stateRow->setSpacing(themeFor(card).space(SpacingRole::ToolbarGap));
        stateRow->addWidget(label(QStringLiteral("State"), card, TypeRole::Label, Ink::Muted));
        auto* stateBadge = new Badge(stateName(form->state()), Badge::Tone::Neutral, card);
        stateRow->addWidget(stateBadge);
        stateRow->addStretch(1);
        body->addLayout(stateRow);

        // The mock handler. Exactly what a consumer writes, minus the network:
        // take the request, answer with success or failure.
        auto* outcome = new Badge(QStringLiteral("Mock: succeed"), Badge::Tone::Success, card);
        QPointer<FeedbackForm> live = form;
        bool* succeed = new bool(true);
        card->connect(card, &QObject::destroyed, [succeed] { delete succeed; });

        QObject::connect(form, &FeedbackForm::stateChanged, card,
                         [stateBadge](FeedbackForm::State state) {
                             stateBadge->setText(stateName(state));
                             stateBadge->setTone(stateTone(state));
                         });
        QObject::connect(form, &FeedbackForm::submitted, card,
                         [live, succeed](const FeedbackRequest& request) {
                             // Delayed, so Sending is a state the eye can catch
                             // rather than a frame nobody sees.
                             QTimer::singleShot(900, live, [live, succeed, request] {
                                 if (!live) {
                                     return;
                                 }
                                 if (*succeed) {
                                     live->reportSuccess(
                                         QObject::tr("Thank you — \"%1\" was sent.")
                                             .arg(request.title));
                                 } else {
                                     live->reportFailure(
                                         QObject::tr("The feedback service did not respond. "
                                                     "Your report was not sent."));
                                 }
                             });
                         });
        QObject::connect(form, &FeedbackForm::cancelled, card,
                         [live] { if (live) live->reset(); });

        QHBoxLayout* controls = exampleRow(body, card, QStringLiteral("Mock handler"));
        Button* flip = secondaryButton(QStringLiteral("Make it fail"), card);
        QObject::connect(flip, &QAbstractButton::clicked, card, [succeed, flip, outcome] {
            *succeed = !*succeed;
            flip->setText(*succeed ? QObject::tr("Make it fail")
                                   : QObject::tr("Make it succeed"));
            outcome->setText(*succeed ? QObject::tr("Mock: succeed")
                                      : QObject::tr("Mock: fail"));
            outcome->setTone(*succeed ? Badge::Tone::Success : Badge::Tone::Danger);
        });
        controls->addWidget(flip);
        controls->addWidget(outcome);
        Button* resetButton = ghostButton(QStringLiteral("Reset"), card);
        QObject::connect(resetButton, &QAbstractButton::clicked, card,
                         [live] { if (live) live->reset(); });
        controls->addWidget(resetButton);
        controls->addStretch(1);

        note(body, card,
             QStringLiteral("Send with the fields empty to see the validation state. Send with "
                            "them filled to see Sending, then Success — or flip the handler "
                            "first to see the submission error, which leaves the form "
                            "editable so the user can retry."));
        column->addWidget(card);
    }

    // ── privacy ───────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("When the host has no logs"),
                                 QStringLiteral("The option is disabled and says why, rather "
                                                "than quietly missing."));
        QVBoxLayout* body = card->body();

        auto* form = new FeedbackForm(card);
        // No diagnostics callback: this consumer cannot supply logs.
        form->setContext(demoContext(false));
        body->addWidget(form);

        note(body, card,
             QStringLiteral("Nothing is attached without a control that says so and can turn "
                            "it off, and the line under each control names the actual values "
                            "rather than promising \"diagnostic information\". Logs are "
                            "collected by a callback at submit time, so ticking the box is "
                            "what causes them to be read."));
        column->addWidget(card);
    }

    // ── the dialog ────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("As a dialog"),
                                 QStringLiteral("What an application's \"Send feedback\" "
                                                "action opens. The Novu window shell, the "
                                                "same form inside it."));
        QVBoxLayout* body = card->body();
        QHBoxLayout* row = exampleRow(body, card, QString());

        Button* open = primaryButton(QStringLiteral("Open feedback dialog"), card);
        QObject::connect(open, &QAbstractButton::clicked, card, [card, profileId] {
            FeedbackDialog dialog(demoContext(true), card);
            ThemeScope::apply(&dialog, profileId);
            dialog.setModal(true);
            QObject::connect(dialog.form(), &FeedbackForm::submitted, &dialog,
                             [&dialog](const FeedbackRequest& request) {
                                 QTimer::singleShot(700, &dialog, [&dialog, request] {
                                     dialog.form()->reportSuccess(
                                         QObject::tr("Thank you — \"%1\" was sent.")
                                             .arg(request.title));
                                 });
                             });
            dialog.exec();
        });
        row->addWidget(open);
        row->addStretch(1);
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec feedbackFormComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("feedback-form");
    spec.name = QStringLiteral("FeedbackForm");
    spec.category = Category::Feedback;
    spec.description = QStringLiteral(
        "Send feedback: type, title, description, and an explicit account of what else is "
        "attached. Produces a FeedbackRequest and hands it to the consumer — NovuUI knows "
        "nothing about transport.");
    spec.headerPath = QStringLiteral("include/novu/ui/FeedbackForm.h");
    spec.sourcePath = QStringLiteral("src/FeedbackForm.cpp");
    spec.usedBy = {};
    // Built here first, ahead of the applications that will open it.
    spec.awaitingAdoption = true;
    spec.variants = {QStringLiteral("Bug"), QStringLiteral("Suggestion"),
                     QStringLiteral("General"), QStringLiteral("FeedbackDialog")};
    spec.states = {QStringLiteral("Empty"), QStringLiteral("Editing"),
                   QStringLiteral("Validation error"), QStringLiteral("Sending"),
                   QStringLiteral("Success"), QStringLiteral("Submission error")};
    spec.keywords = {QStringLiteral("feedback"), QStringLiteral("bug"),
                     QStringLiteral("report"), QStringLiteral("suggestion"),
                     QStringLiteral("submit"), QStringLiteral("diagnostics"),
                     QStringLiteral("logs")};
    spec.properties = {
        {QStringLiteral("context"), QStringLiteral("FeedbackContext"), {}, QString(),
         QStringLiteral("Application, version, platform, workspace and an optional log "
                        "callback. Supplied by the consumer; never discovered here.")},
        {QStringLiteral("type"), QStringLiteral("enum"),
         {QStringLiteral("Bug"), QStringLiteral("Suggestion"), QStringLiteral("General")},
         QStringLiteral("Bug"), QStringLiteral("Chosen with a TabStrip.")},
        {QStringLiteral("includeApplicationInfo"), QStringLiteral("bool"), {},
         QStringLiteral("true"),
         QStringLiteral("On by default, and the values it would send are listed underneath.")},
        {QStringLiteral("includeLogs"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("Off by default. Disabled with an explanation when the consumer "
                        "supplies no log callback.")},
        {QStringLiteral("state"), QStringLiteral("enum"),
         {QStringLiteral("Empty"), QStringLiteral("Editing"), QStringLiteral("Invalid"),
          QStringLiteral("Sending"), QStringLiteral("Sent"), QStringLiteral("Failed")},
         QStringLiteral("Empty"),
         QStringLiteral("Sending locks the fields and refuses a second submit.")},
    };
    spec.scenarios = {Scenario::Normal, Scenario::LongText, Scenario::ErrorState};
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
