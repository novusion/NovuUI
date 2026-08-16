#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Feedback.h"
#include "novu/ui/FlowLayout.h"
#include "novu/ui/Layout.h"
#include "novu/ui/ThemeManager.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground {

namespace {
// Segoe Fluent / MDL2 code points, written as numbers rather than pasted
// glyphs: a literal private-use character does not survive every editor and
// encoding this repository has been through.
constexpr char16_t kGlyphEmpty = 0xE7C3;   // Page
constexpr char16_t kGlyphError = 0xE783;   // Error
} // namespace

QVBoxLayout* previewPage(QWidget* page)
{
    return pageLayout(page);
}

Card* exampleCard(QWidget* parent, const QString& title, const QString& subtitle)
{
    auto* card = new Card(title, parent);
    if (!subtitle.isEmpty()) {
        card->setSubtitle(subtitle);
    }
    return card;
}

QHBoxLayout* exampleRow(QVBoxLayout* into, QWidget* parent, const QString& text)
{
    if (!text.isEmpty()) {
        into->addWidget(caption(text, parent));
    }
    auto* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(themeFor(parent).space(SpacingRole::ToolbarGap));
    into->addLayout(row);
    return row;
}

QLayout* wrappingRow(QVBoxLayout* into, QWidget* parent, const QString& text)
{
    if (!text.isEmpty()) {
        into->addWidget(caption(text, parent));
    }
    // FlowLayout documents why it needs a host widget; this is that host and
    // nothing else — no background, no border, no size of its own.
    auto* host = new QWidget(parent);
    host->setAutoFillBackground(false);
    auto* flow = new FlowLayout(host, themeFor(parent).space(SpacingRole::ToolbarGap));
    flow->setContentsMargins(0, 0, 0, 0);
    into->addWidget(host);
    return flow;
}

QGridLayout* matrix(QVBoxLayout* into, QWidget* parent, const QStringList& columns,
                    const QStringList& rows)
{
    const ThemeProfile& profile = themeFor(parent);

    auto* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(profile.space(SpacingRole::ToolbarGap));
    grid->setVerticalSpacing(profile.space(SpacingRole::FormRowGap));

    for (int c = 0; c < columns.size(); ++c) {
        grid->addWidget(caption(columns.at(c), parent), 0, c + 1);
    }
    for (int r = 0; r < rows.size(); ++r) {
        grid->addWidget(caption(rows.at(r), parent), r + 1, 0);
    }
    // The trailing stretch keeps a three-cell row left-aligned instead of
    // spreading it across the card, so two matrices under each other line up.
    grid->setColumnStretch(columns.size() + 1, 1);
    into->addLayout(grid);
    return grid;
}

void matrixCell(QGridLayout* grid, int row, int column, QWidget* widget)
{
    if (!grid || !widget) {
        return;
    }
    grid->addWidget(widget, row + 1, column + 1, Qt::AlignLeft | Qt::AlignVCenter);
}

QLabel* note(QVBoxLayout* into, QWidget* parent, const QString& text)
{
    QLabel* lb = label(text, parent, TypeRole::Caption, Ink::Muted);
    lb->setWordWrap(true);
    into->addWidget(lb);
    return lb;
}

QWidget* scenarioEmptyState(QWidget* parent, const QString& what)
{
    return new EmptyState(QString(QChar(kGlyphEmpty)),
                          QStringLiteral("No %1").arg(what),
                          QStringLiteral("This scenario supplies no rows. The component "
                                         "has to say so rather than render nothing."),
                          parent);
}

QWidget* scenarioErrorState(QWidget* parent, const QString& what)
{
    return new EmptyState(QString(QChar(kGlyphError)),
                          QStringLiteral("%1 could not be loaded").arg(what),
                          QStringLiteral("Simulated failure. What matters is that the "
                                         "panel keeps its shape and says why."),
                          parent);
}

} // namespace novu::playground
