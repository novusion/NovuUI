#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/Disclosure.h"
#include "novu/ui/ModuleBar.h"
#include "novu/ui/NavList.h"
#include "novu/ui/Segmented.h"
#include "novu/ui/Separator.h"
#include "novu/ui/Sidebar.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

void fillWorkspaces(NavList* nav)
{
    nav->addSection(QStringLiteral("Workspaces"));
    nav->addItem(QStringLiteral("ws.novusion"), QStringLiteral("Novusion"), Glyph::Folder);
    nav->setExpandable(QStringLiteral("ws.novusion"), true);
    nav->setExpanded(QStringLiteral("ws.novusion"), true);
    nav->setStatus(QStringLiteral("ws.novusion"), StatusDot::Tone::Success);
    nav->addChild(QStringLiteral("ws.novusion.docs"), QStringLiteral("Documents"));
    nav->setCount(QStringLiteral("ws.novusion.docs"), 128);
    nav->addChild(QStringLiteral("ws.novusion.mail"), QStringLiteral("Mail"));
    nav->setCount(QStringLiteral("ws.novusion.mail"), 7, NavList::CountTone::Alert);
    nav->addChild(QStringLiteral("ws.novusion.vault"), QStringLiteral("Vault"));

    nav->addItem(QStringLiteral("ws.research"), QStringLiteral("Research"), Glyph::Folder);
    nav->setExpandable(QStringLiteral("ws.research"), true);
    nav->setStatus(QStringLiteral("ws.research"), StatusDot::Tone::Warning);
    nav->setCount(QStringLiteral("ws.research"), 3);

    nav->addItem(QStringLiteral("ws.archive"), QStringLiteral("Archive 2024"), Glyph::Archive);
    nav->setStatus(QStringLiteral("ws.archive"), StatusDot::Tone::Neutral);

    nav->addSection(QStringLiteral("System"));
    nav->addItem(QStringLiteral("sys.activity"), QStringLiteral("Activity"), Glyph::Activity);
    nav->addItem(QStringLiteral("sys.diagnostics"), QStringLiteral("Diagnostics"),
                 Glyph::Diagnostics);
    nav->setCount(QStringLiteral("sys.diagnostics"), 2, NavList::CountTone::Bad);
    nav->addItem(QStringLiteral("sys.settings"), QStringLiteral("Settings"), Glyph::Settings);
    nav->addItem(QStringLiteral("sys.billing"), QStringLiteral("Billing"), Glyph::Scale);
    nav->setRowEnabled(QStringLiteral("sys.billing"), false);
}

// ── NavList ──────────────────────────────────────────────────────────────

