#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Card.h"
#include "novu/ui/Icon.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QFontMetrics>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <QWidget>

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

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);
    const ThemeProfile& profile = themeFor(ctx.parent);

    // ── the set ──────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("The set"),
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

    // ── sizes ────────────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Sizes"),
                                 QStringLiteral("Three, and the profile owns all three."));
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
                            "two sizes."));
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
        "The Novu icon set: stroke outlines taken from the approved design, painted rather "
        "than instantiated. No icon font, no QtSvg, no widget per glyph.");
    spec.headerPath = QStringLiteral("include/novu/ui/Icon.h");
    spec.sourcePath = QStringLiteral("src/Icon.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Small"), QStringLiteral("Medium"), QStringLiteral("Large")};
    spec.states = {QStringLiteral("Normal"), QStringLiteral("Disabled")};
    spec.keywords = {QStringLiteral("glyph"), QStringLiteral("svg"), QStringLiteral("stroke"),
                     QStringLiteral("symbol"), QStringLiteral("pictogram")};
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
