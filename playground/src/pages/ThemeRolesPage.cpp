#include "Pages.h"

#include "novu/playground/ExampleLayout.h"
#include "novu/playground/SampleData.h"

#include "novu/ui/Badge.h"
#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/DropTarget.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/ThemeScope.h"

#include <QGridLayout>
#include <QPainter>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

#include <iterator>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

// A colour patch. Deliberately a QLabel carrying a palette rather than a new
// component: NovuUI has no swatch and does not need one, and the rule for
// this gallery is that it never invents a primitive the library already
// covers — or, as here, deliberately does not have. The palette is the same
// mechanism setInk() uses; no stylesheet is set on anything.
QLabel* swatch(QWidget* parent, const QColor& colour)
{
    auto* patch = new QLabel(parent);
    patch->setAutoFillBackground(true);
    patch->setFixedSize(56, 22);
    QPalette palette = patch->palette();
    palette.setColor(QPalette::Window, colour);
    patch->setPalette(palette);
    patch->setToolTip(colour.name());
    return patch;
}

void colourRow(QGridLayout* grid, int row, QWidget* parent, const QString& name,
               const QColor& colour)
{
    grid->addWidget(label(name, parent, TypeRole::Body), row, 0);
    grid->addWidget(swatch(parent, colour), row, 1);
    grid->addWidget(label(colour.name().toUpper(), parent, TypeRole::Mono, Ink::Muted), row, 2);
}

// A fixed strip standing in for one tree row / one panel, painted through the
// same two functions a real drop handler calls. Not a mock of the contract -
// the contract.
class DropPreview : public QWidget
{
public:
    DropPreview(DropZone zone, QWidget* parent)
        : QWidget(parent)
        , m_zone(zone)
    {
        setFixedHeight(m_zone == DropZone::Row ? 30 : 64);
        setMinimumWidth(220);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRect area = rect().adjusted(0, 0, -1, -1);
        if (m_zone == DropZone::Row) {
            paintDropRow(&painter, area, this);
        } else {
            paintDropRegion(&painter, area, this);
        }
    }

private:
    DropZone m_zone;
};

struct SurfaceRow
{
    const char* name;
    SurfaceRole role;
};

const SurfaceRow kSurfaces[] = {
    {"Card", SurfaceRole::Card},       {"Dialog", SurfaceRole::Dialog},
    {"Button", SurfaceRole::Button},   {"Input", SurfaceRole::Input},
    {"Badge", SurfaceRole::Badge},     {"Toolbar", SurfaceRole::Toolbar},
    {"Table", SurfaceRole::Table},     {"Overlay", SurfaceRole::Overlay},
};

struct SpacingRow
{
    const char* name;
    SpacingRole role;
};

const SpacingRow kSpacings[] = {
    {"CardPadding", SpacingRole::CardPadding},
    {"CardGap", SpacingRole::CardGap},
    {"SectionGap", SpacingRole::SectionGap},
    {"ToolbarGap", SpacingRole::ToolbarGap},
    {"FormRowGap", SpacingRole::FormRowGap},
    {"TableCellPadding", SpacingRole::TableCellPadding},
    {"PagePadding", SpacingRole::PagePadding},
};

struct BorderRow
{
    const char* name;
    BorderRole role;
};

