#pragma once

#include "novu/ui/Icon.h"

#include <QColor>
#include "novu/ui/StyleSpec.h"

#include <QIcon>
#include <QPixmap>
#include <QString>
#include <QWidget>

class QPainter;
class QRectF;

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
    //
    // Gem is the third: the Novusion family mark itself -- an isometric cube
    // of three faces, drawn in three tints of the profile's accent. It is not
    // a plate with something on it, so it carries no glyph and no initials.
    // Every product in the family wears it, and wears it in its own accent:
    // that is the whole point of a family mark, and it is why the tints are
    // derived from the profile rather than listed per application.
    enum class Shape { Plaque, Round, Gem };

    // What the mark stands for. `.avatar`, `.avatar.acc`, `.avatar.agent`.
    //
    // Three of these name a party and the fourth names a condition, which is
    // deliberate: the design's failed-page mark is the same plaque in the
    // danger ink, and a caller that had to reach for a raw colour to draw it
    // would be the one place in a screen where a colour is not a token.
    enum class Tone {
        Neutral,  // a person, or a thing with no identity of its own
        Accent,   // the current user, or this application
        Agent,    // an AI participant — the design's violet, and its own colour
        Danger,   // a thing that failed — `.bpage .mark` on an error page
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

    // The product mark's own three faces.
    //
    // A gem normally tints itself from the profile's accent, which is right
    // for a mark that stands for whatever surface it is on. The application's
    // OWN mark is not that: NovuAI's logo is the blue ramp in every theme, and
    // Hub's is green and Explorer's teal even when drawn inside a NovuAI
    // window. A logo that changed colour with the accent would be a different
    // logo.
    //
    // Set all three, or leave them unset and the accent decides as before.
    void setGemTints(const QColor& left, const QColor& right, const QColor& top);

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
    bool m_gemTinted = false;
    QColor m_gemLeft;
    QColor m_gemRight;
    QColor m_gemTop;
};

// ── the product mark ────────────────────────────────────────────────────
//
// A different drawing from AppMark above and not a shape option on it: the
// isometric gem every window icon, every favicon and the title bar's own
// `.app-ico` are drawn from. Three filled polygons in a 48x48 box, no plate,
// no border, no gradient behind it - AppMark's whole class exists to be that
// background; this is the mark that sometimes sits on nothing at all.
//
// The geometry is identical in every approved mockup that draws it; NovuSpace
// names the reason in its own source, as a small reusable function:
//
//     const novuMark = (fill, size) => `<svg viewBox="0 0 48 48">
//       <polygon .../* left */  fill="#B7C1CE">
//       <polygon .../* right */ ${fill ? `fill="${fill}"` : `class="accent-mark"`}>
//       <polygon .../* bottom */fill="#8793A3">`;
//
// called as `novuMark(null, 28)` at every real call site - the accent face is
// always the CSS variable, never a literal, and the other two are always the
// same neutral pair. `#B7C1CE` and `#8793A3` are not new colours: they are
// this profile's own `textSecondary` and `textDisabled`, which is why the
// functions below take a context rather than three colours - the mark is
// always drawn in the product's own accent and the shared neutrals, never in
// anything a caller invents.
//
// (NovuAdmin's and NovuHub's older mockups draw this filled in three steps of
// their own accent instead - #26A362/#3CCB7F/#6ADB9E, hard-coded rather than
// read off a variable. Superseded here by NovuSpace's own reusable function,
// which is both the newer source and the only one that states the rule
// itself rather than one product's coincidence of it.)
void paintBrandMark(QPainter* painter, const QRectF& box, const QWidget* context);

QPixmap brandMarkPixmap(int size, const QWidget* context, qreal devicePixelRatio = 0);
QIcon brandMarkIcon(int size, const QWidget* context);

// The same mark, drawn to fill the box instead of floating in it.
//
// The three faces occupy 23.40 x 26.77 of the 48-unit grid they are drawn on -
// 49% of its width and 56% of its height - so asking brandMarkPixmap() for a
// 14px icon yields an 8px gem adrift in a 14px box, with air on every side.
// In a title bar that air is what makes the mark read as both too small and
// too far from the name.
//
// This tightens the window to a square around the mark's own centre and
// nothing else: same polygons, same colours, same grid. NovuSpace worked the
// window out in its own shell and said at the time that it belonged here
// rather than in one product; this is that, moved. Its own function rather
// than a change to brandMarkPixmap(), so every existing caller keeps the
// framing it already draws.
QPixmap brandMarkTightPixmap(int size, const QWidget* context, qreal devicePixelRatio = 0);

} // namespace novu::ui
