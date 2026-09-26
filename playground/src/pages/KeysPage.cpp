#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Card.h"
#include "novu/ui/Form.h"
#include "novu/ui/Keys.h"
#include "novu/ui/Separator.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

void fillReference(ShortcutList* list)
{
    list->addGroup(QStringLiteral("Navigation"));
    list->addShortcut(QStringLiteral("Command palette"), QStringLiteral("Ctrl+K"),QStringLiteral("Find actions in the current workspace"));
    list->addShortcut(QStringLiteral("Go to workspace"), QStringLiteral("Ctrl+P"));
    list->addShortcut(QStringLiteral("Next module"), QStringLiteral("Ctrl+Tab"));
    list->addShortcut(QStringLiteral("Toggle sidebar"), QStringLiteral("Ctrl+B"));

    list->addGroup(QStringLiteral("Editing"));
    list->addShortcut(QStringLiteral("Save"), QStringLiteral("Ctrl+S"));
    list->addShortcut(QStringLiteral("Save all"), QStringLiteral("Ctrl+Shift+S"));
    list->addShortcut(QStringLiteral("Format document"), QStringLiteral("Ctrl+K, Ctrl+F"));
    list->addShortcut(QStringLiteral("Zoom in"), QStringLiteral("Ctrl++"));

    list->addGroup(QStringLiteral("View"));
    list->addShortcut(QStringLiteral("Tooltip help"), QStringLiteral("F1"));
    list->addShortcut(QStringLiteral("Full screen"), QStringLiteral("F11"));
    list->addShortcut(QStringLiteral("Developer console"), QStringLiteral("Ctrl+Shift+I"));
}

// ── KeyCap ───────────────────────────────────────────────────────────────