const BorderRow kBorders[] = {
    {"Card", BorderRole::Card},     {"Input", BorderRole::Input},
    {"Table", BorderRole::Table},   {"Divider", BorderRole::Divider},
    {"Focus", BorderRole::Focus},
};

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    // Resolved against the preview surface, which carries the ThemeScope the
    // developer selected. Switch the profile and every number on this page
    // changes, because none of them is written here.
    const ThemeProfile& profile = themeFor(ctx.parent);
    const ThemeColors& c = profile.colors;

    {
        Card* card = exampleCard(page, QStringLiteral("Active profile"),
                                 QStringLiteral("Resolved through themeFor() on the preview "
                                                "surface — nothing on this page is hardcoded."));
        QVBoxLayout* body = card->body();
        QHBoxLayout* row = exampleRow(body, card, QString());
        row->addWidget(new Badge(profile.id, Badge::Tone::Info, card));
        row->addWidget(label(profile.displayName, card, TypeRole::CardTitle));
        row->addStretch(1);
        note(body, card,
             QStringLiteral("Scope on the surface: \"%1\". The gallery chrome around it stays on "
                            "its own profile, so there is always an unchanged reference on "
                            "screen.")
                 .arg(ThemeScope::resolve(ctx.parent)));
        column->addWidget(card);
    }

    // ── semantic palette ─────────────────────────────────────────────────
    //
    // The card that answers "what is shared and what is this application's".
    // Switch the profile above it and the four status rows must not move.
    {
        const SemanticHues& shared = sharedSemanticHues();
        // `.ink` on each: a shared hue is an ink and a plate now, and the
        // question this asks is whether the profile rewrote the ink.
        const bool overridden = profile.colors.success != shared.success.ink
                             || profile.colors.warning != shared.warning.ink
                             || profile.colors.danger != shared.danger.ink
                             || profile.colors.info != shared.info.ink;

        Card* card = exampleCard(page, QStringLiteral("Semantic palette"),
                                 QStringLiteral("Status is shared across every Novusion "
                                                "application. Accent is not."));
        QVBoxLayout* body = card->body();

        auto* row = new QHBoxLayout();
        row->setSpacing(profile.space(SpacingRole::ToolbarGap));
        row->addWidget(new Badge(QStringLiteral("Neutral"), Badge::Tone::Neutral, card));
        row->addWidget(new Badge(QStringLiteral("Success"), Badge::Tone::Success, card));
        row->addWidget(new Badge(QStringLiteral("Warning"), Badge::Tone::Warning, card));
        row->addWidget(new Badge(QStringLiteral("Danger"), Badge::Tone::Danger, card));
        row->addWidget(new Badge(QStringLiteral("Info"), Badge::Tone::Info, card));
        row->addStretch(1);
        body->addLayout(row);

        auto* grid = new QGridLayout();
        grid->setHorizontalSpacing(profile.space(SpacingRole::ToolbarGap));
        grid->setVerticalSpacing(4);
        const QString status = overridden ? QStringLiteral("override")
                                          : QStringLiteral("shared");
        int r = 0;
        const struct { const char* name; SemanticRole role; } roles[] = {
            {"success", SemanticRole::Success}, {"warning", SemanticRole::Warning},
            {"danger", SemanticRole::Danger},   {"info", SemanticRole::Info},
        };
        for (const auto& entry : roles) {
            colourRow(grid, r, card, QString::fromLatin1(entry.name),
                      profile.semantic.role(entry.role).text);
            grid->addWidget(label(status, card, TypeRole::Caption,
                                  overridden ? Ink::Warning : Ink::Success), r, 3);
            ++r;
        }
        // Neutral is the absence of a status, so it follows the surface rather
        // than being shared - that is a different answer, and it is labelled as
        // one.
        colourRow(grid, r, card, QStringLiteral("neutral"),
                  profile.semantic.role(SemanticRole::Neutral).text);
        grid->addWidget(label(QStringLiteral("per surface"), card, TypeRole::Caption,
                              Ink::Muted), r, 3);
        ++r;
        // The accent sits in the same table on purpose: it is the layer a
        // product does own, and seeing it move while the four above it stay put
        // is the whole point of this card.
        colourRow(grid, r, card, QStringLiteral("accent"), profile.colors.accent);
        grid->addWidget(label(QStringLiteral("application"), card, TypeRole::Caption,
                              Ink::Accent), r, 3);
        grid->setColumnStretch(3, 1);
        body->addLayout(grid);

        note(body, card,
             overridden
                 ? QStringLiteral("This profile overrides the shared status hues. The only "
                                  "reason that is allowed is surface polarity: on a light "
                                  "panel the shared hues fall to about 2.5:1 and stop being "
                                  "readable.")
                 : QStringLiteral("This profile takes the shared hues unchanged. Switch to "
                                  "another application profile — the four status rows do not "
                                  "move, the accent row does."));
        column->addWidget(card);
    }

    // ── colours ──────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Colours"),
                                 QStringLiteral("Every colour a component may read."));
        QVBoxLayout* body = card->body();
        auto* grid = new QGridLayout();
        grid->setHorizontalSpacing(profile.space(SpacingRole::ToolbarGap));
        grid->setVerticalSpacing(profile.space(SpacingRole::FormRowGap) / 2);
        grid->setContentsMargins(0, 0, 0, 0);

        int r = 0;
        colourRow(grid, r++, card, QStringLiteral("windowBg"), c.windowBg);
        colourRow(grid, r++, card, QStringLiteral("panelBg"), c.panelBg);
        colourRow(grid, r++, card, QStringLiteral("panelBgAlt"), c.panelBgAlt);
        colourRow(grid, r++, card, QStringLiteral("headerBg"), c.headerBg);
        colourRow(grid, r++, card, QStringLiteral("surfaceHover"), c.surfaceHover);
        colourRow(grid, r++, card, QStringLiteral("surfaceActive"), c.surfaceActive);
        colourRow(grid, r++, card, QStringLiteral("border"), c.border);
        colourRow(grid, r++, card, QStringLiteral("borderSubtle"), c.borderSubtle);
        colourRow(grid, r++, card, QStringLiteral("borderHover"), c.borderHover);
        colourRow(grid, r++, card, QStringLiteral("textPrimary"), c.textPrimary);
        colourRow(grid, r++, card, QStringLiteral("textSecondary"), c.textSecondary);
        colourRow(grid, r++, card, QStringLiteral("textDisabled"), c.textDisabled);
        colourRow(grid, r++, card, QStringLiteral("textOnAccent"), c.textOnAccent);
        colourRow(grid, r++, card, QStringLiteral("accent"), c.accent);
        colourRow(grid, r++, card, QStringLiteral("accentLight"), c.accentLight);
        colourRow(grid, r++, card, QStringLiteral("success"), c.success);
        colourRow(grid, r++, card, QStringLiteral("warning"), c.warning);
        colourRow(grid, r++, card, QStringLiteral("danger"), c.danger);
        grid->setColumnStretch(3, 1);
        body->addLayout(grid);
        column->addWidget(card);
    }

    // ── radius roles ─────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Surface roles"),
                                 QStringLiteral("A component names what it is; the profile "
                                                "decides how round that is."));
        QVBoxLayout* body = card->body();
        auto* grid = new QGridLayout();
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setHorizontalSpacing(profile.space(SpacingRole::ToolbarGap));
        for (int i = 0; i < static_cast<int>(std::size(kSurfaces)); ++i) {
            grid->addWidget(label(QString::fromLatin1(kSurfaces[i].name), card, TypeRole::Body),
                            i, 0);
            grid->addWidget(label(QStringLiteral("%1 px").arg(profile.radius(kSurfaces[i].role)),
                                  card, TypeRole::Mono, Ink::Accent),
                            i, 1);
        }
        grid->setColumnStretch(2, 1);
        body->addLayout(grid);
        note(body, card,
             QStringLiteral("Pill is stored as a marker (%1) resolved against the widget's own "
                            "height at paint time, so a pill stays a pill at any size.")
                 .arg(profile.radii.pill));
        column->addWidget(card);
    }

    // ── spacing and borders ──────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Spacing, borders, controls"),
                                 QStringLiteral("Steps are systemic; values are per profile."));
        QVBoxLayout* body = card->body();
        auto* grid = new QGridLayout();
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setHorizontalSpacing(profile.space(SpacingRole::ToolbarGap));

        int r = 0;
        for (const SpacingRow& row : kSpacings) {
            grid->addWidget(label(QString::fromLatin1(row.name), card, TypeRole::Body), r, 0);
            grid->addWidget(label(QStringLiteral("%1 px").arg(profile.space(row.role)), card,
                                  TypeRole::Mono, Ink::Accent),
                            r, 1);
            ++r;
        }
        for (const BorderRow& row : kBorders) {
            grid->addWidget(label(QStringLiteral("border / %1").arg(QLatin1String(row.name)),
                                  card, TypeRole::Body),
                            r, 0);
            grid->addWidget(label(QStringLiteral("%1 px").arg(profile.border(row.role)), card,
                                  TypeRole::Mono, Ink::Accent),
                            r, 1);
            ++r;
        }
        const std::pair<const char*, ControlSize> sizes[] = {
            {"control / Small", ControlSize::Small},
            {"control / Medium", ControlSize::Medium},
            {"control / Large", ControlSize::Large}};
        for (const auto& size : sizes) {
            grid->addWidget(label(QString::fromLatin1(size.first), card, TypeRole::Body), r, 0);
            grid->addWidget(label(QStringLiteral("%1 px").arg(profile.controlHeight(size.second)),
                                  card, TypeRole::Mono, Ink::Accent),
                            r, 1);
            ++r;
        }
        grid->setColumnStretch(2, 1);
        body->addLayout(grid);
        column->addWidget(card);
    }

    // ── chart and table tokens ───────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Chart and table tokens"),
                                 QStringLiteral("Part of the theme, not of the charting code."));
        QVBoxLayout* body = card->body();
        auto* grid = new QGridLayout();
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setHorizontalSpacing(profile.space(SpacingRole::ToolbarGap));
        int r = 0;
        colourRow(grid, r++, card, QStringLiteral("chart.grid"), profile.chart.grid);
        colourRow(grid, r++, card, QStringLiteral("chart.axis"), profile.chart.axis);
        colourRow(grid, r++, card, QStringLiteral("chart.bullish"), profile.chart.bullish);
        colourRow(grid, r++, card, QStringLiteral("chart.bearish"), profile.chart.bearish);
        colourRow(grid, r++, card, QStringLiteral("chart.volume"), profile.chart.volume);
        colourRow(grid, r++, card, QStringLiteral("chart.crosshair"), profile.chart.crosshair);
        colourRow(grid, r++, card, QStringLiteral("table.headerBg"), profile.table.headerBg);
        colourRow(grid, r++, card, QStringLiteral("table.gridLine"), profile.table.gridLine);
        colourRow(grid, r++, card, QStringLiteral("table.selectionBg"), profile.table.selectionBg);
        colourRow(grid, r++, card, QStringLiteral("table.hoverBg"), profile.table.hoverBg);
        grid->setColumnStretch(3, 1);
        body->addLayout(grid);

        if (!profile.chart.series.isEmpty()) {
            QHBoxLayout* series = exampleRow(body, card, QStringLiteral("Categorical series"));
            for (const QColor& colour : profile.chart.series) {
                series->addWidget(swatch(card, colour));
            }
            series->addStretch(1);
        }

        note(body, card,
             QStringLiteral("Animation: instant %1 ms, fast %2 ms, normal %3 ms, slow %4 ms. "
                            "Icons: %5 / %6 / %7 px.")
                 .arg(profile.animation.instantMs)
                 .arg(profile.animation.fastMs)
                 .arg(profile.animation.normalMs)
                 .arg(profile.animation.slowMs)
                 .arg(profile.icons.small)
                 .arg(profile.icons.medium)
                 .arg(profile.icons.large));
        column->addWidget(card);
    }

    // ── ink and plate ────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Status ink and plate"),
                                 QStringLiteral("Two colours per status, stated separately by "
                                                "the approved design — never one derived from "
                                                "the other."));
        QVBoxLayout* body = card->body();
        auto* grid = new QGridLayout();
        grid->setHorizontalSpacing(profile.space(SpacingRole::ToolbarGap));
        grid->setVerticalSpacing(4);
        grid->addWidget(label(QStringLiteral("role"), card, TypeRole::Caption, Ink::Muted), 0, 0);
        grid->addWidget(label(QStringLiteral("ink"), card, TypeRole::Caption, Ink::Muted), 0, 1);
        grid->addWidget(label(QStringLiteral("plate"), card, TypeRole::Caption, Ink::Muted), 0, 3);
        const struct { const char* name; SemanticRole role; QColor plate; } rows[] = {
            {"success", SemanticRole::Success, c.successPlate},
            {"warning", SemanticRole::Warning, c.warningPlate},
            {"danger", SemanticRole::Danger, c.dangerPlate},
            {"info", SemanticRole::Info, c.infoPlate},
        };
        int r = 1;
        for (const auto& entry : rows) {
            const SemanticColor& sc = profile.semantic.role(entry.role);
            grid->addWidget(label(QString::fromLatin1(entry.name), card, TypeRole::Body), r, 0);
            grid->addWidget(swatch(card, sc.text), r, 1);
            grid->addWidget(label(sc.text.name().toUpper(), card, TypeRole::Mono, Ink::Muted), r, 2);
            grid->addWidget(swatch(card, entry.plate), r, 3);
            grid->addWidget(label(entry.plate.name().toUpper(), card, TypeRole::Mono, Ink::Muted), r, 4);
            ++r;
        }
        grid->setColumnStretch(4, 1);
        body->addLayout(grid);
        note(body, card,
             QStringLiteral("A Badge reads the ink for its text and mixes its plate from the "
                            "plate colour at the role's own alpha — %1's success plate is "
                            "%2, not a lightened %3.")
                 .arg(profile.id, c.successPlate.name(), c.success.name()));
        column->addWidget(card);
    }

    // ── the family palette ───────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Family palette"),
                                 QStringLiteral("Ten named hues, the same value in every "
                                                "product. Shared colour, not shared meaning — "
                                                "purple is agents in NovuAI and a product mark "
                                                "in NovuSpace."));
        QVBoxLayout* body = card->body();
        auto* grid = new QGridLayout();
        grid->setHorizontalSpacing(profile.space(SpacingRole::ToolbarGap));
        grid->setVerticalSpacing(6);
        int i = 0;
        for (const FamilyHue& hue : familyHues()) {
            const int col = i % 5;
            const int row = i / 5;
            auto* cell = new QVBoxLayout();
            cell->setSpacing(2);
            cell->addWidget(swatch(card, hue.color));
            cell->addWidget(label(hue.name, card, TypeRole::Caption, Ink::Muted));
            grid->addLayout(cell, row, col);
            ++i;
        }
        body->addLayout(grid);
        note(body, card, QStringLiteral("Swatch's tagPalette() offers exactly this set — a tag "
                                        "is a label a person picks, never the colour that means "
                                        "a status failed."));
        column->addWidget(card);
    }

    // ── drop target ──────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Drop target"),
                                 QStringLiteral("Two outcomes, two weights, never both at once "
                                                "— \"a row lights when the folder will be bound "
                                                "to that workspace, the region lights when it "
                                                "will make a new one.\""));
        QVBoxLayout* body = card->body();
        auto* row = exampleRow(body, card, QStringLiteral("Row target — one exact destination"));
        row->addWidget(new DropPreview(DropZone::Row, card));
        auto* row2 = exampleRow(body, card, QStringLiteral("Region target — anywhere in here"));
        row2->addWidget(new DropPreview(DropZone::Region, card));
        note(body, card,
             QStringLiteral("Both painted from dropRowSpec() / dropRegionSpec() — the tokens "
                            "a real drop handler resolves, not a copy of them."));
        column->addWidget(card);
    }

    // ── chrome and density ───────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Chrome and density"),
                                 QStringLiteral("Stated per product, not shared — the "
                                                "authoritative mockups disagree on all four."));
        QVBoxLayout* body = card->body();
        auto* grid = new QGridLayout();
        grid->setHorizontalSpacing(profile.space(SpacingRole::ToolbarGap));
        grid->setVerticalSpacing(4);
        int r = 0;
        const struct { const char* name; int value; } metrics[] = {
            {"window.titleBarHeight", profile.window.titleBarHeight},
            {"window.statusBarHeight", profile.window.statusBarHeight},
            {"controls.rowHeight", profile.controls.rowHeight},
            {"controls.navRowHeight", profile.controls.navRowHeight},
            {"controls.treeStep", profile.controls.treeStep},
        };
        for (const auto& m : metrics) {
            grid->addWidget(label(QString::fromLatin1(m.name), card, TypeRole::Mono, Ink::Muted), r, 0);
            grid->addWidget(label(QString::number(m.value) + QStringLiteral("px"), card,
                                  TypeRole::Body), r, 1);
            ++r;
        }
        grid->setColumnStretch(1, 1);
        body->addLayout(grid);
        note(body, card,
             QStringLiteral("Switch profile: NovuAI reads 26 / 26 / 26, NovuExplorer 24 / 24, "
                            "NovuSpace's title bar reads 44 like the other three even though "
                            "its own mockup states 42 — recorded as unreconciled in "
                            "docs/DESIGN_CONTRACT.md §4, not silently kept or dropped."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec themeRolesComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("theme-roles");
    spec.name = QStringLiteral("Theme roles");
    spec.category = Category::Foundation;
    spec.description = QStringLiteral(
        "Every token the active ThemeProfile resolves: colours, surface radii, spacing, "
        "borders, control heights, chart and table tokens. Switch the profile above to diff "
        "two products.");
    spec.headerPath = QStringLiteral("include/novu/ui/ThemeProfile.h");
    spec.sourcePath = QStringLiteral("src/ThemeProfile.cpp");
    spec.designedFor = {QStringLiteral("novuspace"),
                   QStringLiteral("novuai"),
                   QStringLiteral("novuexplorer"),
                   QStringLiteral("novuhub"),
                   QStringLiteral("novuvoice"),
                   QStringLiteral("novutrade")};
    spec.usedBy = {QStringLiteral("novuspace")};
    spec.variants = {QStringLiteral("Colours"), QStringLiteral("SurfaceRole"),
                     QStringLiteral("SpacingRole"), QStringLiteral("BorderRole"),
                     QStringLiteral("ControlSize"), QStringLiteral("Chart"),
                     QStringLiteral("Table")};
    spec.states = {QStringLiteral("Per profile")};
    spec.keywords = {QStringLiteral("theme"), QStringLiteral("colour"), QStringLiteral("color"),
                     QStringLiteral("token"), QStringLiteral("radius"), QStringLiteral("spacing"),
                     QStringLiteral("profile"), QStringLiteral("palette"),
                     QStringLiteral("design system")};
    spec.properties = {
        {QStringLiteral("id"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("Profile id, as registered with ThemeManager.")},
        {QStringLiteral("colors"), QStringLiteral("ThemeColors"), {}, QString(),
         QStringLiteral("18 named colours. No component contains a hex literal.")},
        {QStringLiteral("roleRadius"), QStringLiteral("QHash<SurfaceRole,RadiusStep>"), {},
         QString(), QStringLiteral("What each surface's roundness step is.")},
    };
    spec.scenarios = {Scenario::Normal};
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
