#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/Form.h"
#include "novu/ui/IconButton.h"
#include "novu/ui/NavList.h"
#include "novu/ui/Separator.h"
#include "novu/ui/Splitter.h"
#include "novu/ui/TabBar.h"
#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/TreeView.h"
#include "novu/ui/WidgetEditor.h"
#include "novu/ui/Feedback.h"
#include "novu/ui/Segmented.h"

#include <QHBoxLayout>

#include <utility>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

QWidget* panel(QWidget* parent, const QString& caption, ColorRole ground)
{
    auto* widget = new QWidget(parent);
    widget->setAutoFillBackground(true);
    QPalette palette = widget->palette();
    palette.setColor(QPalette::Window, token(ground).resolve(themeFor(parent)));
    widget->setPalette(palette);
    widget->setMinimumWidth(60);

    auto* column = new QVBoxLayout(widget);
    column->setContentsMargins(12, 12, 12, 12);
    column->addWidget(label(caption, widget, TypeRole::Eyebrow, Ink::Muted));
    column->addStretch(1);
    return widget;
}

// ── Splitter ─────────────────────────────────────────────────────────────

QWidget* buildSplitter(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    {
        Card* card = exampleCard(page, QStringLiteral("Drag, or put a panel away"),
                                 QStringLiteral("The handle is two buttons and a "
                                                "drag band."));
        QVBoxLayout* body = card->body();

        auto* splitter = new Splitter(Qt::Horizontal, card);
        splitter->setMinimumHeight(240);
        splitter->addWidget(panel(splitter, QStringLiteral("LEFT"), ColorRole::PanelBg));
        splitter->addWidget(panel(splitter, QStringLiteral("RIGHT"), ColorRole::WindowBg));
        splitter->setSizes({240, 360});
        body->addWidget(splitter);

        auto* state = label(QStringLiteral("both open"), card, TypeRole::Mono,
                            Ink::Secondary);
        QObject::connect(splitter, &Splitter::panelCollapsedChanged, state,
                         [state, splitter](int index, bool collapsed) {
                             state->setText(QStringLiteral("panel %1 %2")
                                                .arg(index)
                                                .arg(collapsed ? QStringLiteral("collapsed")
                                                               : QStringLiteral("restored")));
                             Q_UNUSED(splitter);
                         });

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("From code"));
        for (const int index : {0, 1}) {
            auto* button = new Button(QStringLiteral("Toggle panel %1").arg(index),
                                      Button::Variant::Secondary, card);
            button->setControlSize(ControlSize::Small);
            QObject::connect(button, &Button::clicked, splitter,
                             [splitter, index] { splitter->togglePanel(index); });
            row->addWidget(button);
        }
        row->addSpacing(12);
        row->addWidget(state);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("The top thirty pixels of the handle collapse the left panel and "
                            "the bottom thirty collapse the right; between them it drags. "
                            "Each triangle turns round once its panel is gone, so the same "
                            "arrow says both \"put this away\" and \"bring it back\"."));
        note(body, card,
             QStringLiteral("Collapse a panel, then restore it: it comes back at the width it "
                            "had, not at whatever the layout would decide. Collapsing sets a "
                            "size to zero rather than hiding the widget — a hidden widget "
                            "loses its place in the splitter's size list, which is why the "
                            "naive implementation always restores to the wrong width."));
        note(body, card,
             QStringLiteral("Press an arrow and release in the drag band: nothing happens. A "
                            "press that began on the arrow never becomes a drag either, "
                            "however far the pointer travels."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("Vertical, and without arrows"),
                                 QStringLiteral("A plain drag handle for panels "
                                                "that are not optional."));
        QVBoxLayout* body = card->body();

        auto* splitter = new Splitter(Qt::Vertical, card);
        splitter->setCollapseArrows(false);
        splitter->setMinimumHeight(200);
        splitter->addWidget(panel(splitter, QStringLiteral("TOP"), ColorRole::PanelBg));
        splitter->addWidget(panel(splitter, QStringLiteral("BOTTOM"), ColorRole::WindowBg));
        splitter->setSizes({120, 80});
        body->addWidget(splitter);

        note(body, card,
             QStringLiteral("Qt's own handle is a raised bevel with dots in it — the one piece "
                            "of platform chrome that survived every previous attempt at a Novu "
                            "look, because no application styled it. A stylesheet fixes the "
                            "colour and nothing else: it cannot put a control on a handle, and "
                            "the arrows are the reason this is a component."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("A strip that only folds"),
                                 QStringLiteral("CollapseDivider — no drag, "
                                                "because there is nothing to "
                                                "resize."));
        QVBoxLayout* body = card->body();

        auto* strip = new QWidget(card);
        auto* stripRow = new QHBoxLayout(strip);
        stripRow->setContentsMargins(0, 4, 0, 4);
        stripRow->setSpacing(6);
        for (const char* name : {"Novusion", "Research", "Archive"}) {
            auto* button = new Button(QString::fromLatin1(name), Button::Variant::Ghost,
                                      strip);
            button->setControlSize(ControlSize::Small);
            stripRow->addWidget(button);
        }
        stripRow->addStretch(1);
        body->addWidget(strip);

        auto* divider = new CollapseDivider(strip, card);
        body->addWidget(divider);

        auto* state = label(QStringLiteral("open"), card, TypeRole::Mono, Ink::Secondary);
        QObject::connect(divider, &CollapseDivider::collapsedChanged, state,
                         [state](bool collapsed) {
                             state->setText(collapsed ? QStringLiteral("folded")
                                                      : QStringLiteral("open"));
                         });
        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("State"));
        row->addWidget(state);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Click the divider. A fixed-height toolbar strip wants to be put "
                            "away, not resized — and a QSplitter offering a drag the layout "
                            "will immediately undo is a control that lies about what it does."));
        column->addWidget(card);
    }

    auto* thinCard=exampleCard(page,QStringLiteral("One-pixel divider"),QStringLiteral("Explicit handle width is preserved."));
    auto* thin=new Splitter(Qt::Horizontal,thinCard);
    thin->setHandleWidth(1);
    thin->setCollapseArrows(false);
    thin->addWidget(panel(thin,QStringLiteral("LEFT"),ColorRole::PanelBg));
    thin->addWidget(panel(thin,QStringLiteral("RIGHT"),ColorRole::WindowBg));
    thin->setMinimumHeight(120);
    thinCard->body()->addWidget(thin);
    column->addWidget(thinCard);
    auto* editing=exampleCard(page,QStringLiteral("Edit real widgets"),QStringLiteral("Pick a widget, resize a constraint, or drag to request a document move."));
    auto* stage=new QWidget(editing);stage->setMinimumHeight(170);
    auto* sample=new Button(QStringLiteral("Pick and resize"),Button::Variant::Primary,stage);sample->setGeometry(40,60,150,32);
    auto* editor=new WidgetEditor(stage);editor->registerWidget(sample,QStringLiteral("Button"));editor->setSnapStep(8);editor->setCurrentWidget(sample);
    QObject::connect(editor,&WidgetEditor::dragMoved,stage,[editor,stage](QWidget*,QPoint global){
        const auto local=stage->mapFromGlobal(global);
        editor->setDropFeedback(local.x()>200?QRect(210,40,110,90):QRect(),true);
    });
    auto* readout=label(QStringLiteral("No document move requested"),editing,TypeRole::Caption,Ink::Muted);
    QObject::connect(editor,&WidgetEditor::dropRequested,readout,[readout](QWidget*,QPoint){readout->setText(QStringLiteral("Document move requested"));});
    auto* progress=new ProgressBar(50,editing);progress->setTrackMetrics(5,3);progress->setTrackBackground(token(ColorRole::BorderHover));editing->body()->addWidget(progress);
    auto* filename=new ElidedLabel(QStringLiteral("ui/src/views/chat/ComposerPanel.cpp"),editing);filename->setElideMode(Qt::ElideLeft);filename->setMaximumWidth(160);editing->body()->addWidget(filename);
    auto* noSelection=new Segmented({QStringLiteral("One"),QStringLiteral("Two")},editing,-1);editing->body()->addWidget(noSelection);
    editing->body()->addWidget(stage);editing->body()->addWidget(readout);column->addWidget(editing);
    column->addStretch(1);
    return page;
}

