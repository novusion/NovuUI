// Two surfaces that hid their own content.
//
// A submenu measured its room against the menu it opened from -- itself a
// window -- and was squeezed back over that menu instead of opening beside it.
// The message dialogs (confirm, inform, reportError) had a fixed size, so a
// message longer than about two lines was clipped at the top and the bottom.

#include "novu/ui/Popover.h"
#include "novu/ui/WindowShell.h"

#include <QApplication>
#include <QLabel>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>
#include <QWidget>

using novu::ui::Dialog;
using novu::ui::Popover;

class MenuDialogFitTest : public QObject
{
    Q_OBJECT

private slots:
    void submenuOpensBesideItsMenu()
    {
        QWidget host;
        host.setGeometry(100, 100, 1000, 600);
        host.show();
        QVERIFY(QTest::qWaitForWindowExposed(&host));

        auto* menu = new Popover(&host);
        menu->addItem(QStringLiteral("a"), QStringLiteral("First"));
        Popover* sub = menu->addSubmenu(QStringLiteral("s"), QStringLiteral("Load preset"));
        sub->addItem(QStringLiteral("p"), QStringLiteral("Taxonomy - Graphics"));
        menu->popupAt(host.mapToGlobal(QPoint(20, 20)));
        QTRY_VERIFY(menu->isVisible());

        QTest::keyClick(menu, Qt::Key_Down);
        QTest::keyClick(menu, Qt::Key_Down);
        QTest::keyClick(menu, Qt::Key_Right);
        QTRY_VERIFY(sub->isVisible());
        // beside, not over: the submenu starts at the menu's right edge
        QVERIFY2(sub->geometry().left() >= menu->geometry().right() - 4,
                 qPrintable(QStringLiteral("menu %1..%2, submenu at %3")
                                .arg(menu->geometry().left()).arg(menu->geometry().right())
                                .arg(sub->geometry().left())));
        menu->close();
        delete menu;
    }

    // the pointer goes from the item to its submenu across the menu's right
    // padding: the submenu stays open and its item can be clicked
    void submenuSurvivesThePointerCrossingThePadding()
    {
        QWidget host;
        host.setGeometry(100, 100, 1000, 600);
        host.show();
        QVERIFY(QTest::qWaitForWindowExposed(&host));

        auto* menu = new Popover(&host);
        menu->addItem(QStringLiteral("a"), QStringLiteral("New dataset"));
        Popover* sub = menu->addSubmenu(QStringLiteral("s"), QStringLiteral("Load preset"));
        sub->addItem(QStringLiteral("p"), QStringLiteral("Taxonomy - Graphics Core"));
        menu->popupAt(host.mapToGlobal(QPoint(20, 20)));
        QTRY_VERIFY(menu->isVisible());

        const QRect row = menu->itemRect(menu->indexOfKey(QStringLiteral("s")));
        QTest::mouseMove(menu, row.center());
        QTRY_VERIFY(sub->isVisible());
        // the menu's own padding right of the item, not yet under the submenu
        // (it starts 2 px inside the menu): no item there
        QTest::mouseMove(menu, QPoint(menu->width() - 4, row.center().y()));
        QVERIFY2(sub->isVisible(), "the submenu closed while the pointer crossed the padding");
        // ... and into the submenu, onto its item
        QSignalSpy chosen(sub, &Popover::triggered);
        const QRect target = sub->itemRect(sub->indexOfKey(QStringLiteral("p")));
        QTest::mouseMove(sub, target.center());
        QTest::mouseClick(sub, Qt::LeftButton, {}, target.center());
        QTRY_COMPARE(chosen.count(), 1);
        menu->close();
        delete menu;
    }

    void submenuFlipsLeftWhenTheWindowHasNoRoomRight()
    {
        QWidget host;
        host.setGeometry(100, 100, 600, 500);
        host.show();
        QVERIFY(QTest::qWaitForWindowExposed(&host));

        auto* menu = new Popover(&host);
        Popover* sub = menu->addSubmenu(QStringLiteral("s"), QStringLiteral("Load preset"));
        sub->addItem(QStringLiteral("p"), QStringLiteral("Taxonomy - Graphics"));
        menu->popupAt(host.mapToGlobal(QPoint(host.width() - 260, 20)));
        QTRY_VERIFY(menu->isVisible());
        QTest::keyClick(menu, Qt::Key_Down);
        QTest::keyClick(menu, Qt::Key_Right);
        QTRY_VERIFY(sub->isVisible());
        QVERIFY(sub->geometry().right() <= menu->geometry().left() + 4);
        menu->close();
        delete menu;
    }

    void confirmFitsALongMessage()
    {
        QWidget host;
        host.resize(900, 700);
        host.show();
        const QString message = QStringLiteral(
            "dataset has the files of an earlier version (manifest.json, index.jsonl, crawl-state.json) "
            "and no database yet. Importing reads them into dataset.sqlite3 and leaves every original "
            "file where it is; the import can be repeated and is checked against the files afterwards.");
        // measured inside the modal loop, asserted after it: a failed check
        // must close the dialog, not leave it waiting for the test's timeout
        int shown = -1, needed = -1;
        QTimer::singleShot(0, [&] {
            QWidget* dialog = QApplication::activeModalWidget();
            if (!dialog)
                return;
            for (QLabel* l : dialog->findChildren<QLabel*>()) {
                if (l->text() == message) {
                    shown = l->height();
                    needed = l->heightForWidth(l->width());
                }
            }
            static_cast<QDialog*>(dialog)->reject();
        });
        Dialog::confirm(&host, QStringLiteral("Import dataset into the database?"), message,
                        QStringLiteral("Import"));
        QVERIFY2(needed > 0, "the message label was not found");
        QVERIFY2(shown >= needed, qPrintable(QStringLiteral("label %1 px tall, needs %2").arg(shown).arg(needed)));
    }

    void aShortMessageKeepsTheDialogsSize()
    {
        QWidget host;
        host.show();
        QSize size;
        QTimer::singleShot(0, [&] {
            QWidget* dialog = QApplication::activeModalWidget();
            if (!dialog)
                return;
            size = dialog->size();
            static_cast<QDialog*>(dialog)->reject();
        });
        Dialog::confirm(&host, QStringLiteral("Delete?"), QStringLiteral("Delete it."));
        QVERIFY(!size.isEmpty());
        QVERIFY2(size.width() < 460, "a short message must not widen the dialog");
    }
};

QTEST_MAIN(MenuDialogFitTest)
#include "tst_menu_dialog_fit.moc"
