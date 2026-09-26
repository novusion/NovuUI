#pragma once

#include <QColor>
#include <QWidget>

class QPainter;

namespace novu::ui {

// A pill-shaped label (status, tag, plan, release, price).
//
// The badge owns its own geometry. It paints itself rather than relying on a
// stylesheet, because QSS padding only reaches sizeHint() after the widget
// happens to be polished, which yields half-cut pills. Painting directly makes
// sizeHint() correct from construction, which is the contract every layout
// depends on.
//
// Pages never set a width on a badge.
class Badge : public QWidget
{
    Q_OBJECT

public:
    enum class Tone {
        Neutral,   // tags, plans, generic values
        Success,   // active, healthy, online
        Warning,   // pending, expiring, trial
        Danger,    // failed, revoked, offline
        Info,      // release channels, versions
        // The product accent. Not a status: it marks the thing the product
        // itself is pointing at — a selected preset, the current column set,
        // the module a row belongs to. A status never uses it and it never
        // states a status, which is the rule the approved design states
        // outright and the reason it is a tone rather than a sixth colour
        // somebody reaches for.
        Accent,
    };

    // Compact is the dense-table treatment (`.drow .tag`); Regular is the
    // standard row/status badge and Large sits beside a header title.
    enum class Size {
        Regular,
        Large,
        Compact,
    };

    explicit Badge(QWidget* parent = nullptr);
    Badge(const QString& text, Tone tone, QWidget* parent = nullptr);

    QString text() const { return m_text; }
    void setText(const QString& text);

    Tone tone() const { return m_tone; }
    void setTone(Tone tone);

    Size size() const { return m_size; }
    void setSize(Size size);

    // A filled dot before the word, in the badge's own ink.
    //
    // It is not decoration. A status badge reads as a status at a glance
    // because of the dot; a badge without one reads as a tag. The approved
    // design draws every live status with it and every classification without,
    // so the flag is the difference between the two meanings.
    bool hasDot() const { return m_dot; }
    void setDot(bool dot);

    // Uppercase, for the classification badges the design sets that way — a
    // plan name, a release channel, an environment. Applied at paint time
    // rather than to the stored text, so text() still returns what was set.
    bool isUppercase() const { return m_uppercase; }
    void setUppercase(bool uppercase);

    // A non-semantic family hue for classifications such as model kinds.
    // This is deliberately separate from Tone: "image" is not Warning and
    // "audio" is not Success merely because their approved hues happen to be
    // purple and green. Pass a colour from familyHues(); an invalid QColor
    // restores the semantic tone.
    QColor classificationHue() const { return m_classificationHue; }
    void setClassificationHue(const QColor& hue);

    // Both hints are the same: a badge has exactly one correct size.
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    bool event(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    QString displayText() const;

    QString m_text;
    Tone m_tone = Tone::Neutral;
    Size m_size = Size::Regular;
    bool m_dot = false;
    bool m_uppercase = false;
    QColor m_classificationHue;
};

// The design's `.pill`: an uppercase neutral classification, never a status.
// A named factory because the combination is what carries the meaning, and
// "neutral, uppercase, no dot" spelled out at forty call sites is how the
// meaning drifts.
Badge* pill(const QString& text, QWidget* parent = nullptr);

// A live status: tone, a dot, sentence case. The other half of the same
// distinction.
Badge* statusBadge(const QString& text, Badge::Tone tone, QWidget* parent = nullptr);

// The badge, without the widget.
//
// A row in a painted list draws these by the dozen, and a Badge instance per
// tag is exactly what a painted row exists to avoid - a table of 28 models
// with three capabilities each is 84 widgets that only ever draw a pill with
// a word in it. Badge itself paints through these two functions, so a badge
// in a row and a Badge on a card are the same badge, measured and drawn by
// the same code.
//
// `context` is the widget whose theme scope decides the colours and the font;
// it is the row's view, not the badge, because there is no badge.
QSize badgeSizeFor(const QWidget* context, const QString& text, Badge::Size size,
                   bool withDot = false, bool uppercase = false);
void paintBadge(QPainter* painter, const QRectF& box, const QWidget* context,
                const QString& text, Badge::Tone tone, Badge::Size size,
                bool withDot = false, bool uppercase = false,
                const QColor& classificationHue = QColor());

} // namespace novu::ui
