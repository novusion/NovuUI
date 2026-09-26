#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/Field.h"
#include "novu/ui/Fields.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/StyleSheet.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);
    auto inputProfile=novusionBaseProfile();inputProfile.id="input-tokens-gallery";inputProfile.colors.inputBorder=inputProfile.colors.borderHover;inputProfile.colors.inputHoverBorder=inputProfile.colors.textDisabled;inputProfile.colors.inputFocusBg=inputProfile.colors.panelBgAlt;inputProfile.colors.inputInk=inputProfile.colors.textSecondary;ThemeManager::instance().registerProfile(inputProfile);
    auto* inputCard=exampleCard(page,QStringLiteral("Input-specific tokens"),QStringLiteral("The input owns its outline and focused surface without changing adjacent cards."));applyStyleSheet(inputCard,inputProfile.id);inputCard->body()->addWidget(lineEdit(QStringLiteral("Focus to inspect the surface"),inputCard));column->addWidget(inputCard);
    const bool disabledScenario = ctx.scenario == Scenario::Disabled;

    // ── the three states ─────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("States"),
                                 QStringLiteral("A caption, the control, and one line "
                                                "underneath — hint or error, never both."));
        QVBoxLayout* body = card->body();

        field(body, card, QStringLiteral("Workspace name"),
              lineEdit(QStringLiteral("Trading desk"), card));

        body->addSpacing(14);
        field(body, card, QStringLiteral("API key"),
              lineEdit(QStringLiteral("kraken-…"), card),
              QStringLiteral("Read-only keys are enough for market data. A key with "
                             "withdrawal rights is never needed and never asked for."));

        body->addSpacing(14);
        auto* bad = field(body, card, QStringLiteral("Poll interval"),
                          lineEdit(QStringLiteral("seconds"), card, 0,
                                   QStringLiteral("0")),
                          QStringLiteral("How often the panel asks for prices."));
        bad->setError(QStringLiteral("Must be at least one second. Zero would ask the "
                                     "exchange as fast as the network allows, and the "
                                     "exchange will stop answering."));

        note(body, card,
             QStringLiteral("The error replaces the hint rather than joining it: two "
                            "sentences under one control is how a form grows taller than "
                            "the screen, and the hint is advice the error has already "
                            "superseded."));
        column->addWidget(card);
    }

    // ── it really validates ──────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Live"),
                                 QStringLiteral("A real check on a real control — type "
                                                "in it."));
        QVBoxLayout* body = card->body();

        auto* input = lineEdit(QStringLiteral("name@example.com"), card);
        auto* email = field(body, card, QStringLiteral("Notify"), input,
                            QStringLiteral("Where a finished backtest is announced."));
        email->setEnabled(!disabledScenario);

        auto* state = label(QStringLiteral("valid"), card, TypeRole::Mono, Ink::Success);
        QObject::connect(email, &Field::validityChanged, state, [state, card](bool valid) {
            state->setText(valid ? QStringLiteral("valid") : QStringLiteral("invalid"));
            setInk(state, valid ? Ink::Success : Ink::Danger);
            Q_UNUSED(card);
        });

        auto* check = new Button(QStringLiteral("Check"), Button::Variant::Secondary, card);
        check->setControlSize(ControlSize::Small);
        QObject::connect(check, &Button::clicked, email, [email, input] {
            const QString value = input->text().trimmed();
            if (value.isEmpty()) {
                email->setError(QStringLiteral("Enter an address, or turn the "
                                               "notification off."));
            } else if (!value.contains(QLatin1Char('@'))) {
                email->setError(QStringLiteral("That is not an address — it has no @."));
            } else {
                email->clearError();
            }
        });

        auto* row = new QHBoxLayout;
        row->setSpacing(themeFor(card).space(SpacingRole::ToolbarGap));
        row->addWidget(check);
        row->addWidget(state);
        row->addStretch(1);
        body->addLayout(row);

        note(body, card,
             QStringLiteral("Press Check with the box empty, then start typing. The message "
                            "clears on the first keystroke, because a message that stays "
                            "after the problem is fixed teaches people to ignore messages. "
                            "validityChanged() is what a dialog connects its submit button "
                            "to."));
        column->addWidget(card);
    }

    // ── across the controls ──────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Any control"),
                                 QStringLiteral("The mark is a property the scope sheet "
                                                "reads, not a border written on a widget."));
        QVBoxLayout* body = card->body();

        auto* combo = field(body, card, QStringLiteral("Exchange"),
                            comboBox({QStringLiteral("Choose…"), QStringLiteral("Kraken"),
                                      QStringLiteral("Binance")}, card));
        combo->setError(QStringLiteral("Pick one."));

        body->addSpacing(14);
        auto* area = field(body, card, QStringLiteral("Strategy note"),
                           textArea(QStringLiteral("What this bot is for"), card, 3));
        area->setError(QStringLiteral("Say what it is for — in six months this is the only "
                                      "thing that will explain it."));

        note(body, card,
             QStringLiteral("Line edit, combo box, text area and date edit all take the same "
                            "danger outline from one rule in the scope sheet, so a theme "
                            "change re-resolves it and nothing here holds a colour."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec fieldComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("field");
    spec.name = QStringLiteral("Field");
    spec.category = Category::Input;
    spec.description = QStringLiteral(
        "One labelled input and what is wrong with it. Caption, control, and a hint that "
        "an error replaces. Caption and message are painted; only the control is a widget.");
    spec.headerPath = QStringLiteral("include/novu/ui/Field.h");
    spec.sourcePath = QStringLiteral("src/Field.cpp");
    spec.designedFor = {QStringLiteral("novuspace"), QStringLiteral("novuadmin"),
                   QStringLiteral("novuai")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Hint"), QStringLiteral("Error"), QStringLiteral("Bare")};
    spec.states = {QStringLiteral("Valid"), QStringLiteral("Invalid"),
                   QStringLiteral("Disabled")};
    spec.keywords = {QStringLiteral("validation"), QStringLiteral("form"),
                     QStringLiteral("label"), QStringLiteral("error"),
                     QStringLiteral("hint"), QStringLiteral("input")};
    spec.properties = {
        {QStringLiteral("caption"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("Painted above the control, uppercase and tracked.")},
        {QStringLiteral("hint"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("The line under the control while nothing is wrong.")},
        {QStringLiteral("error"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("Replaces the hint and marks the control. Empty clears.")},
        {QStringLiteral("clearsErrorOnEdit"), QStringLiteral("bool"), {},
         QStringLiteral("true"),
         QStringLiteral("Off only for a field whose validity the server decides.")},
    };
    spec.scenarios = {Scenario::Normal, Scenario::Disabled};
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
