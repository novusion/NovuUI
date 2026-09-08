#pragma once

#include "novu/ui/WindowShell.h"

#include <QString>
#include <QWidget>

#include <functional>

class QLabel;
class QLineEdit;
class QPlainTextEdit;

namespace novu::ui {

class Button;
class CheckBox;
class InfoBanner;
class TabStrip;

// What the user is telling us.
enum class FeedbackType { Bug, Suggestion, General };

QString feedbackTypeKey(FeedbackType type);      // "bug" | "suggestion" | "general"
QString feedbackTypeLabel(FeedbackType type);

// What the consumer knows about itself and this library cannot.
//
// NovuUI has no idea which application it is inside, what version it is, or
// where that application keeps its logs - and it must not learn, or the shared
// component becomes one application's component with the others bolted on.
struct FeedbackContext
{
    QString application;
    QString version;
    // Left empty, a readable platform string is filled in from Qt.
    QString platform;
    // Whatever the product calls the thing the user is currently in - a
    // workspace, a project, a channel. Optional.
    QString workspace;

    // Recent logs, fetched only if the user asks for them.
    //
    // A callback rather than a string, and the difference is the point: with a
    // string the logs are read, held in memory and possibly written to a crash
    // dump before the user has decided anything. With a callback, ticking the
    // box is what causes them to be collected, so "include logs" is a decision
    // rather than a default that already happened.
    //
    // Unset means this application cannot supply logs, and the option is
    // disabled with a line saying so rather than silently missing.
    std::function<QString()> diagnostics;
};

// What the form produces and hands to the consumer.
//
// A struct on purpose. A screenshot, a file attachment, a category, a reply
// address are all *additions to this* rather than changes to the signal that
// carries it - which is what keeps them from needing an API redesign later.
// A signal with eight parameters would not have that property.
struct FeedbackRequest
{
    FeedbackType type = FeedbackType::Bug;
    QString title;
    QString description;

    // Present only when includeApplicationInfo is true. Copied rather than
    // referenced so the consumer's transport can outlive the dialog.
    QString application;
    QString version;
    QString platform;
    QString workspace;

    bool includeApplicationInfo = true;
    bool includeLogs = false;
    // Empty unless includeLogs is true - and collected at submit time, not at
    // construction.
    QString diagnosticData;
};

// The feedback form: type, title, description, what to attach, and two actions.
//
// A widget rather than only a dialog, so it can be a page in a settings window
// as easily as a window of its own. FeedbackDialog below is the usual case.
//
// It knows nothing about transport. submitted() hands the consumer a request
// and the form goes to Sending; the consumer answers with reportSuccess() or
// reportFailure() when its network call returns. NovuUI does not know that
// NovuServer exists, and an application that changes where feedback goes
// changes nothing here.
class FeedbackForm : public QWidget
{
    Q_OBJECT

public:
    // Where the form is in its own lifecycle. Exposed because a host window
    // needs it - a dialog must not close itself out from under a send in
    // flight - and because it is what the gallery demonstrates.
    enum class State {
        Empty,     // nothing typed yet
        Editing,   // the user is filling it in
        Invalid,   // submit was pressed with something missing
        Sending,   // handed to the consumer, waiting
        Sent,      // the consumer reported success
        Failed,    // the consumer reported an error; the user may retry
    };
    Q_ENUM(State)

    explicit FeedbackForm(QWidget* parent = nullptr);

    void setContext(const FeedbackContext& context);
    FeedbackContext context() const { return m_context; }

    State state() const { return m_state; }

    // What would be submitted right now. Note that diagnostics are collected
    // here, so calling this is what runs the consumer's log callback.
    FeedbackRequest request() const;

    // Back to Empty, with the fields cleared and the context kept.
    void reset();

    // The consumer's answer to submitted(). Anything else is ignored, so a
    // late reply from a cancelled send cannot resurrect a closed form.
    void reportSuccess(const QString& message = QString());
    void reportFailure(const QString& message);

signals:
    // The consumer's cue to send. The form is in Sending until answered, and
    // will not emit this again until then - which is what stops a double
    // submit.
    void submitted(const novu::ui::FeedbackRequest& request);
    void cancelled();
    void stateChanged(novu::ui::FeedbackForm::State state);

private:
    void setState(State state);
    void applyState();
    void refreshAttachmentSummary();
    void onSubmit();
    QString validationProblem() const;

    FeedbackContext m_context;
    State m_state = State::Empty;

    TabStrip* m_type = nullptr;
    QLineEdit* m_title = nullptr;
    QPlainTextEdit* m_description = nullptr;
    CheckBox* m_includeInfo = nullptr;
    CheckBox* m_includeLogs = nullptr;
    QLabel* m_attachmentSummary = nullptr;
    QLabel* m_logsHint = nullptr;
    InfoBanner* m_banner = nullptr;
    Button* m_cancel = nullptr;
    Button* m_send = nullptr;
};

// The form in a Novu dialog. What an application's "Send feedback" action
// opens.
//
//     FeedbackContext context;
//     context.application = QStringLiteral("NovuAI");
//     context.version = appVersion();
//     context.diagnostics = [] { return recentLog(200); };
//
//     auto* dialog = new FeedbackDialog(context, this);
//     connect(dialog->form(), &FeedbackForm::submitted, this,
//             [dialog, this](const FeedbackRequest& request) {
//                 post(request, [dialog](bool ok, const QString& error) {
//                     ok ? dialog->form()->reportSuccess()
//                        : dialog->form()->reportFailure(error);
//                 });
//             });
//     dialog->exec();
class FeedbackDialog : public Dialog
{
    Q_OBJECT

public:
    explicit FeedbackDialog(const FeedbackContext& context, QWidget* parent = nullptr);

    FeedbackForm* form() const { return m_form; }

private:
    FeedbackForm* m_form = nullptr;
};

} // namespace novu::ui
