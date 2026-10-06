#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/Icon.h"
#include "novu/ui/StyleSpec.h"

#include <QPushButton>
#include <QString>

namespace novu::ui {

// A selectable card: a title, a sentence under it, and an optional badge.
//
// One component covers every place the approved design uses it — choosing an
// exchange (with a "Soon" badge on the ones not yet available), choosing an
// optimisation profile, and the design's own `.rcard` inside a `.radiogrid`,
// which is the settings-page form of the same choice. They differ in their
// text, in whether a glyph leads the title, and in whether the choice carries a
// risk. All three of those are data.
//
// Self-painted so the selected state is one fill and one border read from the
// profile, rather than a stylesheet whose :checked rule would have to repeat
// every colour.
//
// ── on the description ───────────────────────────────────────────────────
//
// It wraps, and the tile grows to fit it. `.rcard .rd` is a sentence, not a
// caption, and a card that elides its own explanation is choosing between two
// options the user can no longer tell apart. That is why this has
// heightForWidth() — put these in a grid and let the grid ask.
class NOVUUI_API Tile : public QPushButton
{
    Q_OBJECT

public:
    // What choosing this one means. Normal is a choice; Risky is a choice with
    // a cost, and the design marks it in amber rather than in the accent —
    // `.rcard.risky.on`. It is deliberately *not* Danger: danger is for an
    // action that destroys something, and this is still only a setting.
    enum class Tone { Normal, Risky };

    Tile(const QString& title, const QString& subtitle,
         QWidget* parent = nullptr, bool checked = false,
         const QString& badge = {});

    QString title() const { return m_title; }
    void setTitle(const QString& title);
    QString subtitle() const { return m_subtitle; }
    void setSubtitle(const QString& subtitle);

    // A tile carrying a badge is unavailable: the badge exists to say why.
    void setBadge(const QString& badge);
    QString badge() const { return m_badge; }

    // `.rcard .rt svg` — a mark in front of the title. None by default, because
    // a grid of four cards each with an icon is four icons competing with the
    // four words that actually distinguish them.
    Glyph glyph() const { return m_glyph; }
    void setGlyph(Glyph glyph);

    Tone tone() const { return m_tone; }
    void setTone(Tone tone);

    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    int heightForWidth(int width) const override;
    bool hasHeightForWidth() const override { return true; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    bool event(QEvent* event) override;

private:
    int descriptionHeight(int width) const;

    QString m_title;
    QString m_subtitle;
    QString m_badge;
    Glyph m_glyph = Glyph::None;
    Tone m_tone = Tone::Normal;
    bool m_hovered = false;
};

} // namespace novu::ui
