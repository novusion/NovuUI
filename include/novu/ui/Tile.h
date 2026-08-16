#pragma once

#include <QPushButton>
#include <QString>

namespace novu::ui {

// A selectable two-line tile: a title, a subtitle, and an optional badge.
//
// One component covers both places the approved design uses it — choosing an
// exchange (with a "Soon" badge on the ones not yet available) and choosing an
// optimisation profile. They differ only in their text and whether the badge is
// present, which is data, not a reason for a second class.
//
// Self-painted so the selected state is one fill and one border read from the
// profile, rather than a stylesheet whose :checked rule would have to repeat
// every colour.
class Tile : public QPushButton
{
    Q_OBJECT

public:
    Tile(const QString& title, const QString& subtitle,
         QWidget* parent = nullptr, bool checked = false,
         const QString& badge = {});

    // A tile carrying a badge is unavailable: the badge exists to say why.
    void setBadge(const QString& badge);
    QString badge() const { return m_badge; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_title;
    QString m_subtitle;
    QString m_badge;
};

} // namespace novu::ui
