// Favourites and the remembered folder in the shared file picker.
//
// Three layers, tested where each one lives:
//
//   FilePickerPlaces   the store - what is pinned, under what name, where the
//                      person last was. Redirected to a temporary INI so the
//                      real one is never read or written.
//   FilePicker         the gestures - a right-click on a tree row or a listing
//                      row, and an in-row rename, each arriving as the signal
//                      the driver listens for.
//   FilePickerDialog   the driver - pinned folders first under their own name,
//                      the window opening where the last choice was made, and
//                      pin / unpin / rename going through to the store and the
//                      tree together.

#include "novu/ui/FilePicker.h"
#include "novu/ui/FilePickerDialog.h"
#include "novu/ui/FilePickerPlaces.h"
#include "novu/ui/Popover.h"
#include "novu/ui/TableView.h"
#include "novu/ui/TreeView.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QLineEdit>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

#include <functional>

using novu::ui::FilePicker;
using novu::ui::FilePickerDialog;
using novu::ui::FilePickerPlaces;
using novu::ui::Popover;

namespace {

// Runs `body` against the picker's window once it is up, then closes it. A
// modal exec() is the only way in, so the test reaches it from a timer the way
// a person reaches it from the screen.
void whenOpen(std::function<void(QDialog*, FilePicker*)> body)
{
    auto* timer = new QTimer;
    timer->setInterval(10);
    QObject::connect(timer, &QTimer::timeout, [timer, body] {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (!dialog) {
            return;
        }
        timer->stop();
        timer->deleteLater();
        body(dialog, dialog->findChild<FilePicker*>());
        dialog->reject();
    });
    timer->start();
}

QString mkdir(const QTemporaryDir& root, const QString& name)
{
    QDir(root.path()).mkpath(name);
    return FilePickerPlaces::normalised(QDir(root.path()).filePath(name));
}

} // namespace

