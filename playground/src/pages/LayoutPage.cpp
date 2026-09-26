#include "Pages.h"

#include "novu/playground/ExampleLayout.h"
#include "novu/playground/SampleData.h"

#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/Fields.h"
#include "novu/ui/Layout.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/StyleSheet.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);
    auto hoverProfile=novusionBaseProfile();hoverProfile.id="gallery-hover-scroll";hoverProfile.controls.scrollThumbOnAreaHover=true;hoverProfile.controls.scrollThumbInset=3;ThemeManager::instance().registerProfile(hoverProfile);
    auto* hoverArea=new QScrollArea(page);hoverArea->setFixedHeight(100);hoverArea->setWidgetResizable(true);applyStyleSheet(hoverArea,hoverProfile.id);
    auto* sample=paragraph("Move over this area to reveal its scroll thumb.",hoverArea);sample->setMinimumHeight(400);hoverArea->setWidget(sample);column->addWidget(hoverArea);
    const ThemeProfile& profile = themeFor(ctx.parent);

    // ── splitHeader ──────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("splitHeader()"),
                                 QStringLiteral("Titles left, actions right. Returns the actions "
                                                "layout."));
        QVBoxLayout* body = card->body();
        QHBoxLayout* actions = splitHeader(body, card, QStringLiteral("Trade history"),
                                           QStringLiteral("Last 30 days, closed positions only"),
                                           QStringLiteral("NovuTrade"));
        actions->addWidget(secondaryButton(QStringLiteral("Export"), card));
        actions->addWidget(primaryButton(QStringLiteral("Refresh"), card));
        note(body, card,
             QStringLiteral("The eyebrow says which product a page belongs to. NovuSpace hosts "
                            "NovuTrade, NovuMail and NovuVault as modules in one window, and a "
                            "bare \"Trade history\" does not tell the user whose it is."));

        QHBoxLayout* noSubtitle = splitHeader(body, card, QStringLiteral("Without a subtitle"));
        noSubtitle->addWidget(ghostButton(QStringLiteral("Settings"), card));
        column->addWidget(card);
    }

    // ── toolbar ──────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("toolbar()"),
                                 QStringLiteral("A row with the profile's toolbar gap (%1 px). "
                                                "Creates no widget at all.")
                                     .arg(profile.space(SpacingRole::ToolbarGap)));
        QVBoxLayout* body = card->body();
        QHBoxLayout* row = toolbar(body);
        row->addWidget(lineEdit(QStringLiteral("Filter"), card, 180));
        row->addWidget(comboBox({QStringLiteral("All"), QStringLiteral("Open"),
                                 QStringLiteral("Closed")},
                                card, 120));
        row->addWidget(secondaryButton(QStringLiteral("Apply"), card));
        row->addStretch(1);
        row->addWidget(dangerButton(QStringLiteral("Clear"), card));
        note(body, card,
             QStringLiteral("A row of buttons does not need a QWidget to live in — the parent "
                            "layout already groups it, and the wrapper would only add a paint "
                            "pass, a size hint and a place for a stray stylesheet to land."));
        column->addWidget(card);
    }

    // ── pageLayout ───────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("pageLayout()"),
                                 QStringLiteral("The outer layout of the page you are looking "
                                                "at."));
        QVBoxLayout* body = card->body();
        note(body, card,
             QStringLiteral("Page padding %1 px, card gap %2 px, both from the active profile. "
                            "Every preview page in this gallery is built with it, which means "
                            "the gallery itself breaks first if the values are wrong.")
                 .arg(profile.space(SpacingRole::PagePadding))
                 .arg(profile.space(SpacingRole::CardGap)));
        column->addWidget(card);
    }

    // ── scrollPage ───────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("scrollPage()"),
                                 QStringLiteral("The one layout helper that creates a widget, "
                                                "and says why."));
        QVBoxLayout* body = card->body();

        auto* host = new QWidget(card);
        host->setFixedHeight(180);
        QScrollArea* area = nullptr;
        QVBoxLayout* inner = scrollPage(host, &area);
        const int rows = ctx.scenario == Scenario::SingleItem ? 1
            : ctx.scenario == Scenario::Empty                 ? 0
                                                              : 20;
        for (int i = 0; i < rows; ++i) {
            inner->addWidget(label(QStringLiteral("Scrollable row %1").arg(i + 1), host,
                                   TypeRole::Body, Ink::Secondary));
        }
        if (rows == 0) {
            inner->addWidget(scenarioEmptyState(host, QStringLiteral("rows")));
        }
        inner->addStretch(1);
        body->addWidget(host);

        note(body, card,
             QStringLiteral("A QScrollArea cannot scroll without a viewport widget, so that "
                            "container is mechanism rather than decoration — and it is kept "
                            "transparent and frameless so it adds no surface of its own."));
        column->addWidget(card);
    }

    // ── a scroll area's own surface ──────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("fillScrollViewport()"),
                                 QStringLiteral("Why a scrolling page states the surface "
                                                "it stands on."));
        QVBoxLayout* body = card->body();
        note(body, card,
             QStringLiteral("A viewport that paints nothing is not the same as one you can "
                            "see through. Qt treats a styled background as opaque and stops "
                            "repainting the parent under it, so on a window whose frame is "
                            "translucent - every Novu shell - the area comes out as a hole: "
                            "the desktop, not the page. scrollPage() fills its own viewport "
                            "with the page ground; an application that builds its own scroll "
                            "area calls fillScrollViewport(area) - or "
                            "fillScrollViewport(area, ColorRole::PanelBg) inside a card."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec layoutComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("layout");
    spec.name = QStringLiteral("Layout");
    spec.category = Category::Foundation;
    spec.description = QStringLiteral(
        "toolbar(), pageLayout(), splitHeader() and scrollPage(). Layout constructs, not "
        "widgets — each returns a layout wherever a layout is enough.");
    spec.headerPath = QStringLiteral("include/novu/ui/Layout.h");
    spec.sourcePath = QStringLiteral("src/Layout.cpp");
    spec.designedFor = {QStringLiteral("novuspace"),
                   QStringLiteral("novuai"),
                   QStringLiteral("novuexplorer"),
                   QStringLiteral("novuhub"),
                   QStringLiteral("novuvoice"),
                   QStringLiteral("novutrade")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("toolbar()"), QStringLiteral("pageLayout()"),
                     QStringLiteral("splitHeader()"), QStringLiteral("scrollPage()")};
    spec.states = {QStringLiteral("Normal"), QStringLiteral("Empty"), QStringLiteral("Scrolling")};
    spec.keywords = {QStringLiteral("layout"), QStringLiteral("page"), QStringLiteral("toolbar"),
                     QStringLiteral("header"), QStringLiteral("scroll"),
                     QStringLiteral("spacing"), QStringLiteral("padding")};
    spec.properties = {
        {QStringLiteral("title"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("splitHeader() title.")},
        {QStringLiteral("subtitle"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("splitHeader() subtitle; omitted when empty.")},
    };
    spec.scenarios = {Scenario::Normal, Scenario::Empty, Scenario::SingleItem};
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
