#pragma once

#include <QWidget>

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
    };

    explicit Badge(QWidget* parent = nullptr);
    Badge(const QString& text, Tone tone, QWidget* parent = nullptr);

    QString text() const { return m_text; }
    void setText(const QString& text);

    Tone tone() const { return m_tone; }
    void setTone(Tone tone);

    // Both hints are the same: a badge has exactly one correct size.
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    bool event(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_text;
    Tone m_tone = Tone::Neutral;
};

} // namespace novu::ui
