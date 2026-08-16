#pragma once

#include "novu/ui/Badge.h"

#include <QFrame>
#include <QString>
#include <QWidget>

class QLabel;

namespace novu::ui {

// What a panel shows instead of a table when there is nothing to show.
//
// A glyph, a statement of what is missing, and a line saying what would make it
// appear. The third line matters: "No open trades" alone reads as a fault, and
// users report it as one.
class EmptyState : public QWidget
{
    Q_OBJECT

public:
    EmptyState(const QString& glyph, const QString& title,
               const QString& subtitle, QWidget* parent = nullptr);

    void setTitle(const QString& title);
    void setSubtitle(const QString& subtitle);

private:
    QLabel* m_title = nullptr;
    QLabel* m_subtitle = nullptr;
};

// A tinted strip carrying one sentence of context, optionally with an action.
//
// Painted rather than styled: a tint per tone would otherwise need one
// stylesheet rule for each, and a QFrame rule cascades into every QLabel inside
// it — the defect this codebase has hit most often.
class InfoBanner : public QFrame
{
    Q_OBJECT

public:
    InfoBanner(const QString& text, Badge::Tone tone = Badge::Tone::Info,
               QWidget* parent = nullptr, QWidget* trailing = nullptr);

    void setText(const QString& text);
    void setTone(Badge::Tone tone);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QLabel* m_label = nullptr;
    Badge::Tone m_tone = Badge::Tone::Info;
};

// A determinate progress strip for long local work: a backup, a retention
// sweep, an export.
class ProgressBar : public QWidget
{
    Q_OBJECT

public:
    explicit ProgressBar(int value = 0, QWidget* parent = nullptr);

    int value() const { return m_value; }
    void setValue(int value);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int m_value = 0;
};

} // namespace novu::ui
