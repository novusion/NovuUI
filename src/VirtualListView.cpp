#include "novu/ui/VirtualListView.h"

#include <QAbstractItemView>

namespace novu::ui {

VirtualListView::VirtualListView(QWidget* parent)
    : QListView(parent)
{
    setObjectName(QStringLiteral("NovuVirtualListView"));
    setFrameShape(QFrame::NoFrame);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setUniformItemSizes(true);
    setLayoutMode(QListView::Batched);
    setBatchSize(64);
    setMouseTracking(true);
    setAutoScroll(true);
    setStyleSheet(QStringLiteral(
        "QListView#NovuVirtualListView { background: transparent; border: none; outline: none; }"));
}

} // namespace novu::ui
