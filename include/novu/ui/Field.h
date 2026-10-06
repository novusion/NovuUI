#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/StyleSpec.h"

#include <QString>
#include <QWidget>

class QBoxLayout;

namespace novu::ui {

// One labelled input, and what is wrong with it.
//
// The approved design's `.field`: a tracked uppercase caption, the control, and
// under it either a muted hint or — when the value is refused — a danger line
// with a small glyph in front of it. `.field.bad .input` borders the control at
// the same moment, so the eye finds the offending control and the sentence
// explaining it in one movement.
//
// `formRow()` in Form.h is the *other* shape: caption in a fixed left column,
// control on the right, for a settings sheet where rows must line up down a
// page. This one stacks, which is what a dialog and a create/edit form use.
// Both exist in the approved designs and neither is a variant of the other.
//
// ── why this is a component and not a helper ─────────────────────────────
//
// Validation was the half every form re-invented. NovuAdmin has seventeen
// create/edit forms and they disagreed on where the message goes, whether the
// control is marked, whether the hint survives an error and whether the message
// clears when the user types again. Those are not style choices, they are the
// behaviour — so they live here once.
//
// The caption, the hint and the error sentence are painted. They are three
// lines of text nobody selects, and a QLabel for each would treble the widget
// count of every form in the product. The control is a real widget, because a
// control is the half that is interacted with.
class NOVUUI_API Field : public QWidget
{
    Q_OBJECT

public:
    // Takes ownership of `control` by reparenting it, the way a layout would.
    Field(const QString& caption, QWidget* control, QWidget* parent = nullptr);
    Field(const QString& caption, QWidget* control, const QString& hint,
          QWidget* parent = nullptr);

    QWidget* control() const { return m_control; }

    QString caption() const { return m_caption; }
    void setCaption(const QString& caption);

    // The line under the control when nothing is wrong. An error replaces it
    // rather than joining it: two sentences under one control is how a form
    // ends up taller than the screen, and the hint is advice the error has
    // already superseded.
    QString hint() const { return m_hint; }
    void setHint(const QString& hint);

    // ── validation ───────────────────────────────────────────────────────

    // A value this form will not be submitted without.
    //
    // Drawn as the design does it: a mark after the caption, not a sentence in
    // the hint. NovuAdmin's forms mark the required ones because most of their
    // fields are, and a form where the *optional* ones are called out reads
    // backwards.
    bool isRequired() const { return m_required; }
    void setRequired(bool required);

    bool isValid() const { return m_error.isEmpty(); }
    QString error() const { return m_error; }

    // Marks the control and shows the message. Empty clears, so a caller can
    // pass the result of its own check without branching.
    void setError(const QString& message);
    void clearError() { setError(QString()); }

    // Clears the error the moment the user changes the value.
    //
    // On by default, and the default matters: a message that stays after the
    // problem is fixed teaches people to ignore messages. Turn it off only for
    // a field whose validity cannot be judged locally — one the server decides.
    bool clearsErrorOnEdit() const { return m_clearOnEdit; }
    void setClearsErrorOnEdit(bool clear);

    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    // Emitted when the field starts or stops reporting a problem, so a dialog
    // can enable its submit button without polling every field.
    void validityChanged(bool valid);

protected:
    void paintEvent(QPaintEvent* event) override;
    bool event(QEvent* event) override;

private:
    void build();
    void relayout();
    void watchControl();
    int captionHeight() const;
    int footerHeight() const;

    QString m_caption;
    QString m_hint;
    QString m_error;
    QWidget* m_control = nullptr;
    bool m_clearOnEdit = true;
    bool m_required = false;
};

// Adds a field to a column that already exists, and hands back the Field so the
// caller can validate it later.
Field* field(QBoxLayout* into, QWidget* parent, const QString& caption,
             QWidget* control, const QString& hint = {});

} // namespace novu::ui
