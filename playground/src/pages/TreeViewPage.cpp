#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/Form.h"
#include "novu/ui/NavList.h"
#include "novu/ui/SelectionBar.h"
#include "novu/ui/Separator.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/TreeView.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

void fillWorkspace(TreeView* tree)
{
    tree->addNode(QString(), QStringLiteral("ws"), QStringLiteral("Workspaces"),
                  Glyph::Folder, TreeView::Kind::Group);
    tree->setExpanded(QStringLiteral("ws"), true);

    tree->addNode(QStringLiteral("ws"), QStringLiteral("nv"), QStringLiteral("Novusion"),
                  Glyph::Folder, TreeView::Kind::Emphasis);
    tree->setMark(QStringLiteral("nv"), TreeView::Mark::Active);
    tree->setExpanded(QStringLiteral("nv"), true);
    tree->setMarks(QStringLiteral("nv"), {Glyph::Users, Glyph::Star});

    tree->addNode(QStringLiteral("nv"), QStringLiteral("nv.src"), QStringLiteral("src"),
                  Glyph::Folder);
    tree->setExpanded(QStringLiteral("nv.src"), true);
    tree->setCount(QStringLiteral("nv.src"), 128);

    tree->addNode(QStringLiteral("nv.src"), QStringLiteral("nv.src.core"),
                  QStringLiteral("core"), Glyph::Folder);
    tree->addNode(QStringLiteral("nv.src.core"), QStringLiteral("nv.src.core.net"),
                  QStringLiteral("net"), Glyph::Folder);
    tree->addNode(QStringLiteral("nv.src.core.net"), QStringLiteral("f1"),
                  QStringLiteral("TunnelController.cpp"), Glyph::File);
    tree->setTag(QStringLiteral("f1"), QStringLiteral("M"), Badge::Tone::Warning);
    tree->addNode(QStringLiteral("nv.src.core.net"), QStringLiteral("f2"),
                  QStringLiteral("DeviceIdentity.cpp"), Glyph::File);
    tree->addNode(QStringLiteral("nv.src.core.net"), QStringLiteral("f3"),
                  QStringLiteral("ConnectionProfile.cpp"), Glyph::File);
    tree->setTag(QStringLiteral("f3"), QStringLiteral("A"), Badge::Tone::Success);

    tree->addNode(QStringLiteral("nv.src"), QStringLiteral("nv.src.ui"),
                  QStringLiteral("ui"), Glyph::Folder);
    tree->setCount(QStringLiteral("nv.src.ui"), 41);

    tree->addNode(QStringLiteral("nv"), QStringLiteral("nv.docs"),
                  QStringLiteral("documents"), Glyph::Book);
    tree->setMarks(QStringLiteral("nv.docs"), {Glyph::Link});

    tree->addNode(QStringLiteral("ws"), QStringLiteral("rs"), QStringLiteral("Research"),
                  Glyph::Folder, TreeView::Kind::Emphasis);
    tree->addNode(QStringLiteral("rs"), QStringLiteral("rs.a"),
                  QStringLiteral("embeddings"), Glyph::Folder);
    tree->addNode(QStringLiteral("rs"), QStringLiteral("rs.b"),
                  QStringLiteral("benchmarks"), Glyph::Chart);

    tree->addNode(QStringLiteral("ws"), QStringLiteral("ar"),
                  QStringLiteral("Archive 2024"), Glyph::Archive,
                  TreeView::Kind::Emphasis);
    tree->setMark(QStringLiteral("ar"), TreeView::Mark::Detached);

    tree->addNode(QString(), QStringLiteral("sys"), QStringLiteral("System"),
                  Glyph::Settings, TreeView::Kind::Group);
    tree->setExpanded(QStringLiteral("sys"), true);
    tree->addNode(QStringLiteral("sys"), QStringLiteral("sys.a"),
                  QStringLiteral("Diagnostics"), Glyph::Diagnostics);
    tree->setCount(QStringLiteral("sys.a"), 2);
    tree->addNode(QStringLiteral("sys"), QStringLiteral("sys.b"),
                  QStringLiteral("Licences"), Glyph::Key);
    tree->setNodeEnabled(QStringLiteral("sys.b"), false);
}

