#include "novu/ui/WidgetEditor.h"
#include <QTest>
#include <QPushButton>
#include <QSignalSpy>
#include <QMouseEvent>
using namespace novu::ui;
class WidgetEditorTest:public QObject{
    Q_OBJECT
private slots:
    void selectionLabelCanExtendBeyondANarrowWidget(){
        QWidget stage;stage.resize(400,200);QPushButton button("Run",&stage);button.setGeometry(40,50,50,24);
        WidgetEditor editor(&stage);editor.registerWidget(&button,"Long component name");stage.show();editor.setCurrentWidget(&button);QCoreApplication::processEvents();
        auto* overlay=stage.findChild<QWidget*>("widgetEditor.selection");QVERIFY(overlay);QVERIFY(overlay->width()>button.width()+8);QVERIFY(overlay->accessibleName().endsWith("hug"));
        QCOMPARE(stage.findChild<QWidget*>("widgetEditor.resize.w")->size(),QSize(7,16));
        QCOMPARE(stage.findChild<QWidget*>("widgetEditor.resize.h")->size(),QSize(16,7));
        button.setFixedWidth(50);editor.syncGeometry();QVERIFY(!overlay->accessibleName().endsWith("hug"));
    }

    void pickingPreservesTheLayout(){
        QWidget stage;stage.resize(400,200);QPushButton button("Run",&stage);button.setGeometry(40,50,100,28);
        WidgetEditor editor(&stage);editor.registerWidget(&button,"Run");QSignalSpy picked(&editor,&WidgetEditor::picked);QSignalSpy clicked(&button,&QPushButton::clicked);
        stage.show();QCoreApplication::processEvents();const QRect before=button.geometry();
        QTest::mouseClick(&button,Qt::LeftButton);
        QCOMPARE(editor.currentWidget(),&button);QCOMPARE(picked.count(),1);QCOMPARE(clicked.count(),0);QCOMPARE(button.geometry(),before);
    }
    void handleChangesOnlyItsAxisAndSnaps(){
        QWidget stage;stage.resize(400,200);QPushButton button("Run",&stage);button.setGeometry(40,50,100,28);
        WidgetEditor editor(&stage);editor.registerWidget(&button,"Run");editor.setSnapStep(8);stage.show();editor.setCurrentWidget(&button);QCoreApplication::processEvents();
        auto* handle=stage.findChild<QWidget*>("widgetEditor.resize.w");QVERIFY(handle&&handle->isVisible());QSignalSpy resized(&editor,&WidgetEditor::sizeEdited);
        const QPointF from=handle->mapToGlobal(QPoint(4,4));
        QMouseEvent press(QEvent::MouseButtonPress,QPointF(4,4),from,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(handle,&press);
        QMouseEvent move(QEvent::MouseMove,QPointF(17,4),from+QPointF(13,0),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(handle,&move);
        QMouseEvent up(QEvent::MouseButtonRelease,QPointF(4,4),from+QPointF(13,0),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(handle,&up);
        QCOMPARE(button.width(),112);QCOMPARE(button.height(),28);QCOMPARE(resized.count(),2);QCOMPARE(resized.last().last().toBool(),true);
    }
    void dragFeedbackShowsDocumentTargetAndClearsOnRelease(){
        QWidget stage;stage.resize(400,200);QPushButton button("Run",&stage);button.setGeometry(40,50,100,28);
        WidgetEditor editor(&stage);editor.registerWidget(&button,"Run");stage.show();
        QObject::connect(&editor,&WidgetEditor::dragMoved,&stage,[&](QWidget*,QPoint global){editor.setDropFeedback(global.x()>button.mapToGlobal(QPoint(60,0)).x()?QRect(220,50,90,80):QRect(),true);});
        QTest::mousePress(&button,Qt::LeftButton,Qt::NoModifier,QPoint(20,12));
        auto move=[&](int x){QMouseEvent e(QEvent::MouseMove,QPointF(x,32),button.mapToGlobal(QPoint(x,32)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&button,&e);};
        move(50);auto* ghost=stage.findChild<QWidget*>("widgetEditor.dragLabel");auto* marker=stage.findChild<QWidget*>("widgetEditor.drop");
        QVERIFY(ghost->isVisible());QVERIFY(ghost->accessibleName().contains("no drop here"));QVERIFY(!marker->isVisible());
        move(90);QVERIFY(marker->isVisible());QCOMPARE(marker->geometry(),QRect(220,50,90,80));QVERIFY(!ghost->accessibleName().contains("no drop here"));
        QTest::mouseRelease(&button,Qt::LeftButton,Qt::NoModifier,QPoint(90,32));QVERIFY(!ghost->isVisible());QVERIFY(!marker->isVisible());
    }
    void droppingDoesNotReparentOrPositionWidgets(){
        QWidget stage;stage.resize(400,200);QPushButton button("Run",&stage);button.setGeometry(40,50,100,28);
        WidgetEditor editor(&stage);editor.registerWidget(&button,"Run");stage.show();QSignalSpy drop(&editor,&WidgetEditor::dropRequested);
        QTest::mousePress(&button,Qt::LeftButton,Qt::NoModifier,QPoint(20,12));
        QMouseEvent move(QEvent::MouseMove,QPointF(50,32),button.mapToGlobal(QPoint(50,32)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&button,&move);
        QTest::mouseRelease(&button,Qt::LeftButton,Qt::NoModifier,QPoint(50,32));
        QCOMPARE(drop.count(),1);QCOMPARE(button.parentWidget(),&stage);QCOMPARE(button.pos(),QPoint(40,50));
    }
};
QTEST_MAIN(WidgetEditorTest)
#include "tst_widget_editor.moc"
