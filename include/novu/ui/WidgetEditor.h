#pragma once

#include "novu/ui/Export.h"
#include <QObject>
#include <QPointer>
#include <QHash>
#include <QPoint>
#include <QSize>
#include <QRect>
class QWidget;
namespace novu::ui {
// Edits constraints and selection around real widgets without wrapping them
// or changing their layouts. The owning document decides where a drop belongs.
class NOVUUI_API WidgetEditor : public QObject {
    Q_OBJECT
public:
    explicit WidgetEditor(QWidget* stage);
    void registerWidget(QWidget* widget,const QString& label,bool movable=true,bool resizable=true);
    void setCurrentWidget(QWidget* widget);
    QWidget* currentWidget() const;
    void setSnapStep(int pixels);
    void setGridVisible(bool visible);
    void syncGeometry();
    // Stage coordinates; an empty rectangle declares an invalid drop.
    void setDropFeedback(const QRect& bounds, bool area=false);
signals:
    void picked(QWidget* widget);
    void sizeEdited(QWidget* widget,QSize size,Qt::Orientations axes,bool finished);
    void dragMoved(QWidget* widget,QPoint globalPosition);
    void dropRequested(QWidget* widget,QPoint globalPosition);
protected:
    bool eventFilter(QObject*,QEvent*) override;
private:
    struct Entry { QString label;bool movable=true,resizable=true; };
    QPointer<QWidget> m_stage,m_current,m_pressed;
    QWidget* m_overlay=nullptr;
    QWidget* m_grid=nullptr;
    QWidget* m_drop=nullptr;
    QWidget* m_ghost=nullptr;
    QWidget* m_handles[3]{};
    QHash<QWidget*,Entry> m_entries;
    QPoint m_origin;
    QSize m_originalSize;
    Qt::Orientations m_axes;
    bool m_dragging=false;
    int m_snap=1;
};
}
