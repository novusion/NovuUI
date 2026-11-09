// TabBar as a browser tab strip: what a browser owner relies on.
//
//   Renaming in place follows NavList's convention: Enter submits, Escape
//   abandons, focus leaving submits, a refusal keeps the editor open on Enter,
//   and the editor goes away with its tab. The bar never changes the label
//   itself.
//   A right click on a tab asks for that tab's menu, with the screen position.
//   Past the width floor the strip scrolls instead of clipping: the wheel
//   moves it, the current tab is kept in view, and hit-testing follows the
//   scroll - so a drag still reorders, and reports the move.
//   A speaker mark is per tab, travels with a reorder, takes room only while
//   it is shown, and pressing it asks for a toggle.
//   A close request's key outlives the tab: the owner removes the tab first
//   and goes on using the key it was given.

#include "novu/ui/TabBar.h"

#include <QContextMenuEvent>
#include <QFocusEvent>
#include <QLineEdit>
#include <QSignalSpy>
#include <QTest>
#include <QWheelEvent>

#include <functional>

using namespace novu::ui;

namespace {

QLineEdit* editorOf(TabBar& bar)
{
    return bar.findChild<QLineEdit*>(QStringLiteral("novuTabInlineEditor"));
}

void populate(TabBar& bar, int count)
{
    for (int i = 0; i < count; ++i) {
        const QString key = QStringLiteral("t%1").arg(i);
        bar.addTab(key, QStringLiteral("Tab number %1 with a title").arg(i));
        bar.setMarkText(key, QStringLiteral("T"));
    }
}

void wheel(TabBar& bar, int notches)
{
    const QPointF at(bar.tabsViewport().center());
    QWheelEvent event(at, bar.mapToGlobal(at), QPoint(), QPoint(0, notches * 120),
                      Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QCoreApplication::sendEvent(&bar, &event);
}

} // namespace

class TabBarBrowserWorkflowTest : public QObject
{
    Q_OBJECT

private slots:
    void renameEnterSubmitsWithoutChangingTheLabel()
    {
        TabBar bar;
        bar.resize(900, 39);
        bar.setNewTabButton(true);
        populate(bar, 3);
        bar.show();
        QSignalSpy committed(&bar, &TabBar::inlineEditCommitted);
        QSignalSpy cancelled(&bar, &TabBar::inlineEditCancelled);
        QVERIFY(!bar.beginInlineEdit(QStringLiteral("missing")));
        QVERIFY(bar.beginInlineEdit(QStringLiteral("t1")));
        QLineEdit* editor = editorOf(bar);
        QVERIFY(editor);
        QVERIFY(editor->isVisible());
        QCOMPARE(editor->selectedText(), bar.label(QStringLiteral("t1")));
        QVERIFY(bar.tabRect(1).intersects(editor->geometry()));
        editor->setText(QStringLiteral("Mail"));
        QTest::keyClick(editor, Qt::Key_Return);
        QCOMPARE(committed.count(), 1);
        QCOMPARE(committed.at(0).at(0).toString(), QStringLiteral("t1"));
        QCOMPARE(committed.at(0).at(1).toString(), QStringLiteral("Mail"));
        QCOMPARE(cancelled.count(), 0);
        QVERIFY(!bar.isInlineEditing());
        // The owner applies a name it accepted.
        QCOMPARE(bar.label(QStringLiteral("t1")), QStringLiteral("Tab number 1 with a title"));
    }

    void renameEscapeAbandonsAndFocusOutSubmits()
    {
        TabBar bar;
        bar.resize(900, 39);
        populate(bar, 3);
        bar.show();
        QSignalSpy committed(&bar, &TabBar::inlineEditCommitted);
        QSignalSpy cancelled(&bar, &TabBar::inlineEditCancelled);
        bar.beginInlineEdit(QStringLiteral("t0"));
        QTest::keyClick(editorOf(bar), Qt::Key_Escape);
        QCOMPARE(committed.count(), 0);
        QCOMPARE(cancelled.count(), 1);
        QVERIFY(!bar.isInlineEditing());

        bar.beginInlineEdit(QStringLiteral("t0"));
        QFocusEvent out(QEvent::FocusOut, Qt::MouseFocusReason);
        QCoreApplication::sendEvent(editorOf(bar), &out);
        QCOMPARE(committed.count(), 1);
        QVERIFY(!bar.isInlineEditing());

        // A popup taking focus is not the person leaving the field.
        bar.beginInlineEdit(QStringLiteral("t0"));
        QFocusEvent popup(QEvent::FocusOut, Qt::PopupFocusReason);
        QCoreApplication::sendEvent(editorOf(bar), &popup);
        QVERIFY(bar.isInlineEditing());
        bar.cancelInlineEdit();
        QVERIFY(!bar.isInlineEditing());
    }

