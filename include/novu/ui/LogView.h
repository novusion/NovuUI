#pragma once

#include "novu/ui/MonoLineView.h"

#include <QDateTime>
#include <QString>
#include <QVector>

namespace novu::ui {

// A stream of log lines: timestamp, level, message.
//
// The design's `.logs` — monospace, `line-height:1.75`, on the window ground
// rather than a card, with the level in its own 52px column coloured by
// severity. Three applications draw this and all three drew it differently:
// NovuAdmin's console, NovuAI's runtime log and NovuExplorer's transfer log.
//
// ── why not a QPlainTextEdit ─────────────────────────────────────────────
//
// MonoLineView's own note covers it: a level has to be a column, and a
// document of ten thousand lines re-lays out ten thousand lines when you
// append to it. This holds lines in a vector; the base paints the twenty that
// are on screen. Appending is a push and a scroll, and a log at its cap costs
// the same as an empty one.
//
// What is left here is what a *log* is: levels, a timestamp, a cap, a filter,
// and following the tail.
//
// ── the cap ───────────────────────────────────────────────────────────────
//
// A log view must have one. A process that logs steadily for a day fills
// memory otherwise, and the oldest lines are the ones nobody will read. The
// default is generous and the oldest go first.
class LogView : public MonoLineView
{
    Q_OBJECT

public:
    // Named as the design names them - `.lv.DEBUG`, `.lv.INFO`, `.lv.WARNING`,
    // `.lv.ERROR` - because these strings appear in the output of every
    // Novusion service and a mismatch between the filter and the log is a bug
    // nobody finds.
    enum class Level { Debug, Info, Warning, Error };

    explicit LogView(QWidget* parent = nullptr);

    void append(Level level, const QString& message,
                const QDateTime& when = QDateTime::currentDateTime());
    // Bulk, for a log read back from a file or a first page from an endpoint.
    // One repaint rather than one per line.
    void appendAll(const QVector<QPair<Level, QString>>& lines);
    void clear();

    int lineCount() const { return m_lines.size(); }
    int visibleLineCount() const;
    int rowCount() const override { return m_visible.size(); }
    QString lineText(int index) const;
    Level lineLevel(int index) const;

    int maximumLines() const { return m_maximum; }
    void setMaximumLines(int lines);

    // Only lines at or above `level`. Debug shows everything, which is the
    // default. Filtering is here rather than at the caller because a log that
    // discards what it is not currently showing cannot be un-filtered.
    Level minimumLevel() const { return m_minimum; }
    void setMinimumLevel(Level level);

    // Substring match against the message. Case-insensitive; empty clears it.
    QString filter() const { return m_filter; }
    void setFilter(const QString& text);

    // Whether the view sticks to the newest line. Set false automatically when
    // the user scrolls up - a log that yanks itself back to the bottom while
    // someone is reading it is unusable - and back to true when they return.
    bool isFollowing() const { return m_following; }
    void setFollowing(bool following);

    void setShowTimestamps(bool show);
    bool showsTimestamps() const { return m_timestamps; }

    // Everything currently shown, as text, for a copy action.
    QString selectedText() const;

signals:
    void followingChanged(bool following);

protected:
    void paintRow(QPainter* painter, int row, const QRect& box) override;
    void scrollContentsBy(int dx, int dy) override;

private:
    struct Line
    {
        Level level = Level::Info;
        QString message;
        QDateTime when;
    };

    bool passes(const Line& line) const;
    void rebuildVisible();
    QColor levelColor(Level level) const;

    QVector<Line> m_lines;
    // Indices into m_lines that survive the filter. Rebuilt when the filter
    // changes and appended to otherwise, so a steady stream does not re-scan
    // the whole buffer on every line.
    QVector<int> m_visible;

    int m_maximum = 5000;
    Level m_minimum = Level::Debug;
    QString m_filter;
    bool m_following = true;
    bool m_timestamps = true;
};

} // namespace novu::ui
