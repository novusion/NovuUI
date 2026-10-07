// NavList inline editing: the contract a module relies on to rename a rail row.
//
// Every product rail is one NavList, and a row is renamed in place by laying
// an editor over its label. What this pins is the behaviour a caller depends on,
// not the drawing:
//
//   Enter submits, Escape abandons, and focus leaving submits;
//   a caller that refuses the text keeps the editor open on Enter, and the
//   refusal turns into an abandon when focus leaves, so a field never holds
//   focus hostage;
//   the editor survives the rail being rebuilt from data - which a shell does
//   on every chrome change - and goes away when its row does.

#include "novu/ui/NavList.h"

#include <QFocusEvent>
#include <QLineEdit>
#include <QSignalSpy>
#include <QTest>

using namespace novu::ui;

namespace {

QLineEdit* editorOf(NavList& list)
{
    return list.findChild<QLineEdit*>(QStringLiteral("novuNavInlineEditor"));
}

void populate(NavList& list, bool withNewRow)
{
    list.clear();
    list.addSection(QStringLiteral("Collections"), QStringLiteral("sec:collections"));
    list.addItem(QStringLiteral("vl:personal"), QStringLiteral("Personal"));
    if (withNewRow) {
        list.addItem(QStringLiteral("vl:__new__"), QStringLiteral("New collection"));
    }
}

void pressKey(QLineEdit* editor, Qt::Key key)
{
    QTest::keyClick(editor, key);
}

} // namespace

class NavListInlineEditTest : public QObject
{
    Q_OBJECT

private slots:
    void refusesAMissingRow()
    {
        NavList list;
        populate(list, false);
        QVERIFY(!list.beginInlineEdit(QStringLiteral("vl:nope")));
        QVERIFY(!list.isInlineEditing());
    }

    void opensOverTheRowWithItsLabelSelected()
    {
        NavList list;
        list.resize(238, 300);
        populate(list, true);
        list.show();
        QVERIFY(list.beginInlineEdit(QStringLiteral("vl:__new__")));
        QLineEdit* editor = editorOf(list);
        QVERIFY(editor);
        QVERIFY(editor->isVisible());
        QCOMPARE(editor->text(), QStringLiteral("New collection"));
        // Selected, so typing replaces the placeholder name in one gesture.
        QCOMPARE(editor->selectedText(), QStringLiteral("New collection"));
        QCOMPARE(list.inlineEditKey(), QStringLiteral("vl:__new__"));
    }

    void enterSubmitsAndCloses()
    {
        NavList list;
        list.resize(238, 300);
        populate(list, true);
        list.show();
        QSignalSpy committed(&list, &NavList::inlineEditCommitted);
        QSignalSpy cancelled(&list, &NavList::inlineEditCancelled);
        list.beginInlineEdit(QStringLiteral("vl:__new__"));
        QLineEdit* editor = editorOf(list);
        editor->setText(QStringLiteral("Development"));
        pressKey(editor, Qt::Key_Return);
        QCOMPARE(committed.count(), 1);
        QCOMPARE(committed.at(0).at(0).toString(), QStringLiteral("vl:__new__"));
        QCOMPARE(committed.at(0).at(1).toString(), QStringLiteral("Development"));
        QCOMPARE(cancelled.count(), 0);
        QVERIFY(!list.isInlineEditing());
        QVERIFY(!editor->isVisible());
    }

    void escapeAbandons()
    {
        NavList list;
        list.resize(238, 300);
        populate(list, true);
        list.show();
        QSignalSpy committed(&list, &NavList::inlineEditCommitted);
        QSignalSpy cancelled(&list, &NavList::inlineEditCancelled);
        list.beginInlineEdit(QStringLiteral("vl:__new__"));
        pressKey(editorOf(list), Qt::Key_Escape);
        QCOMPARE(committed.count(), 0);
        QCOMPARE(cancelled.count(), 1);
        QCOMPARE(cancelled.at(0).at(0).toString(), QStringLiteral("vl:__new__"));
        QVERIFY(!list.isInlineEditing());
    }

    void aRefusalOnEnterKeepsTheEditorOpen()
    {
        NavList list;
        list.resize(238, 300);
        populate(list, true);
        list.show();
        connect(&list, &NavList::inlineEditCommitted, &list,
                [&list](const QString&, const QString&) {
                    list.rejectInlineEdit(QStringLiteral("A collection with this name already exists."));
                });
        QSignalSpy cancelled(&list, &NavList::inlineEditCancelled);
        list.beginInlineEdit(QStringLiteral("vl:__new__"));
        QLineEdit* editor = editorOf(list);
        editor->setText(QStringLiteral("Personal"));
        pressKey(editor, Qt::Key_Return);
        QVERIFY(list.isInlineEditing());
        QVERIFY(editor->isVisible());
        QCOMPARE(editor->text(), QStringLiteral("Personal"));
        QCOMPARE(editor->property("invalid").toBool(), true);
        QVERIFY(editor->toolTip().contains(QStringLiteral("already exists")));
        QCOMPARE(cancelled.count(), 0);
        // Typing again clears the mark.
        QTest::keyClick(editor, Qt::Key_X);
        QCOMPARE(editor->property("invalid").toBool(), false);
    }

    void focusLeavingSubmitsAndARefusalThenAbandons()
    {
        NavList list;
        list.resize(238, 300);
        populate(list, true);
        list.show();
        bool refuse = false;
        connect(&list, &NavList::inlineEditCommitted, &list,
                [&list, &refuse](const QString&, const QString&) {
                    if (refuse) {
                        list.rejectInlineEdit(QStringLiteral("no"));
                    }
                });
        QSignalSpy committed(&list, &NavList::inlineEditCommitted);
        QSignalSpy cancelled(&list, &NavList::inlineEditCancelled);

        list.beginInlineEdit(QStringLiteral("vl:__new__"));
        QFocusEvent out(QEvent::FocusOut, Qt::MouseFocusReason);
        QCoreApplication::sendEvent(editorOf(list), &out);
        QCOMPARE(committed.count(), 1);
        QCOMPARE(cancelled.count(), 0);
        QVERIFY(!list.isInlineEditing());

        refuse = true;
        list.beginInlineEdit(QStringLiteral("vl:__new__"));
        QCoreApplication::sendEvent(editorOf(list), &out);
        QCOMPARE(committed.count(), 2);
        QCOMPARE(cancelled.count(), 1);
        QVERIFY(!list.isInlineEditing());
    }

    void survivesARebuildAndEndsWhenItsRowDoes()
    {
        NavList list;
        list.resize(238, 300);
        populate(list, true);
        list.show();
        QSignalSpy cancelled(&list, &NavList::inlineEditCancelled);
        list.beginInlineEdit(QStringLiteral("vl:__new__"));
        QLineEdit* editor = editorOf(list);
        editor->setText(QStringLiteral("Dev"));

        // A shell rebuilds the rail from data on every chrome change.
        populate(list, true);
        QCoreApplication::processEvents();
        QVERIFY(list.isInlineEditing());
        QCOMPARE(editor->text(), QStringLiteral("Dev"));
        QCOMPARE(cancelled.count(), 0);

        // Rebuilt without the row: nothing left to rename.
        populate(list, false);
        QCoreApplication::processEvents();
        QVERIFY(!list.isInlineEditing());
        QCOMPARE(cancelled.count(), 1);
    }
};

QTEST_MAIN(NavListInlineEditTest)
#include "tst_navlist_inline_edit.moc"