// ── TabBar ───────────────────────────────────────────────────────────────

QWidget* buildTabBar(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    if (ctx.scenario == Scenario::Empty) {
        column->addWidget(scenarioEmptyState(page, QStringLiteral("open documents")));
        column->addStretch(1);
        return page;
    }

    {
        Card* card = exampleCard(page, QStringLiteral("Open documents"),
                                 QStringLiteral("States, colours, reorder and a "
                                                "close box that is its own "
                                                "target."));
        QVBoxLayout* body = card->body();

        auto* tabs = new TabBar(card);
        tabs->setNewTabButton(true);

        const struct {
            const char* key;
            const char* label;
            Glyph glyph;
            TabBar::State state;
            const char* colour;
        } sample[] = {
            {"a", "TunnelController.cpp", Glyph::File, TabBar::State::Modified, "#4DA3FF"},
            {"b", "DeviceIdentity.cpp", Glyph::File, TabBar::State::Idle, "#4DA3FF"},
            {"c", "peers.py", Glyph::File, TabBar::State::Loading, "#6ADB9E"},
            {"d", "schema.sql", Glyph::File, TabBar::State::Failed, ""},
            {"e", "NOTES.md", Glyph::Book, TabBar::State::Asleep, "#FFB14D"},
        };
        for (const auto& entry : sample) {
            const QString key = QString::fromLatin1(entry.key);
            tabs->addTab(key, QString::fromLatin1(entry.label), entry.glyph);
            tabs->setState(key, entry.state);
            if (*entry.colour) {
                tabs->setColor(key, QColor(QString::fromLatin1(entry.colour)));
            }
        }
        tabs->setCurrentKey(QStringLiteral("a"));
        body->addWidget(tabs);

        auto* signal = label(QStringLiteral("a"), card, TypeRole::Mono, Ink::Secondary);
        QObject::connect(tabs, &TabBar::currentChanged, signal, &QLabel::setText);
        QObject::connect(tabs, &TabBar::closeRequested, tabs,
                         [tabs, signal](const QString& key) {
                             signal->setText(QStringLiteral("close %1").arg(key));
                             tabs->removeTab(key);
                         });
        QObject::connect(tabs, &TabBar::tabMoved, signal,
                         [signal](const QString& key, int from, int to) {
                             signal->setText(QStringLiteral("%1 moved %2 → %3")
                                                 .arg(key)
                                                 .arg(from)
                                                 .arg(to));
                         });
        int* opened = new int(0);
        QObject::connect(tabs, &TabBar::newTabRequested, tabs, [tabs, opened] {
            const int index = ++(*opened);
            const QString key = QStringLiteral("new%1").arg(index);
            tabs->addTab(key, QObject::tr("Untitled %1").arg(index), Glyph::File);
            tabs->setCurrentKey(key);
        });
        QObject::connect(tabs, &QObject::destroyed, card, [opened] { delete opened; });

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Last signal"));
        row->addWidget(signal);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Drag a tab sideways. The colour and the state travel with it, "
                            "because both live on the tab rather than in a list beside it — "
                            "NovuExplorer's ColoredTabBar got this right, and every "
                            "implementation that kept colours alongside got them out of step "
                            "the first time a tab moved."));
        note(body, card,
             QStringLiteral("The close box appears on the current tab and the hovered one, and "
                            "nowhere else: a strip of thirty close boxes is thirty invitations "
                            "to lose work. It is a separate target from the tab, because "
                            "closing a document and switching to it are very different "
                            "mistakes to make. Middle-click closes too, as it does in every "
                            "browser."));
        note(body, card,
             QStringLiteral("Closing does not remove the tab — the bar asks. A document with "
                            "unsaved changes has to be able to answer, and a bar that had "
                            "already closed it would be asking about something gone. This page "
                            "removes it in the handler, which is what an application without "
                            "unsaved state does."));
        column->addWidget(card);
    }

    {
        // The browser case: no icon set has a glyph for `novuai.app`, so the
        // page's own initial is the mark - and the state is a dot after the
        // label rather than something drawn over that initial.
        Card* card = exampleCard(page, QStringLiteral("Pages, not files"),
                                 QStringLiteral("A lettered mark, a state dot, "
                                                "and the strip's own actions."));
        QVBoxLayout* body = card->body();

        auto* tabs = new TabBar(card);
        tabs->setNewTabButton(true);

        const struct {
            const char* key;
            const char* label;
            const char* mark;
            TabBar::State state;
        } pages[] = {
            {"p1", "NovuAI - Console", "N", TabBar::State::Idle},
            {"p2", "arXiv - retrieval", "A", TabBar::State::Idle},
            {"p3", "Benchmarks sheet", "B", TabBar::State::Asleep},
            {"p4", "Status page", "S", TabBar::State::Failed},
            {"p5", "Grafana - inference", "G", TabBar::State::Loading},
        };
        for (const auto& entry : pages) {
            const QString key = QString::fromLatin1(entry.key);
            tabs->addTab(key, QString::fromLatin1(entry.label));
            tabs->setMarkText(key, QString::fromLatin1(entry.mark));
            tabs->setState(key, entry.state);
        }
        tabs->setCurrentKey(QStringLiteral("p1"));

        // `.btabs-row .bacts` - what belongs to the set rather than to one tab.
        auto* acts = new QWidget(tabs);
        auto* actsRow = new QHBoxLayout(acts);
        actsRow->setContentsMargins(8, 0, 8, 0);
        actsRow->setSpacing(5);
        for (const auto& pair : {std::make_pair(Glyph::Users, "Sessions here"),
                                 std::make_pair(Glyph::Overview, "Overview")}) {
            auto* action = new IconButton(pair.first, acts);
            action->setVariant(IconButton::Variant::Ghost);
            action->setControlSize(ControlSize::Small);
            action->setToolTip(QString::fromLatin1(pair.second));
            actsRow->addWidget(action);
        }
        tabs->setTrailingWidget(acts);
        body->addWidget(tabs);

        note(body, card,
             QStringLiteral("The mark is the document's, so the document's state does not "
                            "take its place: a failed page keeps its initial and says so "
                            "with a dot after the label. Loading is the exception - a page "
                            "still arriving has no identity to protect yet, so the spinner "
                            "sits where its mark will be."));
        note(body, card,
             QStringLiteral("The two actions at the trailing edge belong to the whole set "
                            "rather than to any one tab, and they ride on the strip instead "
                            "of getting a bar of their own. The tabs lay out in what is "
                            "left, so they shrink around the actions rather than sliding "
                            "underneath them."));
        note(body, card,
             QStringLiteral("Each tab is as wide as its own title needs, up to the cap. "
                            "Equal widths are the crowded case, not the normal one - the "
                            "card below shows what happens when the strip runs out of "
                            "room."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("Too many"),
                                 QStringLiteral("They shrink together, down to a "
                                                "floor."));
        QVBoxLayout* body = card->body();

        auto* tabs = new TabBar(card);
        for (int i = 1; i <= 14; ++i) {
            const QString key = QStringLiteral("t%1").arg(i);
            tabs->addTab(key, QStringLiteral("document-%1.md").arg(i), Glyph::File);
        }
        tabs->setCurrentIndex(3);
        body->addWidget(tabs);

        note(body, card,
             QStringLiteral("Every tab is the same width and they shrink together, which is "
                            "what a browser does. Tabs that each sized to their own title make "
                            "a ragged row whose close boxes are never in the same place twice. "
                            "Below the floor the label goes and the tooltip takes over."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("Not a TabStrip"),
                                 QStringLiteral("Two controls, not one with a "
                                                "flag."));
        QVBoxLayout* body = card->body();

        note(body, card,
             QStringLiteral("TabStrip is a row of buttons where exactly one is active: a "
                            "section switcher, a period selector. Its items are a fixed set "
                            "decided by the page, none can be closed, and they never move."));
        note(body, card,
             QStringLiteral("These are documents. They are opened and closed by the user, they "
                            "can be dragged, there can be thirty, each has a state of its own, "
                            "and the set is never known in advance. Making one class do both "
                            "is what produced three tab bars in three applications."));
        column->addWidget(card);
    }

    // ── the other strip the design draws ─────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Faces of one card"),
                                 QStringLiteral("The same component, saying "
                                                "something else entirely."));
        card->setBodyMetrics(QMargins(0, 0, 0, 0), 0);

        auto* tabs = new TabBar(card);
        // `.tabs` / `.tab` - no plate, no close box, the accent ink and a 2px
        // rule inset under the current word.
        tabs->setVariant(TabBar::Variant::Underline);
        tabs->addTab(QStringLiteral("ai"), QStringLiteral("NovuAI access"));
        tabs->addTab(QStringLiteral("general"), QStringLiteral("General"));
        tabs->addTab(QStringLiteral("notify"), QStringLiteral("Notifications"));
        tabs->addTab(QStringLiteral("data"), QStringLiteral("Data & storage"));
        tabs->setCurrentKey(QStringLiteral("ai"));
        card->body()->addWidget(tabs);

        auto* shown = label(QStringLiteral("NovuAI access"), card, TypeRole::Body,
                            Ink::Secondary);
        shown->setContentsMargins(16, 14, 16, 14);
        QObject::connect(tabs, &TabBar::currentChanged, shown,
                         [tabs, shown](const QString& key) {
                             shown->setText(tabs->label(key));
                         });
        card->body()->addWidget(shown);

        note(card->body(), card,
             QStringLiteral("Underline is the settings card, the market panel, the "
                            "parameter groups of the bot builder: a fixed set of faces "
                            "of one surface. Browser is a document strip. Drawing the "
                            "first as the second gave four faces of one card four "
                            "separate plates and put a close box on the one you were "
                            "reading."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

// ── the active panel ─────────────────────────────────────────────────────

QWidget* buildActivePanel(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    {
        Card* card = exampleCard(page, QStringLiteral("Two panels, one current row"),
                                 QStringLiteral("A selection that stays, and dims."));
        QVBoxLayout* body = card->body();

        auto* holder = new QWidget(card);
        auto* row = new QHBoxLayout(holder);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(themeFor(card).spacing.lg);

        QVector<QWidget*> panels;
        QVector<NavList*> lists;
        for (int side = 0; side < 2; ++side) {
            auto* box = new QWidget(holder);
            // The property an application sets to say which panel has the
            // user's attention. Named once in Theme.h so a panel, a list and a
            // table cannot spell it three ways.
            box->setProperty(activePanelProperty, side == 0);
            auto* stack = new QVBoxLayout(box);
            stack->setContentsMargins(0, 0, 0, 0);
            stack->setSpacing(6);
            stack->addWidget(label(side == 0 ? QStringLiteral("ACTIVE PANEL")
                                             : QStringLiteral("THE OTHER PANEL"),
                                   box, TypeRole::Eyebrow, Ink::Muted));

            auto* list = new NavList(box);
            list->addSection(QStringLiteral("Workspaces"));
            list->addItem(QStringLiteral("a"), QStringLiteral("Novusion"), Glyph::Folder);
            list->addItem(QStringLiteral("b"), QStringLiteral("Research"), Glyph::Folder);
            list->addItem(QStringLiteral("c"), QStringLiteral("Archive"), Glyph::Archive);
            list->setCurrentKey(QStringLiteral("b"));
            list->setFixedWidth(220);
            stack->addWidget(list);
            stack->addStretch(1);

            panels.append(box);
            lists.append(list);
            row->addWidget(box, 0, Qt::AlignTop);
        }
        row->addStretch(1);
        body->addWidget(holder);

        QHBoxLayout* controls = exampleRow(body, card, QStringLiteral("Focus"));
        for (int side = 0; side < 2; ++side) {
            auto* button = new Button(side == 0 ? QStringLiteral("Activate left")
                                                : QStringLiteral("Activate right"),
                                      Button::Variant::Secondary, card);
            button->setControlSize(ControlSize::Small);
            QObject::connect(button, &Button::clicked, card, [panels, lists, side] {
                for (int i = 0; i < panels.size(); ++i) {
                    panels.at(i)->setProperty(activePanelProperty, i == side);
                    lists.at(i)->update();
                }
            });
            controls->addWidget(button);
        }
        controls->addStretch(1);

        note(body, card,
             QStringLiteral("Both lists keep their selection; only the active panel's is at "
                            "full strength. A list that cleared its selection on losing focus "
                            "would forget where the user was, and one that kept it at full "
                            "strength gives a window two rows that both look current."));
        note(body, card,
             QStringLiteral("The rule came out of NovuExplorer, where a two-panel file manager "
                            "hits it hardest. The property is on the panel, not on the list, "
                            "so a search box above a list is still that list's panel — typing "
                            "in it must not dim the row the user is about to act on."));
        note(body, card,
             QStringLiteral("A widget with no panel ancestor — a dialog, a popover, a preview "
                            "— is always active. Those have no sibling to be confused with, "
                            "and the alternative is every dialog in the system drawing a "
                            "dimmed selection for no reason a user could see."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec splitterComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("splitter");
    spec.name = QStringLiteral("Splitter");
    spec.category = Category::Foundation;
    spec.description = QStringLiteral(
        "The handle between two panels, with a collapse arrow at each end — and "
        "CollapseDivider, for a fixed strip that folds but does not resize.");
    spec.headerPath = QStringLiteral("include/novu/ui/Splitter.h");
    spec.sourcePath = QStringLiteral("src/Splitter.cpp");
    spec.designedFor = {QStringLiteral("novuexplorer"), QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Horizontal"), QStringLiteral("Vertical"),
                     QStringLiteral("No arrows"), QStringLiteral("CollapseDivider")};
    spec.states = {QStringLiteral("Idle"), QStringLiteral("Hover"),
                   QStringLiteral("Dragging"), QStringLiteral("Collapsed")};
    spec.keywords = {QStringLiteral("split"), QStringLiteral("splitter"),
                     QStringLiteral("handle"), QStringLiteral("collapse"),
                     QStringLiteral("panel"), QStringLiteral("resize")};
    spec.properties = {
        {QStringLiteral("collapseArrows"), QStringLiteral("bool"), {},
         QStringLiteral("true"), QStringLiteral("Off leaves a plain drag handle.")},
        {QStringLiteral("panelCollapsed"), QStringLiteral("bool"), {},
         QStringLiteral("false"),
         QStringLiteral("Sets a size to zero; restoring returns the size it had.")},
    };
    spec.build = buildSplitter;
    return spec;
}

ComponentSpec tabBarComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("tab-bar");
    spec.name = QStringLiteral("TabBar");
    spec.category = Category::Navigation;
    spec.description = QStringLiteral(
        "Open documents across the top of a window: per-tab colour and state, drag to "
        "reorder, a close box that is its own target, and a new-tab button.");
    spec.headerPath = QStringLiteral("include/novu/ui/TabBar.h");
    spec.sourcePath = QStringLiteral("src/TabBar.cpp");
    spec.designedFor = {QStringLiteral("novuexplorer"), QStringLiteral("novuai"),
                   QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Idle"), QStringLiteral("Loading"),
                     QStringLiteral("Failed"), QStringLiteral("Modified"),
                     QStringLiteral("Asleep")};
    spec.states = {QStringLiteral("Current"), QStringLiteral("Hover"),
                   QStringLiteral("Dragging"), QStringLiteral("Shrunken")};
    spec.scenarios = {Scenario::Normal, Scenario::Empty};
    spec.keywords = {QStringLiteral("tabs"), QStringLiteral("btab"),
                     QStringLiteral("utabs"), QStringLiteral("documents"),
                     QStringLiteral("close"), QStringLiteral("reorder"),
                     QStringLiteral("qtabbar")};
    spec.properties = {
        {QStringLiteral("state"), QStringLiteral("enum"),
         {QStringLiteral("Idle"), QStringLiteral("Loading"), QStringLiteral("Failed"),
          QStringLiteral("Modified"), QStringLiteral("Asleep")},
         QStringLiteral("Idle"), QStringLiteral("One at a time. Two marks is a bug.")},
        {QStringLiteral("color"), QStringLiteral("QColor"), {}, QStringLiteral("invalid"),
         QStringLiteral("Held on the tab, so a reorder carries it.")},
        {QStringLiteral("reorderable"), QStringLiteral("bool"), {}, QStringLiteral("true"),
         QStringLiteral("Drag past a threshold, so a click is not a one-pixel drag.")},
        {QStringLiteral("newTabButton"), QStringLiteral("bool"), {},
         QStringLiteral("false"), QStringLiteral("Not every tab bar opens things.")},
    };
    spec.build = buildTabBar;
    return spec;
}

ComponentSpec activePanelComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("active-panel");
    spec.name = QStringLiteral("Active panel");
    spec.category = Category::Foundation;
    spec.description = QStringLiteral(
        "The rule every component that draws a selection follows: a panel that is not "
        "the active one keeps its selection, dimmed, rather than clearing it or "
        "competing with the one that is.");
    spec.headerPath = QStringLiteral("include/novu/ui/Theme.h");
    spec.sourcePath = QStringLiteral("src/Theme.cpp");
    spec.designedFor = {QStringLiteral("novuexplorer")};
    spec.usedBy = {QStringLiteral("novuadmin")};
    spec.states = {QStringLiteral("Active"), QStringLiteral("Inactive")};
    spec.keywords = {QStringLiteral("selection"), QStringLiteral("focus"),
                     QStringLiteral("panel"), QStringLiteral("dim"),
                     QStringLiteral("two-panel"), QStringLiteral("inactive")};
    spec.properties = {
        {QStringLiteral("novuActivePanel"), QStringLiteral("bool"), {},
         QStringLiteral("(unset)"),
         QStringLiteral("Set on the panel. Unset means always active, which is what a "
                        "dialog wants.")},
    };
    spec.build = buildActivePanel;
    return spec;
}

} // namespace novu::playground::pages