QWidget* buildNavList(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    if (ctx.scenario == Scenario::Empty) {
        column->addWidget(scenarioEmptyState(page, QStringLiteral("navigation rows")));
        column->addStretch(1);
        return page;
    }

    {
        Card* card = exampleCard(page, QStringLiteral("Everything a row can carry"),
                                 QStringLiteral("Glyph, label, twisty, status dot, "
                                                "count — and none of it a widget."));
        QVBoxLayout* body = card->body();

        auto* nav = new NavList(card);
        fillWorkspaces(nav);
        nav->setCurrentKey(QStringLiteral("ws.novusion.docs"));
        nav->setFixedWidth(250);

        auto* chosen = label(QStringLiteral("ws.novusion.docs"), card, TypeRole::Mono,
                             Ink::Secondary);
        QObject::connect(nav, &NavList::activated, chosen, &QLabel::setText);
        QObject::connect(nav, &NavList::expandToggled, chosen,
                         [chosen](const QString& key, bool open) {
                             chosen->setText(QStringLiteral("%1 %2").arg(
                                 key, open ? QStringLiteral("expanded")
                                           : QStringLiteral("collapsed")));
                         });

        auto* holder = new QWidget(card);
        auto* row = new QHBoxLayout(holder);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(themeFor(card).spacing.lg);
        row->addWidget(nav, 0, Qt::AlignTop);

        auto* side = new QWidget(holder);
        auto* sideColumn = new QVBoxLayout(side);
        sideColumn->setContentsMargins(0, 0, 0, 0);
        sideColumn->setSpacing(6);
        sideColumn->addWidget(label(QStringLiteral("LAST SIGNAL"), side, TypeRole::Eyebrow,
                                    Ink::Muted));
        sideColumn->addWidget(chosen);
        sideColumn->addStretch(1);
        row->addWidget(side, 1);
        body->addWidget(holder);

        note(body, card,
             QStringLiteral("Click a row, then click the chevron on the same row. They are "
                            "separate targets: opening a workspace and expanding it are "
                            "different actions on one line, exactly as they are in a file "
                            "tree. Press and release must land in the same zone, so dragging "
                            "off a chevron before letting go cancels rather than toggling."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("Counts"),
                                 QStringLiteral("Three tones, and only one of them "
                                                "is a colour."));
        QVBoxLayout* body = card->body();

        auto* nav = new NavList(card);
        nav->addItem(QStringLiteral("a"), QStringLiteral("Normal count"), Glyph::Inbox);
        nav->setCount(QStringLiteral("a"), 42);
        nav->addItem(QStringLiteral("b"), QStringLiteral("Needs a person"), Glyph::Bell);
        nav->setCount(QStringLiteral("b"), 3, NavList::CountTone::Alert);
        nav->addItem(QStringLiteral("c"), QStringLiteral("Something failed"), Glyph::Warning);
        nav->setCount(QStringLiteral("c"), 2, NavList::CountTone::Bad);
        nav->addItem(QStringLiteral("d"), QStringLiteral("Zero is not nothing"), Glyph::File);
        nav->setCount(QStringLiteral("d"), 0);
        nav->addItem(QStringLiteral("e"), QStringLiteral("No count at all"), Glyph::File);
        nav->setCurrentKey(QStringLiteral("a"));
        nav->setFixedWidth(250);
        body->addWidget(nav);

        note(body, card,
             QStringLiteral("The design is explicit: a count that needs a person is amber, "
                            "and it is the one place a status colour appears in the chrome. "
                            "Note the current row's pill — it joins the row rather than "
                            "staying a neutral chip on a tinted plate."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("Collapsed"),
                                 QStringLiteral("A property of the list, not of "
                                                "each row."));
        QVBoxLayout* body = card->body();

        auto* nav = new NavList(card);
        fillWorkspaces(nav);
        nav->setCurrentKey(QStringLiteral("sys.activity"));
        nav->setCollapsed(true);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Icons only"));
        row->addWidget(nav, 0, Qt::AlignTop);

        auto* toggle = new Button(QStringLiteral("Expand"), Button::Variant::Secondary, card);
        toggle->setControlSize(ControlSize::Small);
        QObject::connect(toggle, &Button::clicked, nav, [nav, toggle] {
            const bool collapsed = !nav->isCollapsed();
            nav->setCollapsed(collapsed);
            toggle->setText(collapsed ? QObject::tr("Expand") : QObject::tr("Collapse"));
        });
        row->addWidget(toggle, 0, Qt::AlignTop);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Labels, counts, twisties and captions all go; the glyphs centre "
                            "and the active marker stays, because at this width the marker is "
                            "the only thing distinguishing the current row from a hovered one."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

// ── Sidebar ──────────────────────────────────────────────────────────────

QWidget* buildSidebar(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    {
        Card* card = exampleCard(page, QStringLiteral("The whole rail"),
                                 QStringLiteral("Identity, transport, navigation, "
                                                "actions — and only one of them "
                                                "scrolls."));
        QVBoxLayout* body = card->body();

        auto* rail = new Sidebar(card);
        rail->setIdentity(Glyph::Vault, QStringLiteral("Vault"),
                          QStringLiteral("NovuVault v0.67.11"));
        rail->setFixedHeight(420);

        ConnectionStrip* strip = rail->connection();
        strip->setHost(QStringLiteral("hub.novusion.net"));
        strip->setDetail(QStringLiteral("52 ms"));
        strip->setAction(Glyph::Lock, QObject::tr("Lock the vault"));

        fillWorkspaces(rail->nav());
        rail->nav()->setCurrentKey(QStringLiteral("ws.novusion.vault"));

        auto* newItem = new Button(QStringLiteral("New secret"), Button::Variant::Primary, rail);
        newItem->setControlSize(ControlSize::Small);
        rail->actions()->addWidget(newItem);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Rail"));
        row->addWidget(rail, 0, Qt::AlignTop);

        auto* side = new QWidget(card);
        auto* sideColumn = new QVBoxLayout(side);
        sideColumn->setContentsMargins(0, 0, 0, 0);
        sideColumn->setSpacing(8);

        auto* collapse = new Button(QStringLiteral("Collapse rail"),
                                    Button::Variant::Secondary, side);
        collapse->setControlSize(ControlSize::Small);
        QObject::connect(collapse, &Button::clicked, rail, [rail, collapse] {
            rail->toggleCollapsed();
            collapse->setText(rail->isCollapsed() ? QObject::tr("Expand rail")
                                                  : QObject::tr("Collapse rail"));
        });
        sideColumn->addWidget(collapse);

        const struct { const char* caption; ConnectionStrip::State state; const char* detail; }
            states[] = {
                {"Online", ConnectionStrip::State::Online, "52 ms"},
                {"Degraded", ConnectionStrip::State::Degraded, "410 ms"},
                {"Offline", ConnectionStrip::State::Offline, "—"},
            };
        for (const auto& entry : states) {
            auto* button = new Button(QString::fromLatin1(entry.caption),
                                      Button::Variant::Ghost, side);
            button->setControlSize(ControlSize::Small);
            const ConnectionStrip::State state = entry.state;
            const QString detail = QString::fromLatin1(entry.detail);
            QObject::connect(button, &Button::clicked, strip, [strip, state, detail] {
                strip->setState(state);
                strip->setDetail(detail);
            });
            sideColumn->addWidget(button);
        }
        sideColumn->addStretch(1);
        row->addWidget(side, 0, Qt::AlignTop);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Collapse it. The head centres and loses its text, the connection "
                            "strip becomes its light alone, and every NavList inside the "
                            "scrolling region collapses with it — including ones a caller put "
                            "there inside Disclosure blocks. Switch the transport to Offline "
                            "and the light stops breathing: a light that pulses while nothing "
                            "is connected is the one that stops being read."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("Grouped navigation"),
                                 QStringLiteral("Disclosure blocks inside the "
                                                "scrolling region."));
        QVBoxLayout* body = card->body();

        auto* rail = new Sidebar(card);
        rail->setIdentity(Glyph::Trades, QStringLiteral("Trade"),
                          QStringLiteral("NovuTrade v0.67.11"));
        rail->setFixedHeight(360);

        const struct { const char* title; int count; const char* rows[3]; } blocks[] = {
            {"Live", 3, {"Positions", "Orders", "Fills"}},
            {"Research", 2, {"Markets", "Backtests", nullptr}},
        };
        for (const auto& block : blocks) {
            auto* group = new Disclosure(QString::fromLatin1(block.title),
                                         rail->navigationWidget(),
                                         Disclosure::Variant::Block);
            group->setCount(block.count);
            group->setAction(Glyph::Plus, QObject::tr("Add"));

            auto* list = new NavList(group->bodyWidget());
            for (const char* name : block.rows) {
                if (!name) {
                    continue;
                }
                list->addItem(QString::fromLatin1(name).toLower(),
                              QString::fromLatin1(name), Glyph::Chart);
            }
            group->body()->addWidget(list);
            rail->navigation()->insertWidget(rail->navigation()->count() - 1, group);
        }

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Blocks"));
        row->addWidget(rail, 0, Qt::AlignTop);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Collapsing a block and expanding a row inside it are separate "
                            "states, which the design says outright: collapsing WORKSPACES "
                            "does not forget which workspaces were open inside it. The plus "
                            "beside a caption adds to the block rather than collapsing it, "
                            "because it is hit-tested before the header is."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

// ── ModuleBar ────────────────────────────────────────────────────────────

QWidget* buildModuleBar(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    const auto makeBar = [](QWidget* parent) {
        auto* bar = new ModuleBar(parent);
        bar->addModule(QStringLiteral("web"), QStringLiteral("Web"), Glyph::Globe);
        bar->addModule(QStringLiteral("mail"), QStringLiteral("Mail"), Glyph::Mail);
        bar->setCount(QStringLiteral("mail"), 12);
        bar->addModule(QStringLiteral("vault"), QStringLiteral("Vault"), Glyph::Vault);
        bar->addModule(QStringLiteral("trade"), QStringLiteral("Trade"), Glyph::Trades);
        bar->setCount(QStringLiteral("trade"), 2, ModuleBar::CountTone::Alert);
        return bar;
    };

    {
        Card* card = exampleCard(page, QStringLiteral("Where you are"),
                                 QStringLiteral("The one accented thing in the "
                                                "window chrome."));
        QVBoxLayout* body = card->body();

        auto* bar = makeBar(card);
        bar->setCurrentKey(QStringLiteral("vault"));

        auto* chosen = label(QStringLiteral("vault"), card, TypeRole::Mono, Ink::Secondary);
        QObject::connect(bar, &ModuleBar::moduleChosen, chosen, &QLabel::setText);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Modules"));
        row->addWidget(bar);
        row->addSpacing(16);
        row->addWidget(chosen);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("The design puts this in the title bar and says why: the four "
                            "modules ARE the application's navigation. Purple marks the active "
                            "one and nothing else in the chrome uses it, so a glance at the "
                            "frame answers \"where am I\" before anything inside it is read."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("Narrow"),
                                 QStringLiteral("Glyphs only, with the label on "
                                                "the tooltip."));
        QVBoxLayout* body = card->body();

        auto* bar = makeBar(card);
        bar->setCompact(true);
        bar->setCurrentKey(QStringLiteral("mail"));
        bar->setModuleEnabled(QStringLiteral("trade"), false);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Compact"));
        row->addWidget(bar);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Hover one. The tip appears only when the label is not on screen — "
                            "a tip repeating a word the pointer is already over is noise, so "
                            "the expanded bar above has none. Trade is disabled here: arrowing "
                            "across the bar skips it rather than landing on it."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("Not a Segmented"),
                                 QStringLiteral("Same machinery, different "
                                                "control."));
        QVBoxLayout* body = card->body();

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Side by side"));
        auto* bar = makeBar(card);
        bar->setCurrentKey(QStringLiteral("web"));
        row->addWidget(bar);
        row->addSpacing(24);
        row->addWidget(new Segmented({QStringLiteral("Basic"), QStringLiteral("Extended"),
                                      QStringLiteral("Pro")},
                                     card, 1));
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Both are ItemStrip: the hit testing, the press-and-release "
                            "ownership and the arrow keys are one implementation. They are two "
                            "classes because they are two controls — a picker sets something "
                            "and lives in a sunken tray; navigation takes you somewhere and "
                            "sits on the bare title bar. A tray here would draw a box round "
                            "the middle of the chrome."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec navListComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("nav-list");
    spec.name = QStringLiteral("NavList");
    spec.category = Category::Navigation;
    spec.description = QStringLiteral(
        "A whole sidebar navigation tree in one painted widget: glyphs, labels, counts, "
        "status dots, twisties, sections and a collapsed icon mode.");
    spec.headerPath = QStringLiteral("include/novu/ui/NavList.h");
    spec.sourcePath = QStringLiteral("src/NavList.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Item"), QStringLiteral("Child"),
                     QStringLiteral("Section")};
    spec.states = {QStringLiteral("Normal"), QStringLiteral("Hover"),
                   QStringLiteral("Current"), QStringLiteral("Disabled"),
                   QStringLiteral("Expanded"), QStringLiteral("Collapsed rail")};
    spec.keywords = {QStringLiteral("navbtn"), QStringLiteral("rail"),
                     QStringLiteral("sidebar"), QStringLiteral("tree"),
                     QStringLiteral("menu"), QStringLiteral("workspace")};
    spec.scenarios = {Scenario::Normal, Scenario::Empty};
    spec.properties = {
        {QStringLiteral("currentKey"), QStringLiteral("QString"), {}, QStringLiteral(""),
         QStringLiteral("Caller's own identifier; never shown, so it survives translation.")},
        {QStringLiteral("countTone"), QStringLiteral("enum"),
         {QStringLiteral("Normal"), QStringLiteral("Alert"), QStringLiteral("Bad")},
         QStringLiteral("Normal"),
         QStringLiteral("Amber for a count that needs a person — the design's rule.")},
        {QStringLiteral("collapsed"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("Hides labels, counts and twisties; centres the glyphs.")},
    };
    spec.build = buildNavList;
    return spec;
}

ComponentSpec sidebarComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("sidebar");
    spec.name = QStringLiteral("Sidebar");
    spec.category = Category::Navigation;
    spec.description = QStringLiteral(
        "The left rail: an identity head, a connection strip, a scrolling navigation "
        "region and a pinned action column. Only the navigation scrolls.");
    spec.headerPath = QStringLiteral("include/novu/ui/Sidebar.h");
    spec.sourcePath = QStringLiteral("src/Sidebar.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Expanded"), QStringLiteral("Collapsed"),
                     QStringLiteral("With blocks")};
    spec.states = {QStringLiteral("Online"), QStringLiteral("Degraded"),
                   QStringLiteral("Offline")};
    spec.keywords = {QStringLiteral("rail"), QStringLiteral("navigation"),
                     QStringLiteral("connection"), QStringLiteral("rail-conn"),
                     QStringLiteral("rail-head"), QStringLiteral("module")};
    spec.properties = {
        {QStringLiteral("collapsed"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("Folds the rail to a strip of icons, lists included.")},
        {QStringLiteral("expandedWidth"), QStringLiteral("int"), {}, QStringLiteral("236"),
         QStringLiteral("The collapsed width is derived from the mark, not set.")},
    };
    spec.build = buildSidebar;
    return spec;
}

ComponentSpec moduleBarComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("module-bar");
    spec.name = QStringLiteral("ModuleBar");
    spec.category = Category::Navigation;
    spec.description = QStringLiteral(
        "Top-level navigation in the title bar: the modules an application is made of, "
        "with the active one in the accent — the only accented thing in the chrome.");
    spec.headerPath = QStringLiteral("include/novu/ui/ModuleBar.h");
    spec.sourcePath = QStringLiteral("src/ModuleBar.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Full"), QStringLiteral("Compact")};
    spec.states = {QStringLiteral("Normal"), QStringLiteral("Hover"),
                   QStringLiteral("Current"), QStringLiteral("Disabled")};
    spec.keywords = {QStringLiteral("modbar"), QStringLiteral("modbtn"),
                     QStringLiteral("titlebar"), QStringLiteral("tabs"),
                     QStringLiteral("modules"), QStringLiteral("itemstrip")};
    spec.properties = {
        {QStringLiteral("currentKey"), QStringLiteral("QString"), {}, QStringLiteral(""),
         QStringLiteral("What moduleChosen() carries.")},
        {QStringLiteral("compact"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("Glyphs only, with the label moved onto the tooltip.")},
        {QStringLiteral("countTone"), QStringLiteral("enum"),
         {QStringLiteral("Normal"), QStringLiteral("Alert")}, QStringLiteral("Normal"),
         QStringLiteral("Amber when the count needs a person.")},
    };
    spec.build = buildModuleBar;
    return spec;
}

} // namespace novu::playground::pages
