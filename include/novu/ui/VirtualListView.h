#pragma once

#include <QListView>

namespace novu::ui {

// A model-backed, painted list for large homogeneous datasets.
//
// The view never installs index widgets. Rows are represented exclusively by
// the caller's QAbstractItemModel and QAbstractItemDelegate, so the QWidget
// count stays constant as the model grows.
class VirtualListView : public QListView
{
    Q_OBJECT

public:
    explicit VirtualListView(QWidget* parent = nullptr);
};

} // namespace novu::ui
