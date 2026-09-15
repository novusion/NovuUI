#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/BottomPanel.h"
#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/IconButton.h"
#include "novu/ui/LogView.h"
#include "novu/ui/StatusDot.h"
#include "novu/ui/TabBar.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

QWidget* logPage(QWidget* parent, const QStringList& lines)
{
    auto* view = new LogView(parent);
    for (const QString& line : lines) {
        const QString level = line.section(QLatin1Char('|'), 0, 0);
        const QString text = line.section(QLatin1Char('|'), 1);
        LogView::Level l = LogView::Level::Info;
        if (level == QLatin1String("W")) {
            l = LogView::Level::Warning;
        } else if (level == QLatin1String("E")) {
            l = LogView::Level::Error;
        } else if (level == QLatin1String("D")) {
            l = LogView::Level::Debug;
        }
        view->append(l, text);
    }
    return view;
}

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    // ── the panel, with the host holding the state ───────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("A console"),
                                 QStringLiteral("The panel draws the state. This page "
                                                "owns it — which is the whole contract."));
        QVBoxLayout* body = card->body();

        auto* panel = new BottomPanel(card);
        panel->setExpandedHeight(180);

        panel->addPage(QStringLiteral("build"), QStringLiteral("Build"),
                       logPage(panel, {QStringLiteral("I|Configuring NovuUI 0.1.0"),
                                       QStringLiteral("I|Automatic MOC for target NovuUI"),
                                       QStringLiteral("D|  47 headers scanned, 12 moc'd"),
                                       QStringLiteral("I|NovuUI.vcxproj -> NovuUI.lib"),
                                       QStringLiteral("W|Segmented.cpp(88): unused parameter"),
                                       QStringLiteral("I|Built.")}),
                       Glyph::Cpu);
        panel->addPage(QStringLiteral("tests"), QStringLiteral("Tests"),
                       logPage(panel, {QStringLiteral("I|121 passed, 0 failed"),
                                       QStringLiteral("D|aFieldReportsWhatIsWrong… 4ms"),
                                       QStringLiteral("D|aPickerActsOnPressAndRelease… 2ms")}),
                       Glyph::Check);
        panel->addPage(QStringLiteral("transfer"), QStringLiteral("Transfers"),
                       logPage(panel, {QStringLiteral("I|3 files queued"),
                                       QStringLiteral("E|kraken-keys.json: permission denied")}),
                       Glyph::Upload);

        // The strip's right-hand end belongs to whatever is hosting the panel.
        panel->stripTrailing()->addWidget(new StatusDot(StatusDot::Tone::Success, true, card));
        panel->stripTrailing()->addWidget(label(QStringLiteral("watching"), card,
                                                TypeRole::Mono, Ink::Muted));
        auto* clear = new IconButton(Glyph::Trash, IconButton::Variant::Ghost, card);
        clear->setControlSize(ControlSize::Small);
        clear->setToolTip(QStringLiteral("Clear this log"));
        panel->stripTrailing()->addWidget(clear);

        // *** The contract, in four lines. ***
        //
        // The panel asks; this page answers. Nothing else is allowed to move
        // the state, which is why a host can restore it from settings and be
        // certain the panel will not disagree.
        QObject::connect(panel, &BottomPanel::expandRequested, panel,
                         [panel](bool expanded) { panel->setExpanded(expanded); });
        QObject::connect(panel, &BottomPanel::pageCloseRequested, panel,
                         [panel](const QString& key) { panel->removePage(key); });

        body->addWidget(panel);
        panel->setExpanded(true);

        note(body, card,
             QStringLiteral("Click the chevron, or a tab while it is shut. The panel never "
                            "changes its own state — every gesture emits expandRequested() "
                            "and waits. NovuExplorer's console keeps session lifecycle and "
                            "persistence, and a shared panel with an isOpen of its own would "
                            "give that window two answers to one question."));
        column->addWidget(card);
    }

    // ── collapsed ────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Shut"),
                                 QStringLiteral("Still a working surface: the tabs and the "
                                                "status stay."));
        QVBoxLayout* body = card->body();

        auto* panel = new BottomPanel(card);
        panel->addPage(QStringLiteral("out"), QStringLiteral("Output"),
                       logPage(panel, {QStringLiteral("I|nothing yet")}), Glyph::Orders);
        panel->addPage(QStringLiteral("problems"), QStringLiteral("Problems"),
                       logPage(panel, {QStringLiteral("W|1 warning")}), Glyph::Warning);
        panel->tabs()->setState(QStringLiteral("problems"), TabBar::State::Modified);
        panel->stripTrailing()->addWidget(label(QStringLiteral("1 warning"), card,
                                                TypeRole::Mono, Ink::Warning));
        QObject::connect(panel, &BottomPanel::expandRequested, panel,
                         [panel](bool expanded) { panel->setExpanded(expanded); });
        body->addWidget(panel);

        note(body, card,
             QStringLiteral("Collapsed it is one strip tall and the tabs still work — "
                            "clicking one asks for the panel at that tab, because making "
                            "somebody click twice to open a console is friction nobody "
                            "reports and everybody feels."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec bottomPanelComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("bottom-panel");
    spec.name = QStringLiteral("BottomPanel");
    spec.category = Category::Navigation;
    spec.description = QStringLiteral(
        "The console strip across the bottom of a window. Holds tabs and pages; owns "
        "neither whether it is open nor what is in it — the host answers both.");
    spec.headerPath = QStringLiteral("include/novu/ui/BottomPanel.h");
    spec.sourcePath = QStringLiteral("src/BottomPanel.cpp");
    spec.designedFor = {QStringLiteral("novuexplorer"), QStringLiteral("novuai")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Expanded"), QStringLiteral("Collapsed")};
    spec.states = {QStringLiteral("Normal"), QStringLiteral("Modified tab")};
    spec.keywords = {QStringLiteral("console"), QStringLiteral("output"),
                     QStringLiteral("log"), QStringLiteral("terminal"),
                     QStringLiteral("dock"), QStringLiteral("drawer")};
    spec.properties = {
        {QStringLiteral("expanded"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("An input. The panel applies it and never changes it.")},
        {QStringLiteral("expandedHeight"), QStringLiteral("int"), {}, QStringLiteral("220"),
         QStringLiteral("How tall when open, not counting the strip. The host's number.")},
        {QStringLiteral("currentKey"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("Which page is showing, by the caller's own identifier.")},
    };
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
