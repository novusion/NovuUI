#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Card.h"
#include "novu/ui/Feedback.h"
#include "novu/ui/Icon.h"
#include "novu/ui/StatusDot.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/StyleSheet.h"

#include <QFontMetrics>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <QWidget>

#include <iterator>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

// The whole set, painted.
//
// Not sixty-seven widgets in a flow layout, and the difference is the point of
// the page as much as the glyphs are: an icon is decoration, and NovuUI paints
// decoration. This is one widget calling the library's own paintGlyph() in a
// loop — the same call an IconButton, a table cell or a menu item makes.
class GlyphSheet : public QWidget
{
public:
    explicit GlyphSheet(QWidget* parent) : QWidget(parent)
    {
        m_glyphs = allGlyphs();
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    bool hasHeightForWidth() const override { return true; }

    int heightForWidth(int width) const override
    {
        const int columns = qMax(1, width / kCellWidth);
        const int rows = (m_glyphs.size() + columns - 1) / columns;
        return rows * kCellHeight;
    }

    QSize sizeHint() const override
    {
        return QSize(kCellWidth * 6, heightForWidth(kCellWidth * 6));
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        const ThemeProfile& profile = themeFor(this);
        const int columns = qMax(1, width() / kCellWidth);
        const int glyphSize = profile.icons.large;

        QPainter painter(this);
        const QFont nameFont = roleFont(this, TypeRole::Eyebrow);
        painter.setFont(nameFont);

        for (int i = 0; i < m_glyphs.size(); ++i) {
            const int column = i % columns;
            const int row = i / columns;
            const QRect cell(column * width() / columns, row * kCellHeight,
                             width() / columns, kCellHeight);

            const QRectF box((cell.width() - glyphSize) / 2.0 + cell.left(),
                             cell.top() + 10.0, glyphSize, glyphSize);
            paintGlyph(&painter, m_glyphs.at(i), box, profile.colors.textSecondary);

            painter.setPen(profile.colors.textDisabled);
            const QRect caption(cell.left() + 2, cell.top() + 12 + glyphSize,
                                cell.width() - 4, kCellHeight - glyphSize - 14);
            const QString name = QFontMetrics(nameFont)
                                     .elidedText(glyphName(m_glyphs.at(i)), Qt::ElideRight,
                                                 caption.width());
            painter.drawText(caption, Qt::AlignHCenter | Qt::AlignTop, name);
        }
    }

private:
    static constexpr int kCellWidth = 92;
    static constexpr int kCellHeight = 58;

    QVector<Glyph> m_glyphs;
};

struct InkRow
{
    const char* name;
    Ink ink;
};

const InkRow kInks[] = {
    {"Default", Ink::Default}, {"Secondary", Ink::Secondary}, {"Muted", Ink::Muted},
    {"Accent", Ink::Accent},   {"Success", Ink::Success},     {"Warning", Ink::Warning},
    {"Danger", Ink::Danger},   {"Info", Ink::Info},
};

// A glyph as a pixmap in a QLabel — the other half of the public surface, and
// the one every Qt API that insists on a QIcon goes through.
QLabel* glyphLabel(QWidget* parent, Glyph glyph, int size, Ink ink)
{
    auto* lb = new QLabel(parent);
    lb->setPixmap(glyphPixmap(glyph, size, inkColor(parent, ink)));
    lb->setFixedSize(size + 8, size + 8);
    lb->setAlignment(Qt::AlignCenter);
    return lb;
}

// This page is the symbolic language as a whole, not just the glyph outlines
// — StatusDot and Spinner are their own components with their own detailed
// pages, and stay that way. What is shared here is a page-local caption
// helper, not a second copy of either.
struct ToneRow
{
    const char* name;
    StatusDot::Tone tone;
};

const ToneRow kTones[] = {
    {"Neutral", StatusDot::Tone::Neutral}, {"Success", StatusDot::Tone::Success},
    {"Warning", StatusDot::Tone::Warning}, {"Danger", StatusDot::Tone::Danger},
    {"Info", StatusDot::Tone::Info},       {"Accent", StatusDot::Tone::Accent},
};

QWidget* dotWithCaption(QWidget* parent, StatusDot::Tone tone, bool pulsing, const char* name)
{
    auto* cell = new QWidget(parent);
    auto* layout = new QVBoxLayout(cell);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    layout->setAlignment(Qt::AlignHCenter);
    auto* dot = new StatusDot(tone, pulsing, cell);
    dot->setDiameter(11);
    layout->addWidget(dot, 0, Qt::AlignHCenter);
    layout->addWidget(label(QString::fromLatin1(name), cell, TypeRole::Caption, Ink::Muted));
    return cell;
}

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);
    registerGlyphSet("gallery-document-icons",{{Glyph::Book,"book","<path d=\"M2 3h5v10H2zM9 3h5v10H9z\"/>"}},16,1.7);
    auto glyphProfile=novusionBaseProfile();glyphProfile.id="gallery-document-icons";glyphProfile.icons.family="gallery-document-icons";ThemeManager::instance().registerProfile(glyphProfile);
    auto* customBook=new QLabel(page);applyStyleSheet(customBook,glyphProfile.id);customBook->setPixmap(glyphPixmap(Glyph::Book,32,glyphProfile.colors.textPrimary,0,0,customBook));column->addWidget(customBook);
    const ThemeProfile& profile = themeFor(ctx.parent);

    // This page is the symbolic language as a whole: static glyphs, the
    // status dots and their breathing state, the loading marks, and the
    // sizes and inks that cut across all three. Every widget below is the
    // real production component — StatusDot and Spinner keep their own
    // detailed pages for their full API, and nothing here is a second
    // implementation of either.

    // ── glyphs ───────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Glyphs"),
                                 QStringLiteral("Every glyph, painted from the approved "
                                                "outlines."));
        QVBoxLayout* body = card->body();
        body->addWidget(new GlyphSheet(card));
        note(body, card,
             QStringLiteral("One widget, %1 glyphs. paintGlyph() is called in a loop, which is "
                            "what a table cell, a menu item and an IconButton all do — an icon "
                            "is decoration, and decoration never becomes a QWidget here.")
                 .arg(allGlyphs().size()));
        column->addWidget(card);
    }

    // ── status indicators ───────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Status indicators"),
                                 QStringLiteral("StatusDot's six tones, still — the smallest "
                                                "symbol in the language and one of the most "
                                                "used."));
        QVBoxLayout* body = card->body();
        QHBoxLayout* row = exampleRow(body, card, QString());
        for (const ToneRow& t : kTones) {
            row->addWidget(dotWithCaption(card, t.tone, false, t.name));
            row->addSpacing(profile.space(SpacingRole::ToolbarGap));
        }
        row->addStretch(1);
        note(body, card,
             QStringLiteral("A tone, not a colour — the same six meanings Badge and the status "
                            "palette carry, so a dot and a badge for the same status never "
                            "disagree. The full contract, including the cross-fade on a state "
                            "change, is on StatusDot's own page."));
        column->addWidget(card);
    }

    // ── animated indicators ─────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Animated indicators"),
                                 QStringLiteral("The same dots, pulsing — the design's "
                                                "softglow, on the shared clock."));
        QVBoxLayout* body = card->body();
        QHBoxLayout* row = exampleRow(body, card, QString());
        for (const ToneRow& t : kTones) {
            row->addWidget(dotWithCaption(card, t.tone, true, t.name));
            row->addSpacing(profile.space(SpacingRole::ToolbarGap));
        }
        row->addStretch(1);
        note(body, card,
             QStringLiteral("This is the glow a live engine or a connected service points at — "
                            "not a separate component, but StatusDot's own `pulsing` state: a "
                            "halo breathing on Pulse, the one animation clock every looping "
                            "primitive in NovuUI shares. Only the tones that mean something is "
                            "actually live are ever pulsed; a dot that breathes while nothing "
                            "is happening is a dot nobody reads any more."));
        column->addWidget(card);
    }

    // ── loading / process indicators ────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Loading / process indicators"),
                                 QStringLiteral("Spinner: \"something is happening and I "
                                                "cannot tell you how long.\""));
        QVBoxLayout* body = card->body();
        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Small · Medium · Large"));
        const struct { const char* name; ControlSize size; } spinnerSizes[] = {
            {"Small", ControlSize::Small}, {"Medium", ControlSize::Medium},
            {"Large", ControlSize::Large},
        };
        for (const auto& s : spinnerSizes) {
            row->addWidget(new Spinner(s.size, card));
            row->addWidget(label(QString::fromLatin1(s.name), card, TypeRole::Caption,
                                 Ink::Muted));
            row->addSpacing(profile.space(SpacingRole::ToolbarGap));
        }
        row->addStretch(1);
        note(body, card,
             QStringLiteral("Also on the shared clock, sized from the same IconMetrics the "
                            "glyphs above are. The running ring that travels round a Primary "
                            "button mid-action is the same motion again, drawn by "
                            "motion::paintRunningRing() directly onto the button's own outline "
                            "rather than through a widget — see the Motion page for that "
                            "primitive and the shimmer it shares a clock with."));
        column->addWidget(card);
    }

    // ── sizes & states ───────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Sizes & states"),
                                 QStringLiteral("Three sizes, and the profile owns all three."));
        QVBoxLayout* body = card->body();
        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Small · Medium · Large"));
        for (const int size : {profile.icons.small, profile.icons.medium, profile.icons.large}) {
            row->addWidget(glyphLabel(card, Glyph::Settings, size, Ink::Secondary));
            row->addWidget(label(QStringLiteral("%1px").arg(size), card, TypeRole::Mono,
                                 Ink::Muted));
            row->addSpacing(profile.space(SpacingRole::ToolbarGap));
        }
        row->addStretch(1);
        note(body, card,
             QStringLiteral("The stroke weight scales with the box, so the same outline at "
                            "13px and at 20px is one drawing rather than one line weight at "
                            "two sizes.\n\nglyphIcon() is the only state a bare glyph has: it "
                            "supplies a faded pixmap for QIcon::Disabled so a disabled control "
                            "reads as disabled instead of lightening toward Qt's own grey. "
                            "There is no hover or pressed state here, because a glyph is "
                            "decoration rather than a widget that tracks the pointer — those "
                            "belong to what is interactive around it, on IconButton's own "
                            "page."));
        column->addWidget(card);
    }

    // ── ink ──────────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Ink"),
                                 QStringLiteral("A glyph takes a colour; it does not carry "
                                                "one."));
        QVBoxLayout* body = card->body();

        QStringList columns;
        for (const InkRow& ink : kInks) {
            columns.append(QString::fromLatin1(ink.name));
        }
        QGridLayout* grid = matrix(body, card, columns,
                                   {QStringLiteral("Status"), QStringLiteral("Action")});

        const Glyph rows[] = {Glyph::Info, Glyph::Refresh};
        for (int r = 0; r < 2; ++r) {
            int c = 0;
            for (const InkRow& ink : kInks) {
                matrixCell(grid, r, c++,
                           glyphLabel(card, rows[r], profile.icons.medium, ink.ink));
            }
        }
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec iconComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("icon");
    spec.name = QStringLiteral("Icon");
    spec.category = Category::Foundation;
    spec.description = QStringLiteral(
        "The symbolic language as a whole, not just the glyph outlines: static glyphs, "
        "StatusDot's still and breathing tones, and Spinner's loading marks, at the sizes "
        "and inks that cut across all three. Every widget shown is the real production "
        "component — StatusDot and Spinner keep their own detailed pages for their full API.");
    spec.headerPath = QStringLiteral("include/novu/ui/Icon.h");
    spec.sourcePath = QStringLiteral("src/Icon.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Small"), QStringLiteral("Medium"), QStringLiteral("Large")};
    spec.states = {QStringLiteral("Normal"), QStringLiteral("Disabled")};
    spec.keywords = {QStringLiteral("glyph"), QStringLiteral("svg"), QStringLiteral("stroke"),
                     QStringLiteral("symbol"), QStringLiteral("pictogram"),
                     QStringLiteral("status"), QStringLiteral("indicator"),
                     QStringLiteral("spinner"), QStringLiteral("loading"),
                     QStringLiteral("pulse"), QStringLiteral("glow"),
                     QStringLiteral("animated")};
    spec.properties = {
        {QStringLiteral("glyph"), QStringLiteral("enum"), {}, QStringLiteral("None"),
         QStringLiteral("Which outline. Named, so it follows the theme.")},
        {QStringLiteral("size"), QStringLiteral("int"), {}, QString(),
         QStringLiteral("From IconMetrics — a component never invents one.")},
        {QStringLiteral("color"), QStringLiteral("QColor"), {}, QString(),
         QStringLiteral("Resolved from an Ink role by the caller.")},
    };
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
