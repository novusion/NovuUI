#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Card.h"
#include "novu/ui/FilePicker.h"
#include "novu/ui/TableView.h"
#include "novu/ui/Text.h"

#include <QHash>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

// One small filesystem, held by the page rather than by FilePicker - exactly
// the split the component's own header describes: FilePicker paints a tree
// and a table and reports what was clicked, and whoever owns the data
// answers with setCurrentLocation() + setEntries(). A real caller would ask
// the OS here instead of a QHash.
struct Sample
{
    QHash<QString, QVector<FilePicker::Entry>> byLocation;
};

Sample buildSample()
{
    Sample sample;
    sample.byLocation[QStringLiteral("c:/")] = {
        {QStringLiteral("Program Files"), FilePicker::EntryKind::Folder,
         QStringLiteral("3/1/2025 12:25 PM"), QStringLiteral("File folder"), QString()},
        {QStringLiteral("Users"), FilePicker::EntryKind::Folder,
         QStringLiteral("7/15/2024 2:33 PM"), QStringLiteral("File folder"), QString()},
        {QStringLiteral("Windows"), FilePicker::EntryKind::Folder,
         QStringLiteral("7/15/2024 2:09 PM"), QStringLiteral("File folder"), QString()},
    };
    sample.byLocation[QStringLiteral("j:/backup")] = {
        {QStringLiteral("Documents_Win"), FilePicker::EntryKind::Folder,
         QStringLiteral("7/15/2024 2:33 PM"), QStringLiteral("File folder"), QString()},
        {QStringLiteral("Houdini_20.5"), FilePicker::EntryKind::Folder,
         QStringLiteral("3/1/2025 12:25 PM"), QStringLiteral("File folder"), QString()},
        {QStringLiteral("3d_Tiles_Earth_Mesh.txt"), FilePicker::EntryKind::File,
         QStringLiteral("7/15/2024 2:09 PM"), QStringLiteral("Text Document"),
         QStringLiteral("2 KB")},
        {QStringLiteral("obj_Cave_Generator_1_0.png"), FilePicker::EntryKind::File,
         QStringLiteral("11/13/2025 6:32 PM"), QStringLiteral("PNG File"),
         QStringLiteral("1,53x KB")},
        {QStringLiteral("obj_Cave_Generator_1_0.meta"), FilePicker::EntryKind::File,
         QStringLiteral("11/13/2025 6:32 PM"), QStringLiteral("META File"),
         QStringLiteral("1 KB")},
    };
    sample.byLocation[QStringLiteral("j:/backup/documents_win")] = {
        {QStringLiteral("Resume.docx"), FilePicker::EntryKind::File,
         QStringLiteral("2/2/2025 9:14 AM"), QStringLiteral("Word Document"),
         QStringLiteral("48 KB")},
        {QStringLiteral("Taxes"), FilePicker::EntryKind::Folder,
         QStringLiteral("1/9/2025 6:02 PM"), QStringLiteral("File folder"), QString()},
    };
    return sample;
}