QWidget* buildKeyCap(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    {
        Card* card = exampleCard(page, QStringLiteral("Chords"),
                                 QStringLiteral("A chord is several caps, not one "
                                                "wide chip."));
        QVBoxLayout* body = card->body();

        const char* chords[] = {"Ctrl+K", "Ctrl+Shift+R", "F11", "Alt+Left",
                                "Ctrl++", "Ctrl+K, Ctrl+F", "+"};
        QStringList rows;
        for (const char* chord : chords) {
            rows.append(QString::fromLatin1(chord));
        }
        QGridLayout* grid = matrix(body, card,
                                   {QStringLiteral("Drawn"), QStringLiteral("Split into")},
                                   rows);
        for (int i = 0; i < rows.size(); ++i) {
            matrixCell(grid, i, 0, new KeyCap(rows.at(i), card));
            matrixCell(grid, i, 1,
                       label(KeyCap::split(rows.at(i)).join(QStringLiteral(" · ")), card,
                             TypeRole::Mono, Ink::Muted));
        }

        note(body, card,
             QStringLiteral("Splitting happens in the component so no caller has to know that "
                            "a chord uses + and a sequence uses a comma. The last two are the "
                            "cases a naive split gets wrong: Ctrl++ ends in a separator that "
                            "is a key, and a lone + is a key on its own."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("In place"),
                                 QStringLiteral("The same chip the tooltip and the "
                                                "reference draw."));
        QVBoxLayout* body = card->body();

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Beside a sentence"));
        row->addWidget(label(QStringLiteral("Press"), card, TypeRole::Body, Ink::Secondary));
        row->addWidget(new KeyCap(QStringLiteral("Ctrl+Shift+P"), card));
        row->addWidget(label(QStringLiteral("to open the palette."), card, TypeRole::Body,
                             Ink::Secondary));
        row->addStretch(1);

        note(body, card,
             QStringLiteral("paintKeyCaps() is public and shared: KeyCap, ShortcutList and the "
                            "tooltip's shortcut chip are one drawing. Three call sites drawing "
                            "their own rounded rectangle is three chances for the radius to "
                            "drift, and the tooltip's used to be a different shape."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

// ── ShortcutList ─────────────────────────────────────────────────────────

QWidget* buildShortcutList(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    if (ctx.scenario == Scenario::Empty) {
        column->addWidget(scenarioEmptyState(page, QStringLiteral("shortcuts")));
        column->addStretch(1);
        return page;
    }

    {
        Card* card = exampleCard(page, QStringLiteral("The reference"),
                                 QStringLiteral("Eleven rows, three groups, one "
                                                "widget."));
        QVBoxLayout* body = card->body();

        auto* list = new ShortcutList(card);
        fillReference(list);

        auto* filter = searchBar(card, QObject::tr("Filter shortcuts…"), 220);
        QObject::connect(filter, &QLineEdit::textChanged, list, &ShortcutList::setFilter);

        QHBoxLayout* row = exampleRow(body, card, QStringLiteral("Filter"));
        row->addWidget(filter);
        auto* count = label(QString(), card, TypeRole::Mono, Ink::Muted);
        const auto refresh = [count, list] {
            count->setText(QStringLiteral("%1 rows, %2 groups")
                               .arg(list->rowCount())
                               .arg(list->groupCount()));
        };
        refresh();
        row->addWidget(count);
        row->addStretch(1);

        body->addWidget(list);

        note(body, card,
             QStringLiteral("Type \"ctrl+s\" — the filter matches the keys as well as the "
                            "label, and a caption whose rows have all gone is dropped rather "
                            "than left standing over nothing. Type \"zzz\" for the same reason "
                            "in reverse: the sheet empties rather than showing three headings "
                            "with nothing under them."));
        note(body, card,
             QStringLiteral("Forty rows here would be four hundred widgets built as labels and "
                            "chips, all of them inert, in a window that is read once and "
                            "closed. This is one widget: the rows are data and the sheet is a "
                            "paint."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec keyCapComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("key-cap");
    spec.name = QStringLiteral("KeyCap");
    spec.category = Category::DataDisplay;
    spec.description = QStringLiteral(
        "A key drawn as a key. Chords split into separate caps, sequences keep their "
        "comma, and the drawing is shared with the tooltip's shortcut chip.");
    spec.headerPath = QStringLiteral("include/novu/ui/Keys.h");
    spec.sourcePath = QStringLiteral("src/Keys.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Single"), QStringLiteral("Chord"),
                     QStringLiteral("Sequence")};
    spec.keywords = {QStringLiteral("kb"), QStringLiteral("keyboard"),
                     QStringLiteral("shortcut"), QStringLiteral("key"),
                     QStringLiteral("chord"), QStringLiteral("accelerator")};
    spec.properties = {
        {QStringLiteral("keys"), QStringLiteral("QString"), {}, QStringLiteral(""),
         QStringLiteral("\"Ctrl+Shift+R\", or a QKeySequence in the platform's spelling.")},
    };
    spec.build = buildKeyCap;
    return spec;
}

ComponentSpec shortcutListComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("shortcut-list");
    spec.name = QStringLiteral("ShortcutList");
    spec.category = Category::DataDisplay;
    spec.description = QStringLiteral(
        "A keyboard reference as groups of labelled shortcuts, with a filter. The whole "
        "sheet is one widget.");
    spec.headerPath = QStringLiteral("include/novu/ui/Keys.h");
    spec.sourcePath = QStringLiteral("src/Keys.cpp");
    spec.designedFor = {QStringLiteral("novuspace")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Group"), QStringLiteral("Row")};
    spec.states = {QStringLiteral("Unfiltered"), QStringLiteral("Filtered"),
                   QStringLiteral("Empty")};
    spec.scenarios = {Scenario::Normal, Scenario::Empty};
    spec.keywords = {QStringLiteral("kb"), QStringLiteral("keyboard shortcuts"),
                     QStringLiteral("reference"), QStringLiteral("cheatsheet"),
                     QStringLiteral("help")};
    spec.properties = {
        {QStringLiteral("filter"), QStringLiteral("QString"), {}, QStringLiteral(""),
         QStringLiteral("Matches label or keys; empty captions are dropped.")},
    };
    spec.build = buildShortcutList;
    return spec;
}

} // namespace novu::playground::pages
