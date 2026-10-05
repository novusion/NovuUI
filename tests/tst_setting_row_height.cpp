// A settings row is as tall as the control it carries, not as tall as the
// control asked to be.
//
// The bug this pins: QPlainTextEdit is a scroll area, so its size hint is
// Qt's generous default however few lines it is meant to show, and
// setFixedHeight() does not change that hint - it sets the widget's minimum
// and maximum. A row that measured itself from the hint was therefore far
// taller than its own field, which sat at the top of it, and the leftover
// read as an unexplained gap before the next row.

#include "novu/ui/SettingRow.h"
#include "novu/ui/Settings.h"

#include <QPlainTextEdit>
#include <QLineEdit>
#include <QTest>

using namespace novu::ui;

class SettingRowHeight : public QObject
{
    Q_OBJECT

private slots:
    // The premise. If this ever stops holding, the row maths below is
    // measuring nothing and the test is worthless rather than passing.
    void plainTextEditAsksForMoreThanItIsGiven()
    {
        QPlainTextEdit field;
        field.setFixedHeight(64);
        QVERIFY2(field.sizeHint().height() > 64,
                 "premise gone: QPlainTextEdit no longer over-asks, so this "
                 "test no longer exercises the clamp");
        QCOMPARE(field.maximumHeight(), 64);
    }

    void rowFollowsTheControlsMaximum()
    {
        SettingRow row(QStringLiteral("Description"), QString());
        row.resize(520, 10);

        auto* field = new QPlainTextEdit(&row);
        field->setFixedHeight(64);
        row.setControl(field);

        const int hinted = field->sizeHint().height();
        const int measured = row.heightForWidth(520);

        QVERIFY2(measured < hinted,
                 "the row is still being measured from the control's hint");
        // The row is the field plus its own padding, and nothing else.
        QVERIFY(measured >= 64);
        QVERIFY2(measured - 64 < 40, "padding has turned into a gap");

        // And the field is drawn at the height the row reserved for it, so
        // there is no empty band underneath it.
        row.resize(520, measured);
        QCOMPARE(field->height(), 64);
        QVERIFY(field->geometry().bottom() <= row.height());
        QVERIFY2(row.height() - field->geometry().bottom() < 40,
                 "empty space left below the control");
    }

    // A section of rows is the sum of its rows. With the Description row
    // over-measured it was not, and the surplus landed between two rows.
    void sectionIsNoTallerThanItsRows()
    {
        SettingsSection section(QStringLiteral("Identity"), QString());
        section.resize(520, 10);

        SettingRow* name = section.addRow(QStringLiteral("Name"));
        name->setControl(new QLineEdit(name));

        SettingRow* description = section.addRow(QStringLiteral("Description"));
        auto* field = new QPlainTextEdit(description);
        field->setFixedHeight(64);
        description->setControl(field);

        SettingRow* technology = section.addRow(QStringLiteral("Technology"));
        technology->setControl(new QLineEdit(technology));

        section.resize(520, section.sizeHint().height());
        section.show();
        QCoreApplication::processEvents();

        // The gap that was reported: between the Description field and the
        // row under it.
        const int fieldBottom = field->mapTo(&section, QPoint(0, field->height())).y();
        const int technologyTop = technology->mapTo(&section, QPoint(0, 0)).y();
        QVERIFY2(technologyTop - fieldBottom < 40,
                 qPrintable(QStringLiteral("gap of %1px between Description and "
                                           "the next row")
                                .arg(technologyTop - fieldBottom)));
    }
};

QTEST_MAIN(SettingRowHeight)
#include "tst_setting_row_height.moc"
