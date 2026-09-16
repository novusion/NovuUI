#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Card.h"
#include "novu/ui/DropTarget.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

// The two highlights, painted through the real functions a drop handler
// calls - paintDropRow() and paintDropRegion() - not a copy of what they draw.
// A caption inside each panel shows which token backs it, since a colour
// swatch alone cannot show "inset" or "the label bolds".
class DropTargetBoard : public QWidget
{
public:
    explicit DropTargetBoard(QWidget* parent)
        : QWidget(parent)
    {
        setMinimumHeight(120);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const int gap = 16;
        const int rowH = 30;
        const int regionH = 66;
        const int w = width();

        const QRect rowRect(0, 0, w, rowH);
        paintDropRow(&painter, rowRect, this);
        painter.setPen(dropLabelInk().resolve(themeFor(this)));
        QFont f = roleFont(this, TypeRole::Body);
        f.setWeight(static_cast<QFont::Weight>(dropLabelWeight()));
        painter.setFont(f);
        painter.drawText(rowRect.adjusted(10, 0, -10, 0), Qt::AlignVCenter | Qt::AlignLeft,
                         QStringLiteral("workspace-tree-item"));

        const QRect regionRect(0, rowH + gap, w, regionH);
        paintDropRegion(&painter, regionRect, this);
        painter.setPen(themeFor(this).colors.textSecondary);
        painter.setFont(roleFont(this, TypeRole::Body));
        painter.drawText(regionRect, Qt::AlignCenter, QStringLiteral("drop anywhere to create"));
    }
};

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    {
        Card* card = exampleCard(
            page, QStringLiteral("Row vs. region"),
            QStringLiteral("Two outcomes, so two different highlights, and never both at "
                           "once — a row lights when the drag will bind to that exact "
                           "item, a region lights when it will make a new one."));
        QVBoxLayout* body = card->body();
        body->addWidget(new DropTargetBoard(card));
        note(body, card,
             QStringLiteral("Row: dropRowSpec() — accent@20 fill, solid accent border, bold "
                            "accent label. Region: dropRegionSpec() — accent@7 fill, the "
                            "heavier of the two borders. The row is emphatic because it "
                            "names one destination; the region is faint because it means "
                            "\"somewhere in here\"."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("The state machine"),
                                 QStringLiteral("DropHighlight owns the invariant: setting a "
                                                "row clears the region and vice versa, and "
                                                "every mutator reports whether the picture "
                                                "changed."));
        QVBoxLayout* body = card->body();
        note(body, card,
             QStringLiteral("void dragMoveEvent(QDragMoveEvent* e) {\n"
                            "    if (m_drop.setRow(indexAt(e->position().toPoint()))) update();\n"
                            "}\n"
                            "void dragLeaveEvent(QDragLeaveEvent*) { if (m_drop.clear()) update(); }"));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec dropTargetComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("drop-target");
    spec.name = QStringLiteral("Drop target");
    spec.category = Category::Foundation;
    spec.description = QStringLiteral(
        "The drag-and-drop highlight contract every tree and list shares: a row target for "
        "one exact destination, a region target for \"anywhere in here\", and the state "
        "machine that keeps the two from lighting at once.");
    spec.headerPath = QStringLiteral("include/novu/ui/DropTarget.h");
    spec.sourcePath = QStringLiteral("src/DropTarget.cpp");
    spec.designedFor = {QStringLiteral("novuai"), QStringLiteral("novuexplorer")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Row"), QStringLiteral("Region")};
    spec.states = {QStringLiteral("None"), QStringLiteral("Row"), QStringLiteral("Region")};
    spec.keywords = {QStringLiteral("drag"), QStringLiteral("drop"), QStringLiteral("dnd"),
                     QStringLiteral("highlight"), QStringLiteral("tree"),
                     QStringLiteral("workspace")};
    spec.properties = {
        {QStringLiteral("zone"), QStringLiteral("DropZone"), {}, QStringLiteral("None"),
         QStringLiteral("None, Row or Region. Never more than one at once.")},
        {QStringLiteral("row"), QStringLiteral("int"), {}, QStringLiteral("-1"),
         QStringLiteral("The lit row, or -1 whenever the zone is not Row.")},
    };
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
