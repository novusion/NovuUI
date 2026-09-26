#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/Fields.h"
#include "novu/ui/IconButton.h"
#include "novu/ui/Segmented.h"
#include "novu/ui/ThemeScope.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/Separator.h"
#include "novu/ui/SettingRow.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/Toggle.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);
    // The base profile carries the shared semantic palette every profile
    // outside novu-light is required to agree on; only the accent - what a
    // focused Segmented's ring actually reads (Segmented::paintEvent()) - is
    // taken from the gallery's active theme, so this compact control's focus
    // ring is that theme's own accent rather than novusionBaseProfile's
    // unrelated one.
    auto* compact=new Segmented({"Properties","Screen"},page);auto profile=novusionBaseProfile();profile.id=ctx.profileId+QStringLiteral("-compact-segments-gallery");
    const ThemeProfile& activeSegProfile=ThemeManager::instance().profile(ctx.profileId);
    profile.colors.accent=activeSegProfile.colors.accent;
    profile.colors.accentLight=activeSegProfile.colors.accentLight;
    profile.colors.textOnAccent=activeSegProfile.colors.textOnAccent;
    profile.controls.segmentedHeight=26;profile.controls.segmentedPadding=2;ThemeManager::instance().registerProfile(profile);ThemeScope::apply(compact,profile.id);compact->setFill(true);compact->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);column->addWidget(compact);


    const bool longText = ctx.scenario == Scenario::LongText;

    // ── a settings panel ─────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("A settings panel"),
                                 QStringLiteral("A name, what it means, and the "
                                                "control that changes it."));
        QVBoxLayout* body = card->body();
        body->setSpacing(0);

        auto* tunnel = new SettingRow(
            QStringLiteral("Managed tunnel"),
            QStringLiteral("Routes platform traffic through the WireGuard interface. "
                           "Ordinary traffic is unaffected."),
            card);
        tunnel->setControl(new Toggle(tunnel));
        tunnel->setControlColumn(120);
        body->addWidget(tunnel);

        auto* retention = new SettingRow(
            QStringLiteral("Retention period"),
            longText
                ? QStringLiteral("How long audit records are kept before the nightly sweep "
                                 "removes them. Records referenced by an open investigation "
                                 "are exempt and are kept until the investigation is closed, "
                                 "regardless of this setting. Shortening the period does not "
                                 "delete anything until the next sweep runs.")
                : QStringLiteral("How long audit records are kept before the nightly "
                                 "sweep removes them."),
            card);
        auto* days = lineEdit(QStringLiteral("90"), retention, 110);
        days->setAlignment(Qt::AlignRight);
        retention->setControl(days);
        retention->setControlColumn(120);
        body->addWidget(retention);

        auto* density = new SettingRow(
            QStringLiteral("Index density"),
            QStringLiteral("More detail costs more disk and more time to rebuild."),
            card);
        auto* picker = new Segmented({QStringLiteral("Basic"), QStringLiteral("Extended"),
                                      QStringLiteral("Pro")},
                                     density, 1);
        density->setControl(picker);
        density->setControlColumn(picker->sizeHint().width());
        body->addWidget(density);

        auto* region = new SettingRow(
            QStringLiteral("Region"),
            QStringLiteral("Where new workspaces are created. Existing ones do not move."),
            card);
        auto* combo = new ComboBox(region);
        combo->addItems({QStringLiteral("eu-central"), QStringLiteral("eu-west"),
                         QStringLiteral("us-east")});
        region->setControl(combo);
        region->setControlColumn(140);
        region->setDivider(false);
        body->addWidget(region);

        note(body, card,
             QStringLiteral("The control is aligned to the name's line rather than to the "
                            "middle of the row: with a three-line hint a centred control "
                            "floats beside the sentence instead of beside the thing it "
                            "changes. Switch the scenario to Long text to see it."));
        note(body, card,
             QStringLiteral("This is not formRow(). That one is a fixed label column with the "
                            "control beside it, which is right for a dialog where six fields "
                            "must line up. This is right for a settings page, where each row "
                            "explains itself and the explanation is a sentence. One function "
                            "doing both is how the settings pages ended up with captions too "
                            "short to explain anything."));
        column->addWidget(card);
    }

    // ── read-only and masked ─────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Read-only, and secret"),
                                 QStringLiteral("A value instead of a control."));
        QVBoxLayout* body = card->body();
        body->setSpacing(0);

        auto* version = new SettingRow(QStringLiteral("Server version"),
                                       QStringLiteral("Reported by the platform at "
                                                      "connect time."),
                                       card);
        version->setValue(QStringLiteral("0.67.11+build.2841"));
        body->addWidget(version);

        auto* device = new SettingRow(QStringLiteral("Device identifier"),
                                      QStringLiteral("This installation, as the platform "
                                                     "knows it."),
                                      card);
        device->setValue(QStringLiteral("f3b1c0e4-2d77-4a19-9c30-8e5a1b7d4f62"));
        body->addWidget(device);

        auto* secret = new SettingRow(QStringLiteral("API secret"),
                                      QStringLiteral("Shown once, when it is created. "
                                                     "It is never printed anywhere "
                                                     "outside this window."),
                                      card);
        secret->setValue(QStringLiteral("sk_live_9f2b8c14a77e4d03b6e1"));
        secret->setMasked(true);
        secret->setDivider(false);
        body->addWidget(secret);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Reveal"));
        auto* reveal = new Button(QStringLiteral("Show"), Button::Variant::Secondary, card);
        reveal->setControlSize(ControlSize::Small);
        QObject::connect(reveal, &Button::clicked, secret, [secret, reveal] {
            const bool masked = !secret->isMasked();
            secret->setMasked(masked);
            reveal->setText(masked ? QObject::tr("Show") : QObject::tr("Hide"));
        });
        row->addWidget(reveal);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("Masking draws a fixed run of bullets, not one per character. "
                            "Masking that preserves the length leaks the length, which for a "
                            "passphrase is most of what an attacker wanted to know."));
        column->addWidget(card);
    }

    // ── validation ───────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("When it is wrong"),
                                 QStringLiteral("The message belongs to the setting, "
                                                "not to the control."));
        QVBoxLayout* body = card->body();
        body->setSpacing(0);

        auto* endpoint = new SettingRow(
            QStringLiteral("Endpoint"),
            QStringLiteral("Where the hub is reached. A hostname or an address, with "
                           "an optional port."),
            card);
        auto* field = lineEdit(QStringLiteral("hub.novusion.net:51820"), endpoint, 220);
        field->setText(QStringLiteral("hub.novusion.net:51820"));
        endpoint->setControl(field);
        endpoint->setControlColumn(230);
        endpoint->setDivider(false);
        body->addWidget(endpoint);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Validate"));
        auto* bad = new Button(QStringLiteral("Break it"), Button::Variant::Secondary, card);
        bad->setControlSize(ControlSize::Small);
        QObject::connect(bad, &Button::clicked, endpoint, [endpoint, field] {
            field->setText(QStringLiteral("hub.novusion.net:518200"));
            endpoint->setError(QObject::tr("Port must be between 1 and 65535."));
        });
        row->addWidget(bad);

        auto* fix = new Button(QStringLiteral("Fix it"), Button::Variant::Secondary, card);
        fix->setControlSize(ControlSize::Small);
        QObject::connect(fix, &Button::clicked, endpoint, [endpoint, field] {
            field->setText(QStringLiteral("hub.novusion.net:51820"));
            endpoint->setError(QString());
        });
        row->addWidget(fix);
        row->addStretch(1);

        note(body, card,
             QStringLiteral("The message goes under the row rather than beside the control, so "
                            "a long one does not push the control off the right edge — and the "
                            "row grows to make space for it rather than clipping. The row's "
                            "styleSpec() changes too, so a test can assert that an invalid "
                            "setting is marked without measuring a pixel."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec settingRowComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("setting-row");
    spec.name = QStringLiteral("SettingRow");
    spec.category = Category::Input;
    spec.description = QStringLiteral(
        "A name, what it means, and the control that changes it — or a read-only value, "
        "or a masked secret, with the validation message under the row.");
    spec.headerPath = QStringLiteral("include/novu/ui/SettingRow.h");
    spec.sourcePath = QStringLiteral("src/SettingRow.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("With control"), QStringLiteral("Read-only"),
                     QStringLiteral("Masked")};
    spec.states = {QStringLiteral("Normal"), QStringLiteral("Error"),
                   QStringLiteral("Last in list")};
    spec.scenarios = {Scenario::Normal, Scenario::LongText};
    spec.keywords = {QStringLiteral("kv"), QStringLiteral("prow"),
                     QStringLiteral("settings"), QStringLiteral("preference"),
                     QStringLiteral("secretrow"), QStringLiteral("ferr"),
                     QStringLiteral("validation")};
    spec.properties = {
        {QStringLiteral("hint"), QStringLiteral("QString"), {}, QStringLiteral(""),
         QStringLiteral("Wraps, capped at the design's 58ch measure.")},
        {QStringLiteral("value"), QStringLiteral("QString"), {}, QStringLiteral(""),
         QStringLiteral("A read-only value instead of a control; setting one clears the "
                        "other.")},
        {QStringLiteral("masked"), QStringLiteral("bool"), {}, QStringLiteral("false"),
         QStringLiteral("A fixed run of bullets, so the length does not leak.")},
        {QStringLiteral("error"), QStringLiteral("QString"), {}, QStringLiteral(""),
         QStringLiteral("The validation line. Changes the row's styleSpec() too.")},
        {QStringLiteral("controlColumn"), QStringLiteral("int"), {}, QStringLiteral("0"),
         QStringLiteral("Fixed, so rows in one panel line up. 0 measures the control.")},
    };
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
