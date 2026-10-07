// A switch row is as tall as the hint under it, and its switch stays clickable.
//
// The bug this pins, from NovuAI's Share workspace form: three toggleRow()s
// with two-line hints drew on top of each other. The second line of the first
// hint was clipped and painted over the row below it - and because a QLabel
// sitting over a control eats the press, the switch underneath could not be
// turned on at all. One cause, two symptoms, and the second one reads as
// "the feature is not wired up" rather than as a layout fault.
//
// The row is built the way callers build it: a QVBoxLayout in a container of
// a realistic width, with a hint long enough to wrap.

#include "novu/ui/Card.h"
#include "novu/ui/Toggle.h"

#include <QLabel>
#include <QScrollArea>
#include <QTest>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace {

// The width a form column actually gets, narrow enough that these hints wrap.
constexpr int kColumnWidth = 320;

const char* kLongHint =
    "Claude Code and Codex installed here - your subscription and your rate limits.";

}

class ToggleRowHeightTest : public QObject
{
    Q_OBJECT

private slots:
    void aWrappedHintIsInsideItsRow();
    void rowsDoNotOverlap();
    void theSwitchIsOnTopAtItsOwnPosition();
    void aRowWithoutAHintIsStillSound();
    void insideACardInAScrollArea();

private:
    // Three rows in a column, laid out at a real width.
    static QWidget* buildForm(QVector<Toggle*>* toggles)
    {
        auto* host = new QWidget;
        auto* column = new QVBoxLayout(host);
        column->setContentsMargins(0, 0, 0, 0);
        column->setSpacing(4);
        for (int i = 0; i < 3; ++i) {
            *toggles << toggleRow(column, host,
                                  QStringLiteral("Remote CLI agents %1").arg(i),
                                  QString::fromLatin1(kLongHint));
        }
        host->resize(kColumnWidth, host->sizeHint().height());
        host->layout()->activate();
        // Lay out at the height the content asks for once the width is known,
        // which is what a scroll area with a resizable widget does.
        host->resize(kColumnWidth, host->layout()->sizeHint().height());
        host->layout()->activate();
        return host;
    }
};

void ToggleRowHeightTest::aWrappedHintIsInsideItsRow()
{
    QVector<Toggle*> toggles;
    QScopedPointer<QWidget> host(buildForm(&toggles));
    QCOMPARE(toggles.size(), 3);

    // Every label the form made, with the hint ones identified by their text.
    const auto labels = host->findChildren<QLabel*>();
    int wrapped = 0;
    for (QLabel* note : labels) {
        if (note->text() != QString::fromLatin1(kLongHint)) {
            continue;
        }
        ++wrapped;
        QVERIFY2(note->width() > 0, "the hint was never given a width");
        const int needed = note->heightForWidth(note->width());
        QVERIFY2(note->height() >= needed,
                 qPrintable(QStringLiteral("hint is %1px tall and needs %2px at %3px wide")
                                .arg(note->height()).arg(needed).arg(note->width())));
        QVERIFY2(note->geometry().bottom() <= host->height(),
                 "the hint runs past the bottom of the form");
    }
    QCOMPARE(wrapped, 3);
}

void ToggleRowHeightTest::rowsDoNotOverlap()
{
    QVector<Toggle*> toggles;
    QScopedPointer<QWidget> host(buildForm(&toggles));

    // A switch must not sit inside the band another row's text occupies: that
    // is exactly the overlap that swallowed the click.
    for (int i = 0; i < toggles.size(); ++i) {
        const QRect own = toggles.at(i)->geometry();
        for (int j = 0; j < toggles.size(); ++j) {
            if (i == j) {
                continue;
            }
            QVERIFY2(!own.intersects(toggles.at(j)->geometry()),
                     "two switches overlap");
        }
    }

    const auto labels = host->findChildren<QLabel*>();
    for (QLabel* note : labels) {
        if (note->text() != QString::fromLatin1(kLongHint)) {
            continue;
        }
        for (Toggle* toggle : toggles) {
            // Vertical bands only: the switch is beside the text, not under
            // it, so sharing rows is expected and sharing a BAND is not.
            const bool sameBand = note->geometry().top() < toggle->geometry().bottom()
                                  && toggle->geometry().top() < note->geometry().bottom();
            if (!sameBand) {
                continue;
            }
            QVERIFY2(!note->geometry().intersects(toggle->geometry()),
                     "a hint overlaps a switch - the press would land on the label");
        }
    }
}

