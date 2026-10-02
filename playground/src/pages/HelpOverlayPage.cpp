#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Badge.h"
#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/Fields.h"
#include "novu/ui/HelpOverlay.h"
#include "novu/ui/StatCard.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

// A small stand-in screen for the overlay to annotate.
//
// Built out of NovuUI components rather than mocked up, because an overlay that
// only looks right over a drawing is not evidence of anything: the notes anchor
// to real widgets and follow them when this card is resized.
struct Scene
{
    QWidget* root = nullptr;
    QWidget* rail = nullptr;
    QWidget* metric = nullptr;
    QWidget* action = nullptr;
    QWidget* danger = nullptr;
};

Scene buildScene(QWidget* parent)
{
    Scene scene;
    scene.root = new QWidget(parent);
    scene.root->setMinimumHeight(280);

    auto* row = new QHBoxLayout(scene.root);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(themeFor(parent).space(SpacingRole::CardGap));

    auto* rail = new Card(QStringLiteral("Workspaces"), scene.root);
    rail->setFixedWidth(150);
    for (const QString& name : {QStringLiteral("NovuBuilder"), QStringLiteral("NovuServer"),
                                QStringLiteral("NovuAI")}) {
        rail->body()->addWidget(label(name, rail, TypeRole::Body, Ink::Secondary));
    }
    rail->body()->addStretch(1);
    scene.rail = rail;
    row->addWidget(rail);

    auto* column = new QVBoxLayout();
    column->setSpacing(themeFor(parent).space(SpacingRole::CardGap));
    auto* metrics = new QHBoxLayout();
    metrics->setSpacing(themeFor(parent).space(SpacingRole::CardGap));
    auto* entries = new StatCard(QStringLiteral("Knowledge Entries"),
                                 QStringLiteral("0"), scene.root);
    scene.metric = entries;
    metrics->addWidget(entries);
    metrics->addWidget(new StatCard(QStringLiteral("Findings"), QStringLiteral("0"),
                                    scene.root));
    column->addLayout(metrics);
    column->addStretch(1);
    row->addLayout(column, 1);

    auto* side = new QVBoxLayout();
    side->setSpacing(themeFor(parent).space(SpacingRole::ToolbarGap));
    Button* update = primaryButton(QStringLiteral("Update"), scene.root);
    Button* rebuild = dangerButton(QStringLiteral("Full Rebuild"), scene.root);
    scene.action = update;
    scene.danger = rebuild;
    side->addWidget(update);
    side->addWidget(rebuild);
    side->addStretch(1);
    row->addLayout(side);

    return scene;
}

