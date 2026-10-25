#pragma once

#include "novu/ui/Export.h"

#include <QCheckBox>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QDate>
#include <QDateEdit>
#include <QString>
#include <QStringList>

class QLineEdit;
class QWidget;

namespace novu::ui {

// A combo box that draws its own arrow.
//
// Qt cannot render a CSS border triangle for ::down-arrow — it comes out as a
// filled rectangle that reads as a dash, and the control then looks exactly
// like a line edit. Supplying an image would tie the arrow colour to a file
// instead of the profile. So the arrow is painted, from the theme.
class Popover;

class NOVUUI_API ComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit ComboBox(QWidget* parent = nullptr);

    // Type-to-filter. The control stays a combo - it still carries a fixed set
    // of options and item data - but the closed state becomes a text field with
    // a contains-matching completer behind it. For lists nobody can scroll,
    // market symbols above all.
    void setSearchable(bool searchable);

    // The list is showing. Exposed because the closed and open states are
    // drawn differently -- `.select.open` takes the accent border and turns
    // its chevron over -- and a test should not have to look for a popup
    // window to know which one is on screen.
    bool isOpen() const { return m_open; }

    // The list a closed (non-searchable) box opens: the same Popover as
    // SelectButton - the approved dropdown - so every standard dropdown has
    // one look, one row height, one hover and one way of opening up or down,
    // seven rows at most and the rest scrolling. Null until first opened.
    Popover* menu() const { return m_menu; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void showPopup() override;
    void hidePopup() override;

private:
    void setOpen(bool open);

    bool m_open = false;
    Popover* m_menu = nullptr;
};

// A checkbox that says which mark it wants.
//
// The drawing is not here. A stylesheet can colour a check indicator and size
// it but cannot put a shape inside it, and it reaches no indicator at all in a
// table cell or a tree item - so NovuStyle draws every one of them in the
// process, and this class only declares the mark. That way a checkbox built by
// an application, a checkable cell in a TableView and this widget are all the
// same drawing rather than three that have to be kept in step.
//
// A plain QCheckBox is therefore already correct inside a Novu application;
// this exists for the choice of mark and for the profile's font.
class NOVUUI_API CheckBox : public QCheckBox
{
    Q_OBJECT

public:
    // Check for "included", Cross for the places where a tick would read as
    // approval of something being off. Both are drawn at the same weight, so a
    // column mixing them stays even.
    enum class Mark { Check, Cross };

    explicit CheckBox(QWidget* parent = nullptr);
    CheckBox(const QString& text, QWidget* parent = nullptr);

    Mark mark() const { return m_mark; }
    void setMark(Mark mark);

private:
    Mark m_mark = Mark::Check;
};

// Factories that apply the profile's control geometry. Appearance comes from
// the scope stylesheet; these set only what a stylesheet cannot: height, font
// and, where the caller needs it, a fixed width.
ComboBox* comboBox(const QStringList& items, QWidget* parent, int width = 0);
QLineEdit* lineEdit(const QString& placeholder, QWidget* parent, int width = 0,
                    const QString& value = {});
CheckBox* checkBox(const QString& text, QWidget* parent, bool checked = false);

// A multi-line text field: a description, a note, a commit message.
//
// Plain text rather than rich: everything Novu collects in one of these is
// carried as a string to somewhere else, and a QTextEdit would quietly accept
// pasted formatting that the receiving end has to strip.
//
// `rows` is in lines of the profile's body font, so the field is the same
// height on every display and in every product.
QPlainTextEdit* textArea(const QString& placeholder, QWidget* parent, int rows = 4,
                         const QString& value = {});

// A date field that draws its own calendar mark.
//
// Same reason ComboBox paints its arrow: the stylesheet can hide Qt's default
// sub-controls but cannot supply a shape to replace them, so a styled QDateEdit
// ends up with an invisible strip at its right edge that opens the popup when
// clicked and gives no sign it is there.
class NOVUUI_API DateEdit : public QDateEdit
{
    Q_OBJECT

public:
    explicit DateEdit(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
};

// A date field with a calendar popup, in ISO order so a typed date and a picked
// one look the same. The popup is themed here rather than at each call site:
// an unstyled QCalendarWidget arrives with the light system palette and is
// unreadable on every dark surface in the product.
DateEdit* dateEdit(const QDate& value, QWidget* parent, int width = 0);

} // namespace novu::ui