// The Novu sidebar, as NovuAI, NovuSpace and NovuExplorer draw it: a section
// heading, workspaces one step under it, categories one step under those.
// Every row keeps a place for its colour, so the names of a level line up.
// The colours are sample data - what a person picked for their workspaces.
void fillSidebar(TreeView* tree)
{
    const QColor green(0x58, 0xB0, 0x9E);
    const QColor orange(0xE0, 0xA0, 0x50);
    const QColor lime(0xB9, 0xCC, 0x5A);

    tree->setGuidesVisible(false);
    tree->setMarkerColumn(true);

    tree->addNode(QString(), QStringLiteral("sec"), QStringLiteral("Workspaces"),
                  Glyph::None, TreeView::Kind::Group);
    tree->setExpanded(QStringLiteral("sec"), true);
    tree->setRowAction(QStringLiteral("sec"), Glyph::Plus, QStringLiteral("New workspace"));

    const auto workspace = [tree](const QString& id, const QString& name, const QColor& colour,
                                  bool open) {
        tree->addNode(QStringLiteral("sec"), id, name, Glyph::Workspace,
                      TreeView::Kind::Emphasis);
        tree->setMarker(id, colour);
        tree->setExpanded(id, open);
        tree->setRowAction(id, Glyph::Plus, QStringLiteral("New category"));
    };
    const auto category = [tree](const QString& parent, const QString& id, const QString& name,
                                 const QColor& colour, int count) {
        tree->addNode(parent, id, name, Glyph::Layers);
        tree->setMarker(id, colour);
        tree->setCount(id, count);
    };

    workspace(QStringLiteral("gen"), QStringLiteral("General"), QColor(), true);
    category(QStringLiteral("gen"), QStringLiteral("gen.home"), QStringLiteral("Home"), QColor(), 0);

    workspace(QStringLiteral("novu"), QStringLiteral("Novu"), green, true);
    for (const char* name : {"General", "NovuAI", "NovuSpace"}) {
        const QString label = QString::fromLatin1(name);
        category(QStringLiteral("novu"), QStringLiteral("novu.") + label, label, green, 0);
    }
    // A category with a colour of its own wears it; the rest wear the
    // workspace's.
    category(QStringLiteral("novu"), QStringLiteral("novu.hub"), QStringLiteral("NovuHub"), lime, 4);
    category(QStringLiteral("novu"), QStringLiteral("novu.exp"), QStringLiteral("NovuExplorer"),
             green, 3);
    tree->setExpanded(QStringLiteral("novu.exp"), true);
    // A subcategory is a folder: one level down, a different shape.
    tree->addNode(QStringLiteral("novu.exp"), QStringLiteral("novu.exp.rel"),
                  QStringLiteral("Releases"), Glyph::Folder);
    tree->setMarker(QStringLiteral("novu.exp.rel"), green);
    tree->addNode(QStringLiteral("novu.exp"), QStringLiteral("novu.exp.design"),
                  QStringLiteral("Design"), Glyph::Folder);
    tree->setMarker(QStringLiteral("novu.exp.design"), green);
    // NovuAI's chats keep the chat bubble.
    tree->addNode(QStringLiteral("novu.exp"), QStringLiteral("novu.exp.chat"),
                  QStringLiteral("Sidebar review"), Glyph::Chat);
    tree->setMarker(QStringLiteral("novu.exp.chat"), green);
    for (const char* name : {"NovuVoice", "Houdini", "Design"}) {
        const QString label = QString::fromLatin1(name);
        category(QStringLiteral("novu"), QStringLiteral("novu.") + label, label, green, 0);
    }
    tree->setCount(QStringLiteral("novu"), 9);

    workspace(QStringLiteral("cl"), QStringLiteral("Clients"), orange, false);
    category(QStringLiteral("cl"), QStringLiteral("cl.es"), QStringLiteral("Esantirion"), orange, 2);
    tree->setCount(QStringLiteral("cl"), 1);
    tree->setCount(QStringLiteral("gen"), 1);

    tree->setCurrentId(QStringLiteral("novu.exp"));
}

