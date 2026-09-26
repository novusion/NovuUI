#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Badge.h"
#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/DiffView.h"
#include "novu/ui/Form.h"
#include "novu/ui/Separator.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/Toggle.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

const char* const kSample = R"(diff --git a/src/Motion.cpp b/src/Motion.cpp
index 3f1a0c2..9b4e7d1 100644
--- a/src/Motion.cpp
+++ b/src/Motion.cpp
@@ -12,9 +12,7 @@ namespace motion {
 double phase(int periodMs)
 {
     if (periodMs <= 0) {
-        return 0.0;
-    }
-    const qint64 now = QDateTime::currentMSecsSinceEpoch();
+        return 0.0;    // a period of zero is "not animated", not a divide by zero
+    }
+    const qint64 now = elapsed().elapsed();
     return double(now % periodMs) / double(periodMs);
 }
@@ -48,6 +46,18 @@ double breathe(int periodMs, double low, double high)
     return low + (high - low) * eased;
 }

+void paintSpinner(QPainter* painter, const QRectF& box, const QColor& track,
+                  const QColor& head, double strokeWidth, int periodMs, bool animated)
+{
+    if (!painter || box.isEmpty()) {
+        return;
+    }
+    const double inset = strokeWidth / 2.0;
+    const QRectF ring = box.adjusted(inset, inset, -inset, -inset);
+    painter->setPen(QPen(track, strokeWidth));
+    painter->drawEllipse(ring);
+}
+
 bool enabled(const QWidget* context)
 {
     return themeFor(context).animation.normalMs > 0;
 })";

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    if (ctx.scenario == Scenario::Empty) {
        column->addWidget(scenarioEmptyState(page, QStringLiteral("changes")));
        column->addStretch(1);
        return page;
    }

    {
        Card* card = exampleCard(page, QStringLiteral("A unified diff"),
                                 QStringLiteral("Parsed from the text every tool "
                                                "emits."));
        QVBoxLayout* body = card->body();

        auto* diff = new DiffView(card);
        diff->setUnifiedDiff(QString::fromLatin1(kSample));
        diff->setMinimumHeight(320);

        QHBoxLayout* head = exampleRow(body, card, QStringLiteral("src/Motion.cpp"));
        head->addWidget(new Badge(QStringLiteral("+%1").arg(diff->addedCount()),
                                  Badge::Tone::Success, card));
        head->addWidget(new Badge(QStringLiteral("−%1").arg(diff->removedCount()),
                                  Badge::Tone::Danger, card));
        head->addSpacing(16);

        auto* numbers = new Toggle(card);
        numbers->setChecked(true);
        QObject::connect(numbers, &QAbstractButton::toggled, diff,
                         &DiffView::setShowLineNumbers);
        head->addWidget(numbers);
        head->addWidget(label(QStringLiteral("line numbers"), card, TypeRole::Caption,
                              Ink::Muted));
        head->addStretch(1);

        body->addWidget(diff);

        note(body, card,
             QStringLiteral("The counts come from the view rather than from a second walk of "
                            "the text at the call site — which is where a header saying "
                            "\"+128 −34\" usually gets its numbers, and where they usually "
                            "drift from what is drawn below them."));
        note(body, card,
             QStringLiteral("Line numbers are the diff's own, on both sides, taken from the "
                            "@@ header. A viewer that renumbers what it is shown is a viewer "
                            "whose numbers cannot be quoted in a review comment — which is "
                            "most of what the column is for."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("What the parser has to survive"),
                                 QStringLiteral("The headers, and the lines that "
                                                "look like body but are not."));
        QVBoxLayout* body = card->body();

        const QVector<DiffView::Line> lines = DiffView::parseUnified(
            QString::fromLatin1(kSample));

        const struct { DiffView::Kind kind; const char* name; } kinds[] = {
            {DiffView::Kind::File, "File"},
            {DiffView::Kind::Hunk, "Hunk"},
            {DiffView::Kind::Added, "Added"},
            {DiffView::Kind::Removed, "Removed"},
            {DiffView::Kind::Context, "Context"},
        };
        for (const auto& entry : kinds) {
            int count = 0;
            for (const DiffView::Line& line : lines) {
                if (line.kind == entry.kind) {
                    ++count;
                }
            }
            propertyRow(body, card, QString::fromLatin1(entry.name),
                        QString::number(count), Ink::Secondary, true);
        }

        note(body, card,
             QStringLiteral("`diff --git`, `index`, `---` and `+++` are headers, not a removed "
                            "line and an added one — classifying them by their leading "
                            "character is the mistake every first attempt at this makes, and "
                            "it paints the file header red and green. A `\\ No newline at end "
                            "of file` is context, and a line the parser cannot classify is "
                            "context too: showing it unstyled beats dropping it."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("Shared with the log"),
                                 QStringLiteral("One virtualisation, two "
                                                "components."));
        QVBoxLayout* body = card->body();

        note(body, card,
             QStringLiteral("DiffView and LogView are both MonoLineView: the row height, the "
                            "scroll range and the decision about which rows are on screen are "
                            "one implementation. What each adds is what it actually is — a log "
                            "has levels, a cap and a tail to follow; a diff has hunks, two "
                            "line numbers and tinted rows."));
        note(body, card,
             QStringLiteral("They hold no line struct in common, and that is deliberate. A "
                            "shared one would carry a level, a timestamp, a hunk header and "
                            "two line numbers, of which either subclass uses half — a base "
                            "class shaped by the union of its users rather than by what they "
                            "have in common."));
        column->addWidget(card);
    }

    // ── quoted inline ────────────────────────────────────────────────────
    {
        Card* card = exampleCard(page, QStringLiteral("Quoted inline"),
                                 QStringLiteral("A patch inside a column, as tall "
                                                "as the patch."));
        QVBoxLayout* body = card->body();

        auto* quoted = new DiffView(card);
        quoted->setUnifiedDiff(QString::fromLatin1(kSample));
        quoted->setShowLineNumbers(false);
        quoted->setFitToContent(true, 8);
        body->addWidget(quoted);

        body->addWidget(label(QStringLiteral("%1 rows · row height %2 · content %3 · "
                                             "hint %4, capped at 8 rows")
                                  .arg(quoted->rowCount())
                                  .arg(quoted->rowHeight())
                                  .arg(quoted->contentHeight())
                                  .arg(quoted->sizeHint().height()),
                              card, TypeRole::Mono, Ink::Muted));

        note(body, card,
             QStringLiteral("A scroll area has no height of its own, which is right for a "
                            "log filling a panel and wrong for a patch quoted in a chat "
                            "turn or a manifest under a file row. setFitToContent() makes "
                            "the view report what its rows need, and the cap is the point "
                            "past which a quotation stops being one and starts scrolling."));
        note(body, card,
             QStringLiteral("The row height is the font size times the design's "
                            "line-height, 1.65 here and 1.75 for a log. Multiplying the "
                            "font's metric height instead — ascent plus descent plus "
                            "leading — adds about a third to every row, in every "
                            "application at once."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec diffViewComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("diff-view");
    spec.name = QStringLiteral("DiffView");
    spec.category = Category::DataDisplay;
    spec.description = QStringLiteral(
        "A unified diff: tinted added and removed rows, hunk headers in the accent, both "
        "sides' line numbers in a gutter. Parses the text format, renders nothing else.");
    spec.headerPath = QStringLiteral("include/novu/ui/DiffView.h");
    spec.sourcePath = QStringLiteral("src/DiffView.cpp");
    spec.designedFor = {QStringLiteral("novuai")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Fit to content"), QStringLiteral("Context"), QStringLiteral("Added"),
                     QStringLiteral("Removed"), QStringLiteral("Hunk"),
                     QStringLiteral("File")};
    spec.states = {QStringLiteral("With line numbers"), QStringLiteral("Without")};
    spec.scenarios = {Scenario::Normal, Scenario::Empty};
    spec.keywords = {QStringLiteral("diff"), QStringLiteral("patch"),
                     QStringLiteral("git"), QStringLiteral("review"),
                     QStringLiteral("scm"), QStringLiteral("changes"),
                     QStringLiteral("unified")};
    spec.properties = {
        {QStringLiteral("showLineNumbers"), QStringLiteral("bool"), {},
         QStringLiteral("true"),
         QStringLiteral("Off for a narrow panel, where the gutter costs more than it "
                        "gives.")},
    };
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
