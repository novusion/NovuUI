#include "novu/ui/Novusion.h"

#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QBoxLayout>
#include <QFont>
#include <QLabel>
#include <QSizePolicy>
#include <QWidget>

namespace novu::ui::novusion {
namespace {

// The small uppercase mono treatment the reference uses for keys, counts and
// section captions. Three places wanted it and three places wrote it out.
void applyMicroMono(QLabel* label, const QWidget* context, bool uppercase,
                    int letterSpacing)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font(profile.type.monoFamily);
    font.setPixelSize(profile.type.xxs);
    if (uppercase) {
        font.setCapitalization(QFont::AllUppercase);
    }
    if (letterSpacing > 0) {
        font.setLetterSpacing(QFont::PercentageSpacing, letterSpacing);
    }
    label->setFont(font);
    setInk(label, Ink::Muted);
}

// The bare host every line in this file is built on: a widget with a
// horizontal layout and no margins, because the card's own gap is the only
// spacing between rows.
QWidget* lineHost(QVBoxLayout* into, int spacing, QHBoxLayout** lineOut)
{
    auto* row = new QWidget(into->parentWidget());
    auto* line = new QHBoxLayout(row);
    line->setContentsMargins(0, 0, 0, 0);
    line->setSpacing(spacing);
    *lineOut = line;
    return row;
}

} // namespace

// -----------------------------------------------------------------------------
// surfaces
// -----------------------------------------------------------------------------

Card* railCard(QWidget* parent, const QString& title, bool rows)
{
    auto* card = title.isEmpty() ? new Card(parent) : new Card(title, parent);
    card->setHeaderDivider(false);
    card->setHeaderMetrics({kRailPad, kRailPadV, kRailPad, kRailTitleGap}, 24);
    card->setBodyMetrics({kRailPad, 0, kRailPad, kRailPadV}, rows ? 0 : kRailBlockGap);
    if (title.isEmpty()) {
        card->setHeaderVisible(false);
    }
    return card;
}

Card* workspaceCard(QWidget* parent, const QString& title)
{
    auto* card = title.isEmpty() ? new Card(parent) : new Card(title, parent);
    card->setSurface(Card::Surface::Window);   // `.ag-tab .card{background:var(--window)}`
    card->setHeaderDivider(false);
    card->setHeaderMetrics({15, 13, 15, 11}, 26);
    card->setBodyMetrics({15, 0, 15, 14}, 10);   // `.card.pad{padding:14px 15px}`
    if (title.isEmpty()) {
        card->setHeaderVisible(false);
    }
    return card;
}

Card* nestedBox(QWidget* parent, const QMargins& padding, int spacing)
{
    auto* box = new Card(parent);
    box->setHeaderVisible(false);
    box->setSurface(Card::Surface::Raised);     // `background:var(--panel)`
    box->setRadiusStep(RadiusStep::Medium);     // `border-radius:var(--r-md)`
    box->setBodyMetrics(padding, spacing);
    return box;
}

Card* railRow(QWidget* parent, QWidget* mark, const QString& text,
              const QString& trailing, bool monoText)
{
    auto* row = new Card(parent);
    row->setHeaderVisible(false);
    row->setBorderVisible(false);
    row->setRadiusStep(RadiusStep::Small);
    row->setHoverSurface(Card::Surface::Raised);   // `.erow:hover{background:var(--panel)}`
    row->setInteractive(true);
    row->setBodyMetrics({kRailRowPad, 0, kRailRowPad, 0}, 0);
    row->setFixedHeight(kRailRowHeight);

    auto* line = new QHBoxLayout;
    line->setContentsMargins(0, 0, 0, 0);
    line->setSpacing(kRailRowGap);
    if (mark) {
        mark->setParent(row);
        line->addWidget(mark, 0, Qt::AlignVCenter);
    }

    // Ignored horizontally is what keeps a long name from reporting a width the
    // rail would have to grow to; the tooltip is what makes the elision
    // recoverable.
    auto* name = elidedLabel(text, row, monoText ? TypeRole::Mono : TypeRole::Body,
                             monoText ? Ink::Muted : Ink::Tertiary);
    name->setMinimumWidth(0);
    name->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    name->setToolTip(text);
    line->addWidget(name, 1);

    if (!trailing.isEmpty()) {
        auto* note = label(trailing, row, TypeRole::Mono, Ink::Muted);
        applyMicroMono(note, row, true, 104);   // `.es{font-size:9.5px}`
        line->addWidget(note, 0, Qt::AlignVCenter);
    }

    row->body()->addLayout(line);
    return row;
}

// -----------------------------------------------------------------------------
// lines inside a card
// -----------------------------------------------------------------------------

void propertyRow(QVBoxLayout* into, const QString& caption, const QString& value)
{
    QHBoxLayout* line = nullptr;
    QWidget* row = lineHost(into, 10, &line);

    line->addWidget(label(caption, row, TypeRole::Caption, Ink::Muted), 0, Qt::AlignTop);

    // The value takes the slack and wraps into it. Ignored horizontally is what
    // keeps a long value from reporting a width the rail would have to grow to;
    // heightForWidth is what makes the row taller instead.
    auto* val = paragraph(value, row, TypeRole::Body, Ink::Secondary);
    val->setTextFormat(Qt::PlainText);
    val->setMinimumWidth(1);
    QSizePolicy valuePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
    valuePolicy.setHeightForWidth(true);
    val->setSizePolicy(valuePolicy);
    val->setAlignment(Qt::AlignRight | Qt::AlignTop);
    line->addWidget(val, 1);

    into->addWidget(row);
}

void infoRow(QVBoxLayout* into, const QString& caption, const QString& value, bool monoValue)
{
    QHBoxLayout* line = nullptr;
    QWidget* row = lineHost(into, 10, &line);

    auto* key = label(caption, row, TypeRole::Mono, Ink::Muted);
    applyMicroMono(key, row, true, 104);
    key->setFixedWidth(78);   // `grid-template-columns:78px ...`
    line->addWidget(key, 0, Qt::AlignVCenter);

    // `.iv` elides; it does not wrap and it does not widen the rail.
    auto* val = elidedLabel(value, row, monoValue ? TypeRole::Mono : TypeRole::Body,
                            Ink::Secondary);
    val->setMinimumWidth(0);
    val->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    val->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    val->setToolTip(value);
    line->addWidget(val, 1);

    into->addWidget(row);
}

QLabel* sideSection(QVBoxLayout* into, const QString& caption)
{
    auto* heading = new QLabel(caption.toUpper());
    applyMicroMono(heading, into->parentWidget(), false, 109);
    heading->setContentsMargins(6, 6, 6, 5);
    into->addWidget(heading);
    return heading;
}

// -----------------------------------------------------------------------------
// small marks
// -----------------------------------------------------------------------------

QLabel* toolbarCount(const QString& text, QWidget* parent)
{
    auto* count = new QLabel(text, parent);
    applyMicroMono(count, parent, false, 0);
    return count;
}

StatusDot* denseDot(StatusDot::Tone tone, QWidget* parent)
{
    auto* dot = new StatusDot(tone, parent);
    dot->setFixedWidth(9);
    return dot;
}

} // namespace novu::ui::novusion
