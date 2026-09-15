#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Avatar.h"
#include "novu/ui/Card.h"
#include "novu/ui/Separator.h"
#include "novu/ui/Slider.h"
#include "novu/ui/StatusDot.h"
#include "novu/ui/Swatch.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

QWidget* personRow(QWidget* parent, const QString& name, const QString& role,
                   StatusDot::Tone presence, bool live)
{
    auto* row = new QWidget(parent);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(themeFor(parent).space(SpacingRole::ToolbarGap));

    layout->addWidget(new Avatar(name, 32, row));
    auto* text = new QVBoxLayout;
    text->setSpacing(0);
    text->addWidget(label(name, row, TypeRole::Body));
    text->addWidget(label(role, row, TypeRole::Caption, Ink::Muted));
    layout->addLayout(text, 1);
    layout->addWidget(new StatusDot(presence, live, row));
    return row;
}

// ── Avatar ────────────────────────────────────────────────────────────────

QWidget* buildAvatar(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    {
        Card* card = exampleCard(page, QStringLiteral("Initials"),
                                 QStringLiteral("Taken from the name, and so is the colour."));
        QVBoxLayout* body = card->body();
        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Six people"));
        for (const QString& name : {QStringLiteral("Petr Dostál"),
                                    QStringLiteral("Anna Nováková"),
                                    QStringLiteral("kai.lindgren@novusion.io"),
                                    QStringLiteral("Wu"), QStringLiteral("Marta Šimek"),
                                    QStringLiteral("build-agent-04")}) {
            row->addWidget(new Avatar(name, 40, card));
        }
        row->addStretch(1);
        note(body, card,
             QStringLiteral("The plate is derived from the name by summing its code points "
                            "into the profile's own chart series — not qHash, which Qt salts "
                            "per process and which would give the same person a different "
                            "colour on every run. Nothing stores a colour, and the same "
                            "person is the same colour in every window."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("Sizes and shape"),
                                 QStringLiteral("A person is round; a workspace or a machine "
                                                "is not."));
        QVBoxLayout* body = card->body();

        QHBoxLayout* sizes = exampleRow(body, card, QStringLiteral("20 · 28 · 40 · 64"));
        for (const int size : {20, 28, 40, 64}) {
            sizes->addWidget(new Avatar(QStringLiteral("Anna Nováková"), size, card));
        }
        sizes->addStretch(1);

        QHBoxLayout* shapes = exampleRow(body, card, QStringLiteral("Rounded, and highlighted"));
        auto* square = new Avatar(QStringLiteral("Trading desk"), 40, card);
        square->setShape(Avatar::Shape::Rounded);
        shapes->addWidget(square);
        auto* lit = new Avatar(QStringLiteral("Petr Dostál"), 40, card);
        lit->setHighlighted(true);
        shapes->addWidget(lit);
        shapes->addStretch(1);

        note(body, card,
             QStringLiteral("The type scales with the avatar, so one component covers a 20px "
                            "row mark and a 64px account portrait without a size table."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("In a list"),
                                 QStringLiteral("What it is actually for."));
        QVBoxLayout* body = card->body();
        body->addWidget(personRow(card, QStringLiteral("Petr Dostál"),
                                  QStringLiteral("owner · here"), StatusDot::Tone::Success, true));
        separator(body, card);
        body->addWidget(personRow(card, QStringLiteral("Anna Nováková"),
                                  QStringLiteral("editor · away"), StatusDot::Tone::Warning, true));
        separator(body, card);
        body->addWidget(personRow(card, QStringLiteral("kai.lindgren@novusion.io"),
                                  QStringLiteral("viewer · offline"), StatusDot::Tone::Neutral,
                                  false));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

// ── Slider ────────────────────────────────────────────────────────────────

QWidget* buildSlider(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);
    const bool disabledScenario = ctx.scenario == Scenario::Disabled;

    {
        Card* card = exampleCard(page, QStringLiteral("A value, and the value"),
                                 QStringLiteral("The readout is part of the control."));
        QVBoxLayout* body = card->body();

        QHBoxLayout* plain = exampleRow(body, card, QStringLiteral("Plain"));
        auto* one = new Slider(0, 100, card);
        one->setValue(62);
        one->setEnabled(!disabledScenario);
        plain->addWidget(one, 1);

        QHBoxLayout* formatted = exampleRow(body, card, QStringLiteral("With a unit"));
        auto* two = new Slider(50, 200, card);
        two->setValue(125);
        two->setFormatter([](int v) { return QStringLiteral("%1 %").arg(v); });
        two->setEnabled(!disabledScenario);
        formatted->addWidget(two, 1);

        QHBoxLayout* quiet = exampleRow(body, card, QStringLiteral("No readout"));
        auto* three = new Slider(0, 10, card);
        three->setValue(3);
        three->setShowsReadout(false);
        three->setEnabled(!disabledScenario);
        quiet->addWidget(three, 1);

        note(body, card,
             QStringLiteral("Drag one, or click anywhere on the groove — it jumps to where "
                            "you pointed rather than paging towards it, because Qt's default "
                            "is a hold-over from scroll bars and means two clicks to reach a "
                            "value the user already indicated. The filled half is what says "
                            "which way the value goes."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("In a settings column"),
                                 QStringLiteral("sliderRow(): the same shape toggleRow() "
                                                "gives a switch."));
        QVBoxLayout* body = card->body();
        sliderRow(body, card, QStringLiteral("Interface scale"), 80, 150, 100,
                  QStringLiteral("Everything in the window, including the title bar."))
            ->setFormatter([](int v) { return QStringLiteral("%1 %").arg(v); });
        separator(body, card);
        sliderRow(body, card, QStringLiteral("History kept"), 1, 90, 30,
                  QStringLiteral("Older entries are discarded when the application starts."))
            ->setFormatter([](int v) { return QStringLiteral("%1 d").arg(v); });
        note(body, card,
             QStringLiteral("A settings page of switches and sliders lines up because both "
                            "rows are built the same way."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

// ── Swatch ────────────────────────────────────────────────────────────────

QWidget* buildSwatch(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    {
        Card* card = exampleCard(page, QStringLiteral("Tagging"),
                                 QStringLiteral("One answer: what colour is this folder?"));
        QVBoxLayout* body = card->body();

        auto* one = new Swatch(card);
        one->setColors(tagPalette(card), true, true);
        one->setCurrent(tagPalette(card).value(2));
        body->addWidget(one);

        auto* chosen = label(QStringLiteral("picked: —"), card, TypeRole::Mono, Ink::Muted);
        QObject::connect(one, &Swatch::picked, chosen, [chosen](const QColor& c) {
            chosen->setText(c.isValid() ? QStringLiteral("picked: %1").arg(c.name())
                                        : QStringLiteral("picked: none"));
        });
        QObject::connect(one, &Swatch::customRequested, chosen,
                         [chosen] { chosen->setText(QStringLiteral("picked: custom…")); });
        body->addSpacing(8);
        body->addWidget(chosen);

        note(body, card,
             QStringLiteral("The \"no colour\" swatch is part of the row rather than a button "
                            "beside it, and the multi-hued one asks rather than picking — a "
                            "colour dialog is the application's to choose."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("Filtering"),
                                 QStringLiteral("Many answers: show me any of these."));
        QVBoxLayout* body = card->body();

        auto* many = new Swatch(card);
        many->setMultiSelect(true);
        many->setColors(tagPalette(card), true, false);
        many->setChosen({tagPalette(card).value(0), tagPalette(card).value(3)});
        body->addWidget(many);

        auto* state = label(QStringLiteral("2 colours"), card, TypeRole::Mono, Ink::Muted);
        QObject::connect(many, &Swatch::chosenChanged, state,
                         [state](const QList<QColor>& colors) {
                             state->setText(colors.isEmpty()
                                                ? QStringLiteral("no filter")
                                                : QStringLiteral("%1 colours").arg(colors.size()));
                         });
        body->addSpacing(8);
        body->addWidget(state);

        note(body, card,
             QStringLiteral("Same component, one flag. The design draws them identically "
                            "because they are the same row asked two questions, and the "
                            "\"no colour\" swatch here clears the filter rather than adding "
                            "an absence to it."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec avatarComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("avatar");
    spec.name = QStringLiteral("Avatar");
    spec.category = Category::DataDisplay;
    spec.description = QStringLiteral(
        "Who someone is, in the space of one glyph. Portrait when there is one, initials "
        "when there is not, and a plate colour derived from the name so nothing stores one.");
    spec.headerPath = QStringLiteral("include/novu/ui/Avatar.h");
    spec.sourcePath = QStringLiteral("src/Avatar.cpp");
    spec.designedFor = {QStringLiteral("novuhub"), QStringLiteral("novuvoice"),
                   QStringLiteral("novuai")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Round"), QStringLiteral("Rounded"),
                     QStringLiteral("Portrait"), QStringLiteral("Initials")};
    spec.states = {QStringLiteral("Normal"), QStringLiteral("Highlighted")};
    spec.keywords = {QStringLiteral("person"), QStringLiteral("user"),
                     QStringLiteral("initials"), QStringLiteral("portrait"),
                     QStringLiteral("account"), QStringLiteral("face")};
    spec.properties = {
        {QStringLiteral("name"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("Initials and plate colour are both derived from it.")},
        {QStringLiteral("diameter"), QStringLiteral("int"), {}, QStringLiteral("32"),
         QStringLiteral("The type scales with it; there is no size table.")},
        {QStringLiteral("shape"), QStringLiteral("enum"),
         {QStringLiteral("Round"), QStringLiteral("Rounded")}, QStringLiteral("Round"),
         QStringLiteral("A person is round; a workspace or a machine is not.")},
    };
    spec.build = buildAvatar;
    return spec;
}

ComponentSpec sliderComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("slider");
    spec.name = QStringLiteral("Slider");
    spec.category = Category::Input;
    spec.description = QStringLiteral(
        "A value chosen along a range, with the value shown beside it. Self-painted, "
        "click-to-position, still a QSlider underneath.");
    spec.headerPath = QStringLiteral("include/novu/ui/Slider.h");
    spec.sourcePath = QStringLiteral("src/Slider.cpp");
    spec.designedFor = {QStringLiteral("novuhub"), QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Readout"), QStringLiteral("Formatted"),
                     QStringLiteral("Bare")};
    spec.states = {QStringLiteral("Normal"), QStringLiteral("Hover"),
                   QStringLiteral("Focused"), QStringLiteral("Disabled")};
    spec.keywords = {QStringLiteral("range"), QStringLiteral("scale"),
                     QStringLiteral("value"), QStringLiteral("setting"),
                     QStringLiteral("volume"), QStringLiteral("zoom")};
    spec.properties = {
        {QStringLiteral("value"), QStringLiteral("int"), {}, QStringLiteral("0"),
         QStringLiteral("QSlider's own, with all its keyboard and wheel behaviour.")},
        {QStringLiteral("formatter"), QStringLiteral("std::function"), {}, QString(),
         QStringLiteral("How the readout is written — percent, seconds, pixels.")},
        {QStringLiteral("showsReadout"), QStringLiteral("bool"), {}, QStringLiteral("true"),
         QStringLiteral("Off only when the value is shown elsewhere in the row.")},
    };
    spec.scenarios = {Scenario::Normal, Scenario::Disabled};
    spec.build = buildSlider;
    return spec;
}

ComponentSpec swatchComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("swatch");
    spec.name = QStringLiteral("Swatch");
    spec.category = Category::Input;
    spec.description = QStringLiteral(
        "A row of colours, one chosen — or several, when it is a filter. Painted with hit "
        "testing; wraps, and reports its height for a given width.");
    spec.headerPath = QStringLiteral("include/novu/ui/Swatch.h");
    spec.sourcePath = QStringLiteral("src/Swatch.cpp");
    spec.designedFor = {QStringLiteral("novuexplorer")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Single"), QStringLiteral("Multi"),
                     QStringLiteral("With none"), QStringLiteral("With custom")};
    spec.states = {QStringLiteral("Normal"), QStringLiteral("Hover"),
                   QStringLiteral("Chosen")};
    spec.keywords = {QStringLiteral("colour"), QStringLiteral("color"),
                     QStringLiteral("tag"), QStringLiteral("palette"),
                     QStringLiteral("filter"), QStringLiteral("pills")};
    spec.properties = {
        {QStringLiteral("multiSelect"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("A filter asks \"any of these\"; a tag asks \"which one\".")},
        {QStringLiteral("current"), QStringLiteral("QColor"), {}, QString(),
         QStringLiteral("Single select. An invalid colour means none.")},
        {QStringLiteral("chosen"), QStringLiteral("QList<QColor>"), {}, QString(),
         QStringLiteral("Multi select. Empty means no filter.")},
    };
    spec.build = buildSwatch;
    return spec;
}

} // namespace novu::playground::pages
