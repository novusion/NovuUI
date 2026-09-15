#pragma once

#include "novu/ui/Icon.h"
#include "novu/ui/StyleSpec.h"

#include <QString>
#include <QWidget>

namespace novu::ui {

// The rounded plaque an application, a module or a workspace is identified by.
//
// The design draws it at three sizes and calls it three things — `.appmark`
// (29px, in a header row), `.rail-mark` (30px, at the top of the sidebar) and
// `.bigmark` (58px, centred in an empty state or an about box). The drawing is
// identical in all three: a rounded square, a diagonal panel-to-card gradient,
// a hairline outline, and either a glyph or one or two letters inside it.
//
// Three names for one drawing is how the sizes drifted: NovuExplorer's header
// mark, NovuHub's tray badge and NovuSpace's rail mark were 28, 32 and 30
// pixels, each with its own radius. Here the size is a parameter and the
// proportions are derived from it, so a fourth size cannot be a fourth shape.
//
// Glyph or letters, never both: a mark with an icon *and* initials is two
// identities in one box.
//
// ── the round one ────────────────────────────────────────────────────────
//
// NovuAI's `.avatar` is this drawing at `border-radius:50%`, and it carries a
// second meaning with it: an avatar says *who*, where a plaque says *what*. A
// message from a person and a message from an agent are told apart by the
// colour of the circle, which is why Tone exists and why it is not the semantic
// palette — an agent is not a status.
class AppMark : public QWidget
{
    Q_OBJECT

public:
    enum class Size {
        Small,   // `.appmark` — beside a title, in a list row
        Medium,  // `.rail-mark` — the head of a sidebar
        Large,   // `.bigmark` — an empty state, an about box
    };

    // A plaque names a thing; a circle names a party. The shape is the whole
    // of the difference, and getting it the wrong way round is how a chat
    // transcript ends up looking like a list of applications.
    enum class Shape { Plaque, Round };

    // Who, rather than what state. `.avatar`, `.avatar.acc`, `.avatar.agent`.
    enum class Tone {
        Neutral,  // a person, or a thing with no identity of its own
        Accent,   // the current user, or this application
        Agent,    // an AI participant — the design's violet, and its own colour
    };

    explicit AppMark(Glyph glyph, QWidget* parent = nullptr, Size size = Size::Medium);
    // Initials. Anything longer than two characters is trimmed to two rather
    // than shrunk to fit: a plaque is not a label.
    AppMark(const QString& initials, QWidget* parent, Size size = Size::Medium);

    Glyph glyph() const { return m_glyph; }
    void setGlyph(Glyph glyph);

    QString initials() const { return m_initials; }
    void setInitials(const QString& initials);

    Size markSize() const { return m_size; }
    void setMarkSize(Size size);
    // An exact edge, for the one caller that has to match a neighbouring
    // control rather than a step on the scale. 0 restores the step.
    void setSideOverride(int px);

    // The accent, or the profile's secondary ink. The design's rail mark is in
    // the accent and its plain `.appmark` is not: an application's own mark is
    // the product, a generic one beside a row is not.
    //
    // Kept alongside Tone rather than folded into it: accented is about a
    // *plaque*, tone is about a *party*, and a caller that sets one should not
    // silently change the other.
    bool isAccented() const { return m_accented; }
    void setAccented(bool accented);

    Shape shape() const { return m_shape; }
    void setShape(Shape shape);

    Tone tone() const { return m_tone; }
    void setTone(Tone tone);

    // The common case in one call: a round mark carrying someone's initials.
    static AppMark* avatar(const QString& initials, QWidget* parent,
                           Tone tone = Tone::Neutral, Size size = Size::Small);

    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

    int side() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent* event) override;
    bool event(QEvent* event) override;

private:
    Glyph m_glyph = Glyph::None;
    QString m_initials;
    Size m_size = Size::Medium;
    Shape m_shape = Shape::Plaque;
    Tone m_tone = Tone::Neutral;
    int m_sideOverride = 0;
    bool m_accented = true;
};

} // namespace novu::ui
