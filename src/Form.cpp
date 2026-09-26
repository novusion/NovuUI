#include "novu/ui/Form.h"

#include "novu/ui/Fields.h"
#include "novu/ui/Icon.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QAction>
#include <QBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>

namespace novu::ui {

namespace {

// The label column of a form. Wide enough for the captions the approved
// designs actually use ("Retention period", "API key secret") and identical
// across every form, which is the entire point of fixing it.
constexpr int kLabelColumn = 130;

} // namespace

QHBoxLayout* formRow(QBoxLayout* into, QWidget* parent, const QString& captionText,
                     QWidget* control, const QString& hint)
{
    const ThemeProfile& profile = themeFor(parent);

    auto* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(profile.space(SpacingRole::FormRowGap));

    QLabel* caption = label(captionText, parent, TypeRole::Caption, Ink::Muted);
    // The one fixed width in this library, and it is a column rather than a
    // component: the rule pages must not break is setting a width on a
    // *component*, and a form's label column is layout.
    caption->setFixedWidth(kLabelColumn);
    row->addWidget(caption);

    if (control) {
        control->setParent(parent);
        row->addWidget(control);
    }
    if (!hint.isEmpty()) {
        QLabel* note = label(hint, parent, TypeRole::Caption, Ink::Muted);
        note->setWordWrap(false);
        row->addWidget(note);
    }
    row->addStretch(1);

    if (into) {
        into->addLayout(row);
    }
    return row;
}

QHBoxLayout* propertyRow(QBoxLayout* into, QWidget* parent, const QString& captionText,
                         const QString& value, Ink tone, bool mono)
{
    const ThemeProfile& profile = themeFor(parent);

    auto* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(profile.space(SpacingRole::FormRowGap));

    row->addWidget(label(captionText, parent, TypeRole::Caption, Ink::Muted));
    row->addStretch(1);

    QLabel* right = label(value, parent, mono ? TypeRole::Mono : TypeRole::Body, tone, true);
    if (mono) {
        right->setFont(roleMonoFont(parent, TypeRole::Mono, true));
    }
    row->addWidget(right);

    if (into) {
        into->addLayout(row);
    }
    return row;
}

QLineEdit* searchBar(QWidget* parent, const QString& placeholder, int width)
{
    QLineEdit* field = lineEdit(
        placeholder.isEmpty() ? QObject::tr("Search…") : placeholder, parent, width);
    // The affordance is part of the component, not of the page: a search box
    // a user cannot clear in one gesture is one they clear by selecting the
    // text, and half of them do not.
    field->setClearButtonEnabled(true);
    // `.search{position:relative}` + `svg("search")` - every search field in
    // the design carries its own glyph, not just the clear affordance.
    // QLineEdit's own leading action reserves the room for it; nothing about
    // the field's own padding has to know it is there.
    const QIcon glyph = glyphIcon(Glyph::Search, 14, themeFor(field).colors.textDisabled,field);
    field->addAction(glyph, QLineEdit::LeadingPosition);
    return field;
}

} // namespace novu::ui
