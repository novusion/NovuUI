#pragma once

#include <QString>
#include <QWidget>

class QLabel;

namespace novu::ui {

class ProgressBar;

// The window every Novu application opens before it opens: the design's
// `.splash` — the family mark, the product's own name and one line under
// it, a version/build pair pinned to the corner, and a status line over a
// progress track.
//
// What changes "per application" (the user's own phrase) is data, not
// shape: setTitle()/setSubtitle() are NovuSpace's name and tagline today
// and NovuAI's or NovuExplorer's tomorrow, and the mark already carries the
// active profile's own accent (see AppMark::Shape::Gem) rather than a
// colour named here. Startup drives setStatusText()/setPercent() as it
// actually progresses - the mockup's own five-stage timer is a preview
// convenience, not something this component runs itself.
class SplashScreen : public QWidget
{
    Q_OBJECT

public:
    explicit SplashScreen(QWidget* parent = nullptr);

    QString title() const;
    void setTitle(const QString& title);

    QString subtitle() const;
    void setSubtitle(const QString& subtitle);

    // The whole line, exactly as it should read - "Version 0.6" is the
    // caller's wording, not a number this component prefixes itself.
    QString versionLine() const;
    void setVersionLine(const QString& text);

    QString buildLine() const;
    void setBuildLine(const QString& text);

    QString statusText() const;
    void setStatusText(const QString& text);

    // 0..100. Drives the track's own fill and the percent label beside the
    // status text together, so the two can never disagree about how far
    // along startup is.
    int percent() const { return m_percent; }
    void setPercent(int percent);

    ProgressBar* progressBar() const { return m_progress; }

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QLabel* m_title = nullptr;
    QLabel* m_subtitle = nullptr;
    QLabel* m_versionLabel = nullptr;
    QLabel* m_buildLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_percentLabel = nullptr;
    ProgressBar* m_progress = nullptr;
    int m_percent = 0;
};

} // namespace novu::ui
