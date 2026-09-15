#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Card.h"
#include "novu/ui/Segmented.h"
#include "novu/ui/Separator.h"
#include "novu/ui/Text.h"
#include "novu/ui/Toggle.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

QWidget* buildToggle(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);
    const bool disabledScenario = ctx.scenario == Scenario::Disabled;

    // ── states ───────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("States"),
                                 QStringLiteral("Off, on, and neither of them "
                                                "available."));
        QVBoxLayout* body = card->body();
        QGridLayout* grid = matrix(body, card,
                                   {QStringLiteral("Off"), QStringLiteral("On"),
                                    QStringLiteral("Focused"), QStringLiteral("Disabled")},
                                   {QStringLiteral("Switch")});

        auto* off = new Toggle(card);
        matrixCell(grid, 0, 0, off);

        auto* on = new Toggle(card);
        on->setChecked(true);
        matrixCell(grid, 0, 1, on);

        auto* focused = new Toggle(card);
        focused->setChecked(true);
        focused->setFocus(Qt::TabFocusReason);
        matrixCell(grid, 0, 2, focused);

        auto* disabled = new Toggle(card);
        disabled->setChecked(true);
        disabled->setEnabled(false);
        matrixCell(grid, 0, 3, disabled);

        note(body, card,
             QStringLiteral("Click one. The knob slides rather than jumping, because on a "
                            "control with no other feedback the movement is what says the "
                            "change was accepted. A profile with motion switched off gets the "
                            "same two positions with no travel."));
        column->addWidget(card);
    }

    // ── settings rows ────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("In a settings column"),
                                 QStringLiteral("toggleRow(): what every settings screen in "
                                                "every module is made of."));
        QVBoxLayout* body = card->body();

        const struct { const char* caption; const char* hint; bool on; } rows[] = {
            {"Start with the system", "NovuSpace opens when you sign in, and waits in the "
                                      "tray until you need it.", true},
            {"Live market data", "Streams prices while a trading panel is open. Off, the "
                                 "panel refreshes when you ask it to.", true},
            {"Send diagnostics", "Crash reports and timing, with no document contents and no "
                                 "identifiers.", false},
            {"Confirm destructive actions", "Ask before deleting a workspace, a vault entry "
                                            "or a running bot.", true},
        };
        bool first = true;
        for (const auto& row : rows) {
            if (!first) {
                separator(body, card);
            }
            first = false;
            Toggle* toggle = toggleRow(body, card, QString::fromLatin1(row.caption),
                                       QString::fromLatin1(row.hint), row.on);
            toggle->setEnabled(!disabledScenario);
        }

        note(body, card,
             QStringLiteral("The switch is top-aligned against the caption, so a hint that "
                            "wraps to three lines does not leave it floating in the middle of "
                            "the row."));
        column->addWidget(card);
    }

    // ── what it is not ───────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Not a check box"),
                                 QStringLiteral("The distinction is when the change takes "
                                                "effect."));
        QVBoxLayout* body = card->body();
        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Switch"));
        auto* live = new Toggle(card);
        live->setChecked(true);
        row->addWidget(live);
        row->addWidget(label(QStringLiteral("takes effect now"), card,
                             TypeRole::Caption, Ink::Muted));
        row->addStretch(1);
        note(body, card,
             QStringLiteral("A switch states something that is already true; a check box "
                            "states a choice a form will submit later. Using one for the "
                            "other is why a settings page grows a Save button it does not "
                            "need — or loses one it does."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

QWidget* buildSegmented(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);
    const bool disabledScenario = ctx.scenario == Scenario::Disabled;

    // ── words ────────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Words"),
                                 QStringLiteral("The design's column presets, which is where "
                                                "this control comes from."));
        QVBoxLayout* body = card->body();

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Density"));
        auto* density = new Segmented({QStringLiteral("Basic"), QStringLiteral("Extended"),
                                       QStringLiteral("Pro"), QStringLiteral("MAX")},
                                      card, 1);
        // MAX is the wide one: choosing it has a cost, and the design marks it
        // whether or not it is the one in force.
        density->setSegmentAttention(3, true);
        density->setEnabled(!disabledScenario);
        row->addWidget(density);

        auto* echo = label(QStringLiteral("Extended"), card, TypeRole::Mono, Ink::Muted);
        QObject::connect(density, &Segmented::currentIndexChanged, echo,
                         [density, echo](int index) {
                             echo->setText(density->segmentText(index));
                         });
        row->addSpacing(12);
        row->addWidget(echo);
        row->addStretch(1);

        QHBoxLayout* two = exampleRow(body, card, QStringLiteral("Two segments"));
        two->addWidget(new Segmented({QStringLiteral("Paper"), QStringLiteral("Live")}, card));
        two->addStretch(1);

        note(body, card,
             QStringLiteral("Click a segment and the label beside it follows — it is a real "
                            "signal, not a painted state. Arrow keys move between segments; "
                            "the tray is one tab stop, the way a radio group is."));
        column->addWidget(card);
    }

    // ── glyphs ───────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Glyphs"),
                                 QStringLiteral("The same control with icons instead of "
                                                "words — not a second class."));
        QVBoxLayout* body = card->body();
        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("View mode"));

        auto* view = new Segmented(card);
        view->addSegment(Glyph::Overview, QStringLiteral("Grid"));
        view->addSegment(Glyph::Orders, QStringLiteral("List"));
        view->addSegment(Glyph::Chart, QStringLiteral("Chart"));
        view->setEnabled(!disabledScenario);
        row->addWidget(view);

        auto* small = new Segmented(card);
        small->addSegment(Glyph::Overview, QStringLiteral("Grid"));
        small->addSegment(Glyph::Orders, QStringLiteral("List"));
        small->setControlSize(ControlSize::Small);
        row->addSpacing(12);
        row->addWidget(small);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Hover a glyph segment for its tooltip. There is no child widget "
                            "to carry one — the segments are painted, so the control answers "
                            "for them itself."));
        column->addWidget(card);
    }

    // ── what it costs ────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("One widget, four segments"),
                                 QStringLiteral("Where the paint/widget line falls."));
        QVBoxLayout* body = card->body();
        QHBoxLayout* row = exampleRow(body, card, QString());
        row->addWidget(new Segmented({QStringLiteral("Day"), QStringLiteral("Week"),
                                      QStringLiteral("Month"), QStringLiteral("Year")},
                                     card, 2));
        row->addStretch(1);
        note(body, card,
             QStringLiteral("Four labels nobody can select, focus or drag are four paint "
                            "calls, not four QPushButtons. The control keeps the interaction "
                            "— press, release, hover, tooltips, arrow keys — because that is "
                            "the half that needs a real widget."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec toggleComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("toggle");
    spec.name = QStringLiteral("Toggle");
    spec.category = Category::Input;
    spec.description = QStringLiteral(
        "The switch: something is on, or it is off, and changing it takes effect at once. "
        "Self-painted, with the knob sliding on Qt's shared animation driver.");
    spec.headerPath = QStringLiteral("include/novu/ui/Toggle.h");
    spec.sourcePath = QStringLiteral("src/Toggle.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Off"), QStringLiteral("On")};
    spec.states = {QStringLiteral("Normal"), QStringLiteral("Hover"), QStringLiteral("Focused"),
                   QStringLiteral("Disabled")};
    spec.keywords = {QStringLiteral("switch"), QStringLiteral("on"), QStringLiteral("off"),
                     QStringLiteral("setting"), QStringLiteral("boolean")};
    spec.properties = {
        {QStringLiteral("checked"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("The state it reports, and the one it slides to.")},
        {QStringLiteral("knobPosition"), QStringLiteral("double"), {}, QStringLiteral("0"),
         QStringLiteral("Read-only: 0 off, 1 on. The seam a test asserts travel on.")},
    };
    spec.scenarios = {Scenario::Normal, Scenario::Disabled};
    spec.build = buildToggle;
    return spec;
}

ComponentSpec segmentedComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("segmented");
    spec.name = QStringLiteral("Segmented");
    spec.category = Category::Input;
    spec.description = QStringLiteral(
        "Mutually exclusive choices in one tray, one of them in force. Words or glyphs, "
        "painted segments with real press/release hit testing.");
    spec.headerPath = QStringLiteral("include/novu/ui/Segmented.h");
    spec.sourcePath = QStringLiteral("src/Segmented.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Words"), QStringLiteral("Glyphs"),
                     QStringLiteral("Small"), QStringLiteral("Medium")};
    spec.states = {QStringLiteral("Normal"), QStringLiteral("Hover"), QStringLiteral("Pressed"),
                   QStringLiteral("Focused"), QStringLiteral("Disabled")};
    spec.keywords = {QStringLiteral("chipset"), QStringLiteral("segmented"),
                     QStringLiteral("picker"), QStringLiteral("mode"),
                     QStringLiteral("density"), QStringLiteral("tabs")};
    spec.properties = {
        {QStringLiteral("currentIndex"), QStringLiteral("int"), {}, QStringLiteral("0"),
         QStringLiteral("Which segment is in force.")},
        {QStringLiteral("controlSize"), QStringLiteral("enum"),
         {QStringLiteral("Small"), QStringLiteral("Medium")}, QStringLiteral("Medium"),
         QStringLiteral("Steps type and glyph together.")},
        {QStringLiteral("segmentAttention"), QStringLiteral("bool"), {},
         QStringLiteral("false"),
         QStringLiteral("Marks a choice that has a cost, current or not.")},
    };
    spec.scenarios = {Scenario::Normal, Scenario::Disabled};
    spec.build = buildSegmented;
    return spec;
}

} // namespace novu::playground::pages
