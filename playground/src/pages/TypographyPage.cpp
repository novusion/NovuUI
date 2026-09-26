#include "Pages.h"

#include "novu/playground/ExampleLayout.h"
#include "novu/playground/SampleData.h"

#include "novu/ui/Card.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeScope.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/ThemeManager.h"

#include <QGridLayout>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <iterator>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

struct RoleRow
{
    const char* name;
    TypeRole role;
    const char* use;
};

const RoleRow kRoles[] = {
    {"PageTitle", TypeRole::PageTitle, "the title of a page"},
    {"SectionTitle", TypeRole::SectionTitle, "a heading inside a page"},
    {"CardTitle", TypeRole::CardTitle, "a card's own title"},
    {"Body", TypeRole::Body, "everything else"},
    {"Caption", TypeRole::Caption, "muted helper text"},
    {"Label", TypeRole::Label, "form labels"},
    {"Metric", TypeRole::Metric, "the large number on a StatCard"},
    {"Mono", TypeRole::Mono, "ids, hashes, versions"},
    {"Eyebrow", TypeRole::Eyebrow, "the name of a group: a caption, a column head"},
};

struct InkRow
{
    const char* name;
    Ink ink;
};

const InkRow kInks[] = {
    {"Default", Ink::Default},   {"Secondary", Ink::Secondary}, {"Muted", Ink::Muted},
    {"Accent", Ink::Accent},     {"Success", Ink::Success},     {"Warning", Ink::Warning},
    {"Danger", Ink::Danger},     {"Info", Ink::Info}, {"Tertiary", Ink::Tertiary},
};

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    auto* elision = elidedLabel(QStringLiteral("A long path / workspace / document that must not widen its column"), page, TypeRole::Mono);
    elision->setFixedWidth(240);
    column->addWidget(elision);
    auto* natural=new Card(QStringLiteral("Natural text metrics"),page);
    auto naturalProfile=themeFor(page);naturalProfile.id=QStringLiteral("gallery-natural-type");naturalProfile.type.unhintedMetrics=true;
    ThemeManager::instance().registerProfile(naturalProfile);ThemeScope::apply(natural,naturalProfile.id);
    natural->body()->addWidget(label(QStringLiteral("Fractional outlines without grid-fitted advances"),natural));
    column->addWidget(natural);
    const ThemeProfile& profile = themeFor(ctx.parent);
    const QString sample = ctx.scenario == Scenario::LongText
        ? ctx.data->text(QStringLiteral("long_text"))
        : ctx.scenario == Scenario::LongIdentifier
            ? ctx.data->text(QStringLiteral("long_identifier"))
            : QStringLiteral("The quick brown fox jumps over the lazy dog — 0123456789");

    // ── type roles ───────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Type roles"),
                                 QStringLiteral("A page asks for a role; the profile decides the "
                                                "pixels."));
        QVBoxLayout* body = card->body();
        QGridLayout* grid = matrix(body, card,
                                   {QStringLiteral("Sample"), QStringLiteral("Resolved"),
                                    QStringLiteral("Used for")},
                                   [] {
                                       QStringList names;
                                       for (const RoleRow& row : kRoles) {
                                           names.append(QString::fromLatin1(row.name));
                                       }
                                       return names;
                                   }());

        for (int r = 0; r < static_cast<int>(std::size(kRoles)); ++r) {
            const RoleRow& row = kRoles[r];
            QLabel* text = label(sample, card, row.role);
            text->setWordWrap(ctx.scenario == Scenario::LongText);
            matrixCell(grid, r, 0, text);

            const TypeStyle style = profile.text(row.role);
            matrixCell(grid, r, 1,
                       label(QStringLiteral("%1px / weight %2%3")
                                 .arg(style.size)
                                 .arg(style.weight)
                                 .arg(style.monospace ? QStringLiteral(" / mono") : QString()),
                             card, TypeRole::Mono, Ink::Muted));
            matrixCell(grid, r, 2,
                       label(QString::fromLatin1(row.use), card, TypeRole::Caption, Ink::Muted));
        }
        column->addWidget(card);
    }

    // ── ink roles ────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Ink roles"),
                                 QStringLiteral("Colour is applied through the palette, so a "
                                                "label can be recoloured live."));
        QVBoxLayout* body = card->body();
        QGridLayout* grid = matrix(body, card,
                                   {QStringLiteral("Body"), QStringLiteral("Bold"),
                                    QStringLiteral("Mono")},
                                   [] {
                                       QStringList names;
                                       for (const InkRow& row : kInks) {
                                           names.append(QString::fromLatin1(row.name));
                                       }
                                       return names;
                                   }());
        for (int r = 0; r < static_cast<int>(std::size(kInks)); ++r) {
            const InkRow& row = kInks[r];
            matrixCell(grid, r, 0,
                       label(QStringLiteral("Connected"), card, TypeRole::Body, row.ink));
            matrixCell(grid, r, 1,
                       label(QStringLiteral("Connected"), card, TypeRole::Body, row.ink, true));
            QLabel* mono = label(QStringLiteral("0x7f3c1e9a"), card, TypeRole::Mono, row.ink);
            mono->setFont(roleMonoFont(card, TypeRole::Mono));
            matrixCell(grid, r, 2, mono);
        }
        column->addWidget(card);
    }

    // ── helpers ──────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("caption() and applyText()"),
                                 QStringLiteral("Creation and everything after it."));
        QVBoxLayout* body = card->body();
        body->addWidget(caption(QStringLiteral("Uppercase heading above a group"), card));
        QLabel* live = label(QStringLiteral("A label built as Body/Default, then re-roled by "
                                            "applyText() to CardTitle/Warning."),
                             card, TypeRole::Body);
        live->setWordWrap(true);
        applyText(live, TypeRole::CardTitle, Ink::Warning, true);
        body->addWidget(live);
        note(body, card,
             QStringLiteral("Without applyText() every consumer writes the same setFont/setInk "
                            "pair, which is how the roles start drifting apart again. The "
                            "monospace family is %1; the body family is %2.")
                 .arg(profile.type.monoFamily, profile.type.family));
        column->addWidget(card);
    }

    // ── what a role carries besides a font ───────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Tracking, line height, case"),
                                 QStringLiteral("Three properties the approved CSS states "
                                                "per role that a font alone does not "
                                                "carry."));
        QVBoxLayout* body = card->body();
        const ThemeProfile& profile = themeFor(card);

        for (const TypeRole role : {TypeRole::Eyebrow, TypeRole::Metric, TypeRole::PageTitle}) {
            const TypeStyle style = profile.text(role);
            auto* sample = label(QStringLiteral("Workspaces 1.3 GB"), card, role);
            body->addWidget(sample);
            body->addWidget(label(QStringLiteral("size %1 · weight %2 · tracking %3em · "
                                                 "line-height %4 · %5")
                                      .arg(style.size)
                                      .arg(style.weight)
                                      .arg(style.letterSpacing)
                                      .arg(style.lineHeight == 0.0
                                               ? QStringLiteral("font's own")
                                               : QString::number(style.lineHeight))
                                      .arg(style.uppercase ? QStringLiteral("uppercase")
                                                           : QStringLiteral("as written")),
                                  card, TypeRole::Caption, Ink::Muted));
        }

        note(body, card,
             QStringLiteral("`letter-spacing:.06em`, `line-height:1.05`, "
                            "`text-transform:uppercase` - stated by the profile, applied by "
                            "roleFont() and label(). A caller never calls toUpper() and two "
                            "call sites cannot disagree about a role's tracking; a display "
                            "face laid out to its own metrics instead of the design's line "
                            "box is several pixels taller in every metric card on a page."));
        column->addWidget(card);
    }

    // ── a paragraph in a column ──────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Paragraph"),
                                 QStringLiteral("Prose that answers for its own "
                                                "height."));
        QVBoxLayout* body = card->body();

        const QString prose =
            QStringLiteral("A wrapped label is asked two questions by every layout it "
                           "sits in -- how wide would you like to be, and how tall are "
                           "you -- and QLabel answers the second one at a width nobody "
                           "asked about. A Paragraph answers it at the width it has.");

        auto* plain = new QLabel(prose, card);
        plain->setWordWrap(true);
        plain->setMinimumWidth(1);
        auto* measured = new Paragraph(prose, card, TypeRole::Body, Ink::Secondary);

        body->addWidget(label(QStringLiteral("setWordWrap(true) on a QLabel"), card,
                              TypeRole::Label, Ink::Muted));
        body->addWidget(plain);
        body->addWidget(label(QStringLiteral("Paragraph"), card, TypeRole::Label,
                              Ink::Muted));
        body->addWidget(measured);

        auto* readout = label(QString(), card, TypeRole::Mono, Ink::Muted);
        auto report = [plain, measured, readout] {
            readout->setText(QStringLiteral("QLabel hint %1x%2, min %3   |   "
                                            "Paragraph hint %4x%5, min %6")
                                 .arg(plain->sizeHint().width())
                                 .arg(plain->sizeHint().height())
                                 .arg(plain->minimumSizeHint().height())
                                 .arg(measured->sizeHint().width())
                                 .arg(measured->sizeHint().height())
                                 .arg(measured->minimumSizeHint().height()));
        };
        report();
        // Both hints depend on the width the column ends up giving them, so
        // the reading is taken once the page has been laid out.
        QTimer::singleShot(0, readout, report);
        body->addWidget(readout);

        note(body, card,
             QStringLiteral("The two heights are the whole of it. A scroll area sizes its "
                            "content to the minimum it is told, so a column of wrapped "
                            "QLabels asks for several times the room it uses and a box "
                            "layout shares the surplus out among whatever else can grow "
                            "-- which is how a code block quoted in a chat turn became a "
                            "thousand pixels of empty surface."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec typographyComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("typography");
    spec.name = QStringLiteral("Typography");
    spec.category = Category::Foundation;
    spec.description = QStringLiteral(
        "Text by role, never by number: label(), caption(), applyText(), setInk(), and the "
        "TypeRole / Ink vocabulary they resolve against.");
    spec.headerPath = QStringLiteral("include/novu/ui/Text.h");
    spec.sourcePath = QStringLiteral("src/Text.cpp");
    spec.designedFor = {QStringLiteral("novuspace"),
                   QStringLiteral("novuai"),
                   QStringLiteral("novuexplorer"),
                   QStringLiteral("novuhub"),
                   QStringLiteral("novuvoice"),
                   QStringLiteral("novutrade")};
    spec.usedBy = {QStringLiteral("novuspace")};
    spec.variants = {QStringLiteral("Paragraph"), QStringLiteral("PageTitle"), QStringLiteral("SectionTitle"),
                     QStringLiteral("CardTitle"), QStringLiteral("Body"),
                     QStringLiteral("Caption"), QStringLiteral("Label"),
                     QStringLiteral("Metric"), QStringLiteral("Mono"),
                     QStringLiteral("Eyebrow")};
    spec.states = {QStringLiteral("Default"), QStringLiteral("Secondary"), QStringLiteral("Muted"),
                   QStringLiteral("Accent"), QStringLiteral("Success"), QStringLiteral("Warning"),
                   QStringLiteral("Danger"), QStringLiteral("Info")};
    spec.keywords = {QStringLiteral("text"), QStringLiteral("font"), QStringLiteral("label"),
                     QStringLiteral("type"), QStringLiteral("ink"), QStringLiteral("colour"),
                     QStringLiteral("color"), QStringLiteral("heading")};
    spec.properties = {
        {QStringLiteral("role"), QStringLiteral("enum"),
         {QStringLiteral("PageTitle"), QStringLiteral("SectionTitle"), QStringLiteral("CardTitle"),
          QStringLiteral("Body"), QStringLiteral("Caption"), QStringLiteral("Label"),
          QStringLiteral("Metric"), QStringLiteral("Mono")},
         QStringLiteral("Body"), QStringLiteral("Size and weight together.")},
        {QStringLiteral("ink"), QStringLiteral("enum"),
         {QStringLiteral("Default"), QStringLiteral("Secondary"), QStringLiteral("Muted"),
          QStringLiteral("Accent"), QStringLiteral("Success"), QStringLiteral("Warning"),
          QStringLiteral("Danger"), QStringLiteral("Info")},
         QStringLiteral("Default"), QStringLiteral("Semantic colour.")},
        {QStringLiteral("bold"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("Overrides the role's weight.")},
    };
    spec.scenarios = {Scenario::Normal, Scenario::LongText, Scenario::LongIdentifier};
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
