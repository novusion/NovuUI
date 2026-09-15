#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/IconButton.h"
#include "novu/ui/Separator.h"
#include "novu/ui/Text.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    // ── weights ──────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Weights"),
                                 QStringLiteral("Two, and each one says something different."));
        QVBoxLayout* body = card->body();

        body->addWidget(caption(QStringLiteral("Hairline — between rows inside a surface"),
                                card));
        body->addWidget(new Separator(Qt::Horizontal, card));
        body->addSpacing(12);

        body->addWidget(caption(QStringLiteral("Strong — between one surface and the next"),
                                card));
        body->addWidget(new Separator(Qt::Horizontal, Separator::Weight::Strong, card));
        body->addSpacing(12);

        auto* inset = new Separator(Qt::Horizontal, card);
        inset->setInset(24);
        body->addWidget(caption(QStringLiteral("Inset 24 — a menu rule, stopping short of "
                                               "both ends"),
                                card));
        body->addWidget(inset);

        note(body, card,
             QStringLiteral("Both colours come from the profile: the hairline is the rule "
                            "inside a surface, the strong one the rule between surfaces. "
                            "Qt's own QFrame::HLine draws two lines in the system palette, "
                            "which is why it is not used anywhere in this library."));
        column->addWidget(card);
    }

    // ── upright, in a toolbar ────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Upright"),
                                 QStringLiteral("toolbarDivider(): the short rule between two "
                                                "groups of controls."));
        QVBoxLayout* body = card->body();
        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Two groups, one rule"));

        row->addWidget(new IconButton(Glyph::Back, IconButton::Variant::Ghost, card));
        row->addWidget(new IconButton(Glyph::Forward, IconButton::Variant::Ghost, card));
        toolbarDivider(row, card);
        row->addWidget(new IconButton(Glyph::Refresh, IconButton::Variant::Ghost, card));
        row->addWidget(new IconButton(Glyph::Star, IconButton::Variant::Ghost, card));
        toolbarDivider(row, card);
        row->addWidget(secondaryButton(QStringLiteral("Open"), card));
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Its length is the design's 18px, not the height of whatever "
                            "toolbar it is dropped into — and it is centred in the row, so a "
                            "taller toolbar does not leave it hanging from the top edge."));
        column->addWidget(card);
    }

    // ── what it costs ────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("A stack of them"),
                                 QStringLiteral("Twelve rules: twelve paint calls, no polish "
                                                "pass, no stylesheet."));
        QVBoxLayout* body = card->body();
        for (int i = 0; i < 12; ++i) {
            body->addWidget(new Separator(Qt::Horizontal,
                                          i % 4 == 3 ? Separator::Weight::Strong
                                                     : Separator::Weight::Hairline,
                                          card));
            body->addSpacing(9);
        }
        note(body, card,
             QStringLiteral("Transparent to the mouse and with no focus policy: a rule is "
                            "decoration, so it never takes a click or a tab stop away from "
                            "the controls around it."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec separatorComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("separator");
    spec.name = QStringLiteral("Separator");
    spec.category = Category::Foundation;
    spec.description = QStringLiteral(
        "The hairline. Painted, with no children and no stylesheet, at the two weights the "
        "design draws: inside a surface, and between surfaces.");
    spec.headerPath = QStringLiteral("include/novu/ui/Separator.h");
    spec.sourcePath = QStringLiteral("src/Separator.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Hairline"), QStringLiteral("Strong"),
                     QStringLiteral("Horizontal"), QStringLiteral("Vertical")};
    spec.states = {QStringLiteral("Normal")};
    spec.keywords = {QStringLiteral("divider"), QStringLiteral("rule"), QStringLiteral("hairline"),
                     QStringLiteral("hr"), QStringLiteral("line"), QStringLiteral("border")};
    spec.properties = {
        {QStringLiteral("orientation"), QStringLiteral("enum"),
         {QStringLiteral("Horizontal"), QStringLiteral("Vertical")},
         QStringLiteral("Horizontal"), QStringLiteral("Which way the rule runs.")},
        {QStringLiteral("weight"), QStringLiteral("enum"),
         {QStringLiteral("Hairline"), QStringLiteral("Strong")}, QStringLiteral("Hairline"),
         QStringLiteral("Inside a surface, or between two of them.")},
        {QStringLiteral("inset"), QStringLiteral("int"), {}, QStringLiteral("0"),
         QStringLiteral("How far it stops short of each end.")},
        {QStringLiteral("length"), QStringLiteral("int"), {}, QStringLiteral("0"),
         QStringLiteral("A fixed run; 0 takes the space it is given.")},
    };
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
