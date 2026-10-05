// Two things a composer needs from these controls.
//
// A select is as wide as its WIDEST option, not as the one it happens to be
// showing. Sized to the current label, a list of "Minimal ... Maximum Detail"
// was as wide as "Short" and the long answers came out as "Sh..." - and the
// control also changed width every time the choice changed, moving everything
// beside it.
//
// And a menu that stays open when a row is picked is a SET: its chosen rows
// carry a mark. Being told apart only by the colour of their text is enough
// for one-in-force and not enough for any-number-on.

#include "novu/ui/Popover.h"

#include <QFontMetrics>
#include <QTest>

using namespace novu::ui;

class SelectAndSetMenu : public QObject
{
    Q_OBJECT

private slots:
    void selectFitsItsLongestOption()
    {
        SelectButton select;
        select.addOption(QStringLiteral("minimal"), QStringLiteral("Minimal"));
        select.addOption(QStringLiteral("veryshort"), QStringLiteral("Very Short"));
        select.addOption(QStringLiteral("short"), QStringLiteral("Short"));
        select.addOption(QStringLiteral("normal"), QStringLiteral("Normal"));
        select.addOption(QStringLiteral("detailed"), QStringLiteral("Detailed"));
        select.addOption(QStringLiteral("max"), QStringLiteral("Maximum Detail"));

        // Standing on the shortest option must not make the control short.
        select.setCurrentKey(QStringLiteral("short"));
        const int onShort = select.sizeHint().width();

        select.setCurrentKey(QStringLiteral("max"));
        const int onLongest = select.sizeHint().width();

        QCOMPARE(onShort, onLongest);

        // And it is genuinely wide enough for the longest label.
        const int longest = QFontMetrics(select.font())
                                .horizontalAdvance(QStringLiteral("Maximum Detail"));
        QVERIFY2(onShort > longest,
                 "the select is narrower than its longest option's text");
    }

    void aSetMarksWhatIsChosen()
    {
        Popover menu;
        menu.setClosesOnTrigger(false);   // a set, not a choice
        menu.addItem(QStringLiteral("a"), QStringLiteral("Workspace 25"));
        menu.addItem(QStringLiteral("b"), QStringLiteral("Houdini_AI_Agent"));
        menu.addItem(QStringLiteral("c"), QStringLiteral("NovuAI"));
        const int plain = menu.sizeHint().width();

        Popover choice;
        choice.setClosesOnTrigger(true);   // one in force
        choice.addItem(QStringLiteral("a"), QStringLiteral("Workspace 25"));
        choice.addItem(QStringLiteral("b"), QStringLiteral("Houdini_AI_Agent"));
        choice.addItem(QStringLiteral("c"), QStringLiteral("NovuAI"));

        // The set keeps a column for the mark so its labels stay on one axis
        // whatever is ticked; a menu of commands does not pay for one.
        QVERIFY2(plain > choice.sizeHint().width(),
                 "a set menu reserves no room for its marks");

        // Checking a row must not move the labels.
        menu.setChecked(QStringLiteral("a"), true);
        menu.setChecked(QStringLiteral("c"), true);
        QCOMPARE(menu.sizeHint().width(), plain);
    }
};

QTEST_MAIN(SelectAndSetMenu)
#include "tst_select_and_set_menu.moc"
