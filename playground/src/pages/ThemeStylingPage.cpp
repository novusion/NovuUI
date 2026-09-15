#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Badge.h"
#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/Field.h"
#include "novu/ui/Fields.h"
#include "novu/ui/Feedback.h"
#include "novu/ui/Icon.h"
#include "novu/ui/NavList.h"
#include "novu/ui/Popover.h"
#include "novu/ui/Segmented.h"
#include "novu/ui/Separator.h"
#include "novu/ui/SettingRow.h"
#include "novu/ui/Slider.h"
#include "novu/ui/StatusDot.h"
#include "novu/ui/StyleSpec.h"
#include "novu/ui/TabStrip.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/Toggle.h"
#include "novu/ui/WindowShell.h"

#include <QFontMetrics>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QVBoxLayout>
#include <QWidget>

#include <iterator>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

// ── the token sheet ───────────────────────────────────────────────────────

// Every swatch on this page, painted from the profile the page is scoped to.
//
// One widget, not forty: a swatch is a filled rectangle and a label, and the
// point of the page is that they all move together when the profile changes.
class TokenSheet : public QWidget
{
public:
    struct Entry
    {
        QString name;     // what a component would ask for
        ColorToken token;
    };

    TokenSheet(const QString& caption, QVector<Entry> entries, QWidget* parent)
        : QWidget(parent)
        , m_caption(caption)
        , m_entries(std::move(entries))
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    bool hasHeightForWidth() const override { return true; }

    int heightForWidth(int width) const override
    {
        const int columns = qMax(1, width / kCell);
        const int rows = (m_entries.size() + columns - 1) / columns;
        return kCaption + rows * kRowHeight;
    }

    QSize sizeHint() const override
    {
        return QSize(kCell * 4, heightForWidth(kCell * 4));
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        const ThemeProfile& profile = themeFor(this);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        QFont caption = roleFont(this, TypeRole::Eyebrow);
        caption.setLetterSpacing(QFont::PercentageSpacing, 110);
        painter.setFont(caption);
        painter.setPen(profile.colors.textDisabled);
        painter.drawText(QRect(0, 0, width(), kCaption), Qt::AlignLeft | Qt::AlignVCenter,
                         m_caption.toUpper());

        const int columns = qMax(1, width() / kCell);
        const int cell = width() / columns;

        for (int i = 0; i < m_entries.size(); ++i) {
            const Entry& entry = m_entries.at(i);
            const QRect box(cell * (i % columns), kCaption + kRowHeight * (i / columns),
                            cell, kRowHeight);

            // The whole point: the *token* is fixed, the colour is resolved.
            const QColor resolved = entry.token.resolve(profile);

            const QRectF chip(box.left(), box.top() + 4, kChip, kChip);
            painter.setPen(QPen(profile.colors.borderHover, 1));
            painter.setBrush(resolved.alpha() == 0 ? Qt::NoBrush : QBrush(resolved));
            painter.drawRoundedRect(chip, 5, 5);

            const int textLeft = box.left() + kChip + 10;
            painter.setFont(roleFont(this, TypeRole::Caption));
            painter.setPen(profile.colors.textSecondary);
            painter.drawText(QRect(textLeft, box.top() + 2, cell - kChip - 14, 15),
                             Qt::AlignLeft | Qt::AlignVCenter, entry.name);

            painter.setFont(roleMonoFont(this, TypeRole::Mono));
            painter.setPen(profile.colors.textDisabled);
            painter.drawText(QRect(textLeft, box.top() + 17, cell - kChip - 14, 14),
                             Qt::AlignLeft | Qt::AlignVCenter,
                             // QColor::name() drops the alpha, so a token asking
                             // for the accent at 12% would read as the solid
                             // accent - which is the one thing this sheet is
                             // supposed to make visible.
                             resolved.alpha() == 0
                                 ? QStringLiteral("transparent")
                                 : resolved.alpha() == 255
                                       ? resolved.name().toUpper()
                                       : QStringLiteral("%1 · %2%")
                                             .arg(resolved.name().toUpper())
                                             .arg(qRound(resolved.alphaF() * 100)));
        }
    }

private:
    static constexpr int kCell = 210;
    static constexpr int kRowHeight = 38;
    static constexpr int kCaption = 22;
    static constexpr int kChip = 28;

