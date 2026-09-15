#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Card.h"
#include "novu/ui/Disclosure.h"
#include "novu/ui/Fields.h"
#include "novu/ui/Form.h"
#include "novu/ui/NavList.h"
#include "novu/ui/Separator.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/Toggle.h"

#include <QHBoxLayout>
#include <QLabel>
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

    // ── inline ───────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Inline"),
                                 QStringLiteral("Advanced belongs to the window it "
                                                "is in, not to a second dialog."));
        QVBoxLayout* body = card->body();

        auto* advanced = new Disclosure(QStringLiteral("Advanced"), card);
        advanced->setHint(QStringLiteral("3 changed"));
        advanced->setOpen(false);

        QVBoxLayout* inside = advanced->body();
        inside->setContentsMargins(0, themeFor(card).spacing.md, 0, 0);
        formRow(inside, advanced->bodyWidget(), QStringLiteral("Retries"),
                lineEdit(QStringLiteral("3"), advanced->bodyWidget(), 80));
        formRow(inside, advanced->bodyWidget(), QStringLiteral("Timeout"),
                lineEdit(QStringLiteral("15s"), advanced->bodyWidget(), 80));
        formRow(inside, advanced->bodyWidget(), QStringLiteral("Verify TLS"),
                new Toggle(advanced->bodyWidget()));

        body->addWidget(advanced);

        auto* closed = new Disclosure(QStringLiteral("Diagnostics"), card);
        closed->setHint(QStringLiteral("default"));
        closed->setOpen(false);
        closed->body()->addWidget(label(QStringLiteral("Nothing to report."),
                                        closed->bodyWidget(), TypeRole::Caption,
                                        Ink::Muted));
        body->addWidget(closed);

        note(body, card,
             QStringLiteral("Open it. The card grows to fit rather than scrolling, which is "
                            "what the design asks for — and the chevron turns over 180ms "
                            "rather than snapping, because a chevron that snaps is the one "
                            "part of a panel a person notices. The hint on the right says what "
                            "is inside without opening it."));
        column->addWidget(card);
    }

    // ── block ────────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Block"),
                                 QStringLiteral("The sidebar group: a caption, a "
                                                "count, and an action that must "
                                                "not collapse it."));
        QVBoxLayout* body = card->body();

        auto* holder = new QWidget(card);
        holder->setFixedWidth(240);
        auto* stack = new QVBoxLayout(holder);
        stack->setContentsMargins(0, 0, 0, 0);
        stack->setSpacing(2);

        const struct { const char* title; int count; const char* rows[4]; } blocks[] = {
            {"Workspaces", 3, {"Novusion", "Research", "Archive 2024", nullptr}},
            {"Shared with me", 2, {"Design review", "Q1 planning", nullptr, nullptr}},
        };
        auto* signal = label(QStringLiteral("—"), card, TypeRole::Mono, Ink::Secondary);

        for (const auto& entry : blocks) {
            auto* group = new Disclosure(QString::fromLatin1(entry.title), holder,
                                         Disclosure::Variant::Block);
            group->setCount(entry.count);
            group->setAction(Glyph::Plus, QObject::tr("New workspace"));

            auto* list = new NavList(group->bodyWidget());
            for (const char* name : entry.rows) {
                if (!name) {
                    continue;
                }
                list->addItem(QString::fromLatin1(name).toLower(),
                              QString::fromLatin1(name), Glyph::Folder);
            }
            group->body()->addWidget(list);

            const QString title = QString::fromLatin1(entry.title);
            QObject::connect(group, &Disclosure::actionTriggered, signal, [signal, title] {
                signal->setText(QStringLiteral("add to %1").arg(title));
            });
            QObject::connect(group, &Disclosure::toggled, signal,
                             [signal, title](bool open) {
                                 signal->setText(QStringLiteral("%1 %2").arg(
                                     title, open ? QStringLiteral("open")
                                                 : QStringLiteral("shut")));
                             });
            stack->addWidget(group);
        }
        stack->addStretch(1);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Sidebar blocks"));
        row->addWidget(holder, 0, Qt::AlignTop);
        row->addSpacing(20);

        auto* side = new QWidget(card);
        auto* sideColumn = new QVBoxLayout(side);
        sideColumn->setContentsMargins(0, 0, 0, 0);
        sideColumn->setSpacing(6);
        sideColumn->addWidget(label(QStringLiteral("LAST SIGNAL"), side, TypeRole::Eyebrow,
                                    Ink::Muted));
        sideColumn->addWidget(signal);
        sideColumn->addStretch(1);
        row->addWidget(side, 1, Qt::AlignTop);

        note(body, card,
             QStringLiteral("Click the plus. The block does not collapse — the action is "
                            "hit-tested before the header, which the design requires outright: "
                            "creating something belongs next to the thing it creates, and it "
                            "must not put away the list you were creating it in. Press the "
                            "plus and release on the caption and nothing happens at all, "
                            "because press and release have to land in the same zone."));
        column->addWidget(card);
    }

    // ── state ────────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Two states, not one"),
                                 QStringLiteral("Collapsing a block does not forget "
                                                "what was open inside it."));
        QVBoxLayout* body = card->body();

        auto* group = new Disclosure(QStringLiteral("Workspaces"), card,
                                     Disclosure::Variant::Block);
        group->setCount(2);

        auto* list = new NavList(group->bodyWidget());
        list->addItem(QStringLiteral("a"), QStringLiteral("Novusion"), Glyph::Folder);
        list->setExpandable(QStringLiteral("a"), true);
        list->setExpanded(QStringLiteral("a"), true);
        list->addChild(QStringLiteral("a.docs"), QStringLiteral("Documents"));
        list->addChild(QStringLiteral("a.mail"), QStringLiteral("Mail"));
        list->addItem(QStringLiteral("b"), QStringLiteral("Research"), Glyph::Folder);
        list->setExpandable(QStringLiteral("b"), true);
        list->setFixedWidth(230);
        group->body()->addWidget(list);
        group->setFixedWidth(240);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Try it"));
        row->addWidget(group, 0, Qt::AlignTop);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Expand Novusion, then collapse the block, then open it again. "
                            "Novusion is still expanded. Hiding the body leaves it intact — "
                            "the body is a real widget the caller filled, not a list this "
                            "component rebuilds, so there is nothing for it to forget."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec disclosureComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("disclosure");
    spec.name = QStringLiteral("Disclosure");
    spec.category = Category::Navigation;
    spec.description = QStringLiteral(
        "A header that puts its content away: an outlined Advanced row inside a form, or "
        "a sidebar group with a count and an action that does not collapse it.");
    spec.headerPath = QStringLiteral("include/novu/ui/Disclosure.h");
    spec.sourcePath = QStringLiteral("src/Disclosure.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Inline"), QStringLiteral("Block")};
    spec.states = {QStringLiteral("Open"), QStringLiteral("Shut"),
                   QStringLiteral("Hover"), QStringLiteral("Focused")};
    spec.keywords = {QStringLiteral("disclose"), QStringLiteral("collapse"),
                     QStringLiteral("expand"), QStringLiteral("accordion"),
                     QStringLiteral("advanced"), QStringLiteral("sb-block"),
                     QStringLiteral("chevron")};
    spec.properties = {
        {QStringLiteral("variant"), QStringLiteral("enum"),
         {QStringLiteral("Inline"), QStringLiteral("Block")}, QStringLiteral("Inline"),
         QStringLiteral("An outlined form row, or a sidebar caption.")},
        {QStringLiteral("open"), QStringLiteral("bool"), {}, QStringLiteral("true"),
         QStringLiteral("Hides the body; never clears it.")},
        {QStringLiteral("count"), QStringLiteral("int"), {}, QStringLiteral("-1"),
         QStringLiteral("-1 draws nothing. 0 draws a zero, which says something else.")},
        {QStringLiteral("hint"), QStringLiteral("QString"), {}, QStringLiteral(""),
         QStringLiteral("Inline only: what is inside, without opening it.")},
    };
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
