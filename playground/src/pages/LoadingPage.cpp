#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/Feedback.h"
#include "novu/ui/Separator.h"
#include "novu/ui/StatusDot.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

// ── Spinner ──────────────────────────────────────────────────────────────

QWidget* buildSpinner(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    {
        Card* card = exampleCard(page, QStringLiteral("Sizes"),
                                 QStringLiteral("From the profile's icon metrics, "
                                                "not from a pixel count."));
        QVBoxLayout* body = card->body();

        QGridLayout* grid = matrix(body, card,
                                   {QStringLiteral("Spinner"), QStringLiteral("Beside a glyph"),
                                    QStringLiteral("Pixels")},
                                   {QStringLiteral("Small"), QStringLiteral("Medium"),
                                    QStringLiteral("Large")});
        const ControlSize sizes[] = {ControlSize::Small, ControlSize::Medium,
                                     ControlSize::Large};
        for (int i = 0; i < 3; ++i) {
            matrixCell(grid, i, 0, new Spinner(sizes[i], card));

            auto* pair = new QWidget(card);
            auto* row = new QHBoxLayout(pair);
            row->setContentsMargins(0, 0, 0, 0);
            row->setSpacing(6);
            row->addWidget(new Spinner(sizes[i], pair));
            row->addWidget(new StatusDot(StatusDot::Tone::Success, true, pair));
            row->addStretch(1);
            matrixCell(grid, i, 1, pair);

            matrixCell(grid, i, 2,
                       label(QString::number(glyphSizeFor(card, sizes[i])), card,
                             TypeRole::Mono, Ink::Muted));
        }

        note(body, card,
             QStringLiteral("A spinner and the glyphs beside it are the same size at every "
                            "control size, because both ask the profile rather than naming a "
                            "number. A 14px spinner next to a 16px icon is the mismatch this "
                            "removes."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("Stopping"),
                                 QStringLiteral("The state that matters most."));
        QVBoxLayout* body = card->body();

        auto* spinner = new Spinner(ControlSize::Large, card);
        auto* state = label(QStringLiteral("spinning"), card, TypeRole::Mono, Ink::Secondary);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("An operation"));
        row->addWidget(spinner);
        row->addSpacing(8);
        row->addWidget(state);
        row->addSpacing(16);

        auto* toggle = new Button(QStringLiteral("Stop"), Button::Variant::Secondary, card);
        toggle->setControlSize(ControlSize::Small);
        QObject::connect(toggle, &Button::clicked, spinner, [spinner, state, toggle] {
            const bool running = !spinner->isSpinning();
            spinner->setSpinning(running);
            state->setText(running ? QStringLiteral("spinning") : QStringLiteral("stopped"));
            toggle->setText(running ? QStringLiteral("Stop") : QStringLiteral("Start"));
        });
        row->addWidget(toggle);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Stopping releases the clock subscription, not merely the "
                            "drawing. A spinner left running behind a finished operation is "
                            "the most common reason a window that looks idle still repaints "
                            "sixty times a second — so the component stops itself when told "
                            "to stop, when hidden, and when the profile disables motion."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("Inside a button"),
                                 QStringLiteral("The one reason setHeadColor() exists."));
        QVBoxLayout* body = card->body();

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Submitting"));

        auto* onPrimary = new Spinner(ControlSize::Small, card);
        onPrimary->setHeadColor(themeFor(card).colors.textOnAccent);
        row->addWidget(onPrimary);
        row->addWidget(label(QStringLiteral("head = textOnAccent"), card, TypeRole::Caption,
                             Ink::Muted));
        row->addSpacing(20);
        row->addWidget(new Spinner(ControlSize::Small, card));
        row->addWidget(label(QStringLiteral("head = accent (default)"), card,
                             TypeRole::Caption, Ink::Muted));
        row->addStretch(1);

        note(body, card,
             QStringLiteral("The default resolves the accent at paint time, so a theme change "
                            "moves it. An explicit head colour is held as an override rather "
                            "than as a resolved default, which is what lets the untouched case "
                            "still follow the theme while a deliberate one stays put."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

// ── Skeleton ─────────────────────────────────────────────────────────────

QWidget* buildSkeleton(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    {
        Card* card = exampleCard(page, QStringLiteral("Lines"),
                                 QStringLiteral("One plate, two heights, any width."));
        QVBoxLayout* body = card->body();

        const struct { const char* caption; Skeleton::Line line; double fill; } rows[] = {
            {"Text, full", Skeleton::Line::Text, 1.0},
            {"Text, 70%", Skeleton::Line::Text, 0.7},
            {"Text, 45%", Skeleton::Line::Text, 0.45},
            {"Block", Skeleton::Line::Block, 1.0},
            {"Block, 30%", Skeleton::Line::Block, 0.3},
        };
        for (const auto& row : rows) {
            QHBoxLayout* line = exampleRow(body, card, QString::fromLatin1(row.caption));
            auto* plate = new Skeleton(row.line, card);
            plate->setFill(row.fill);
            line->addWidget(plate, 1);
        }

        note(body, card,
             QStringLiteral("Vary the fill. Placeholder lines of identical length read as a "
                            "grid rather than as text, which is why the component takes a "
                            "fraction instead of always filling its box."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("A table, loading"),
                                 QStringLiteral("SkeletonList — one widget for the "
                                                "whole placeholder."));
        QVBoxLayout* body = card->body();

        auto* list = new SkeletonList(6, {3.0, 1.0, 1.0, 0.8}, card);
        body->addWidget(list);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Rows"));
        for (const int count : {3, 6, 12, 40}) {
            auto* button = new Button(QString::number(count), Button::Variant::Secondary, card);
            button->setControlSize(ControlSize::Small);
            QObject::connect(button, &Button::clicked, list,
                             [list, count] { list->setRowCount(count); });
            row->addWidget(button);
        }
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Forty rows costs exactly what six do: one widget, one clock "
                            "subscription, one paint. Built out of widgets the same placeholder "
                            "would be a hundred and twenty QWidgets created in order to be "
                            "thrown away as soon as the data arrives. The row height and the "
                            "separators come from the profile's table tokens, so the panel does "
                            "not jump when the real table replaces this."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("Which wait is it"),
                                 QStringLiteral("Three shapes, three different "
                                                "statements."));
        QVBoxLayout* body = card->body();

        QHBoxLayout* spin = exampleRow(body, card, QStringLiteral("Spinner"));
        spin->addWidget(new Spinner(ControlSize::Small, card));
        spin->addWidget(label(QStringLiteral("something is happening and I cannot say "
                                             "how long"),
                              card, TypeRole::Caption, Ink::Muted));
        spin->addStretch(1);

        QHBoxLayout* bar = exampleRow(body, card, QStringLiteral("ProgressBar"));
        auto* progress = new ProgressBar(62, card);
        progress->setFixedWidth(120);
        bar->addWidget(progress);
        bar->addWidget(label(QStringLiteral("this much of a known amount is done"), card,
                             TypeRole::Caption, Ink::Muted));
        bar->addStretch(1);

        QHBoxLayout* skeleton = exampleRow(body, card, QStringLiteral("Skeleton"));
        auto* plate = new Skeleton(Skeleton::Line::Text, card);
        plate->setFixedWidth(120);
        skeleton->addWidget(plate);
        skeleton->addWidget(label(QStringLiteral("the shape is known, only the content "
                                                 "is missing"),
                                  card, TypeRole::Caption, Ink::Muted));
        skeleton->addStretch(1);

        note(body, card,
             QStringLiteral("The difference is not decoration. A skeleton is right for a "
                            "table or a detail panel and wrong for a save, because it promises "
                            "a shape — and a save has no shape to promise."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

// ── StatusLine ───────────────────────────────────────────────────────────

QWidget* buildStatusLine(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    {
        Card* card = exampleCard(page, QStringLiteral("States"),
                                 QStringLiteral("Idle, running, done, failed."));
        QVBoxLayout* body = card->body();

        body->addWidget(new StatusLine(QStringLiteral("Waiting to start"),
                                       StatusLine::State::Idle, card));
        separator(body, card);
        body->addWidget(new StatusLine(QStringLiteral("Checking credentials"),
                                       StatusLine::State::Running, card));
        separator(body, card);
        body->addWidget(new StatusLine(QStringLiteral("Connected to wg-platform"),
                                       StatusLine::State::Done, card));
        separator(body, card);
        body->addWidget(new StatusLine(QStringLiteral("Handshake timed out after 15s"),
                                       StatusLine::State::Failed, card));

        note(body, card,
             QStringLiteral("Only the running line holds the clock. An idle, a done and a "
                            "failed line are static drawings, so a checklist of twenty steps "
                            "animates exactly one of them."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("A sequence"),
                                 QStringLiteral("The behaviour that makes this a "
                                                "component."));
        QVBoxLayout* body = card->body();

        auto* line = new StatusLine(QStringLiteral("Ready"), StatusLine::State::Idle, card);
        body->addWidget(line);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Run it"));
        auto* go = new Button(QStringLiteral("Connect"), Button::Variant::Primary, card);
        go->setControlSize(ControlSize::Small);
        QObject::connect(go, &Button::clicked, line, [line] {
            line->setState(StatusLine::State::Running,
                           QObject::tr("Resolving hub.novusion.net"));
            QTimer::singleShot(900, line, [line] {
                line->setState(StatusLine::State::Running,
                               QObject::tr("Exchanging keys"));
            });
            QTimer::singleShot(1900, line, [line] {
                line->setState(StatusLine::State::Done, QObject::tr("Connected"));
            });
        });
        row->addWidget(go);

        auto* fail = new Button(QStringLiteral("Fail it"), Button::Variant::Secondary, card);
        fail->setControlSize(ControlSize::Small);
        QObject::connect(fail, &Button::clicked, line, [line] {
            line->setState(StatusLine::State::Failed,
                           QObject::tr("No route to host"));
        });
        row->addWidget(fail);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("One row reporting on itself, rather than two labels a page swaps "
                            "over. setState() takes the text as well as the state because in "
                            "practice they always change together — two calls means one frame "
                            "showing the new mark against the old sentence."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

// ── Steps ────────────────────────────────────────────────────────────────

QWidget* buildSteps(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    const QStringList stages = {QStringLiteral("Validate"), QStringLiteral("Upload"),
                                QStringLiteral("Index"), QStringLiteral("Verify")};

    {
        Card* card = exampleCard(page, QStringLiteral("Where it is"),
                                 QStringLiteral("Done, running, pending — and "
                                                "finished."));
        QVBoxLayout* body = card->body();

        for (int at = 0; at <= stages.size(); ++at) {
            QHBoxLayout* row = exampleRow(
                body, card,
                at < stages.size() ? QStringLiteral("At \"%1\"").arg(stages.at(at))
                                   : QStringLiteral("Complete"));
            row->addWidget(new Steps(stages, card, at));
            row->addStretch(1);
        }

        note(body, card,
             QStringLiteral("One index rather than a state per step: a sequence that can be "
                            "done at step four and pending at step two is not a sequence. "
                            "Passing size() means finished — every pill done and none current, "
                            "which is why the setter does not clamp to size()-1."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("Driven"),
                                 QStringLiteral("With the line that says what the "
                                                "current stage is doing."));
        QVBoxLayout* body = card->body();

        auto* steps = new Steps(stages, card, 0);
        auto* line = new StatusLine(QStringLiteral("Validating manifest"),
                                    StatusLine::State::Running, card);
        body->addWidget(steps);
        body->addSpacing(6);
        body->addWidget(line);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Advance"));
        auto* next = new Button(QStringLiteral("Next stage"), Button::Variant::Secondary, card);
        next->setControlSize(ControlSize::Small);
        QObject::connect(next, &Button::clicked, steps, [steps, line, stages] {
            const int at = steps->currentIndex() + 1;
            steps->setCurrentIndex(at);
            if (steps->isComplete()) {
                line->setState(StatusLine::State::Done, QObject::tr("Import finished"));
            } else {
                line->setState(StatusLine::State::Running,
                               QObject::tr("%1…").arg(stages.at(at)));
            }
        });
        row->addWidget(next);

        auto* reset = new Button(QStringLiteral("Reset"), Button::Variant::Ghost, card);
        reset->setControlSize(ControlSize::Small);
        QObject::connect(reset, &Button::clicked, steps, [steps, line, stages] {
            steps->setCurrentIndex(0);
            line->setState(StatusLine::State::Running,
                           QObject::tr("%1…").arg(stages.first()));
        });
        row->addWidget(reset);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Steps says which part; StatusLine says what that part is doing. "
                            "\"Verifying\" and \"Uploading\" are not 40% and 70% of the same "
                            "bar, and a user watching a long import wants the name of the "
                            "stage — which is the question ProgressBar cannot answer."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec spinnerComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("spinner");
    spec.name = QStringLiteral("Spinner");
    spec.category = Category::Feedback;
    spec.description = QStringLiteral(
        "A faint ring with one bright quarter turning round it, for work whose length "
        "is not known. On the shared clock, and it stops when it is told to.");
    spec.headerPath = QStringLiteral("include/novu/ui/Feedback.h");
    spec.sourcePath = QStringLiteral("src/Feedback.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {QStringLiteral("novuspace")};
    spec.variants = {QStringLiteral("Small"), QStringLiteral("Medium"),
                     QStringLiteral("Large")};
    spec.states = {QStringLiteral("Spinning"), QStringLiteral("Stopped")};
    spec.keywords = {QStringLiteral("busy"), QStringLiteral("loading"),
                     QStringLiteral("progress"), QStringLiteral("spin"),
                     QStringLiteral("indeterminate"), QStringLiteral("sp")};
    spec.properties = {
        {QStringLiteral("controlSize"), QStringLiteral("enum"),
         {QStringLiteral("Small"), QStringLiteral("Medium"), QStringLiteral("Large")},
         QStringLiteral("Medium"),
         QStringLiteral("Resolved through the profile's icon metrics.")},
        {QStringLiteral("spinning"), QStringLiteral("bool"), {}, QStringLiteral("true"),
         QStringLiteral("False releases the clock subscription, not only the drawing.")},
        {QStringLiteral("headColor"), QStringLiteral("QColor"), {}, QStringLiteral("accent"),
         QStringLiteral("An override, for a spinner inside a Primary button.")},
    };
    spec.build = buildSpinner;
    return spec;
}

ComponentSpec skeletonComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("skeleton");
    spec.name = QStringLiteral("Skeleton");
    spec.category = Category::Feedback;
    spec.description = QStringLiteral(
        "Placeholder plates with the design's shimmer sweeping across them, for when the "
        "shape of what is coming is already known. SkeletonList draws a whole table in "
        "one widget.");
    spec.headerPath = QStringLiteral("include/novu/ui/Feedback.h");
    spec.sourcePath = QStringLiteral("src/Feedback.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {QStringLiteral("novuspace")};
    spec.variants = {QStringLiteral("Text"), QStringLiteral("Block"),
                     QStringLiteral("SkeletonList")};
    spec.states = {QStringLiteral("Shimmering"), QStringLiteral("Still")};
    spec.keywords = {QStringLiteral("placeholder"), QStringLiteral("shimmer"),
                     QStringLiteral("loading"), QStringLiteral("skel"),
                     QStringLiteral("ghost")};
    spec.properties = {
        {QStringLiteral("line"), QStringLiteral("enum"),
         {QStringLiteral("Text"), QStringLiteral("Block")}, QStringLiteral("Text"),
         QStringLiteral("One line of prose, or a value / heading / control.")},
        {QStringLiteral("fill"), QStringLiteral("double"), {}, QStringLiteral("1.0"),
         QStringLiteral("0..1 of the width. Vary it, or the plates read as a grid.")},
        {QStringLiteral("columns"), QStringLiteral("QVector<double>"), {},
         QStringLiteral("{3, 1, 1}"),
         QStringLiteral("SkeletonList only. Normalised, so {3,1,1} and {.6,.2,.2} agree.")},
    };
    spec.build = buildSkeleton;
    return spec;
}

ComponentSpec statusLineComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("status-line");
    spec.name = QStringLiteral("StatusLine");
    spec.category = Category::Feedback;
    spec.description = QStringLiteral(
        "A sentence about one operation with a mark in front of it saying where that "
        "operation is. The design's .sline.");
    spec.headerPath = QStringLiteral("include/novu/ui/Feedback.h");
    spec.sourcePath = QStringLiteral("src/Feedback.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {QStringLiteral("novuspace")};
    spec.variants = {QStringLiteral("Idle"), QStringLiteral("Running"),
                     QStringLiteral("Done"), QStringLiteral("Failed")};
    spec.states = {QStringLiteral("Idle"), QStringLiteral("Running"),
                   QStringLiteral("Done"), QStringLiteral("Failed")};
    spec.keywords = {QStringLiteral("sline"), QStringLiteral("task"),
                     QStringLiteral("operation"), QStringLiteral("checklist"),
                     QStringLiteral("step")};
    spec.properties = {
        {QStringLiteral("state"), QStringLiteral("enum"),
         {QStringLiteral("Idle"), QStringLiteral("Running"), QStringLiteral("Done"),
          QStringLiteral("Failed")},
         QStringLiteral("Idle"),
         QStringLiteral("Where the operation is. Only Running animates.")},
        {QStringLiteral("text"), QStringLiteral("QString"), {}, QStringLiteral(""),
         QStringLiteral("Set with the state, because they always change together.")},
    };
    spec.build = buildStatusLine;
    return spec;
}

ComponentSpec stepsComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("steps");
    spec.name = QStringLiteral("Steps");
    spec.category = Category::Feedback;
    spec.description = QStringLiteral(
        "Where a multi-part operation has got to, as a row of tracked pills. The one "
        "progress shape that says which part rather than how far.");
    spec.headerPath = QStringLiteral("include/novu/ui/Feedback.h");
    spec.sourcePath = QStringLiteral("src/Feedback.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {QStringLiteral("novuspace")};
    spec.variants = {QStringLiteral("Done"), QStringLiteral("Current"),
                     QStringLiteral("Pending")};
    spec.states = {QStringLiteral("In progress"), QStringLiteral("Complete")};
    spec.keywords = {QStringLiteral("wizard"), QStringLiteral("stages"),
                     QStringLiteral("stepper"), QStringLiteral("progress"),
                     QStringLiteral("sequence")};
    spec.properties = {
        {QStringLiteral("currentIndex"), QStringLiteral("int"), {}, QStringLiteral("0"),
         QStringLiteral("count() is legal and means finished.")},
    };
    spec.build = buildSteps;
    return spec;
}

} // namespace novu::playground::pages