    QString m_caption;
    QVector<TokenSheet::Entry> m_entries;
};

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);
    const ThemeProfile& profile = themeFor(ctx.parent);

    // ── what this page is ────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("One library, every product"),
                                 QStringLiteral("Change Theme above. Nothing on this page is "
                                                "rebuilt — the same widgets resolve different "
                                                "colours."));
        QVBoxLayout* body = card->body();

        auto* who = label(QStringLiteral("%1  ·  accent %2")
                              .arg(profile.displayName,
                                   profile.colors.accent.name().toUpper()),
                          card, TypeRole::Mono, Ink::Accent);
        body->addWidget(who);

        note(body, card,
             QStringLiteral("Structure stays the same. Semantic role stays the same. Only the "
                            "resolved appearance moves. There is no NovuAIButton and no "
                            "ExplorerBadge — a product is a ThemeProfile, not a set of "
                            "classes."));
        column->addWidget(card);
    }

    // ── the tokens ───────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Tokens"),
                                 QStringLiteral("What a component is allowed to ask for."));
        QVBoxLayout* body = card->body();

        body->addWidget(new TokenSheet(
            QStringLiteral("Surfaces — the family's, shared by every product"),
            {{QStringLiteral("windowBg"), token(ColorRole::WindowBg)},
             {QStringLiteral("panelBg"), token(ColorRole::PanelBg)},
             {QStringLiteral("panelBgAlt"), token(ColorRole::PanelBgAlt)},
             {QStringLiteral("headerBg"), token(ColorRole::HeaderBg)},
             {QStringLiteral("surfaceHover"), token(ColorRole::SurfaceHover)},
             {QStringLiteral("surfaceActive"), token(ColorRole::SurfaceActive)},
             {QStringLiteral("border"), token(ColorRole::Border)},
             {QStringLiteral("borderHover"), token(ColorRole::BorderHover)}},
            card));

        body->addSpacing(10);
        body->addWidget(new TokenSheet(
            QStringLiteral("Ink"),
            {{QStringLiteral("textPrimary"), token(ColorRole::TextPrimary)},
             {QStringLiteral("textSecondary"), token(ColorRole::TextSecondary)},
             {QStringLiteral("textDisabled"), token(ColorRole::TextDisabled)},
             {QStringLiteral("textOnAccent"), token(ColorRole::TextOnAccent)}},
            card));

        body->addSpacing(10);
        body->addWidget(new TokenSheet(
            QStringLiteral("Accent — the one thing a product owns"),
            {{QStringLiteral("accent"), token(ColorRole::Accent)},
             {QStringLiteral("accentLight"), token(ColorRole::AccentLight)},
             {QStringLiteral("accent @12%"), token(ColorRole::Accent, 0.12)},
             {QStringLiteral("accent @28%"), token(ColorRole::Accent, 0.28)}},
            card));

        body->addSpacing(10);
        body->addWidget(new TokenSheet(
            QStringLiteral("Status — shared, and never the accent"),
            {{QStringLiteral("status.success.text"),
              statusToken(SemanticRole::Success, ColorRole::StatusText)},
             {QStringLiteral("status.warning.text"),
              statusToken(SemanticRole::Warning, ColorRole::StatusText)},
             {QStringLiteral("status.danger.text"),
              statusToken(SemanticRole::Danger, ColorRole::StatusText)},
             {QStringLiteral("status.info.text"),
              statusToken(SemanticRole::Info, ColorRole::StatusText)},
             {QStringLiteral("status.success.fill"),
              statusToken(SemanticRole::Success, ColorRole::StatusFill)},
             {QStringLiteral("status.warning.fill"),
              statusToken(SemanticRole::Warning, ColorRole::StatusFill)},
             {QStringLiteral("status.danger.fill"),
              statusToken(SemanticRole::Danger, ColorRole::StatusFill)},
             {QStringLiteral("status.info.fill"),
              statusToken(SemanticRole::Info, ColorRole::StatusFill)}},
            card));

        note(body, card,
             QStringLiteral("Surfaces and status come from the shared design system and do "
                            "not move between products; the accent does. A status may never "
                            "resolve to the accent — that rule is why Info is a token of its "
                            "own rather than \"the accent, but blue-ish\"."));
        column->addWidget(card);
    }

    // ── typography ───────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Typography"),
                                 QStringLiteral("A page asks for a role; the profile decides "
                                                "the pixels and the face."));
        QVBoxLayout* body = card->body();

        const struct { const char* name; TypeRole role; } roles[] = {
            {"PageTitle", TypeRole::PageTitle},
            {"SectionTitle", TypeRole::SectionTitle},
            {"CardTitle", TypeRole::CardTitle},
            {"Body", TypeRole::Body},
            {"Caption", TypeRole::Caption},
            {"Label", TypeRole::Label},
            {"Mono", TypeRole::Mono},
            {"Eyebrow", TypeRole::Eyebrow},
        };
        QGridLayout* grid = matrix(body, card,
                                   {QStringLiteral("Sample"), QStringLiteral("Resolved")},
                                   [&] {
                                       QStringList names;
                                       for (const auto& r : roles) {
                                           names.append(QString::fromLatin1(r.name));
                                       }
                                       return names;
                                   }());
        for (int i = 0; i < static_cast<int>(std::size(roles)); ++i) {
            matrixCell(grid, i, 0,
                       label(QStringLiteral("Novusion — 0123456789"), card, roles[i].role));
            const TypeStyle style = profile.text(roles[i].role);
            matrixCell(grid, i, 1,
                       label(QStringLiteral("%1px / %2%3").arg(style.size).arg(style.weight)
                                 .arg(style.monospace ? QStringLiteral(" mono") : QString()),
                             card, TypeRole::Mono, Ink::Muted));
        }
        column->addWidget(card);
    }

    // ── the controls ─────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Controls"),
                                 QStringLiteral("Real components, every one of them."));
        QVBoxLayout* body = card->body();

        QHBoxLayout* buttons = exampleRow(body, card, QStringLiteral("Button"));
        buttons->addWidget(primaryButton(QStringLiteral("Save"), card));
        buttons->addWidget(secondaryButton(QStringLiteral("Cancel"), card));
        buttons->addWidget(ghostButton(QStringLiteral("Reset"), card));
        buttons->addWidget(successButton(QStringLiteral("Start"), card));
        buttons->addWidget(warningButton(QStringLiteral("Go live"), card));
        buttons->addWidget(dangerButton(QStringLiteral("Delete"), card));
        buttons->addStretch(1);

        QHBoxLayout* badges = exampleRow(body, card, QStringLiteral("Badge · StatusDot"));
        badges->addWidget(statusBadge(QStringLiteral("running"), Badge::Tone::Success, card));
        badges->addWidget(statusBadge(QStringLiteral("paused"), Badge::Tone::Warning, card));
        badges->addWidget(statusBadge(QStringLiteral("failed"), Badge::Tone::Danger, card));
        badges->addWidget(new Badge(QStringLiteral("selected"), Badge::Tone::Accent, card));
        badges->addWidget(pill(QStringLiteral("demo"), card));
        badges->addSpacing(12);
        badges->addWidget(new StatusDot(StatusDot::Tone::Success, true, card));
        badges->addWidget(new StatusDot(StatusDot::Tone::Warning, true, card));
        badges->addWidget(new StatusDot(StatusDot::Tone::Danger, card));
        badges->addStretch(1);

        QHBoxLayout* pickers = exampleRow(body, card,
                                          QStringLiteral("Segmented · Toggle · Slider"));
        pickers->addWidget(new Segmented({QStringLiteral("Basic"), QStringLiteral("Pro"),
                                          QStringLiteral("MAX")}, card, 1));
        auto* toggle = new Toggle(card);
        toggle->setChecked(true);
        pickers->addWidget(toggle);
        auto* slider = new Slider(0, 100, card);
        slider->setValue(62);
        slider->setFixedWidth(200);
        pickers->addWidget(slider);
        pickers->addStretch(1);

        QHBoxLayout* tabs = exampleRow(body, card, QStringLiteral("TabStrip · ProgressBar"));
        tabs->addWidget(new TabStrip({QStringLiteral("Overview"), QStringLiteral("Activity"),
                                      QStringLiteral("Settings")}, card, 0));
        tabs->addSpacing(12);
        auto* progress = new ProgressBar(64, card);
        progress->setFixedWidth(180);
        tabs->addWidget(progress);
        tabs->addStretch(1);

        column->addWidget(card);
    }

    // ── a form, and a surface ────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("A form"),
                                 QStringLiteral("Field, SettingRow, and what an error looks "
                                                "like under each profile."));
        QVBoxLayout* body = card->body();

        field(body, card, QStringLiteral("Workspace"),
              lineEdit(QStringLiteral("Trading desk"), card),
              QStringLiteral("Shown in the title bar and in the switcher."));
        body->addSpacing(12);
        auto* bad = field(body, card, QStringLiteral("Poll interval"),
                          lineEdit(QString(), card, 0, QStringLiteral("0")));
        bad->setRequired(true);
        bad->setError(QStringLiteral("Must be at least one second."));

        body->addSpacing(14);
        auto* row = new SettingRow(QStringLiteral("Live market data"),
                                   QStringLiteral("Streams prices while a panel is open."),
                                   card);
        auto* live = new Toggle(card);
        live->setChecked(true);
        row->setControl(live);
        row->setDivider(true);
        body->addWidget(row);

        auto* second = new SettingRow(QStringLiteral("Session token"),
                                      QStringLiteral("Rotated every thirty days."), card);
        second->setValue(QStringLiteral("nv_7f3c1e9ab84d2f60"));
        second->setMasked(true);
        body->addWidget(second);

        column->addWidget(card);
    }

    // ── navigation and overlays ──────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Navigation and overlays"),
                                 QStringLiteral("The selected row, and what opens over it."));
        QVBoxLayout* body = card->body();

        auto* split = new QHBoxLayout;
        split->setSpacing(profile.space(SpacingRole::CardGap));

        auto* nav = new NavList(card);
        nav->addSection(QStringLiteral("Workspace"));
        nav->addItem(QStringLiteral("overview"), QStringLiteral("Overview"), Glyph::Overview);
        nav->addItem(QStringLiteral("agents"), QStringLiteral("Agents"), Glyph::Bots);
        nav->addItem(QStringLiteral("models"), QStringLiteral("Models"), Glyph::Cpu);
        nav->setCount(QStringLiteral("agents"), 12);
        nav->setStatus(QStringLiteral("models"), StatusDot::Tone::Success);
        nav->setCurrentKey(QStringLiteral("agents"));
        nav->setFixedWidth(220);
        split->addWidget(nav, 0, Qt::AlignTop);

        auto* right = new QVBoxLayout;
        right->setSpacing(profile.space(SpacingRole::ToolbarGap));

        auto* triggers = new QHBoxLayout;
        triggers->setSpacing(profile.space(SpacingRole::ToolbarGap));

        auto* menuButton = secondaryButton(QStringLiteral("Open a menu"), card);
        menuButton->setGlyph(Glyph::More);
        QObject::connect(menuButton, &Button::clicked, card, [menuButton, card] {
            auto* menu = new Popover(card);
            menu->addGroup(QStringLiteral("This workspace"));
            menu->addItem(QStringLiteral("rename"), QStringLiteral("Rename…"), Glyph::Edit);
            menu->addItem(QStringLiteral("duplicate"), QStringLiteral("Duplicate"), Glyph::Copy);
            menu->addSeparator();
            menu->addItem(QStringLiteral("delete"), QStringLiteral("Delete"), Glyph::Trash);
            menu->popupUnder(menuButton);
        });
        triggers->addWidget(menuButton);

        auto* dialogButton = secondaryButton(QStringLiteral("Open a dialog"), card);
        dialogButton->setGlyph(Glyph::Warning);
        QObject::connect(dialogButton, &Button::clicked, card, [card] {
            Dialog::confirm(card, QObject::tr("Delete this workspace?"),
                            QObject::tr("Its chats, agents and local caches go with it. "
                                        "Anything already synchronised stays on the server."),
                            QObject::tr("Delete"), QObject::tr("Keep"), true);
        });
        triggers->addWidget(dialogButton);

        auto* tip = secondaryButton(QStringLiteral("Hover for a tooltip"), card);
        tip->setToolTip(QStringLiteral("Tooltips take the profile's surface and its "
                                       "hairline, like everything else."));
        triggers->addWidget(tip);
        triggers->addStretch(1);
        right->addLayout(triggers);

        right->addWidget(new InfoBanner(
            QStringLiteral("A banner states a tone, never a colour."),
            Badge::Tone::Info, card));
        right->addWidget(new InfoBanner(
            QStringLiteral("Which is why this one is the same red in every profile."),
            Badge::Tone::Danger, card));
        right->addStretch(1);

        split->addLayout(right, 1);
        body->addLayout(split);

        note(body, card,
             QStringLiteral("The selected navigation row, the menu's current item and a "
                            "Primary button all take the accent from the same token. Switch "
                            "profile and all three move together, because there is one "
                            "answer to \"what colour is this product\"."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec themeStylingComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("theme-styling");
    spec.name = QStringLiteral("Theme & Styling");
    spec.category = Category::Foundation;
    spec.description = QStringLiteral(
        "The live reference for the styling architecture. Real components and the tokens "
        "they resolve through — change the Theme selector and watch the same widgets move.");
    spec.headerPath = QStringLiteral("include/novu/ui/StyleSpec.h");
    spec.sourcePath = QStringLiteral("src/StyleSpec.cpp");
    spec.designedFor = {QStringLiteral("novuspace"), QStringLiteral("novuai"),
                        QStringLiteral("novuexplorer"), QStringLiteral("novuadmin"),
                        QStringLiteral("novuhub"), QStringLiteral("novuvoice"),
                        QStringLiteral("novutrade")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Every profile")};
    spec.states = {QStringLiteral("Normal"), QStringLiteral("Invalid"),
                   QStringLiteral("Selected")};
    spec.keywords = {QStringLiteral("theme"), QStringLiteral("token"), QStringLiteral("style"),
                     QStringLiteral("palette"), QStringLiteral("accent"),
                     QStringLiteral("profile"), QStringLiteral("sample")};
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
