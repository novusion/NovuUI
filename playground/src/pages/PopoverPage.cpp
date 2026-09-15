#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/IconButton.h"
#include "novu/ui/Popover.h"
#include "novu/ui/Separator.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

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

    // ── a window menu ────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("A window menu"),
                                 QStringLiteral("Groups, glyphs, a meta column, "
                                                "and one entry in red."));
        QVBoxLayout* body = card->body();

        auto* chosen = label(QStringLiteral("—"), card, TypeRole::Mono, Ink::Secondary);

        auto* menu = new Popover(card);
        menu->addGroup(QStringLiteral("Connection"));
        menu->addItem(QStringLiteral("reconnect"), QStringLiteral("Reconnect"), Glyph::Plug);
        menu->setMeta(QStringLiteral("reconnect"), QStringLiteral("Ctrl+R"));
        menu->addItem(QStringLiteral("settings"), QStringLiteral("Connection settings…"),
                      Glyph::Sliders);
        menu->addItem(QStringLiteral("diagnostics"), QStringLiteral("Diagnostics"),
                      Glyph::Diagnostics);
        menu->setMeta(QStringLiteral("diagnostics"), QStringLiteral("2 warnings"));
        menu->addSeparator();
        menu->addGroup(QStringLiteral("Appearance"));
        menu->addItem(QStringLiteral("theme.dark"), QStringLiteral("Graphite"), Glyph::Eye);
        menu->setChecked(QStringLiteral("theme.dark"), true);
        menu->addItem(QStringLiteral("theme.light"), QStringLiteral("Light"), Glyph::Eye);
        menu->addItem(QStringLiteral("scale"), QStringLiteral("Interface scale"),
                      Glyph::Sliders);
        menu->setMeta(QStringLiteral("scale"), QStringLiteral("100%"));
        menu->addSeparator();
        menu->addItem(QStringLiteral("export"), QStringLiteral("Export workspace…"),
                      Glyph::Download);
        menu->addItem(QStringLiteral("forget"), QStringLiteral("Forget this device"),
                      Glyph::Trash);
        menu->setDangerous(QStringLiteral("forget"), true);
        menu->addItem(QStringLiteral("licence"), QStringLiteral("Manage licence"),
                      Glyph::Key);
        menu->setItemEnabled(QStringLiteral("licence"), false);
        menu->setMeta(QStringLiteral("licence"), QStringLiteral("admin only"));

        QObject::connect(menu, &Popover::triggered, chosen, &QLabel::setText);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Open it"));
        auto* anchor = new Button(QStringLiteral("Window menu"), Button::Variant::Secondary,
                                  card);
        anchor->setControlSize(ControlSize::Small);
        QObject::connect(anchor, &Button::clicked, menu,
                         [menu, anchor] { menu->popupUnder(anchor); });
        row->addWidget(anchor);

        auto* kebab = new IconButton(Glyph::More, card);
        QObject::connect(kebab, &IconButton::clicked, menu,
                         [menu, kebab] { menu->popupUnder(kebab); });
        row->addWidget(kebab);
        row->addSpacing(16);
        row->addWidget(chosen);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("QMenu cannot be this. Its item layout, its check column and its "
                            "submenu arrow belong to the platform style, its separator is a "
                            "sunken line, and the one thing it will not do is carry a "
                            "right-aligned meta column — which is what nearly every menu in "
                            "these designs uses."));
        note(body, card,
             QStringLiteral("The menu closes before the signal, not after: a handler that "
                            "opens a dialog would otherwise do so with a menu still on screen "
                            "above it. Clicking outside closes it, because the window carries "
                            "Qt::Popup and therefore the mouse grab a menu needs."));
        column->addWidget(card);
    }

    // ── placement ────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Placement"),
                                 QStringLiteral("Under the anchor, flipped above "
                                                "when there is no room."));
        QVBoxLayout* body = card->body();

        auto* menu = new Popover(card);
        menu->addItem(QStringLiteral("a"), QStringLiteral("Rename"), Glyph::Edit);
        menu->addItem(QStringLiteral("b"), QStringLiteral("Duplicate"), Glyph::Copy);
        menu->addItem(QStringLiteral("c"), QStringLiteral("Open in new window"),
                      Glyph::External);
        menu->addSeparator();
        menu->addItem(QStringLiteral("d"), QStringLiteral("Delete"), Glyph::Trash);
        menu->setDangerous(QStringLiteral("d"), true);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Context menu"));
        auto* here = new Button(QStringLiteral("At the pointer"), Button::Variant::Ghost,
                                card);
        here->setControlSize(ControlSize::Small);
        QObject::connect(here, &Button::clicked, menu, [menu, here] {
            menu->popupAt(here->mapToGlobal(QPoint(here->width() / 2, here->height() / 2)));
        });
        row->addWidget(here);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Drag the gallery window to the bottom of the screen and open "
                            "either menu. It flips above rather than being pushed up over the "
                            "control that opened it, and it is clamped to the screen rather "
                            "than hanging off the right edge."));
        column->addWidget(card);
    }

    // ── keyboard ─────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Keyboard"),
                                 QStringLiteral("Arrows skip captions, rules and "
                                                "disabled entries."));
        QVBoxLayout* body = card->body();

        note(body, card,
             QStringLiteral("Open a menu above and press Down repeatedly. The caption rows and "
                            "the separators are passed over, and so is the disabled Manage "
                            "licence — a keyboard that can land on something that cannot be "
                            "chosen is a keyboard that gets stuck. Escape closes; Enter and "
                            "Space choose."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec popoverComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("popover");
    spec.name = QStringLiteral("Popover");
    spec.category = Category::Navigation;
    spec.description = QStringLiteral(
        "The menu that drops out of a button: captions, glyphs, a right-aligned meta "
        "column, checked and dangerous entries — painted, because QMenu cannot be this.");
    spec.headerPath = QStringLiteral("include/novu/ui/Popover.h");
    spec.sourcePath = QStringLiteral("src/Popover.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Item"), QStringLiteral("Group"),
                     QStringLiteral("Separator"), QStringLiteral("Dangerous")};
    spec.states = {QStringLiteral("Normal"), QStringLiteral("Hover"),
                   QStringLiteral("Checked"), QStringLiteral("Disabled")};
    spec.keywords = {QStringLiteral("popover"), QStringLiteral("menu"),
                     QStringLiteral("pitem"), QStringLiteral("psep"),
                     QStringLiteral("dropdown"), QStringLiteral("context menu"),
                     QStringLiteral("qmenu")};
    spec.properties = {
        {QStringLiteral("meta"), QStringLiteral("QString"), {}, QStringLiteral(""),
         QStringLiteral("The right-aligned note: a shortcut, a count, a version.")},
        {QStringLiteral("checked"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("The accent, for an option in force. Not a radio group.")},
        {QStringLiteral("dangerous"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("Ink and hover both go red, for the one entry to avoid.")},
    };
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