void annotate(HelpOverlay* help, const Scene& scene)
{
    help->addNote(scene.rail, QStringLiteral("Seznam workspace"),
                  QStringLiteral("Všechny vaše workspace a moduly. Číslo ukazuje počet "
                                 "položek v kategorii."),
                  HelpPlacement::Right);
    help->addNote(scene.metric, QStringLiteral("Přehled metrik"),
                  QStringLiteral("Celkový přehled znalostí ve workspace."),
                  HelpPlacement::Below, HelpTone::Info);
    help->addNote(scene.action, QStringLiteral("Rychlé akce"),
                  QStringLiteral("Aktualizuje znalosti z nových provedení a změn."),
                  HelpPlacement::Left, HelpTone::Success);
    help->addNote(scene.danger, QStringLiteral("Full Rebuild"),
                  QStringLiteral("Kompletně přebuduje index. Trvá déle."),
                  HelpPlacement::Below, HelpTone::Danger);
}

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    // ── the overlay over a live screen ────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Over a live screen"),
                                 QStringLiteral("The window underneath keeps working — click "
                                                "the buttons with the help showing."));
        QVBoxLayout* body = card->body();

        const Scene scene = buildScene(card);
        body->addWidget(scene.root);

        auto* help = HelpOverlay::attach(scene.root);
        annotate(help, scene);
        help->show();

        QHBoxLayout* row = exampleRow(body, card, QString());
        Button* toggle = secondaryButton(QStringLiteral("Hide help"), card);
        QPointer<HelpOverlay> overlay = help;
        QObject::connect(toggle, &QAbstractButton::clicked, card, [overlay, toggle] {
            if (!overlay) {
                return;
            }
            const bool showing = overlay->isVisible();
            overlay->setVisible(!showing);
            toggle->setText(showing ? QObject::tr("Show help") : QObject::tr("Hide help"));
        });
        row->addWidget(toggle);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("The overlay is transparent to the mouse, so this is annotation "
                            "over a running application rather than a tour that stops it. "
                            "Nothing is dimmed: dimming would make the thing being explained "
                            "the hardest thing on screen to look at."));
        note(body, card,
             QStringLiteral("Anchors are resolved at paint time from each target's current "
                            "geometry, so notes follow their targets through a resize — drag "
                            "the gallery window and watch the leaders track."));
        column->addWidget(card);
    }

    // ── tones ─────────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Tones"),
                                 QStringLiteral("Accent by default. The semantic roles are "
                                                "here so a note that means something can look "
                                                "like it — and so eight bubbles on one screen "
                                                "can be told apart."));
        QVBoxLayout* body = card->body();

        auto* strip = new QWidget(card);
        strip->setMinimumHeight(150);
        auto* grid = new QGridLayout(strip);
        grid->setContentsMargins(0, 0, 0, 0);

        const struct { const char* name; HelpTone tone; } tones[] = {
            {"Accent", HelpTone::Accent}, {"Info", HelpTone::Info},
            {"Success", HelpTone::Success}, {"Warning", HelpTone::Warning},
            {"Danger", HelpTone::Danger}, {"Neutral", HelpTone::Neutral},
        };
        auto* overlay = HelpOverlay::attach(strip);
        for (int i = 0; i < 6; ++i) {
            auto* anchor = new Badge(QString::fromLatin1(tones[i].name),
                                     Badge::Tone::Neutral, strip);
            grid->addWidget(anchor, i % 2 == 0 ? 0 : 2, i / 2, Qt::AlignCenter);
            overlay->addNote(anchor, QString::fromLatin1(tones[i].name),
                             QStringLiteral("One sentence of explanation."),
                             i % 2 == 0 ? HelpPlacement::Below : HelpPlacement::Above,
                             tones[i].tone);
        }
        overlay->setNoteWidth(140);
        overlay->show();
        body->addWidget(strip);
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec helpOverlayComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("help-overlay");
    spec.name = QStringLiteral("HelpOverlay");
    spec.category = Category::Help;
    spec.description = QStringLiteral(
        "An explanatory layer over a live window: coloured bubbles joined to what they explain "
        "by dashed leaders. The application underneath keeps working.");
    spec.headerPath = QStringLiteral("include/novu/ui/HelpOverlay.h");
    spec.sourcePath = QStringLiteral("src/HelpOverlay.cpp");
    spec.usedBy = {};
    // Nothing consumes it yet - it was built here first, which is what the
    // design system asks for. NovuAI is the first planned consumer.
    spec.awaitingAdoption = true;
    spec.variants = {QStringLiteral("Accent"), QStringLiteral("Info"),
                     QStringLiteral("Success"), QStringLiteral("Warning"),
                     QStringLiteral("Danger"), QStringLiteral("Neutral")};
    spec.states = {QStringLiteral("Shown"), QStringLiteral("Hidden"),
                   QStringLiteral("Target hidden"), QStringLiteral("Clamped to frame")};
    spec.keywords = {QStringLiteral("help"), QStringLiteral("overlay"),
                     QStringLiteral("callout"), QStringLiteral("bubble"),
                     QStringLiteral("annotation"), QStringLiteral("onboarding"),
                     QStringLiteral("coach"), QStringLiteral("hint"),
                     QStringLiteral("nápověda")};
    spec.properties = {
        {QStringLiteral("title"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("The note's heading, drawn in the note's own colour.")},
        {QStringLiteral("body"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("One or two sentences. Wraps; the bubble grows to fit.")},
        {QStringLiteral("placement"), QStringLiteral("enum"),
         {QStringLiteral("Auto"), QStringLiteral("Left"), QStringLiteral("Right"),
          QStringLiteral("Above"), QStringLiteral("Below")},
         QStringLiteral("Auto"),
         QStringLiteral("Auto measures the free space on each side and takes the largest.")},
        {QStringLiteral("tone"), QStringLiteral("enum"),
         {QStringLiteral("Accent"), QStringLiteral("Info"), QStringLiteral("Success"),
          QStringLiteral("Warning"), QStringLiteral("Danger"), QStringLiteral("Neutral")},
         QStringLiteral("Accent"),
         QStringLiteral("Border, title and leader. Colours the semantic palette; Accent is "
                        "the application's.")},
        {QStringLiteral("noteWidth"), QStringLiteral("int"), {}, QStringLiteral("type.md * 14"),
         QStringLiteral("One width for every note: bubbles sized to their own sentence read "
                        "as clutter.")},
    };
    spec.scenarios = {Scenario::Normal, Scenario::LongText};
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