    void aRefusalKeepsTheEditorOpenOnEnter()
    {
        TabBar bar;
        bar.resize(900, 39);
        populate(bar, 2);
        bar.show();
        connect(&bar, &TabBar::inlineEditCommitted, &bar,
                [&bar](const QString&, const QString& text) {
                    if (text.trimmed().isEmpty()) {
                        bar.rejectInlineEdit(QStringLiteral("A name is required."));
                    }
                });
        bar.beginInlineEdit(QStringLiteral("t0"));
        QLineEdit* editor = editorOf(bar);
        editor->setText(QStringLiteral("   "));
        QTest::keyClick(editor, Qt::Key_Return);
        QVERIFY(bar.isInlineEditing());
        QCOMPARE(editor->property("invalid").toBool(), true);
        QTest::keyClick(editor, Qt::Key_A);
        QCOMPARE(editor->property("invalid").toBool(), false);
    }

    void theEditorEndsWithItsTab()
    {
        TabBar bar;
        bar.resize(900, 39);
        populate(bar, 3);
        bar.show();
        QSignalSpy cancelled(&bar, &TabBar::inlineEditCancelled);
        bar.beginInlineEdit(QStringLiteral("t2"));
        bar.removeTab(QStringLiteral("t2"));
        QVERIFY(!bar.isInlineEditing());
        QCOMPARE(cancelled.count(), 1);
    }

    void rightClickAsksForThatTabsMenu()
    {
        TabBar bar;
        bar.resize(900, 39);
        populate(bar, 3);
        bar.show();
        QSignalSpy menu(&bar, &TabBar::contextMenuRequested);
        const QPoint at = bar.tabRect(2).center();
        QContextMenuEvent event(QContextMenuEvent::Mouse, at, bar.mapToGlobal(at));
        QCoreApplication::sendEvent(&bar, &event);
        QCOMPARE(menu.count(), 1);
        QCOMPARE(menu.at(0).at(0).toString(), QStringLiteral("t2"));
        QCOMPARE(menu.at(0).at(1).toPoint(), bar.mapToGlobal(at));

        // Off every tab: no menu.
        const QPoint empty(bar.width() - 3, bar.height() / 2);
        QContextMenuEvent miss(QContextMenuEvent::Mouse, empty, bar.mapToGlobal(empty));
        QCoreApplication::sendEvent(&bar, &miss);
        QCOMPARE(menu.count(), 1);
    }

    void overflowScrollsInsteadOfClipping()
    {
        TabBar bar;
        bar.resize(600, 39);
        bar.setNewTabButton(true);
        populate(bar, 20);
        bar.show();
        QVERIFY(bar.maximumScrollOffset() > 0);
        QCOMPARE(bar.scrollOffset(), 0);

        // The current tab is kept in view.
        bar.setCurrentKey(QStringLiteral("t19"));
        const QRect viewport = bar.tabsViewport();
        QVERIFY(viewport.contains(bar.tabRect(19)));
        QVERIFY(bar.scrollOffset() > 0);
        QVERIFY(!viewport.intersects(bar.tabRect(0)));
        // The "+" stays at the end of the band, not after the last tab.
        QVERIFY(bar.newTabRect().left() > viewport.right());
        QVERIFY(bar.newTabRect().right() < bar.width());

        // The wheel scrolls it back, and hit-testing follows.
        wheel(bar, 40);
        QCOMPARE(bar.scrollOffset(), 0);
        QCOMPARE(bar.indexAt(bar.tabRect(0).center()), 0);
        wheel(bar, -1);
        QVERIFY(bar.scrollOffset() > 0);

        // A scrolled-away tab is not hit through the "+".
        bar.setScrollOffset(bar.maximumScrollOffset());
        QCOMPARE(bar.indexAt(bar.newTabRect().center()), -1);
    }

    void aDragReordersUnderScrollAndReportsTheMove()
    {
        TabBar bar;
        bar.resize(600, 39);
        populate(bar, 20);
        bar.show();
        bar.setScrollOffset(bar.maximumScrollOffset() / 2);
        const int scroll = bar.scrollOffset();
        QVERIFY(scroll > 0);

        int from = -1;
        for (int i = 0; i < bar.count(); ++i) {
            if (bar.tabsViewport().contains(bar.tabRect(i)) && bar.tabsViewport().contains(bar.tabRect(i + 1))) {
                from = i;
                break;
            }
        }
        QVERIFY(from >= 0);
        const QString key = bar.keyAt(from);
        QSignalSpy moved(&bar, &TabBar::tabMoved);
        const QPoint start = bar.tabRect(from).center();
        const QPoint end = bar.tabRect(from + 1).center();
        QTest::mousePress(&bar, Qt::LeftButton, Qt::NoModifier, start);
        QTest::mouseMove(&bar, start + QPoint(10, 0));
        QTest::mouseMove(&bar, end);
        QTest::mouseRelease(&bar, Qt::LeftButton, Qt::NoModifier, end);
        QVERIFY(moved.count() >= 1);
        QCOMPARE(moved.last().at(0).toString(), key);
        QCOMPARE(bar.indexOfKey(key), from + 1);
        QCOMPARE(moved.last().at(2).toInt(), from + 1);
        QCOMPARE(bar.scrollOffset(), scroll);
    }

