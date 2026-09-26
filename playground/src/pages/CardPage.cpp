#include "Pages.h"

#include "novu/playground/ExampleLayout.h"
#include "novu/playground/SampleData.h"

#include "novu/ui/Badge.h"
#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/Separator.h"
#include "novu/ui/Text.h"

#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
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
    auto* annotationCard=new Card("An annotated note",page);annotationCard->setLeadingAccent(token(ColorRole::Accent,.5));annotationCard->setSelectionOpacity(.11,.32);column->addWidget(annotationCard);

    auto* dashed=new Card(page);dashed->setHeaderVisible(false);dashed->setSurface(Card::Surface::Raised);dashed->setBorderStyle(Qt::DashLine,true);dashed->setRadiusStep(RadiusStep::Small);
    dashed->body()->addWidget(paragraph("A dashed note uses the stronger theme border and the raised surface.",dashed,TypeRole::Caption,Ink::Muted));column->addWidget(dashed);
    const bool longText = ctx.scenario == Scenario::LongText;
    const QString title = longText
        ? ctx.data->text(QStringLiteral("long_text")).left(90)
        : QStringLiteral("Open positions");

    // ── header shapes ────────────────────────────────────────────────────
    {
        auto* plain = new Card(page);
        plain->setHeaderVisible(false);
        QLabel* text = label(QStringLiteral("A card with no header at all — the surface "
                                            "primitive on its own. Padding is contentsMargins "
                                            "on the card's own layout; there is no inner body "
                                            "widget."),
                             plain, TypeRole::Body, Ink::Secondary);
        text->setWordWrap(true);
        plain->body()->addWidget(text);
        column->addWidget(plain);
    }

    {
        auto* titled = new Card(title, page);
        titled->setSubtitle(QStringLiteral("Live, WS-only"));
        titled->body()->addWidget(
            label(QStringLiteral("Title and subtitle. The second line says what the data is, "
                                 "which is the difference between a panel a user trusts and "
                                 "one they have to ask about."),
                  titled, TypeRole::Body, Ink::Secondary));
        column->addWidget(titled);
    }

    {
        auto* action = new Card(QStringLiteral("Interactive card"), page);
        action->setInteractive(true);
        action->setSurface(Card::Surface::Window);
        action->setSelected(true);
        auto* result = label(QStringLiteral("Click, Enter or Space to activate"), action);
        action->body()->addWidget(result);
        QObject::connect(action, &Card::clicked, result, [result] {
            result->setText(QStringLiteral("Card activated"));
        });
        column->addWidget(action);
    }

    // ── trailing widgets ─────────────────────────────────────────────────
    {
        auto* card = new Card(QStringLiteral("Header trailing"), page);
        card->setSubtitle(QStringLiteral("A list, not a slot"));
        card->setHeaderTrailing(QList<QWidget*>{
            new Badge(QStringLiteral("Connected"), Badge::Tone::Success, card),
            ghostButton(QStringLiteral("Refresh"), card),
            detachButton(card)});
        note(card->body(), card,
             QStringLiteral("One trailing slot does not survive contact with a real header: a "
                            "live table wants both its connection badge and its detach button, "
                            "and wrapping them would create a container that exists to hold two "
                            "things in a row."));
        column->addWidget(card);
    }

    // ── nesting and data ─────────────────────────────────────────────────
    {
        auto* card = new Card(QStringLiteral("With content"), page);
        QVBoxLayout* body = card->body();
        const QJsonArray rows = ctx.data->rowsFor(QStringLiteral("trades"), ctx.scenario);

        if (ctx.scenario == Scenario::ErrorState) {
            body->addWidget(scenarioErrorState(card, QStringLiteral("Positions")));
        } else if (rows.isEmpty()) {
            body->addWidget(scenarioEmptyState(card, QStringLiteral("positions")));
        } else {
            const int limit = qMin(rows.size(), 8);
            for (int i = 0; i < limit; ++i) {
                const QJsonObject object = rows.at(i).toObject();
                auto* row = new QHBoxLayout();
                row->setContentsMargins(0, 0, 0, 0);
                auto* name = label(object.value(QStringLiteral("symbol")).toString(),
                                   card, TypeRole::Body);
                name->setWordWrap(false);
                row->addWidget(name);
                row->addStretch(1);
                const double pnl = object.value(QStringLiteral("pnl")).toDouble();
                row->addWidget(label(QString::number(pnl, 'f', 2), card, TypeRole::Mono,
                                     pnl >= 0 ? Ink::Success : Ink::Danger));
                body->addLayout(row);
            }
            if (rows.size() > limit) {
                note(body, card, QStringLiteral("Showing %1 of %2 rows.")
                                     .arg(limit)
                                     .arg(rows.size()));
            }
        }
        column->addWidget(card);
    }

    // ── section() ────────────────────────────────────────────────────────
    {
        auto* card = new Card(QStringLiteral("section()"), page);
        QVBoxLayout* body = card->body();
        section(body, QStringLiteral("Connection"), card);
        body->addWidget(label(QStringLiteral("Adds a styled heading to the layout you already "
                                             "have. Creates no container."),
                              card, TypeRole::Body, Ink::Secondary));
        section(body, QStringLiteral("Retention"), card);
        body->addWidget(label(QStringLiteral("Grouping is what the parent layout does."), card,
                              TypeRole::Body, Ink::Secondary));
        column->addWidget(card);
    }

    // ── body metrics and the header rule ─────────────────────────────────
    {
        auto* card = new Card(QStringLiteral("setBodyMetrics() / setHeaderDivider()"), page);
        card->setSubtitle(QStringLiteral("A card whose content is not the usual stack"));
        QVBoxLayout* body = card->body();

        auto* flush = new Card(QStringLiteral("Flush body"), card);
        flush->setSubtitle(QStringLiteral("`.card-body.flush{padding:0}`"));
        // A table, a list of rows that carry their own padding, anything that
        // has to meet the card's own edge.
        flush->setBodyMetrics(QMargins(0, 0, 0, 0), 0);
        for (int i = 0; i < 3; ++i) {
            auto* row = new QWidget(flush);
            auto* line = new QHBoxLayout(row);
            line->setContentsMargins(16, 9, 16, 9);
            line->addWidget(label(QStringLiteral("Row %1").arg(i + 1), row, TypeRole::Body,
                                  Ink::Secondary));
            flush->body()->addWidget(row);
            if (i < 2) {
                flush->body()->addWidget(new Separator(Qt::Horizontal,
                                                       Separator::Weight::Hairline, flush));
            }
        }
        body->addWidget(flush);

        auto* undivided = new Card(QStringLiteral("No header rule"), card);
        undivided->setHeaderDivider(false);
        undivided->body()->addWidget(label(QStringLiteral("Header and body as one block."),
                                           undivided, TypeRole::Body, Ink::Secondary));
        body->addWidget(undivided);

        note(body, card,
             QStringLiteral("Both have to be said through the card: applyTheme() runs again "
                            "on every theme change and on the first polish, and what it "
                            "re-applies is exactly the body's padding and gap - so a margin "
                            "set straight on body() survives until that polish and then "
                            "silently reverts. The header rule is `.card-head"
                            "{border-bottom:1px solid var(--hair)}`, on by default."));
        column->addWidget(card);
    }

    // ── raised ───────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("On a panel"),
                                 QStringLiteral("`.card.raised` — a card that sits "
                                                "on a panel rather than on the page."));
        QVBoxLayout* body = card->body();

        for (int i = 0; i < 3; ++i) {
            auto* row = new Card(card);
            row->setHeaderVisible(false);
            row->setSurface(Card::Surface::Raised);
            row->setBodyMetrics(QMargins(9, 7, 9, 7), 3);
            row->body()->addWidget(label(QStringLiteral("Reuse GitService.stage()"),
                                         row, TypeRole::Body, Ink::Secondary));
            row->body()->addWidget(label(QStringLiteral("Commit panel wiring"), row,
                                         TypeRole::Mono, Ink::Muted));
            body->addWidget(row);
        }

        note(body, card,
             QStringLiteral("A page's cards take the page ground behind a hairline. "
                            "A row inside a rail cannot: the hairline against a "
                            "surface one step away from it reads as a seam, so the "
                            "raised kind takes the panel shade and drops the "
                            "outline. Both are the same component."));
        column->addWidget(card);
    }

    auto* contract = new Card(QStringLiteral("Window surface / panel hover"), page);
    contract->setSurface(Card::Surface::Window);
    contract->setRadiusStep(RadiusStep::Medium);
    contract->setHeaderMetrics(QMargins(13,12,13,11),26);
    contract->setHoverSurface(Card::Surface::Raised);
    contract->setHoverAccentBorder(true);
    contract->setInteractive(true);
    column->addWidget(contract);
    auto* elevated = new Card(QStringLiteral("Elevated surface"), page);
    elevated->setSurface(Card::Surface::Elevated);
    elevated->setBorderStyle(Qt::SolidLine, true);
    column->addWidget(elevated);
    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec cardComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("card");
    spec.name = QStringLiteral("Card");
    spec.category = Category::DataDisplay;
    spec.description = QStringLiteral(
        "The one surface primitive: background, border, themed radius. Self-painted, and "
        "its padding lives on its own layout — there is no inner body widget.");
    spec.headerPath = QStringLiteral("include/novu/ui/Card.h");
    spec.sourcePath = QStringLiteral("src/Card.cpp");
    spec.designedFor = {QStringLiteral("novuspace"),
                   QStringLiteral("novuai"),
                   QStringLiteral("novuexplorer"),
                   QStringLiteral("novuhub"),
                   QStringLiteral("novuvoice"),
                   QStringLiteral("novutrade")};
    spec.usedBy = {QStringLiteral("novuspace")};
    spec.variants = {QStringLiteral("Plain surface"), QStringLiteral("Titled"),
                     QStringLiteral("Titled + subtitle"), QStringLiteral("With header trailing")};
    spec.states = {QStringLiteral("Normal"), QStringLiteral("Empty"), QStringLiteral("Populated")};
    spec.keywords = {QStringLiteral("panel"), QStringLiteral("surface"), QStringLiteral("frame"),
                     QStringLiteral("section"), QStringLiteral("container")};
    spec.properties = {
        {QStringLiteral("title"), QStringLiteral("QString"), {}, QString(), QStringLiteral("Header title.")},
        {QStringLiteral("subtitle"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("Muted second line stating what the data is.")},
        {QStringLiteral("headerVisible"), QStringLiteral("bool"), {}, QStringLiteral("true"),
         QStringLiteral("A card used as a plain surface drops the header entirely.")},
        {QStringLiteral("headerTrailing"), QStringLiteral("QList<QWidget*>"), {}, QString(),
         QStringLiteral("Trailing widgets on the title row. The card owns the row, the caller owns the widgets.")},
    };
    spec.scenarios = {Scenario::Normal,   Scenario::Empty,   Scenario::SingleItem,
                      Scenario::ManyItems, Scenario::LongText, Scenario::ErrorState};
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
