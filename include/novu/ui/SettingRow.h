#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/StyleSpec.h"

#include <QString>
#include <QWidget>

class QHBoxLayout;

namespace novu::ui {

// A name, what it means, and the control that changes it.
//
// The design draws this twice, as `.prow` in a settings page and `.kv` in a
// detail panel, and the two are the same row: *a bold name, a muted sentence
// under it, and something on the right*. The only difference is whether the
// thing on the right is a control or a value, and that is what a caller passes.
//
// ── why this and not formRow() ────────────────────────────────────────────
//
// Form.h's `formRow()` is the other shape: a fixed label column with the
// control beside it, which is right for a dialog where six fields must line up.
// This is right for a settings *page*, where each row explains itself and the
// explanation is a sentence rather than a caption. Trying to make one function
// do both is how the settings pages ended up with captions that were too short
// to explain anything and hints that ran off the right edge.
//
// The distinction in one line: `formRow()` is for a form you fill in,
// `SettingRow` is for a preference you change.
//
// ── what is painted ──────────────────────────────────────────────────────
//
// The name and the hint. Twenty settings rows is twenty QLabel pairs otherwise,
// and the hint is the one that has to wrap — a wrapped QLabel in a fixed-height
// row is the single most common sizing bug in this codebase's history.
//
// The control is a real widget, because it is a control.
class NOVUUI_API SettingRow : public QWidget
{
    Q_OBJECT

public:
    SettingRow(const QString& name, const QString& hint, QWidget* parent = nullptr);

    QString name() const { return m_name; }
    void setName(const QString& name);

    QString hint() const { return m_hint; }
    void setHint(const QString& hint);

    // The control side. The row takes ownership, which is what a caller who
    // built the control inline wants; setting a second one deletes the first
    // rather than leaving it parented and unplaced.
    void setControl(QWidget* control);
    QWidget* control() const { return m_control; }

    // The read-only form: `.kv .v .ro` — a value in the monospace face instead
    // of a control. Setting one clears the other, because a row with both is a
    // row that contradicts itself.
    void setValue(const QString& value, bool mono = true);
    QString value() const { return m_value; }

    // `.secretrow .ro.mask` — the value drawn as dots, with the design's wide
    // tracking so a masked field is obviously masked rather than looking like a
    // short string of full stops.
    bool isMasked() const { return m_masked; }
    void setMasked(bool masked);

    // The validation line under the row — `.ferr`. Empty clears it. It is on
    // the row rather than on the control because the message belongs to the
    // setting, and a control that has to carry its own error text is a control
    // every page then has to lay out around.
    QString error() const { return m_error; }
    void setError(const QString& message);
    bool hasError() const { return !m_error.isEmpty(); }

    // A rule under the row, as `.kv` draws it. The last row in a list turns it
    // off — `.kv:last-child { border-bottom:none }`.
    bool hasDivider() const { return m_divider; }
    void setDivider(bool divider);

    // The width reserved for the control side. The rows in one panel must
    // agree, and a measured column makes every row as wide as its own control.
    int controlColumn() const { return m_controlColumn; }
    void setControlColumn(int px);

    // ── a row that is also a choice ──────────────────────────────────────
    //
    // A list where picking one row decides what the rest of the page is
    // about - a provider, a profile, an account. The alternative a page
    // reaches for is a "Select" button in the control column, which puts a
    // second thing to aim at inside a row that is already one target, and
    // leaves the control column holding two unrelated widgets.
    //
    // A selectable row takes the whole row as the hit area and paints the
    // selected one on the same surface every other selection in the library
    // uses - the resolved style's checked background, as TreeView draws a
    // current row. The control column is untouched, so a toggle or a status
    // that lives there stays on the same axis on every row.
    bool isSelectable() const { return m_selectable; }
    void setSelectable(bool selectable);

    bool isSelected() const { return m_selected; }
    void setSelected(bool selected);

signals:
    // A selectable row was pressed and released inside itself.
    void clicked();

public:

    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    int heightForWidth(int width) const override;
    bool hasHeightForWidth() const override { return true; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool event(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    int textWidth(int width) const;
    int textHeight(int width) const;
    void relayout();
    QString displayValue() const;

    QString m_name;
    QString m_hint;
    QString m_value;
    QString m_error;
    bool m_mono = true;
    bool m_masked = false;
    bool m_selectable = false;
    bool m_selected = false;
    bool m_pressed = false;
    bool m_divider = true;
    int m_controlColumn = 0;
    QWidget* m_control = nullptr;
};

} // namespace novu::ui
