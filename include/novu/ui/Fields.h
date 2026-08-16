#pragma once

#include <QComboBox>
#include <QString>
#include <QStringList>

class QCheckBox;
class QLineEdit;
class QWidget;

namespace novu::ui {

// A combo box that draws its own arrow.
//
// Qt cannot render a CSS border triangle for ::down-arrow — it comes out as a
// filled rectangle that reads as a dash, and the control then looks exactly
// like a line edit. Supplying an image would tie the arrow colour to a file
// instead of the profile. So the arrow is painted, from the theme.
class ComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit ComboBox(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
};

// Factories that apply the profile's control geometry. Appearance comes from
// the scope stylesheet; these set only what a stylesheet cannot: height, font
// and, where the caller needs it, a fixed width.
ComboBox* comboBox(const QStringList& items, QWidget* parent, int width = 0);
QLineEdit* lineEdit(const QString& placeholder, QWidget* parent, int width = 0,
                    const QString& value = {});
QCheckBox* checkBox(const QString& text, QWidget* parent, bool checked = false);

} // namespace novu::ui