class TestFilePickerPlaces : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QVERIFY(m_settings.isValid());
        // Every test below would otherwise write to the person's own
        // %APPDATA%\Novusion\NovuUI.ini.
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
        QCoreApplication::setApplicationName(QStringLiteral("tst_file_picker_places"));
    }

    void init()
    {
        QSettings(QSettings::IniFormat, QSettings::UserScope,
                  QStringLiteral("Novusion"), QStringLiteral("NovuUI")).clear();
        QCoreApplication::setApplicationName(QStringLiteral("tst_file_picker_places"));
    }

    // ── the store ───────────────────────────────────────────────────────────

    void storageIsTheRedirectedIniAndNotTheRealOne()
    {
        QTemporaryDir root;
        QVERIFY(FilePickerPlaces::addFavourite(mkdir(root, QStringLiteral("Builds"))));
        QVERIFY(QFile::exists(m_settings.filePath(QStringLiteral("Novusion/NovuUI.ini"))));
    }

    void oneFolderIsOneFavouriteHoweverItIsSpelled()
    {
        QTemporaryDir root;
        const QString builds = mkdir(root, QStringLiteral("Builds"));
        QVERIFY(FilePickerPlaces::addFavourite(builds));

        const QString native = QDir::toNativeSeparators(builds) + QDir::separator();
        QVERIFY(FilePickerPlaces::isFavourite(native));
        QVERIFY(!FilePickerPlaces::addFavourite(native));
        QVERIFY(!FilePickerPlaces::addFavourite(builds + QStringLiteral("/./")));
#if defined(Q_OS_WIN)
        QVERIFY(!FilePickerPlaces::addFavourite(builds.toUpper()));
#endif
        QCOMPARE(FilePickerPlaces::favourites().size(), 1);
    }

    void onlyAnExistingFolderCanBePinned()
    {
        QTemporaryDir root;
        QVERIFY(!FilePickerPlaces::addFavourite(QDir(root.path()).filePath(QStringLiteral("nope"))));

        const QString file = QDir(root.path()).filePath(QStringLiteral("a.zip"));
        QFile f(file);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.close();
        QVERIFY(!FilePickerPlaces::addFavourite(file));
        QVERIFY(!FilePickerPlaces::addFavourite(QString()));
        QVERIFY(FilePickerPlaces::favourites().isEmpty());
    }

    void aFavouriteIsShownUnderItsOwnNameUntilGivenAnother()
    {
        QTemporaryDir root;
        const QString builds = mkdir(root, QStringLiteral("Builds"));
        const QString dist = mkdir(root, QStringLiteral("dist"));
        QVERIFY(FilePickerPlaces::addFavourite(builds));
        QVERIFY(FilePickerPlaces::addFavourite(dist, QStringLiteral("  NovuAdmin dist  ")));

        const auto list = FilePickerPlaces::favourites();
        QCOMPARE(list.size(), 2);
        QCOMPARE(list.at(0).path, builds);
        QCOMPARE(list.at(0).label, QStringLiteral("Builds"));
        QCOMPARE(list.at(1).label, QStringLiteral("NovuAdmin dist"));
    }

    void renamingChangesTheNameAndNothingElse()
    {
        QTemporaryDir root;
        const QString builds = mkdir(root, QStringLiteral("Builds"));
        QVERIFY(FilePickerPlaces::addFavourite(builds));

        QVERIFY(FilePickerPlaces::renameFavourite(builds, QStringLiteral(" Release builds ")));
        QCOMPARE(FilePickerPlaces::favourites().at(0).label, QStringLiteral("Release builds"));
        QCOMPARE(FilePickerPlaces::favourites().at(0).path, builds);

        // A blank name is not a name, and a folder that is not pinned has
        // no name of its own to change.
        QVERIFY(!FilePickerPlaces::renameFavourite(builds, QStringLiteral("   ")));
        QVERIFY(!FilePickerPlaces::renameFavourite(mkdir(root, QStringLiteral("other")),
                                                   QStringLiteral("x")));
        QCOMPARE(FilePickerPlaces::favourites().at(0).label, QStringLiteral("Release builds"));
    }

    void removingUnpinsOneAndKeepsTheOrderOfTheRest()
    {
        QTemporaryDir root;
        const QString a = mkdir(root, QStringLiteral("a"));
        const QString b = mkdir(root, QStringLiteral("b"));
        const QString c = mkdir(root, QStringLiteral("c"));
        for (const QString& path : {a, b, c}) {
            QVERIFY(FilePickerPlaces::addFavourite(path));
        }

        QVERIFY(FilePickerPlaces::removeFavourite(b));
        QVERIFY(!FilePickerPlaces::removeFavourite(b));

        const auto list = FilePickerPlaces::favourites();
        QCOMPARE(list.size(), 2);
        QCOMPARE(list.at(0).path, a);
        QCOMPARE(list.at(1).path, c);
        // The pin goes, never the folder.
        QVERIFY(QFileInfo(b).isDir());
    }

    void aFavouriteWhoseFolderIsGoneIsKept()
    {
        QTemporaryDir root;
        const QString drive = mkdir(root, QStringLiteral("usb"));
        QVERIFY(FilePickerPlaces::addFavourite(drive, QStringLiteral("USB stick")));
        QVERIFY(QDir(drive).removeRecursively());

        const auto list = FilePickerPlaces::favourites();
        QCOMPARE(list.size(), 1);
        QCOMPARE(list.at(0).label, QStringLiteral("USB stick"));
    }

    void theLastFolderIsRememberedPerApplication()
    {
        QTemporaryDir root;
        const QString builds = mkdir(root, QStringLiteral("Builds"));
        QVERIFY(FilePickerPlaces::lastDirectory().isEmpty());

        FilePickerPlaces::setLastDirectory(QDir::toNativeSeparators(builds));
        QCOMPARE(FilePickerPlaces::lastDirectory(), builds);

        QCoreApplication::setApplicationName(QStringLiteral("SomeOtherProduct"));
        QVERIFY(FilePickerPlaces::lastDirectory().isEmpty());
        QCoreApplication::setApplicationName(QStringLiteral("tst_file_picker_places"));
        QCOMPARE(FilePickerPlaces::lastDirectory(), builds);

        // Somewhere that has gone away is nowhere to open.
        QVERIFY(QDir(builds).removeRecursively());
        QVERIFY(FilePickerPlaces::lastDirectory().isEmpty());
    }

    // ── the gestures ────────────────────────────────────────────────────────

    void rightClickingAListingRowNamesTheRow()
    {
        FilePicker picker;
        picker.resize(800, 500);
        picker.setEntries({{QStringLiteral(".."), FilePicker::EntryKind::Folder},
                           {QStringLiteral("Builds"), FilePicker::EntryKind::Folder},
                           {QStringLiteral("a.zip"), FilePicker::EntryKind::File}});
        picker.show();
        QVERIFY(QTest::qWaitForWindowExposed(&picker));

        QSignalSpy spy(&picker, &FilePicker::entryMenuRequested);
        auto* viewport = picker.table()->viewport();

        const QPoint onRow1 = picker.table()->visualRect(picker.table()->model()->index(1, 1)).center();
        QContextMenuEvent onRow(QContextMenuEvent::Mouse, onRow1, viewport->mapToGlobal(onRow1));
        QApplication::sendEvent(viewport, &onRow);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 1);
        QCOMPARE(spy.at(0).at(1).toPoint(), viewport->mapToGlobal(onRow1));

        const int lastBottom =
            picker.table()->visualRect(picker.table()->model()->index(2, 1)).bottom();
        const QPoint below(10, lastBottom + 10);
        QVERIFY2(below.y() < viewport->height(), "no empty space under the rows to click");
        QContextMenuEvent onNothing(QContextMenuEvent::Mouse, below, viewport->mapToGlobal(below));
        QApplication::sendEvent(viewport, &onNothing);
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.at(1).at(0).toInt(), -1);
    }

    void rightClickingATreeRowNamesTheLocation()
    {
        FilePicker picker;
        picker.resize(800, 500);
        picker.addLocation({}, QStringLiteral("J:/Builds"), QStringLiteral("Builds"));
        picker.show();
        QVERIFY(QTest::qWaitForWindowExposed(&picker));

        QSignalSpy spy(&picker, &FilePicker::locationMenuRequested);
        auto* tree = picker.tree();
        const QPoint at = tree->rowRect(tree->rowOf(QStringLiteral("J:/Builds"))).center();
        QContextMenuEvent event(QContextMenuEvent::Mouse, at, tree->viewport()->mapToGlobal(at));
        QApplication::sendEvent(tree->viewport(), &event);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("J:/Builds"));
    }

    void anInRowRenameArrivesAsLocationRenamed()
    {
        FilePicker picker;
        picker.addLocation({}, QStringLiteral("J:/Builds"), QStringLiteral("Builds"));
        picker.show();
        QVERIFY(QTest::qWaitForWindowExposed(&picker));

        QSignalSpy spy(&picker, &FilePicker::locationRenamed);
        picker.renameLocation(QStringLiteral("J:/Builds"));
        auto* editor = picker.tree()->findChild<QLineEdit*>();
        QVERIFY(editor);
        editor->setText(QStringLiteral("Release builds"));
        picker.tree()->commitRename();

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(1).toString(), QStringLiteral("Release builds"));
        // The tree does not rename itself - that is the caller's answer.
        QCOMPARE(picker.tree()->label(QStringLiteral("J:/Builds")), QStringLiteral("Builds"));
        picker.setLocationLabel(QStringLiteral("J:/Builds"), QStringLiteral("Release builds"));
        QCOMPARE(picker.tree()->label(QStringLiteral("J:/Builds")), QStringLiteral("Release builds"));
    }

    // ── the window ──────────────────────────────────────────────────────────

    void pinnedFoldersComeFirstUnderTheirOwnName()
    {
        QTemporaryDir root;
        const QString builds = mkdir(root, QStringLiteral("Builds"));
        QVERIFY(FilePickerPlaces::addFavourite(builds, QStringLiteral("Release builds")));

        int row = -2;
        QString label;
        whenOpen([&](QDialog*, FilePicker* picker) {
            QVERIFY(picker);
            row = picker->tree()->rowOf(builds);
            label = picker->tree()->label(builds);
        });
        FilePickerDialog::open(nullptr, QStringLiteral("Open"), root.path());
        QCOMPARE(row, 0);
        QCOMPARE(label, QStringLiteral("Release builds"));
    }

    void withNoStartItOpensWhereTheLastFileWasChosen()
    {
        QTemporaryDir root;
        const QString builds = mkdir(root, QStringLiteral("Builds"));
        FilePickerPlaces::setLastDirectory(builds);

        QString opened;
        whenOpen([&](QDialog*, FilePicker* picker) {
            QVERIFY(picker);
            opened = picker->currentLocation();
        });
        FilePickerDialog::open(nullptr, QStringLiteral("Open"), QString());
        QCOMPARE(FilePickerPlaces::normalised(opened), builds);
    }

    void aBareFileNameIsSavedWhereTheLastOneWent()
    {
        QTemporaryDir root;
        const QString exports = mkdir(root, QStringLiteral("exports"));
        FilePickerPlaces::setLastDirectory(exports);

        QString opened;
        QString name;
        whenOpen([&](QDialog*, FilePicker* picker) {
            QVERIFY(picker);
            opened = picker->currentLocation();
            name = picker->selectedName();
        });
        FilePickerDialog::save(nullptr, QStringLiteral("Export CSV"),
                               QStringLiteral("trades_1-50_of_301517.csv"),
                               {{QStringLiteral("CSV files"), {QStringLiteral("*.csv")}}});
        QCOMPARE(FilePickerPlaces::normalised(opened), exports);
        QCOMPARE(name, QStringLiteral("trades_1-50_of_301517.csv"));
    }

    void choosingAFileRemembersItsFolder()
    {
        QTemporaryDir root;
        const QString builds = mkdir(root, QStringLiteral("Builds"));
        QFile f(QDir(builds).filePath(QStringLiteral("novu-1.0.0.zip")));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.close();

        auto* timer = new QTimer;
        timer->setInterval(10);
        QObject::connect(timer, &QTimer::timeout, [timer] {
            auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            if (!dialog) {
                return;
            }
            timer->stop();
            timer->deleteLater();
            emit dialog->findChild<FilePicker*>()->confirmed(QStringLiteral("novu-1.0.0.zip"));
        });
        timer->start();
        const QString chosen = FilePickerDialog::open(nullptr, QStringLiteral("Open"), builds);

        QCOMPARE(FilePickerPlaces::normalised(chosen),
                 FilePickerPlaces::normalised(QDir(builds).filePath(QStringLiteral("novu-1.0.0.zip"))));
        QCOMPARE(FilePickerPlaces::lastDirectory(), builds);
    }

    void pinningFromTheListingPutsTheFolderAtTheTop()
    {
        QTemporaryDir root;
        const QString builds = mkdir(root, QStringLiteral("Builds"));

        bool offered = false;
        int row = -2;
        whenOpen([&](QDialog* dialog, FilePicker* picker) {
            // Row 0 is "..", row 1 the only folder.
            emit picker->entryMenuRequested(1, QPoint(0, 0));
            auto* menu = dialog->findChild<Popover*>();
            QVERIFY(menu);
            offered = menu->indexOfKey(QStringLiteral("pin")) >= 0;
            emit menu->triggered(QStringLiteral("pin"));
            row = picker->tree()->rowOf(builds);
        });
        FilePickerDialog::open(nullptr, QStringLiteral("Open"), root.path());

        QVERIFY(offered);
        QVERIFY(FilePickerPlaces::isFavourite(builds));
        QCOMPARE(row, 0);
    }

    void aSystemPlaceOffersNothing()
    {
        const QString home = FilePickerPlaces::normalised(QDir::homePath());
        bool menuShown = true;
        whenOpen([&](QDialog* dialog, FilePicker* picker) {
            emit picker->locationMenuRequested(home, QPoint(0, 0));
            menuShown = dialog->findChild<Popover*>() != nullptr;
        });
        FilePickerDialog::open(nullptr, QStringLiteral("Open"), QDir::homePath());
        QVERIFY(!menuShown);
    }

    void unpinningAndRenamingGoThroughToTheStoreAndTheTree()
    {
        QTemporaryDir root;
        const QString builds = mkdir(root, QStringLiteral("Builds"));
        QVERIFY(FilePickerPlaces::addFavourite(builds));

        QString renamedLabel;
        bool stillRoot = true;
        whenOpen([&](QDialog* dialog, FilePicker* picker) {
            // Rename first, the way the in-row editor reports it.
            emit picker->locationRenamed(builds, QStringLiteral("  Release builds "));
            renamedLabel = picker->tree()->label(builds);

            emit picker->locationMenuRequested(builds, QPoint(0, 0));
            auto* menu = dialog->findChild<Popover*>();
            QVERIFY(menu);
            QVERIFY(menu->indexOfKey(QStringLiteral("rename")) >= 0);
            emit menu->triggered(QStringLiteral("unpin"));
            // Still in the tree - the listing is open on it - but no longer
            // a root of its own.
            stillRoot = picker->tree()->parentOf(builds).isEmpty();
        });
        FilePickerDialog::open(nullptr, QStringLiteral("Open"), builds);

        QCOMPARE(renamedLabel, QStringLiteral("Release builds"));
        QVERIFY(!FilePickerPlaces::isFavourite(builds));
        QVERIFY(!stillRoot);
    }

    // ── new folder ──────────────────────────────────────────────────────────

    void newFolderIsOfferedWhereAFolderCanBeTheAnswer()
    {
        QTemporaryDir root;
        bool onOpen = true;
        bool onFolder = false;
        bool onSave = false;
        whenOpen([&](QDialog* dialog, FilePicker*) {
            onOpen = dialog->findChild<QWidget*>(QStringLiteral("FilePickerNewFolder"))->isVisible();
        });
        FilePickerDialog::open(nullptr, QStringLiteral("Open"), root.path());
        whenOpen([&](QDialog* dialog, FilePicker*) {
            onFolder = dialog->findChild<QWidget*>(QStringLiteral("FilePickerNewFolder"))->isVisible();
        });
        FilePickerDialog::chooseFolder(nullptr, QStringLiteral("Clone into"), root.path());
        whenOpen([&](QDialog* dialog, FilePicker*) {
            onSave = dialog->findChild<QWidget*>(QStringLiteral("FilePickerNewFolder"))->isVisible();
        });
        FilePickerDialog::save(nullptr, QStringLiteral("Save"), root.path());

        QVERIFY(!onOpen);
        QVERIFY(onFolder);
        QVERIFY(onSave);
    }

    void aNewFolderIsMadeNamedInTheRowAndChosen()
    {
        QTemporaryDir root;
        mkdir(root, QStringLiteral("New folder"));   // taken: the next is (2)
        const QString expected = FilePickerPlaces::normalised(
            QDir(root.path()).filePath(QStringLiteral("NovuExplorer")));

        bool made = false;
        auto* timer = new QTimer;
        timer->setInterval(10);
        QObject::connect(timer, &QTimer::timeout, [&, timer] {
            auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            if (!dialog) {
                return;
            }
            timer->stop();
            timer->deleteLater();
            auto* picker = dialog->findChild<FilePicker*>();
            emit picker->newFolderRequested();
            made = QFileInfo(QDir(root.path()).filePath(QStringLiteral("New folder (2)"))).isDir();

            // The rename the button starts, typed and committed in the row.
            auto* editor = picker->tree()->findChild<QLineEdit*>();
            QVERIFY(editor);
            editor->setText(QStringLiteral("NovuExplorer"));
            picker->tree()->commitRename();

            emit picker->confirmed(picker->selectedName());
        });
        timer->start();
        const QString chosen =
            FilePickerDialog::chooseFolder(nullptr, QStringLiteral("Clone into"), root.path());

        QVERIFY(made);
        QCOMPARE(FilePickerPlaces::normalised(chosen), expected);
        QVERIFY(QFileInfo(expected).isDir());
        QVERIFY(!QFileInfo::exists(QDir(root.path()).filePath(QStringLiteral("New folder (2)"))));
    }

    void renamingANewFolderOntoAnotherIsRefused()
    {
        QTemporaryDir root;
        mkdir(root, QStringLiteral("Taken"));

        whenOpen([&](QDialog*, FilePicker* picker) {
            emit picker->newFolderRequested();
            const QString made = FilePickerPlaces::normalised(
                QDir(root.path()).filePath(QStringLiteral("New folder")));
            // Refused with a message box, which is closed as soon as it shows.
            QTimer::singleShot(10, [] {
                if (auto* box = qobject_cast<QDialog*>(QApplication::activeModalWidget())) {
                    box->reject();
                }
            });
            emit picker->locationRenamed(made, QStringLiteral("Taken"));
            picker->tree()->cancelRename();
        });
        FilePickerDialog::chooseFolder(nullptr, QStringLiteral("Clone into"), root.path());

        QVERIFY(QFileInfo(QDir(root.path()).filePath(QStringLiteral("New folder"))).isDir());
        QVERIFY(QFileInfo(QDir(root.path()).filePath(QStringLiteral("Taken"))).isDir());
    }

private:
    QTemporaryDir m_settings;
};

QTEST_MAIN(TestFilePickerPlaces)
#include "tst_file_picker_places.moc"
