#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/ThemeProfile.h"

#include <QColor>
#include <QFont>
#include <QLabel>
#include <QString>

class QWidget;

namespace novu::ui {

// Text by role, never by number.
//
// A page asks for TypeRole::CardTitle or Ink::Muted; the profile decides the
// pixel size and the colour. This is the difference between a design system and
// a habit: the same label is 13px graphite in one product and 13px slate in
// another without a single conditional at the call site.
//
// Colour is applied through the widget palette rather than a stylesheet, so a
// label can be recoloured live (connected/disconnected, profit/loss) without
// re-parsing a sheet and without competing with the scope's composed sheet.

// Semantic colour roles for text.
enum class Ink {
    Default,     // primary body text
    Secondary,
    Muted,
    Accent,      // the profile's accent
    Success,
    Warning,
    Danger,
    Info,
    Tertiary,
};

QColor inkColor(const QWidget* context, Ink ink);

// Font for a type role, resolved against the widget's scope.
//
// `bold` is one step of emphasis capped at DemiBold(600) - the jump every
// approved design means by "make this stand out" at a *heading* size, where
// the role's own base is already 400 and 600 is the design's actual heading
// weight. It is the wrong step for the far more common case of emphasising a
// value inside a row of Body or Caption text - `.cellmain{font-weight:500}`,
// `.side.long{font-weight:500}`, `.pnl.up{font-weight:500}` are the same one
// step from 400, to Medium, not to DemiBold. `weight`, given nonzero, states
// that step exactly and skips the 600 floor entirely; use it wherever the CSS
// names its own number rather than reaching for `bold` and hoping 600 was
// close enough. A `label()`/`roleFont()` called with an explicit `weight`
// ignores `bold`.
QFont roleFont(const QWidget* context, TypeRole role, bool bold = false, int weight = 0);

// Monospace at the same role size, for ids, prices and paths.
QFont roleMonoFont(const QWidget* context, TypeRole role, bool bold = false, int weight = 0);

// Icon font. Windows 11 ships Segoe Fluent Icons; Segoe MDL2 Assets is the
// fallback on older systems and shares the code points, so a glyph looks the
// same on both.
QFont iconFont(int pixelSize);

// A label carrying a type role and an ink role. Creates no container.
//
// See roleFont() for `bold` versus `weight`: `bold` is the heading step to
// 600, `weight` states any other exact number the CSS names (500, most often,
// for a value emphasised inside a row of Body or Caption text).
QLabel* label(const QString& text, QWidget* parent,
              TypeRole role = TypeRole::Body, Ink ink = Ink::Default,
              bool bold = false, int weight = 0);

// A single line that keeps its full text for accessibility and auditing while
// painting an ellipsis when its allotted width is smaller than the content.
class NOVUUI_API ElidedLabel : public QLabel
{
public:
    explicit ElidedLabel(const QString& text, QWidget* parent = nullptr);
    void setElideMode(Qt::TextElideMode mode){m_elideMode=mode;update();}
    Qt::TextElideMode elideMode()const{return m_elideMode;}

    QSize minimumSizeHint() const override;
protected:
    void paintEvent(QPaintEvent*) override;
private:
    Qt::TextElideMode m_elideMode=Qt::ElideRight;
};
QLabel* elidedLabel(const QString& text, QWidget* parent,
                   TypeRole role = TypeRole::Body, Ink ink = Ink::Default,
                   int weight = 0);

// A paragraph of text in a column: a label that wraps, and that answers for
// the height it will actually take at the width it is given.
//
// setWordWrap(true) is not enough, and this exists because of what it leaves
// behind. A wrapped QLabel measures its sizeHint and its minimumSizeHint at a
// width Qt picks for itself rather than at the width the column will hand it
// -- for rich text, close to the widest unbroken run -- so the label claims
// the height it would need at one or two words to the line. Everything that
// honours those hints then believes it: a scroll area sizes its content to
// several times the real length of the text, and a box layout shares the
// surplus out among whatever else in the column can grow, which is how a code
// block inside a chat turn becomes a thousand pixels of empty surface.
//
// Every application has paragraphs in columns, so every application meets
// this. A Paragraph answers both questions with heightForWidth at the width
// it has, and is otherwise a QLabel -- rich text, selection, links included.
class NOVUUI_API Paragraph : public QLabel
{
    Q_OBJECT

public:
    explicit Paragraph(const QString& text, QWidget* parent = nullptr,
                       TypeRole role = TypeRole::Body, Ink ink = Ink::Default);

    // The width the text would take if nothing wrapped it. A paragraph
    // prefers this, bounded by whatever maximum the caller set, so a column
    // that shrinks to fit its content stops at the end of the sentence rather
    // than at the edge of the window.
    int naturalWidth() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void resizeEvent(QResizeEvent* event) override;
};

// Paragraph, as the other text helpers read.
QLabel* paragraph(const QString& text, QWidget* parent,
                  TypeRole role = TypeRole::Body, Ink ink = Ink::Default);

// Uppercase heading above a group of controls.
QLabel* caption(const QString& text, QWidget* parent);

// Applies a type role and an ink to a label that already exists.
//
// label() covers creation; this covers everything else - a title that changes
// weight, a status line that changes meaning, a label the caller did not build.
// Without it every consumer writes the same setFont/setInk pair, which is how
// the roles start drifting apart again.
void applyText(QLabel* label, TypeRole role, Ink ink = Ink::Default, bool bold = false);

// The CSS `line-height` of one label, when the design states it for that
// element rather than for the whole role - `.fitem .ft .l1{line-height:1.45}`,
// `.page-head .desc{line-height:1.4}`. A multiple of the label's own pixel
// size, the way CSS measures it; 0 restores the font's own line box.
//
// A role-wide line height belongs in TypeStyle; this is for the places a
// design sets one line of prose apart from every other use of the same role.
void setLineHeight(QLabel* label, double multiple);

// New text for a label whose line height has been stated.
//
// It exists because the leading of a *wrapped* label is carried in markup, so
// QLabel::setText() on one would replace the styled block with a bare string
// and silently drop the design's line height. Callers that never state a line
// height do not need this; callers that do must use it, and it is safe on any
// label either way. `labelText()` gives back the sentence rather than the
// markup that is carrying it.
void setLabelText(QLabel* label, const QString& text);
// Trusted document markup, retaining the explicit line height across theme changes.
void setLabelRichText(QLabel* label, const QString& html);
QString labelText(const QLabel* label);

// Recolours a live label by role.
void setInk(QLabel* label, Ink ink);
void setInk(QLabel* label, const QColor& color);

} // namespace novu::ui