void ToggleRowHeightTest::theSwitchIsOnTopAtItsOwnPosition()
{
    QVector<Toggle*> toggles;
    QScopedPointer<QWidget> host(buildForm(&toggles));

    // What a click actually hits. childAt() answers the same question the
    // event system does, so this fails if anything is drawn over the switch.
    for (Toggle* toggle : toggles) {
        const QPoint centre = toggle->geometry().center();
        QWidget* hit = host->childAt(centre);
        QVERIFY2(hit == toggle,
                 qPrintable(QStringLiteral("a click at the switch lands on %1")
                                .arg(hit ? hit->metaObject()->className()
                                         : QStringLiteral("nothing"))));
    }
}

void ToggleRowHeightTest::aRowWithoutAHintIsStillSound()
{
    auto* host = new QWidget;
    QScopedPointer<QWidget> guard(host);
    auto* column = new QVBoxLayout(host);
    Toggle* toggle = toggleRow(column, host, QStringLiteral("Local models"), QString());
    host->resize(kColumnWidth, host->sizeHint().height());
    host->layout()->activate();

    QVERIFY(toggle->height() > 0);
    QVERIFY(toggle->isCheckable());
    QVERIFY(host->childAt(toggle->geometry().center()) == toggle);
}

// The chain the Share form actually uses: a scroll area with a resizable
// widget, a page, a Card, and the rows inside the Card's body. The rows pass
// on their own - this is where the overlap in the screenshot came from, so
// the container is part of the case under test, not scenery.
void ToggleRowHeightTest::insideACardInAScrollArea()
{
    auto* page = new QWidget;
    auto* column = new QVBoxLayout(page);
    column->setContentsMargins(12, 12, 12, 12);

    auto* card = new Card(QStringLiteral("People"), page);
    auto* box = new QWidget(card);
    auto* rows = new QVBoxLayout(box);
    rows->setContentsMargins(0, 0, 0, 0);
    rows->setSpacing(4);

    QVector<Toggle*> toggles;
    for (int i = 0; i < 3; ++i) {
        toggles << toggleRow(rows, box, QStringLiteral("Remote CLI agents %1").arg(i),
                             QString::fromLatin1(kLongHint));
    }
    card->body()->addWidget(box);
    column->addWidget(card);
    column->addStretch(1);

    QScrollArea area;
    area.setWidgetResizable(true);
    area.setWidget(page);
    area.resize(kColumnWidth + 40, 420);
    area.show();
    QVERIFY(QTest::qWaitForWindowExposed(&area));
    QCoreApplication::processEvents();

    for (Toggle* toggle : toggles) {
        QVERIFY2(toggle->height() > 0, "a switch was given no height");
        const QPoint centre = toggle->mapTo(page, toggle->rect().center());
        QWidget* hit = page->childAt(centre);
        QVERIFY2(hit == toggle,
                 qPrintable(QStringLiteral("inside a Card, a click at the switch lands on %1")
                                .arg(hit ? hit->metaObject()->className()
                                         : QStringLiteral("nothing"))));
    }

    const auto labels = page->findChildren<QLabel*>();
    for (QLabel* note : labels) {
        if (note->text() != QString::fromLatin1(kLongHint)) {
            continue;
        }
        const int needed = note->heightForWidth(note->width());
        QVERIFY2(note->height() >= needed,
                 qPrintable(QStringLiteral("in a Card the hint is %1px and needs %2px at %3px")
                                .arg(note->height()).arg(needed).arg(note->width())));
    }
}

QTEST_MAIN(ToggleRowHeightTest)
#include "tst_toggle_row_height.moc"
