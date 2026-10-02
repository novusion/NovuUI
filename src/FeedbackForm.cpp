#include "novu/ui/FeedbackForm.h"

#include "novu/ui/Button.h"
#include "novu/ui/Feedback.h"
#include "novu/ui/Fields.h"
#include "novu/ui/Form.h"
#include "novu/ui/Layout.h"
#include "novu/ui/TabStrip.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSysInfo>
#include <QVBoxLayout>

namespace novu::ui {

namespace {

const QVector<FeedbackType>& typeOrder()
{
    // One order, used by the strip, by the index lookup and by nothing else -
    // so the tab a user picked and the type that reaches the consumer cannot
    // drift apart.
    static const QVector<FeedbackType> order = {FeedbackType::Bug, FeedbackType::Suggestion,
                                                FeedbackType::General};
    return order;
}

} // namespace

QString feedbackTypeKey(FeedbackType type)
{
    switch (type) {
    case FeedbackType::Suggestion:  return QStringLiteral("suggestion");
    case FeedbackType::General:     return QStringLiteral("general");
    case FeedbackType::Improvement: return QStringLiteral("improvement");
    case FeedbackType::Bug:         break;
    }
    return QStringLiteral("bug");
}

QString feedbackTypeLabel(FeedbackType type)
{
    switch (type) {
    case FeedbackType::Suggestion:  return QObject::tr("Suggestion");
    case FeedbackType::General:     return QObject::tr("General");
    case FeedbackType::Improvement: return QObject::tr("Improvement");
    case FeedbackType::Bug:         break;
    }
    return QObject::tr("Bug");
}

FeedbackForm::FeedbackForm(QWidget* parent, const QVector<FeedbackType>& types)
    : QWidget(parent)
    , m_typeOrder(types.isEmpty() ? typeOrder() : types)
{
    const ThemeProfile& profile = themeFor(this);

    auto* column = new QVBoxLayout(this);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(profile.space(SpacingRole::SectionGap));

    QStringList typeLabels;
    for (FeedbackType type : m_typeOrder) {
        typeLabels.append(feedbackTypeLabel(type));
    }
    m_type = new TabStrip(typeLabels, this, 0, TabStrip::Variant::Segment);
    auto* typeRow = new QHBoxLayout();
    typeRow->setSpacing(profile.space(SpacingRole::ToolbarGap));
    typeRow->addWidget(m_type);
    typeRow->addStretch(1);
    column->addLayout(typeRow);

    // Caption above the field rather than formRow()'s label column. Two
    // reasons, both visible the moment it is drawn the other way: these fields
    // want the full width, and a label vertically centred against a five-line
    // description box floats in the middle of nothing.
    // Where the report is about. Hidden until a host supplies options.
    m_moduleRow = new QWidget(this);
    auto* moduleLayout = new QVBoxLayout(m_moduleRow);
    moduleLayout->setContentsMargins(0, 0, 0, 0);
    moduleLayout->setSpacing(4);
    moduleLayout->addWidget(caption(tr("Area"), m_moduleRow));
    m_module = new ComboBox(m_moduleRow);
    moduleLayout->addWidget(m_module);
    m_moduleRow->setVisible(false);
    column->addWidget(m_moduleRow);

    // What the host says the user was looking at. Read-only.
    m_contextLine = label(QString(), this, TypeRole::Caption, Ink::Muted);
    m_contextLine->setVisible(false);
    column->addWidget(m_contextLine);

    column->addWidget(caption(tr("Title"), this));
    m_title = lineEdit(tr("A short summary"), this);
    column->addWidget(m_title);

    column->addWidget(caption(tr("Description"), this));
    m_description = textArea(tr("What happened, and what did you expect instead?"), this, 5);
    column->addWidget(m_description);

    // -- what gets attached ---------------------------------------------------
    //
    // Spelled out rather than implied. Application info and logs are never
    // attached without a control that says so and can turn it off, and the
    // summary underneath names the actual values rather than promising
    // "diagnostic information".
    QLabel* attachHeading = caption(tr("Attached to this report"), this);
    column->addWidget(attachHeading);

    m_includeInfo = checkBox(tr("Include application information"), this, true);
    column->addWidget(m_includeInfo);

    m_attachmentSummary = label(QString(), this, TypeRole::Caption, Ink::Muted);
    m_attachmentSummary->setWordWrap(true);
    m_attachmentSummary->setContentsMargins(profile.spacing.lg + profile.spacing.sm, 0, 0, 0);
    column->addWidget(m_attachmentSummary);

    m_includeLogs = checkBox(tr("Include recent application logs"), this, false);
    column->addWidget(m_includeLogs);

    m_includeScreenshot = checkBox(tr("Include a screenshot"), this, false);
    m_includeScreenshot->setEnabled(false);
    m_includeScreenshot->setVisible(false);
    column->addWidget(m_includeScreenshot);

    m_logsHint = label(QString(), this, TypeRole::Caption, Ink::Muted);
    m_logsHint->setWordWrap(true);
    m_logsHint->setContentsMargins(profile.spacing.lg + profile.spacing.sm, 0, 0, 0);
    column->addWidget(m_logsHint);

    // -- outcome --------------------------------------------------------------
    // Through InfoBanner, so a success here is the same green as a success
    // anywhere else in the product rather than this dialog's own idea of one.
    m_banner = new InfoBanner(QString(), Badge::Tone::Info, this);
    m_banner->hide();
    column->addWidget(m_banner);

    column->addStretch(1);

    auto* actions = new QHBoxLayout();
    actions->setSpacing(profile.space(SpacingRole::ToolbarGap));
    actions->addStretch(1);
    m_cancel = secondaryButton(tr("Cancel"), this);
    m_send = primaryButton(tr("Send feedback"), this);
    actions->addWidget(m_cancel);
    actions->addWidget(m_send);
    column->addLayout(actions);

    connect(m_cancel, &QAbstractButton::clicked, this, &FeedbackForm::cancelled);
    connect(m_send, &QAbstractButton::clicked, this, &FeedbackForm::onSubmit);

    const auto onEdited = [this] {
        // Typing clears a validation complaint. Leaving it up while the user
        // fixes the thing it is complaining about makes the form feel broken.
        if (m_state == State::Invalid || m_state == State::Failed) {
            setState(State::Editing);
        } else if (m_state == State::Empty || m_state == State::Editing) {
            setState(m_title->text().trimmed().isEmpty()
                             && m_description->toPlainText().trimmed().isEmpty()
                         ? State::Empty
                         : State::Editing);
        }
    };
    connect(m_title, &QLineEdit::textChanged, this, onEdited);
    connect(m_description, &QPlainTextEdit::textChanged, this, onEdited);
    connect(m_includeInfo, &QCheckBox::toggled, this,
            &FeedbackForm::refreshAttachmentSummary);
    connect(m_includeLogs, &QCheckBox::toggled, this,
            &FeedbackForm::refreshAttachmentSummary);

    refreshAttachmentSummary();
    applyState();
}

void FeedbackForm::setContext(const FeedbackContext& context)
{
    // The context line is the host's words; an empty one takes no room.
    if (m_contextLine) {
        m_contextLine->setText(context.contextLabel);
        m_contextLine->setVisible(!context.contextLabel.isEmpty());
    }
    m_context = context;
    if (m_context.platform.isEmpty()) {
        // Filled in rather than demanded: every consumer would otherwise write
        // the same two Qt calls, and the ones that forgot would send reports
        // with no platform at all.
        m_context.platform = QStringLiteral("%1 %2 (%3)")
                                 .arg(QSysInfo::prettyProductName(),
                                      QSysInfo::productVersion(),
                                      QSysInfo::currentCpuArchitecture());
    }
    refreshAttachmentSummary();
}

void FeedbackForm::refreshAttachmentSummary()
{
    const bool canSendLogs = static_cast<bool>(m_context.diagnostics);

    // Hidden data is the thing this section exists to prevent, so the option is
    // disabled rather than quietly absent: the user learns that logs exist and
    // that this application does not offer them.
    m_includeLogs->setEnabled(canSendLogs && m_state != State::Sending
                              && m_state != State::Sent);
    if (!canSendLogs && m_includeLogs->isChecked()) {
        m_includeLogs->setChecked(false);
    }

    QStringList lines;
    if (m_includeInfo->isChecked()) {
        const auto add = [&lines](const QString& name, const QString& value) {
            if (!value.trimmed().isEmpty()) {
                lines.append(QStringLiteral("%1: %2").arg(name, value));
            }
        };
        add(tr("Application"), m_context.application);
        add(tr("Version"), m_context.version);
        add(tr("Platform"), m_context.platform);
        add(tr("Workspace"), m_context.workspace);
    }
    m_attachmentSummary->setText(
        lines.isEmpty() ? tr("Nothing about the application will be sent.")
                        : lines.join(QStringLiteral("  ·  ")));

    if (!canSendLogs) {
        m_logsHint->setText(tr("This application does not provide logs to attach."));
    } else if (m_includeLogs->isChecked()) {
        // Measured, not promised. "Recent logs" could be a line or a megabyte,
        // and the user is entitled to know which before sending it.
        const QString diagnostics = m_context.diagnostics();
        const int lineCount = diagnostics.isEmpty()
            ? 0
            : diagnostics.count(QLatin1Char('\n')) + 1;
        m_logsHint->setText(tr("%1 lines, %2 KB will be attached.")
                                .arg(lineCount)
                                .arg(QString::number(diagnostics.size() / 1024.0, 'f', 1)));
    } else {
        m_logsHint->setText(tr("No logs will be attached."));
    }
}

QString FeedbackForm::validationProblem() const
{
    if (m_title->text().trimmed().isEmpty()) {
        return tr("A title is needed — one line saying what this is about.");
    }
    if (m_description->toPlainText().trimmed().isEmpty()) {
        return tr("A description is needed. Without it a report is rarely actionable.");
    }
    return QString();
}

FeedbackRequest FeedbackForm::request() const
{
    FeedbackRequest request;
    const int index = m_type->current();
    request.type = m_typeOrder.value(index, FeedbackType::Bug);
    request.title = m_title->text().trimmed();
    request.description = m_description->toPlainText().trimmed();

    request.moduleId = selectedModuleId();
    request.contextLabel = m_context.contextLabel;
    request.includeScreenshot = m_includeScreenshot->isChecked()
        && m_includeScreenshot->isEnabled();
    request.includeApplicationInfo = m_includeInfo->isChecked();
    if (request.includeApplicationInfo) {
        request.application = m_context.application;
        request.version = m_context.version;
        request.platform = m_context.platform;
        request.workspace = m_context.workspace;
    }

    request.includeLogs = m_includeLogs->isChecked() && m_includeLogs->isEnabled();
    if (request.includeLogs && m_context.diagnostics) {
        request.diagnosticData = m_context.diagnostics();
    }
    return request;
}

void FeedbackForm::onSubmit()
{
    // The double-submit guard. Not the button's disabled state alone: a return
    // key, a shortcut or a second click delivered before the repaint would all
    // arrive here, and only one of them may become a request.
    if (m_state == State::Sending || m_state == State::Sent) {
        return;
    }

    const QString problem = validationProblem();
    if (!problem.isEmpty()) {
        m_banner->setTone(Badge::Tone::Danger);
        m_banner->setText(problem);
        setState(State::Invalid);
        (m_title->text().trimmed().isEmpty() ? static_cast<QWidget*>(m_title)
                                             : static_cast<QWidget*>(m_description))
            ->setFocus();
        return;
    }

    const FeedbackRequest payload = request();
    setState(State::Sending);
    emit submitted(payload);
}

void FeedbackForm::reportSuccess(const QString& message)
{
    // Only a form that is waiting can be answered. A reply arriving after the
    // user cancelled must not put a closed form back into a sent state.
    if (m_state != State::Sending) {
        return;
    }
    m_banner->setTone(Badge::Tone::Success);
    m_banner->setText(message.isEmpty() ? tr("Thank you — your feedback was sent.") : message);
    setState(State::Sent);
}

void FeedbackForm::reportFailure(const QString& message)
{
    if (m_state != State::Sending) {
        return;
    }
    m_banner->setTone(Badge::Tone::Danger);
    m_banner->setText(message.isEmpty()
                          ? tr("The report could not be sent. You can try again.")
                          : message);
    setState(State::Failed);
}

void FeedbackForm::reset()
{
    m_title->clear();
    m_description->clear();
    m_type->setCurrent(0);
    m_includeInfo->setChecked(true);
    m_includeLogs->setChecked(false);
    m_banner->hide();
    setState(State::Empty);
}

void FeedbackForm::setState(State state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    applyState();
    emit stateChanged(m_state);
}

void FeedbackForm::applyState()
{
    const bool sending = m_state == State::Sending;
    const bool sent = m_state == State::Sent;
    const bool locked = sending || sent;

    // Locked rather than merely un-clickable: a field the user can still edit
    // while a send is in flight is a field whose value no longer matches what
    // was sent.
    m_type->setEnabled(!locked);
    m_title->setEnabled(!locked);
    m_description->setEnabled(!locked);
    m_includeInfo->setEnabled(!locked);

    m_send->setEnabled(!locked);
    m_send->setText(sending ? tr("Sending…") : tr("Send feedback"));
    // Once it has gone, the affirmative action is gone with it and the only
    // thing left to do is leave.
    m_cancel->setText(sent ? tr("Close") : tr("Cancel"));
    m_send->setVisible(!sent);

    m_banner->setVisible(m_state == State::Invalid || m_state == State::Failed || sent
                         || sending);
    if (sending) {
        m_banner->setTone(Badge::Tone::Info);
        m_banner->setText(tr("Sending your feedback…"));
    }

    refreshAttachmentSummary();
}

// -- FeedbackDialog ----------------------------------------------------------

void FeedbackForm::setModules(const QVector<FeedbackModuleOption>& modules,
                              const QString& preselectedId)
{
    m_modules = modules;
    m_module->clear();
    for (const FeedbackModuleOption& option : m_modules) {
        m_module->addItem(option.label);
    }

    // One option is not a choice, and none is not a module model.
    m_moduleRow->setVisible(m_modules.size() > 1);

    if (!preselectedId.isEmpty()) {
        for (int i = 0; i < m_modules.size(); ++i) {
            if (m_modules.at(i).id == preselectedId) {
                m_module->setCurrentIndex(i);
                break;
            }
        }
    }
}

QString FeedbackForm::selectedModuleId() const
{
    const int index = m_module->currentIndex();
    if (index < 0 || index >= m_modules.size()) {
        return {};
    }
    return m_modules.at(index).id;
}

void FeedbackForm::setScreenshotAvailable(bool available)
{
    m_screenshotAvailable = available;
    m_includeScreenshot->setVisible(available);
    m_includeScreenshot->setEnabled(available);
    if (!available) {
        m_includeScreenshot->setChecked(false);
    }
}

FeedbackDialog::FeedbackDialog(const FeedbackContext& context, QWidget* parent,
                               const QVector<FeedbackType>& types)
    : Dialog(tr("Send feedback"), parent)
    , m_form(new FeedbackForm(nullptr, types))
{
    const ThemeProfile& profile = themeFor(this);
    const int padding = profile.space(SpacingRole::CardPadding);
    contentLayout()->setContentsMargins(padding, padding, padding, padding);

    m_form->setContext(context);
    setContentWidget(m_form);
    // Resizable, because a description is the one field in this window a user
    // may want more room for.
    shell()->setResizable(true);
    shell()->setContentSize(QSize(460, 520));

    connect(m_form, &FeedbackForm::cancelled, this, &QDialog::reject);
    connect(m_form, &FeedbackForm::stateChanged, this,
            [this](FeedbackForm::State state) {
                // The dialog closes on the user's word, not on the send: after
                // a success the form turns Cancel into Close, and that is what
                // ends it.
                if (state == FeedbackForm::State::Sent) {
                    setResult(QDialog::Accepted);
                }
            });
}

} // namespace novu::ui