// The same sidebar as a NavList rail, NovuSpace's: setMarkerColumn(true)
// makes it TreeView's tree to the pixel.
void fillSidebarRail(NavList* nav)
{
    const QColor green(0x58, 0xB0, 0x9E);
    const QColor orange(0xE0, 0xA0, 0x50);
    const QColor lime(0xB9, 0xCC, 0x5A);

    nav->setMarkerColumn(true);
    nav->setFilledGlyphs(true);
    nav->addSection(QStringLiteral("Workspaces"), QStringLiteral("sec"));
    nav->setSectionCollapsible(QStringLiteral("sec"), true);
    nav->setSectionAction(QStringLiteral("sec"), Glyph::Plus, QStringLiteral("New workspace"));

    nav->addItem(QStringLiteral("gen"), QStringLiteral("General"), Glyph::Workspace);
    nav->setCount(QStringLiteral("gen"), 1);
    nav->setExpandable(QStringLiteral("gen"), true);
    nav->addChild(QStringLiteral("gen.home"), QStringLiteral("Home"), Glyph::Layers);
    nav->setCount(QStringLiteral("gen.home"), 0);
    nav->setExpanded(QStringLiteral("gen"), true);

    nav->addItem(QStringLiteral("novu"), QStringLiteral("Novu"), Glyph::Workspace);
    nav->setMarker(QStringLiteral("novu"), green);
    nav->setCount(QStringLiteral("novu"), 5);
    nav->setExpandable(QStringLiteral("novu"), true);
    for (const char* name : {"General", "NovuAI", "NovuHub", "NovuExplorer", "Houdini"}) {
        const QString label = QString::fromLatin1(name);
        const QString key = QStringLiteral("novu.") + label;
        nav->addChild(key, label, Glyph::Layers);
        nav->setMarker(key, label == QLatin1String("NovuHub") ? lime : green);
        nav->setCount(key, 0);
    }
    nav->setExpanded(QStringLiteral("novu"), true);

    nav->addItem(QStringLiteral("cl"), QStringLiteral("Clients"), Glyph::Workspace);
    nav->setMarker(QStringLiteral("cl"), orange);
    nav->setCount(QStringLiteral("cl"), 1);
    nav->setExpandable(QStringLiteral("cl"), true);
    nav->addChild(QStringLiteral("cl.es"), QStringLiteral("Esantirion"), Glyph::Layers);
    nav->setMarker(QStringLiteral("cl.es"), orange);

    nav->setCurrentKey(QStringLiteral("novu.NovuExplorer"));
}

// ── SelectionBar ─────────────────────────────────────────────────────────

