#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/AppMark.h"
#include "novu/ui/Badge.h"
#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/Feedback.h"
#include "novu/ui/IconButton.h"
#include "novu/ui/PageHeader.h"
#include "novu/ui/Separator.h"
#include "novu/ui/StatusBar.h"
#include "novu/ui/TitleBar.h"
#include "novu/ui/StyleSheet.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

// ── AppMark ──────────────────────────────────────────────────────────────

QWidget* buildAppMark(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    {
        Card* card = exampleCard(page, QStringLiteral("Three sizes, one drawing"),
                                 QStringLiteral("The design calls them .appmark, "
                                                ".rail-mark and .bigmark."));
        QVBoxLayout* body = card->body();

        QGridLayout* grid = matrix(body, card,
                                   {QStringLiteral("Glyph"), QStringLiteral("Initials"),
                                    QStringLiteral("Unaccented"), QStringLiteral("Edge")},
                                   {QStringLiteral("Small"), QStringLiteral("Medium"),
                                    QStringLiteral("Large")});
        const AppMark::Size sizes[] = {AppMark::Size::Small, AppMark::Size::Medium,
                                       AppMark::Size::Large};
        const Glyph glyphs[] = {Glyph::Vault, Glyph::Trades, Glyph::Ai};
        for (int i = 0; i < 3; ++i) {
            matrixCell(grid, i, 0, new AppMark(glyphs[i], card, sizes[i]));
            matrixCell(grid, i, 1, new AppMark(QStringLiteral("NV"), card, sizes[i]));

            auto* plain = new AppMark(glyphs[i], card, sizes[i]);
            plain->setAccented(false);
            matrixCell(grid, i, 2, plain);

            auto* probe = new AppMark(Glyph::None, card, sizes[i]);
            matrixCell(grid, i, 3,
                       label(QStringLiteral("%1px").arg(probe->side()), card,
                             TypeRole::Mono, Ink::Muted));
            probe->deleteLater();
        }

        note(body, card,
             QStringLiteral("Radius and glyph size are derived from the edge rather than "
                            "tabulated, so a fourth size rounds like the other three instead "
                            "of needing a new number. That is what fixed the drift: "
                            "NovuExplorer's header mark, NovuHub's tray badge and NovuSpace's "
                            "rail mark were 28, 32 and 30 pixels, each with its own corner."));
        note(body, card,
             QStringLiteral("Glyph or letters, never both — a plaque carrying an icon and "
                            "initials is two identities in one box, so setting either clears "
                            "the other. Accented is the product's own mark; unaccented is a "
                            "generic one beside a row."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

// ── Product mark ─────────────────────────────────────────────────────────

namespace {

// A bare paint call, one widget per product so each can be scoped to its own
// profile - `paintBrandMark()` resolves the accent through `themeFor()`, the
// same way every painted component here does, so the only way to show five
// products side by side is five small surfaces, not five calls in one.
class BrandMarkSwatch : public QWidget
{
public:
    explicit BrandMarkSwatch(int side, QWidget* parent) : QWidget(parent)
    {
        setFixedSize(side, side);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        paintBrandMark(&painter, rect(), this);
    }
};

} // namespace

QWidget* buildProductMark(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);
    const ThemeProfile& profile = themeFor(ctx.parent);

    // ── one product, one accent ──────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("One product, one accent"),
                                 QStringLiteral("The same three polygons in every approved "
                                                "mockup that draws them. Only the right face "
                                                "moves."));
        QVBoxLayout* body = card->body();
        QHBoxLayout* row = exampleRow(body, card, QString());

        for (const QString& id : ThemeManager::instance().profileIds()) {
            if (!isApplicationProfile(id) || id == QLatin1String("novutrade")) {
                continue;
            }
            const ThemeProfile& p = ThemeManager::instance().profile(id);
            auto* cell = new QWidget(card);
            auto* cellLayout = new QVBoxLayout(cell);
            cellLayout->setContentsMargins(0, 0, 0, 0);
            cellLayout->setSpacing(6);
            cellLayout->setAlignment(Qt::AlignHCenter);
            auto* swatch = new BrandMarkSwatch(32, cell);
            applyStyleSheet(swatch, id);
            cellLayout->addWidget(swatch, 0, Qt::AlignHCenter);
            cellLayout->addWidget(label(p.displayName, cell, TypeRole::Caption, Ink::Muted));
            row->addWidget(cell);
            row->addSpacing(profile.space(SpacingRole::ToolbarGap));
        }
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Left and bottom are this profile's own textSecondary and "
                            "textDisabled - not a colour invented for the mark, the same "
                            "tokens every other painted component reads. NovuVoice uses the "
                            "muted rose stated by its approved brand mockup."));
        column->addWidget(card);
    }

    // ── sizes ─────────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Sizes"),
                                 QStringLiteral("The design's own call sites: the title bar's "
                                                "18px icon, and novuMark()'s own 22, 28 and "
                                                "40."));
        QVBoxLayout* body = card->body();
        QHBoxLayout* row = exampleRow(body, card, QString());
        for (const int side : {18, 22, 28, 40}) {
            row->addWidget(new BrandMarkSwatch(side, card));
            row->addWidget(label(QStringLiteral("%1px").arg(side), card, TypeRole::Mono,
                                 Ink::Muted));
            row->addSpacing(profile.space(SpacingRole::ToolbarGap));
        }
        row->addStretch(1);
        note(body, card,
             QStringLiteral("One drawing scaled, the way every glyph and every AppMark size "
                            "here already is - there is no separate small-size artwork to keep "
                            "in step with the large one."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

// ── StatusBar ────────────────────────────────────────────────────────────

QWidget* buildStatusBar(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    {
        Card* card = exampleCard(page, QStringLiteral("Session, context, transport"),
                                 QStringLiteral("And nothing else — the design is "
                                                "explicit about it."));
        QVBoxLayout* body = card->body();

        auto* bar = new StatusBar(card);
        const int session = bar->addValue(QStringLiteral("p.dostal@novusion.net"));
        bar->addSeparator();
        bar->addText(QStringLiteral("Workspace"));
        bar->addValue(QStringLiteral("Novusion"));
        bar->addSeparator();
        const int transport = bar->addStatus(QStringLiteral("wg-platform"),
                                             StatusDot::Tone::Success);
        const int latency = bar->addMono(QStringLiteral("52 ms"), StatusBar::Slot::Trailing);
        bar->addMono(QStringLiteral("v0.67.11"), StatusBar::Slot::Trailing);
        body->addWidget(bar);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Transport"));
        const struct { const char* caption; bool offline; const char* latency; } states[] = {
            {"Online", false, "52 ms"},
            {"Slow", false, "418 ms"},
            {"Offline", true, "—"},
        };
        for (const auto& state : states) {
            auto* button = new Button(QString::fromLatin1(state.caption),
                                      Button::Variant::Secondary, card);
            button->setControlSize(ControlSize::Small);
            const bool offline = state.offline;
            const QString value = QString::fromLatin1(state.latency);
            QObject::connect(button, &Button::clicked, bar, [bar, latency, offline, value] {
                bar->setOffline(offline);
                bar->setItemText(latency, value);
            });
            row->addWidget(button);
        }

        auto* anon = new Button(QStringLiteral("Sign out"), Button::Variant::Ghost, card);
        anon->setControlSize(ControlSize::Small);
        QObject::connect(anon, &Button::clicked, bar, [bar, session] {
            bar->setItemText(session, QObject::tr("Not signed in"));
        });
        row->addWidget(anon);
        row->addStretch(1);

        Q_UNUSED(transport);

        note(body, card,
             QStringLiteral("Items are addressed by the handle their add call returns, so "
                            "updating the latency four times a second is a string assignment "
                            "and one repaint — not a lookup for a child by name, which is how "
                            "the previous status bars ended up holding pointers to their own "
                            "labels."));
        note(body, card,
             QStringLiteral("Offline is one call on the bar rather than a re-tone of each "
                            "light, because losing the transport is one event and not six. "
                            "The gradient is the same two title-bar tokens the top of the "
                            "window uses, so both ends match without either component knowing "
                            "about the other."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("One interactive slot"),
                                 QStringLiteral("For the things that really are "
                                                "controls."));
        QVBoxLayout* body = card->body();

        auto* bar = new StatusBar(card);
        bar->addText(QStringLiteral("Indexing"));
        bar->addValue(QStringLiteral("4 of 12 workspaces"));
        auto* spinner = new Spinner(ControlSize::Small, bar);
        bar->setTrailingWidget(spinner);
        body->addWidget(bar);

        note(body, card,
             QStringLiteral("A single slot on purpose. A status bar that accepts arbitrary "
                            "widgets in arbitrary places is a toolbar, and the design's rule — "
                            "host-owned, never module-private state — is what stops one "
                            "becoming a second, worse notification system."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

// ── PageHeader ───────────────────────────────────────────────────────────

QWidget* buildPageHeader(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    const bool longText = ctx.scenario == Scenario::LongText;

    {
        Card* card = exampleCard(page, QStringLiteral("The band across the top"),
                                 QStringLiteral("Trail, title, description, "
                                                "actions."));
        QVBoxLayout* body = card->body();

        auto* header = new PageHeader(
            QStringLiteral("Device peers"),
            longText
                ? QStringLiteral("Every device that has been issued a WireGuard identity, "
                                 "with the address it was allocated and the role it was "
                                 "granted. Revoking a peer removes its authorisation "
                                 "immediately; the reconciler applies the change to the "
                                 "interface on its next pass, which is at most thirty "
                                 "seconds later.")
                : QStringLiteral("Devices authorised to reach the platform network."),
            card);
        header->setPath({QStringLiteral("Novusion"), QStringLiteral("Network"),
                         QStringLiteral("Device peers")});

        auto* refresh = new Button(QStringLiteral("Refresh"), Button::Variant::Secondary,
                                   header->actionsWidget());
        refresh->setControlSize(ControlSize::Small);
        header->actions()->addWidget(refresh);
        auto* add = new Button(QStringLiteral("Provision"), Button::Variant::Primary,
                               header->actionsWidget());
        add->setControlSize(ControlSize::Small);
        header->actions()->addWidget(add);

        body->addWidget(header);

        note(body, card,
             QStringLiteral("The trail drops its last segment, because the title next to it "
                            "already says that — a breadcrumb repeating the heading below it "
                            "is a line of text doing no work. Switch the scenario to Long text "
                            "and the description wraps and the band grows: a page heading that "
                            "elides its own explanation explains nothing."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("Variants"),
                                 QStringLiteral("What each part being absent looks "
                                                "like."));
        QVBoxLayout* body = card->body();

        auto* bare = new PageHeader(QStringLiteral("Overview"), card);
        body->addWidget(bare);

        auto* described = new PageHeader(
            QStringLiteral("Audit log"),
            QStringLiteral("Every administrative action, in order."), card);
        body->addWidget(described);

        auto* trailed = new PageHeader(QStringLiteral("Retention"), card);
        trailed->setPath({QStringLiteral("Settings"), QStringLiteral("Data"),
                          QStringLiteral("Retention")});
        trailed->setDivider(false);
        body->addWidget(trailed);

        note(body, card,
             QStringLiteral("A one-segment path draws no trail and reserves no line for it. "
                            "The last header has its divider off, which is what a header "
                            "inside a card wants — the card already has an edge."));
        column->addWidget(card);
    }

    // -- chrome button states ------------------------------------------------
    {
        Card* card = exampleCard(page, QStringLiteral("Title bar controls, state by state"),
                                 QStringLiteral("`.winbtn` - one box and one hover plate "
                                                "for the actions and the three controls "
                                                "alike."));
        QVBoxLayout* body = card->body();

        const WindowTokens& w = themeFor(card).window;
        const auto row = [&](const QString& caption, bool hover, bool pressed) {
            QHBoxLayout* line = exampleRow(body, card, caption);
            const QVector<Glyph> glyphs{Glyph::Sun, Glyph::More, Glyph::Sidebar};
            for (const Glyph glyph : glyphs) {
                auto* action = new IconButton(glyph, IconButton::Variant::Chrome, card);
                action->setDown(pressed);
                action->setAttribute(Qt::WA_UnderMouse, hover);
                line->addWidget(action);
            }
            const QVector<WindowControlButton::Control> controls{
                WindowControlButton::Control::Minimize,
                WindowControlButton::Control::Maximize,
                WindowControlButton::Control::Close};
            for (const WindowControlButton::Control control : controls) {
                auto* button = new WindowControlButton(control, card);
                button->setWindowActive(true);
                button->setDown(pressed);
                button->setAttribute(Qt::WA_UnderMouse, hover);
                line->addWidget(button);
            }
            line->addStretch(1);
        };
        row(QStringLiteral("Normal"), false, false);
        row(QStringLiteral("Pressed"), false, true);

        note(body, card,
             QStringLiteral("All six are the same %1x%2 box with a %3px radius: "
                            "`.winbtn{width:30px;height:24px;border-radius:6px}`. The "
                            "actions are IconButton's Chrome variant, which carries no "
                            "outline in any state and fills to the panel surface under "
                            "the pointer - Ghost grows an outline and takes the product's "
                            "control height instead, which gave the bar a hover plate its "
                            "full height. Close is neutral until hovered, then the danger "
                            "tint; hover over the row above to see both.")
                 .arg(w.controlWidth).arg(w.controlHeight).arg(w.controlRadius));
        column->addWidget(card);
    }

    // -- layout --------------------------------------------------------------
    {
        Card* card = exampleCard(page, QStringLiteral("Stacked and inline"),
                                 QStringLiteral("Two arrangements of the same three "
                                                "strings, both of them in an approved "
                                                "design."));
        QVBoxLayout* body = card->body();

        auto* stacked = new PageHeader(QStringLiteral("Web workspaces"),
                                       QStringLiteral("Organise browser workspaces, "
                                                      "profiles and stored sessions"),
                                       card);
        stacked->setPath({QStringLiteral("NovuSpace"), QStringLiteral("Web"),
                          QStringLiteral("Overview")});
        body->addWidget(stacked);

        auto* inline_ = new PageHeader(QStringLiteral("Web workspaces"),
                                       QStringLiteral("Organise browser workspaces, "
                                                      "profiles and stored sessions"),
                                       card);
        inline_->setPath({QStringLiteral("NovuSpace"), QStringLiteral("Web"),
                          QStringLiteral("Overview")});
        inline_->setLayout(PageHeader::Layout::Inline);
        body->addWidget(inline_);

        note(body, card,
             QStringLiteral("Stacked is the default: the trail above the title, the "
                            "description under it. Inline is NovuSpace's own "
                            "`.page-head` — `.ttl{display:flex;align-items:baseline;"
                            "gap:10px}` puts all three on one baseline, and the band is "
                            "one row of padding tall because of it. Three rows where the "
                            "reference draws one moves every pixel of the page below."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("Breadcrumb alone"),
                                 QStringLiteral("For a header a caller composes "
                                                "itself."));
        QVBoxLayout* body = card->body();

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Path"));
        row->addWidget(new Breadcrumb({QStringLiteral("Novusion"), QStringLiteral("Trade"),
                                       QStringLiteral("Positions"),
                                       QStringLiteral("BTC-PERP")},
                                      card));
        row->addStretch(1);

        QHBoxLayout* single = exampleRow(body, card, QStringLiteral("One segment"));
        single->addWidget(new Breadcrumb({QStringLiteral("Overview")}, card));
        single->addWidget(label(QStringLiteral("(draws nothing)"), card, TypeRole::Caption,
                                Ink::Muted));
        single->addStretch(1);

        note(body, card,
             QStringLiteral("PageHeader paints the same trail itself rather than embedding one "
                            "of these: the whole point of painting the header is that it is "
                            "one widget, and a trail plus a title plus a description as three "
                            "labels is the shape it replaces."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec appMarkComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("app-mark");
    spec.name = QStringLiteral("AppMark");
    spec.category = Category::DataDisplay;
    spec.description = QStringLiteral(
        "The rounded plaque an application, module or workspace is identified by, at the "
        "design's three sizes — with the proportions derived rather than tabulated.");
    spec.headerPath = QStringLiteral("include/novu/ui/AppMark.h");
    spec.sourcePath = QStringLiteral("src/AppMark.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Small"), QStringLiteral("Medium"),
                     QStringLiteral("Large"), QStringLiteral("Initials")};
    spec.states = {QStringLiteral("Accented"), QStringLiteral("Plain")};
    spec.keywords = {QStringLiteral("appmark"), QStringLiteral("rail-mark"),
                     QStringLiteral("bigmark"), QStringLiteral("avatar"),
                     QStringLiteral("icon"), QStringLiteral("identity")};
    spec.properties = {
        {QStringLiteral("markSize"), QStringLiteral("enum"),
         {QStringLiteral("Small"), QStringLiteral("Medium"), QStringLiteral("Large")},
         QStringLiteral("Medium"), QStringLiteral("29, 30 and 58 pixels.")},
        {QStringLiteral("accented"), QStringLiteral("bool"), {}, QStringLiteral("true"),
         QStringLiteral("The product's own mark, or a generic one beside a row.")},
    };
    spec.build = buildAppMark;
    return spec;
}

ComponentSpec productMarkComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("product-mark");
    spec.name = QStringLiteral("Product mark");
    spec.category = Category::TitleBar;
    spec.description = QStringLiteral(
        "The isometric gem every window icon, every favicon and the title bar's own icon "
        "are drawn from — three filled polygons, no plate, no border. Left and bottom are "
        "the shared neutrals; only the right face is the product's own accent.");
    spec.headerPath = QStringLiteral("include/novu/ui/AppMark.h");
    spec.sourcePath = QStringLiteral("src/AppMark.cpp");
    spec.designedFor = {QStringLiteral("novuspace"), QStringLiteral("novuai"),
                        QStringLiteral("novuexplorer"), QStringLiteral("novuadmin"),
                        QStringLiteral("novuhub")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("18px"), QStringLiteral("22px"), QStringLiteral("28px"),
                     QStringLiteral("40px")};
    spec.states = {};
    spec.keywords = {QStringLiteral("app-ico"), QStringLiteral("novuMark"),
                     QStringLiteral("gem"), QStringLiteral("favicon"),
                     QStringLiteral("window icon"), QStringLiteral("logo"),
                     QStringLiteral("brand")};
    spec.properties = {
        {QStringLiteral("accent"), QStringLiteral("QColor"), {}, QString(),
         QStringLiteral("Resolved from the context's own profile — never passed in.")},
        {QStringLiteral("size"), QStringLiteral("int"), {}, QString(),
         QStringLiteral("The box the mark is scaled into. One drawing, any size.")},
    };
    spec.build = buildProductMark;
    return spec;
}

ComponentSpec statusBarComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("status-bar");
    spec.name = QStringLiteral("StatusBar");
    spec.category = Category::Footer;
    spec.description = QStringLiteral(
        "The strip along the bottom of a window: session, context and transport, painted "
        "as addressable items rather than built from labels.");
    spec.headerPath = QStringLiteral("include/novu/ui/StatusBar.h");
    spec.sourcePath = QStringLiteral("src/StatusBar.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Text"), QStringLiteral("Value"),
                     QStringLiteral("Mono"), QStringLiteral("Status"),
                     QStringLiteral("Separator")};
    spec.states = {QStringLiteral("Online"), QStringLiteral("Offline")};
    spec.keywords = {QStringLiteral("statusbar"), QStringLiteral("footer"),
                     QStringLiteral("transport"), QStringLiteral("session"),
                     QStringLiteral("latency"), QStringLiteral("sdot")};
    spec.properties = {
        {QStringLiteral("slot"), QStringLiteral("enum"),
         {QStringLiteral("Leading"), QStringLiteral("Trailing")},
         QStringLiteral("Leading"), QStringLiteral("Which end an item packs against.")},
        {QStringLiteral("offline"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("Every transport light goes to danger at once.")},
    };
    spec.build = buildStatusBar;
    return spec;
}

ComponentSpec pageHeaderComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("page-header");
    spec.name = QStringLiteral("PageHeader");
    spec.category = Category::Navigation;
    spec.description = QStringLiteral(
        "The band across the top of a page: a breadcrumb, a title, a wrapping description "
        "and the page's actions. The most repeated shape in the system.");
    spec.headerPath = QStringLiteral("include/novu/ui/PageHeader.h");
    spec.sourcePath = QStringLiteral("src/PageHeader.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Title only"), QStringLiteral("With description"),
                     QStringLiteral("With trail"), QStringLiteral("Breadcrumb alone")};
    spec.states = {QStringLiteral("Divider"), QStringLiteral("No divider")};
    spec.scenarios = {Scenario::Normal, Scenario::LongText};
    spec.keywords = {QStringLiteral("page-head"), QStringLiteral("crumb"),
                     QStringLiteral("breadcrumb"), QStringLiteral("title"),
                     QStringLiteral("heading"), QStringLiteral("toolbar")};
    spec.properties = {
        {QStringLiteral("path"), QStringLiteral("QStringList"), {}, QStringLiteral("{}"),
         QStringLiteral("The full path; the last segment is dropped as redundant.")},
        {QStringLiteral("description"), QStringLiteral("QString"), {}, QStringLiteral(""),
         QStringLiteral("Wraps, and the header grows to fit it.")},
        {QStringLiteral("divider"), QStringLiteral("bool"), {}, QStringLiteral("true"),
         QStringLiteral("Off for a header inside a card.")},
    };
    spec.build = buildPageHeader;
    return spec;
}

} // namespace novu::playground::pages