void fillLocations(FilePicker* picker)
{
    picker->addLocation(QString(), QStringLiteral("pc"), QStringLiteral("This PC"),
                        Glyph::Cpu);
    picker->setLocationExpanded(QStringLiteral("pc"), true);
    picker->addLocation(QStringLiteral("pc"), QStringLiteral("c:/"),
                        QStringLiteral("Local Disk (C:)"), Glyph::Layers);
    picker->addLocation(QStringLiteral("pc"), QStringLiteral("g:/"),
                        QStringLiteral("Work (G:)"), Glyph::Layers);
    picker->addLocation(QStringLiteral("pc"), QStringLiteral("j:/"),
                        QStringLiteral("Work_2 (J:)"), Glyph::Layers);
    picker->setLocationExpanded(QStringLiteral("j:/"), true);
    picker->addLocation(QStringLiteral("j:/"), QStringLiteral("j:/backup"),
                        QStringLiteral("---- Backup ----"), Glyph::Folder);
    picker->setLocationExpanded(QStringLiteral("j:/backup"), true);
    picker->addLocation(QStringLiteral("j:/backup"),
                        QStringLiteral("j:/backup/documents_win"),
                        QStringLiteral("Documents_Win"), Glyph::Folder);
    picker->addLocation(QStringLiteral("j:/"), QStringLiteral("j:/assets"),
                        QStringLiteral("---- My Assets ----"), Glyph::Folder);
    picker->addLocation(QStringLiteral("j:/"), QStringLiteral("j:/photo"),
                        QStringLiteral("---- Photo ----"), Glyph::Folder);
}

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    {
        Card* card = exampleCard(page, QStringLiteral("Open a file"),
                                 QStringLiteral("Navigation on the left, what it holds on "
                                                "the right, the choice at the bottom."));
        QVBoxLayout* body = card->body();

        // QHash is cheap to copy (implicitly shared), so the sample data
        // rides along in the lambdas by value rather than needing a second
        // owner found for it.
        const Sample sample = buildSample();
        auto* picker = new FilePicker(card);
        picker->setMinimumHeight(360);
        fillLocations(picker);

        const QString startLocation = QStringLiteral("j:/backup");
        picker->setCurrentLocation(startLocation);
        picker->setEntries(sample.byLocation.value(startLocation));

        auto* status = label(QStringLiteral("—"), card, TypeRole::Mono, Ink::Secondary);

        QObject::connect(picker, &FilePicker::locationActivated, picker,
                         [picker, sample](const QString& id) {
                             if (!sample.byLocation.contains(id)) {
                                 return;
                             }
                             picker->setCurrentLocation(id);
                             picker->setEntries(sample.byLocation.value(id));
                         });
        QObject::connect(picker, &FilePicker::entryActivated, picker,
                         [picker, sample](int row) {
                             const auto entries =
                                 sample.byLocation.value(picker->currentLocation());
                             if (row < 0 || row >= entries.size()) {
                                 return;
                             }
                             const FilePicker::Entry& entry = entries.at(row);
                             if (entry.kind != FilePicker::EntryKind::Folder) {
                                 emit picker->confirmed(entry.name);
                                 return;
                             }
                             // The sample keys a subfolder by its parent's own key plus
                             // the folder's name in lower case - a real caller would
                             // join an actual path instead.
                             const QString childId = picker->currentLocation()
                                 + QStringLiteral("/") + entry.name.toLower();
                             if (sample.byLocation.contains(childId)) {
                                 picker->setCurrentLocation(childId);
                                 picker->setEntries(sample.byLocation.value(childId));
                             }
                         });
        QObject::connect(picker, &FilePicker::confirmed, status,
                         [status](const QString& name) {
                             status->setText(QStringLiteral("selected: %1").arg(name));
                         });
        QObject::connect(picker, &FilePicker::cancelled, status, [status] {
            status->setText(QStringLiteral("cancelled"));
        });

        body->addWidget(picker);
        body->addSpacing(6);
        body->addWidget(status);

        note(body, card,
             QStringLiteral("FilePicker does no filesystem I/O of its own: the tree and "
                            "the table are both painted lists a caller fills, exactly the "
                            "way NavList and TreeView already are. Opening a location "
                            "asks the caller for its contents rather than assuming the "
                            "fetch succeeds and moving there itself."));
        note(body, card,
             QStringLiteral("Built from three existing components — TreeView, TableView "
                            "and Splitter — rather than three new ones. Double-click a "
                            "folder row in the table to drill into it the same way the "
                            "tree does; double-click a file, or press Select, to confirm "
                            "it."));
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec filePickerComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("file-picker");
    spec.name = QStringLiteral("FilePicker");
    spec.category = Category::Navigation;
    spec.description = QStringLiteral(
        "An open/save dialog's own shape in NovuUI's style: a location tree, a content "
        "table, a name field and a confirm action. Owns no filesystem access.");
    spec.headerPath = QStringLiteral("include/novu/ui/FilePicker.h");
    spec.sourcePath = QStringLiteral("src/FilePicker.cpp");
    spec.designedFor = {QStringLiteral("novuexplorer")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Folder"), QStringLiteral("File")};
    spec.states = {QStringLiteral("Normal"), QStringLiteral("Selected")};
    spec.keywords = {QStringLiteral("file picker"), QStringLiteral("open dialog"),
                     QStringLiteral("save dialog"), QStringLiteral("explorer"),
                     QStringLiteral("browser"), QStringLiteral("tree"),
                     QStringLiteral("table")};
    spec.properties = {
        {QStringLiteral("currentLocation"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("Which tree node is open. The caller's own id, usually a path.")},
        {QStringLiteral("selectedName"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("What the bottom field holds - what Select confirms.")},
    };
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