    void theSpeakerIsPerTabAndAsksForAToggle()
    {
        TabBar bar;
        bar.resize(900, 39);
        populate(bar, 3);
        bar.show();
        const QString key = QStringLiteral("t1");
        QVERIFY(bar.audioRect(1).isNull());
        const int before = bar.tabRect(1).width();
        bar.setAudio(key, TabBar::Audio::Playing);
        QCOMPARE(bar.audio(key), TabBar::Audio::Playing);
        QVERIFY(!bar.audioRect(1).isNull());
        QVERIFY(bar.tabRect(1).width() >= before);
        QVERIFY(bar.tabRect(1).contains(bar.audioRect(1)));

        QSignalSpy toggled(&bar, &TabBar::audioToggleRequested);
        QSignalSpy current(&bar, &TabBar::currentChanged);
        QTest::mouseClick(&bar, Qt::LeftButton, Qt::NoModifier, bar.audioRect(1).center());
        QCOMPARE(toggled.count(), 1);
        QCOMPARE(toggled.at(0).at(0).toString(), key);
        // Pressing the speaker is not choosing the tab.
        QCOMPARE(current.count(), 0);

        bar.setAudio(key, TabBar::Audio::Muted);
        QCOMPARE(bar.audio(key), TabBar::Audio::Muted);
        bar.setAudio(key, TabBar::Audio::None);
        QVERIFY(bar.audioRect(1).isNull());
        bar.setAudio(key, TabBar::Audio::Muted);
        bar.removeTab(key);
        bar.addTab(key, QStringLiteral("Again"));
        QCOMPARE(bar.audio(key), TabBar::Audio::None);
    }

    void theColourStaysOnTheTabThroughAMove()
    {
        TabBar bar;
        bar.resize(900, 39);
        populate(bar, 3);
        bar.show();
        bar.setColor(QStringLiteral("t0"), QColor(QStringLiteral("#43b374")));
        bar.setAudio(QStringLiteral("t0"), TabBar::Audio::Playing);
        const QPoint start = bar.tabRect(0).center();
        const QPoint end = bar.tabRect(2).center();
        QTest::mousePress(&bar, Qt::LeftButton, Qt::NoModifier, start);
        QTest::mouseMove(&bar, start + QPoint(10, 0));
        QTest::mouseMove(&bar, end);
        QTest::mouseRelease(&bar, Qt::LeftButton, Qt::NoModifier, end);
        QVERIFY(bar.indexOfKey(QStringLiteral("t0")) > 0);
        QCOMPARE(bar.color(QStringLiteral("t0")), QColor(QStringLiteral("#43b374")));
        QCOMPARE(bar.audio(QStringLiteral("t0")), TabBar::Audio::Playing);
    }

    void aClosedTabsKeyOutlivesTheTab()
    {
        // The browser strip removes the tab, then closes its page by the key.
        // A key that referred into the bar named the neighbour after the
        // removal, or freed memory for the first and the last tab.
        const auto closeEach = [](const std::function<void(TabBar&, int)>& gesture) {
            QStringList closed;
            for (int index = 0; index < 3; ++index) {
                TabBar bar;
                bar.resize(900, 39);
                populate(bar, 3);
                bar.show();
                bar.setCurrentKey(bar.keyAt(index));
                connect(&bar, &TabBar::closeRequested, &bar, [&bar, &closed](const QString& key) {
                    bar.removeTab(key);
                    closed << key;
                });
                gesture(bar, index);
            }
            return closed;
        };
        const QStringList all{QStringLiteral("t0"), QStringLiteral("t1"), QStringLiteral("t2")};

        QCOMPARE(closeEach([](TabBar& bar, int index) {
                     QTest::mouseClick(&bar, Qt::MiddleButton, Qt::NoModifier,
                                       bar.tabRect(index).center());
                 }), all);
        QCOMPARE(closeEach([](TabBar& bar, int index) {
                     QTest::mouseClick(&bar, Qt::LeftButton, Qt::NoModifier,
                                       bar.closeRect(index).center());
                 }), all);
        QCOMPARE(closeEach([](TabBar& bar, int) {
                     bar.setFocus();
                     QTest::keyClick(&bar, Qt::Key_W, Qt::ControlModifier);
                 }), all);
    }
};

QTEST_MAIN(TabBarBrowserWorkflowTest)
#include "tst_tabbar_browser_workflow.moc"
