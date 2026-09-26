#include "novu/ui/WidgetEditor.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/Text.h"
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QWidget>
namespace novu::ui {
namespace {
class Grid final : public QWidget {
public:
    explicit Grid(QWidget* p):QWidget(p){setAttribute(Qt::WA_TransparentForMouseEvents);hide();}
    void paintEvent(QPaintEvent*)override{QPainter p(this);auto color=themeFor(this).colors.accent;color.setAlphaF(.09);p.setPen(color);for(int x=0;x<width();x+=16)p.drawLine(x,0,x,height());for(int y=0;y<height();y+=16)p.drawLine(0,y,width(),y);}
};
class Outline final : public QWidget {
public:
    QString label;
    QSize targetSize;
    QFont labelFont()const{auto f=roleMonoFont(this,TypeRole::Mono);f.setPointSizeF(9.5*72/logicalDpiY());f.setWeight(QFont::DemiBold);f.setLetterSpacing(QFont::AbsoluteSpacing,.19);return f;}
    explicit Outline(QWidget* parent):QWidget(parent){setAttribute(Qt::WA_TransparentForMouseEvents);hide();}
    void paintEvent(QPaintEvent*)override{
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
        const auto& c=themeFor(this).colors;const QRectF box(4,26,targetSize.width(),targetSize.height());
        QColor glow=c.accent;glow.setAlphaF(.13);p.setPen(QPen(glow,7));p.setBrush(Qt::NoBrush);p.drawRoundedRect(box,3,3);
        p.setPen(QPen(c.accent,1.5));p.drawRoundedRect(box.adjusted(-1,-1,1,1),3,3);
        p.setFont(labelFont());
        QRect tag(3,1,p.fontMetrics().horizontalAdvance(label)+14,18);
        p.setPen(Qt::NoPen);p.setBrush(c.accent);p.drawRoundedRect(tag,themeFor(this).radii.small,themeFor(this).radii.small);
        p.setPen(c.textOnAccent);p.drawText(tag.adjusted(7,0,-7,0),Qt::AlignVCenter,label);
    }
};
class DropMark final : public QWidget {
public:
    bool area=false;
    explicit DropMark(QWidget* parent):QWidget(parent){setAttribute(Qt::WA_TransparentForMouseEvents);hide();}
    void paintEvent(QPaintEvent*)override{
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);auto c=themeFor(this).colors.accent;
        if(area){auto fill=c;fill.setAlphaF(.12);p.setBrush(fill);p.setPen(QPen(c,1.5,Qt::DashLine));p.drawRoundedRect(QRectF(rect()).adjusted(1,1,-1,-1),themeFor(this).radii.small,themeFor(this).radii.small);}
        else {auto fill=c;fill.setAlphaF(.22);p.fillRect(rect(),fill);p.fillRect(rect().adjusted(3,3,-3,-3),c);}
    }
};
class DragLabel final : public QWidget {
public:
    QString label;bool valid=false;
    explicit DragLabel(QWidget* parent):QWidget(parent){setAttribute(Qt::WA_TransparentForMouseEvents);hide();}
    void refresh(){setFont(roleFont(this,TypeRole::Caption));setAccessibleName(label+(valid?QString():QStringLiteral(" / no drop here")));resize(QFontMetrics(font()).horizontalAdvance(accessibleName())+20,27);update();}
    void paintEvent(QPaintEvent*)override{
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);const auto& c=themeFor(this).colors;
        p.setPen(valid?c.accent:inkColor(this,Ink::Danger));p.setBrush(c.surfaceHover);p.drawRoundedRect(QRectF(rect()).adjusted(.5,.5,-.5,-.5),themeFor(this).radii.small,themeFor(this).radii.small);
        p.setFont(font());p.setPen(valid?c.textPrimary:c.textTertiary);p.drawText(rect().adjusted(10,0,-10,0),Qt::AlignVCenter,accessibleName());
    }
};
class Handle final : public QWidget {
public:
    explicit Handle(QWidget* parent):QWidget(parent){setFixedSize(8,8);hide();}
    void paintEvent(QPaintEvent*)override{QPainter p(this);p.setRenderHint(QPainter::Antialiasing);p.setPen(QPen(themeFor(this).colors.windowBg,1.5));p.setBrush(themeFor(this).colors.accent);p.drawRoundedRect(QRectF(rect()).adjusted(.75,.75,-.75,-.75),2,2);}
};
}
WidgetEditor::WidgetEditor(QWidget* stage):QObject(stage),m_stage(stage){
    m_grid=new Grid(stage);m_grid->lower();
    m_drop=new DropMark(stage);m_drop->setObjectName("widgetEditor.drop");
    m_ghost=new DragLabel(stage);m_ghost->setObjectName("widgetEditor.dragLabel");
    m_overlay=new Outline(stage);m_overlay->setObjectName("widgetEditor.selection");
    for(int i=0;i<3;++i){m_handles[i]=new Handle(stage);m_handles[i]->setFixedSize(i==0?QSize(7,16):i==1?QSize(16,7):QSize(8,8));m_handles[i]->setObjectName(i==0?"widgetEditor.resize.w":i==1?"widgetEditor.resize.h":"widgetEditor.resize.wh");m_handles[i]->setCursor(i==0?Qt::SizeHorCursor:i==1?Qt::SizeVerCursor:Qt::SizeFDiagCursor);}
    qApp->installEventFilter(this);
}
void WidgetEditor::registerWidget(QWidget* widget,const QString& label,bool movable,bool resizable){
    if(!widget||!m_stage||!m_stage->isAncestorOf(widget))return;
    m_entries.insert(widget,{label,movable,resizable});
    connect(widget,&QObject::destroyed,this,[this,widget]{m_entries.remove(widget);QTimer::singleShot(0,this,[this]{syncGeometry();});});
}
void WidgetEditor::setCurrentWidget(QWidget* widget){m_current=m_entries.contains(widget)?widget:nullptr;syncGeometry();}
QWidget* WidgetEditor::currentWidget()const{return m_current;}
void WidgetEditor::setSnapStep(int pixels){m_snap=qMax(1,pixels);}
void WidgetEditor::setGridVisible(bool visible){m_grid->setVisible(visible);m_grid->lower();syncGeometry();}
void WidgetEditor::setDropFeedback(const QRect& bounds,bool area){
    auto* mark=static_cast<DropMark*>(m_drop);mark->area=area;
    mark->setGeometry(area?bounds:bounds.adjusted(-3,-3,3,3));mark->setVisible(m_dragging&&!bounds.isEmpty());mark->raise();mark->update();
    auto* ghost=static_cast<DragLabel*>(m_ghost);ghost->valid=!bounds.isEmpty();ghost->refresh();ghost->raise();
}
void WidgetEditor::syncGeometry(){
    if(m_stage)m_grid->setGeometry(m_stage->rect());
    const bool visible=m_current&&m_stage&&m_current->isVisibleTo(m_stage);
    m_overlay->setVisible(visible);for(auto* h:m_handles)h->setVisible(visible&&m_entries.value(m_current).resizable);
    if(!visible)return;
    const QPoint pos=m_current->mapTo(m_stage,QPoint());const QSize size=m_current->size();
    auto* outline=static_cast<Outline*>(m_overlay);outline->targetSize=size;
    const bool hugs=m_current->minimumWidth()!=m_current->maximumWidth()&&m_current->minimumHeight()!=m_current->maximumHeight();
    outline->label=m_entries.value(m_current).label+"  "+QString::number(size.width())+QString::fromUtf8(" × ")+QString::number(size.height())+(hugs?"  hug":"");
    outline->setAccessibleName(outline->label);
    const int labelWidth=QFontMetrics(outline->labelFont(),outline).horizontalAdvance(outline->label)+17;
    outline->setGeometry(pos.x()-4,pos.y()-26,qMax(size.width()+8,labelWidth),size.height()+30);outline->raise();outline->update();
    const QPoint corners[3]={QPoint(size.width(),size.height()/2),QPoint(size.width()/2,size.height()),QPoint(size.width(),size.height())};
    for(int i=0;i<3;++i){m_handles[i]->move(pos+corners[i]-QPoint(m_handles[i]->width()/2,m_handles[i]->height()/2));m_handles[i]->raise();}
}
bool WidgetEditor::eventFilter(QObject* watched,QEvent* event){
    auto* widget=qobject_cast<QWidget*>(watched);if(!widget||!m_stage)return false;
    if(event->type()==QEvent::Resize||event->type()==QEvent::Move||event->type()==QEvent::Show||event->type()==QEvent::Hide){
        if(widget==m_stage||m_entries.contains(widget))QTimer::singleShot(0,this,[this]{syncGeometry();});
        return false;
    }
    if(event->type()!=QEvent::MouseButtonPress&&event->type()!=QEvent::MouseMove&&event->type()!=QEvent::MouseButtonRelease)return false;
    auto* e=static_cast<QMouseEvent*>(event);
    if(event->type()==QEvent::MouseButtonPress){
        if(e->button()!=Qt::LeftButton)return false;
        m_axes={};
        for(int i=0;i<3;++i)if(widget==m_handles[i])m_axes=i==0?Qt::Orientations(Qt::Horizontal):i==1?Qt::Orientations(Qt::Vertical):Qt::Orientations(Qt::Horizontal|Qt::Vertical);
        if(m_axes&&m_current)m_pressed=m_current;
        else{auto* target=widget;while(target&&target!=m_stage&&!m_entries.contains(target))target=target->parentWidget();if(!m_entries.contains(target))return false;m_pressed=target;}
        m_origin=e->globalPosition().toPoint();m_originalSize=m_pressed->size();m_dragging=false;return true;
    }
    if(!m_pressed)return false;
    const QPoint delta=e->globalPosition().toPoint()-m_origin;
    if(event->type()==QEvent::MouseMove){
        if(!(e->buttons()&Qt::LeftButton))return false;
        if(m_axes){QSize size=m_originalSize;auto snap=[this](int n){return qMax(16,qRound(n/double(m_snap))*m_snap);};if(m_axes.testFlag(Qt::Horizontal)){size.setWidth(snap(size.width()+delta.x()));m_pressed->setFixedWidth(size.width());}if(m_axes.testFlag(Qt::Vertical)){size.setHeight(snap(size.height()+delta.y()));m_pressed->setFixedHeight(size.height());}syncGeometry();emit sizeEdited(m_pressed,size,m_axes,false);}
        else if(m_entries.value(m_pressed).movable&&(m_dragging||qAbs(delta.x())>=4||qAbs(delta.y())>=4)){
            m_dragging=true;auto* ghost=static_cast<DragLabel*>(m_ghost);ghost->label=m_entries.value(m_pressed).label;
            ghost->move(m_stage->mapFromGlobal(e->globalPosition().toPoint())+QPoint(13,13));ghost->show();setDropFeedback({});
            emit dragMoved(m_pressed,e->globalPosition().toPoint());ghost->raise();
        }
        return true;
    }
    if(e->button()!=Qt::LeftButton)return false;
    m_drop->hide();m_ghost->hide();
    QPointer<QWidget> target=m_pressed;m_pressed=nullptr;
    if(m_axes){auto axes=m_axes;m_axes={};emit sizeEdited(target,target->size(),axes,true);}
    else if(m_dragging){m_dragging=false;emit dropRequested(target,e->globalPosition().toPoint());}
    else {setCurrentWidget(target);emit picked(target);}
    return true;
}
}
