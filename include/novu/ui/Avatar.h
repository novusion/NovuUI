#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/Icon.h"
#include "novu/ui/StyleSpec.h"

class QPainter;

#include <QPixmap>
#include <QString>
#include <QWidget>

namespace novu::ui {

// Who someone is, in the space of one glyph.
//
// A portrait when there is one, initials when there is not, and — because the
// two are never both available across a whole list — a plate whose colour is
// derived from the name, so the same person is the same colour everywhere
// without anybody storing a colour.
//
// Three of the approved designs draw this: NovuHub's account row, NovuVoice's
// call faces, NovuAI's chat. That is what makes it shared rather than one
// application's decoration.
//
// Ported from NovusionUI's `Avatar`, which was a QLabel carrying a stylesheet.
// Painted here instead: a list of twenty avatars is twenty labels each with its
// own composed sheet, and the derived plate colour cannot be written as a
// static rule anyway.
class NOVUUI_API Avatar : public QWidget
{
    Q_OBJECT

public:
    explicit Avatar(const QString& name, QWidget* parent = nullptr);
    Avatar(const QString& name, int size, QWidget* parent = nullptr);

    // The whole name. Initials are taken from it rather than passed in, so
    // "Petr Dostál" and "petr.dostal@novusion.io" both come out as the same two
    // letters instead of one call site deciding and the next disagreeing.
    QString name() const { return m_name; }
    void setName(const QString& name);

    // What is actually drawn when there is no portrait. Exposed for a test and
    // for a caller that wants to show the same initials in a tooltip.
    QString initials() const;

    // A portrait. Scaled and clipped to the avatar's own shape; the initials
    // stay underneath and come back if it is cleared.
    QPixmap portrait() const { return m_portrait; }
    void setPortrait(const QPixmap& portrait);

    int diameter() const { return m_diameter; }
    void setDiameter(int px);

    // The design's `.av.square` — a person is round, a workspace or a machine
    // is not. Same component, because the only difference is the corner.
    enum class Shape { Round, Rounded };
    Shape shape() const { return m_shape; }
    void setShape(Shape shape);

    // The radius a Rounded avatar is drawn with, in pixels. -1 - the default -
    // takes the profile's own role radius, which is what every existing caller
    // gets. A design that draws a specific corner on a specific size states it:
    // NovuVoice's assistant mark is 9px on 32 and its group mark 10px, and the
    // difference between the two shapes is how a person, a group and an
    // assistant stay apart in greyscale.
    int cornerRadius() const { return m_cornerRadius; }
    void setCornerRadius(int px);

    // ── the plate ─────────────────────────────────────────────────────────
    //
    // Unset, the plate is still derived from the name, which is what keeps one
    // person one colour everywhere without anything storing a colour per user.
    // Set, it is painted solid: a design that assigns identity hues has already
    // checked them against its surfaces, and tinting them to 0.26 on top would
    // be the library second-guessing that.
    QColor plate() const { return m_plate; }
    void setPlate(const QColor& plate);
    void setPlateGradient(const QColor& from, const QColor& to);

    // A mark instead of initials: a group, a channel, a device. Glyph::None -
    // the default - keeps the initials.
    Glyph glyph() const { return m_glyph; }
    void setGlyph(Glyph glyph);

    // ── presence ──────────────────────────────────────────────────────────
    //
    // The dot on the corner. It is a *status*, so it is drawn from the semantic
    // palette and never from the product accent - online is the same green in
    // every Novusion window. InCall is Online with a ring round it: still
    // reachable, already talking.
    enum class Presence { None, Online, Busy, Away, Offline, InCall };
    Presence presence() const { return m_presence; }
    void setPresence(Presence presence);

    // Lifts the plate and the outline a step: the avatar is the visible handle
    // of something interactive, or the person it names is the current one.
    bool isHighlighted() const { return m_highlighted; }
    void setHighlighted(bool highlighted);

    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent* event) override;
    bool event(QEvent* event) override;

private:
    void paintPresence(QPainter* painter, const ThemeProfile& profile) const;

private:
    QString m_name;
    QPixmap m_portrait;
    int m_diameter = 32;
    Shape m_shape = Shape::Round;
    bool m_highlighted = false;
    int m_cornerRadius = -1;
    QColor m_plate;
    QColor m_plateTo;
    Glyph m_glyph = Glyph::None;
    Presence m_presence = Presence::None;
};

} // namespace novu::ui