QWidget* buildSelectionBar(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);
    auto* structure=new TreeView(page);structure->addNode({},"root","Composer",Glyph::Layers);structure->addNode("root","button","Send",Glyph::Check);structure->setExpanded("root",true);structure->setMeta("button","button");structure->setBranchesVisible(false);structure->setGuidesVisible(false);structure->setRowHeight(26);structure->setFixedHeight(90);column->addWidget(structure);


    {
        Card* card = exampleCard(page, QStringLiteral("Counts"),
                                 QStringLiteral("And the one it refuses to show."));
        QVBoxLayout* body = card->body();

        auto* bar = new SelectionBar(card);
        bar->setNoun(QStringLiteral("workspace"));
        auto* archive = new Button(QStringLiteral("Archive"), Button::Variant::Secondary,
                                   bar->actionsWidget());
        archive->setControlSize(ControlSize::Small);
        bar->actions()->addWidget(archive);
        auto* remove = new Button(QStringLiteral("Delete"), Button::Variant::Danger,
                                  bar->actionsWidget());
        remove->setControlSize(ControlSize::Small);
        bar->actions()->addWidget(remove);
        body->addWidget(bar);

        auto* state = label(QStringLiteral("hidden"), card, TypeRole::Mono, Ink::Secondary);
        QObject::connect(bar, &SelectionBar::shownChanged, state, [state](bool shown) {
            state->setText(shown ? QStringLiteral("shown") : QStringLiteral("hidden"));
        });

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Select"));
        for (const int count : {0, 1, 2, 17}) {
            auto* button = new Button(QString::number(count), Button::Variant::Secondary, card);
            button->setControlSize(ControlSize::Small);
            QObject::connect(button, &Button::clicked, bar,
                             [bar, count] { bar->setCount(count); });
            row->addWidget(button);
        }
        row->addSpacing(12);
        row->addWidget(state);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("One and two: the singular and the plural are produced by the "
                            "component, so twelve call sites do not each write their own "
                            "\"1 item\" branch. Zero: the bar is gone, and shownChanged tells "
                            "a caller so it can reclaim the space without watching visibility."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("A sentence of its own"),
                                 QStringLiteral("When a count is not the whole "
                                                "story."));
        QVBoxLayout* body = card->body();

        auto* bar = new SelectionBar(card);
        bar->setCount(12);
        bar->setText(QStringLiteral("12 of 40 rows · 3 cannot be exported"));
        auto* go = new Button(QStringLiteral("Export the rest"), Button::Variant::Primary,
                              bar->actionsWidget());
        go->setControlSize(ControlSize::Small);
        bar->actions()->addWidget(go);
        body->addWidget(bar);

        note(body, card,
             QStringLiteral("setText() overrides the built sentence for the cases a count "
                            "alone cannot express. The count still governs whether the bar is "
                            "there, because that is the decision it exists to make."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    if (ctx.scenario == Scenario::Empty) {
        column->addWidget(scenarioEmptyState(page, QStringLiteral("a structure")));
        column->addStretch(1);
        return page;
    }

    // ── the Novu sidebar ─────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("The Novu sidebar"),
                                 QStringLiteral("Workspaces under a heading, categories "
                                                "under them - one tree."));
        QVBoxLayout* body = card->body();

        auto* tree = new TreeView(card);
        tree->setObjectName(QStringLiteral("novuSidebarExample"));
        fillSidebar(tree);
        tree->setFixedSize(238, 420);

        auto* rail = new NavList(card);
        rail->setObjectName(QStringLiteral("novuSidebarRailExample"));
        fillSidebarRail(rail);
        rail->setFixedWidth(238);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Sidebar"));
        row->addWidget(tree, 0, Qt::AlignTop);
        row->addSpacing(24);
        row->addWidget(rail, 0, Qt::AlignTop);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("setMarkerColumn(true): every row keeps the colour bar's place, "
                            "so names of one level line up whether or not they have a colour. "
                            "A workspace is a hexagon, a category the layers, a subcategory a "
                            "folder and a chat a bubble. Each level is one treeStep (14px) "
                            "further in, the workspaces too: a sidebar is always a tree. "
                            "Right: the same sidebar as a NavList rail (NovuSpace's) with "
                            "NavList::setMarkerColumn(true) - the same tree to the pixel."));
        column->addWidget(card);
    }

    // ── the tree ─────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("A structure"),
                                 QStringLiteral("Arbitrary depth, an indent guide "
                                                "per level, and one widget."));
        QVBoxLayout* body = card->body();

        auto* tree = new TreeView(card);
        fillWorkspace(tree);
        tree->setCurrentId(QStringLiteral("f1"));
        tree->setFixedSize(300, 360);

        auto* signal = label(QStringLiteral("f1"), card, TypeRole::Mono, Ink::Secondary);
        QObject::connect(tree, &TreeView::activated, signal, &QLabel::setText);
        QObject::connect(tree, &TreeView::expandToggled, signal,
                         [signal](const QString& id, bool open) {
                             signal->setText(QStringLiteral("%1 %2").arg(
                                 id, open ? QStringLiteral("open")
                                          : QStringLiteral("shut")));
                         });

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Tree"));
        row->addWidget(tree, 0, Qt::AlignTop);
        row->addSpacing(20);

        auto* side = new QWidget(card);
        auto* sideColumn = new QVBoxLayout(side);
        sideColumn->setContentsMargins(0, 0, 0, 0);
        sideColumn->setSpacing(8);
        sideColumn->addWidget(label(QStringLiteral("LAST SIGNAL"), side, TypeRole::Eyebrow,
                                    Ink::Muted));
        sideColumn->addWidget(signal);
        sideColumn->addSpacing(8);

        const struct { const char* caption; int kind; } buttons[] = {
            {"Expand all", 0}, {"Collapse all", 1},
            {"Reveal TunnelController.cpp", 2}, {"Compact", 3},
        };
        for (const auto& entry : buttons) {
            auto* button = new Button(QString::fromLatin1(entry.caption),
                                      Button::Variant::Secondary, side);
            button->setControlSize(ControlSize::Small);
            const int kind = entry.kind;
            QObject::connect(button, &Button::clicked, tree, [tree, kind, button] {
                switch (kind) {
                case 0: tree->expandAll(); break;
                case 1: tree->collapseAll(); break;
                case 2:
                    tree->revealNode(QStringLiteral("f1"));
                    tree->setCurrentId(QStringLiteral("f1"));
                    break;
                default:
                    tree->setCompact(!tree->isCompact());
                    button->setText(tree->isCompact() ? QObject::tr("Comfortable")
                                                      : QObject::tr("Compact"));
                    break;
                }
            });
            sideColumn->addWidget(button);
        }
        sideColumn->addStretch(1);
        row->addWidget(side, 1, Qt::AlignTop);

        note(body, card,
             QStringLiteral("Click a chevron, then click the name on the same row. Separate "
                            "targets, as in every file manager: opening a folder and selecting "
                            "it are different actions. Collapse all, then press Reveal — the "
                            "tree opens every ancestor and scrolls to the file, which is what "
                            "\"go to definition\" needs and what a caller should not have to "
                            "write."));
        note(body, card,
             QStringLiteral("Two hundred visible nodes here would be a thousand widgets built "
                            "the obvious way, and most of them hidden most of the time — a "
                            "thousand widgets that exist to be collapsed. The expanded set is "
                            "flattened into a vector and only the rows on screen are drawn."));
        column->addWidget(card);
    }

    // ── what a row carries ───────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("What a row carries"),
                                 QStringLiteral("Kinds, marks and states."));
        QVBoxLayout* body = card->body();

        auto* tree = new TreeView(card);
        tree->addNode(QString(), QStringLiteral("g"), QStringLiteral("A caption"),
                      Glyph::Folder, TreeView::Kind::Group);
        tree->setExpanded(QStringLiteral("g"), true);
        tree->setCount(QStringLiteral("g"), 6);

        tree->addNode(QStringLiteral("g"), QStringLiteral("a"), QStringLiteral("Ordinary"),
                      Glyph::File);
        tree->addNode(QStringLiteral("g"), QStringLiteral("b"),
                      QStringLiteral("Emphasised"), Glyph::Folder,
                      TreeView::Kind::Emphasis);
        tree->addNode(QStringLiteral("g"), QStringLiteral("c"),
                      QStringLiteral("In scope"), Glyph::Folder, TreeView::Kind::Emphasis);
        tree->setMark(QStringLiteral("c"), TreeView::Mark::Active);
        tree->addNode(QStringLiteral("g"), QStringLiteral("d"),
                      QStringLiteral("Not loaded"), Glyph::Archive);
        tree->setMark(QStringLiteral("d"), TreeView::Mark::Detached);
        tree->addNode(QStringLiteral("g"), QStringLiteral("e"),
                      QStringLiteral("A drag is over this"), Glyph::Folder);
        tree->setMark(QStringLiteral("e"), TreeView::Mark::Dropping);
        tree->addNode(QStringLiteral("g"), QStringLiteral("f"),
                      QStringLiteral("Unavailable"), Glyph::Lock);
        tree->setNodeEnabled(QStringLiteral("f"), false);
        tree->addNode(QStringLiteral("g"), QStringLiteral("h"),
                      QStringLiteral("With a tag and marks"), Glyph::File);
        tree->setTag(QStringLiteral("h"), QStringLiteral("main"), Badge::Tone::Info);
        tree->setMarks(QStringLiteral("h"), {Glyph::Eye, Glyph::Link});
        tree->setCurrentId(QStringLiteral("a"));
        tree->setFixedSize(320, 250);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Rows"));
        row->addWidget(tree, 0, Qt::AlignTop);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Hover any row: the marks on the right brighten. They are "
                            "properties of the node rather than actions on it — shared, "
                            "watched, modified — so they never take a click, and a row whose "
                            "marks were buttons would be a row with three ways to miss it."));
        note(body, card,
             QStringLiteral("\"Not loaded\" is italic rather than a second shade of grey. The "
                            "row is already muted, and a muted grey on a muted grey is "
                            "invisible — which is what the design's `.detached` avoids."));
        column->addWidget(card);
    }

    // ── with a selection bar ─────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("With a selection"),
                                 QStringLiteral("The bar that appears under a "
                                                "list — and is not there at zero."));
        QVBoxLayout* body = card->body();

        auto* holder = new QWidget(card);
        holder->setFixedWidth(320);
        auto* stack = new QVBoxLayout(holder);
        stack->setContentsMargins(0, 0, 0, 0);
        stack->setSpacing(0);

        auto* tree = new TreeView(holder);
        tree->addNode(QString(), QStringLiteral("root"), QStringLiteral("orders"),
                      Glyph::Folder);
        tree->setExpanded(QStringLiteral("root"), true);
        for (int i = 1; i <= 6; ++i) {
            const QString id = QStringLiteral("f%1").arg(i);
            tree->addNode(QStringLiteral("root"), id,
                          QStringLiteral("orders-2026-%1.csv").arg(i, 2, 10, QLatin1Char('0')),
                          Glyph::File);
        }
        tree->setFixedHeight(220);
        stack->addWidget(tree);

        auto* bar = new SelectionBar(holder);
        bar->setNoun(QStringLiteral("file"));
        auto* remove = new Button(QStringLiteral("Delete"), Button::Variant::Danger,
                                  bar->actionsWidget());
        remove->setControlSize(ControlSize::Small);
        bar->actions()->addWidget(remove);
        auto* exportButton = new Button(QStringLiteral("Export"), Button::Variant::Secondary,
                                        bar->actionsWidget());
        exportButton->setControlSize(ControlSize::Small);
        bar->actions()->addWidget(exportButton);
        stack->addWidget(bar);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("List and bar"));
        row->addWidget(holder, 0, Qt::AlignTop);
        row->addSpacing(20);

        auto* side = new QWidget(card);
        auto* sideColumn = new QVBoxLayout(side);
        sideColumn->setContentsMargins(0, 0, 0, 0);
        sideColumn->setSpacing(6);
        for (const int count : {0, 1, 3, 6}) {
            auto* button = new Button(QStringLiteral("%1 selected").arg(count),
                                      Button::Variant::Ghost, side);
            button->setControlSize(ControlSize::Small);
            QObject::connect(button, &Button::clicked, bar,
                             [bar, count] { bar->setCount(count); });
            sideColumn->addWidget(button);
        }
        sideColumn->addStretch(1);
        row->addWidget(side, 1, Qt::AlignTop);

        note(body, card,
             QStringLiteral("Press \"0 selected\". The bar is not there — hiding at zero is the "
                            "component's job, not a state twelve call sites have to handle. A "
                            "bar reading \"0 selected\" with a Delete button in it invites "
                            "exactly the wrong click, which is what every hand-rolled version "
                            "of this did."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec treeViewComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("tree-view");
    spec.name = QStringLiteral("TreeView");
    spec.category = Category::Navigation;
    spec.description = QStringLiteral(
        "A structure of nested things at arbitrary depth, with indent guides, per-row "
        "chevrons, marks, counts and tags — flattened and virtualised into one widget.");
    spec.headerPath = QStringLiteral("include/novu/ui/TreeView.h");
    spec.sourcePath = QStringLiteral("src/TreeView.cpp");
    spec.designedFor = {QStringLiteral("novuai")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Item"), QStringLiteral("Group"),
                     QStringLiteral("Emphasis")};
    spec.states = {QStringLiteral("Current"), QStringLiteral("Hover"),
                   QStringLiteral("Active"), QStringLiteral("Detached"),
                   QStringLiteral("Dropping"), QStringLiteral("Disabled")};
    spec.scenarios = {Scenario::Normal, Scenario::Empty};
    spec.keywords = {QStringLiteral("tree"), QStringLiteral("trow"),
                     QStringLiteral("tkids"), QStringLiteral("files"),
                     QStringLiteral("explorer"), QStringLiteral("hierarchy"),
                     QStringLiteral("workspace")};
    spec.properties = {
        {QStringLiteral("kind"), QStringLiteral("enum"),
         {QStringLiteral("Item"), QStringLiteral("Group"), QStringLiteral("Emphasis")},
         QStringLiteral("Item"), QStringLiteral("Typography, and whether it is a caption.")},
        {QStringLiteral("mark"), QStringLiteral("enum"),
         {QStringLiteral("None"), QStringLiteral("Active"), QStringLiteral("Detached"),
          QStringLiteral("Dropping")},
         QStringLiteral("None"), QStringLiteral("State that is not selection.")},
        {QStringLiteral("compact"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("A density for the whole tree; per-row heights would make every "
                        "hit test a search.")},
    };
    spec.build = build;
    return spec;
}

ComponentSpec selectionBarComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("selection-bar");
    spec.name = QStringLiteral("SelectionBar");
    spec.category = Category::Navigation;
    spec.description = QStringLiteral(
        "The tinted strip under a list saying how much is selected and what can be done "
        "with it. Hides itself at zero, which is the state every hand-rolled one got wrong.");
    spec.headerPath = QStringLiteral("include/novu/ui/SelectionBar.h");
    spec.sourcePath = QStringLiteral("src/SelectionBar.cpp");
    spec.designedFor = {QStringLiteral("novuai")};
    spec.usedBy = {};
    spec.states = {QStringLiteral("Hidden"), QStringLiteral("One"),
                   QStringLiteral("Many")};
    spec.keywords = {QStringLiteral("sel-bar"), QStringLiteral("selection"),
                     QStringLiteral("bulk"), QStringLiteral("multi-select"),
                     QStringLiteral("batch")};
    spec.properties = {
        {QStringLiteral("count"), QStringLiteral("int"), {}, QStringLiteral("0"),
         QStringLiteral("Zero hides the bar entirely.")},
        {QStringLiteral("noun"), QStringLiteral("QString"), {}, QStringLiteral(""),
         QStringLiteral("\"file\" becomes \"3 files selected\"; the plural is derived.")},
    };
    spec.build = buildSelectionBar;
    return spec;
}

} // namespace novu::playground::pages
