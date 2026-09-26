#include "novu/ui/LogView.h"

#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QFontMetrics>
#include <QPainter>
#include <QScrollBar>

namespace novu::ui {

namespace {

// `.logs { padding:10px 14px }`. The 1.75 line-height is MonoLineView's
// default, which is where it came from.
constexpr int kPaddingH = 14;

// `.logs .ln { gap:10px }` and `.logs .lv { width:52px }`.
constexpr int kColumnGap = 10;
constexpr int kLevelColumn = 52;

QString levelName(LogView::Level level)
{
    switch (level) {
    case LogView::Level::Debug:   return QStringLiteral("DEBUG");
    case LogView::Level::Info:    return QStringLiteral("INFO");
    case LogView::Level::Warning: return QStringLiteral("WARNING");
    case LogView::Level::Error:   return QStringLiteral("ERROR");
    }
    return QString();
}

} // namespace

LogView::LogView(QWidget* parent)
    : MonoLineView(parent)
{
}

bool LogView::passes(const Line& line) const
{
    if (static_cast<int>(line.level) < static_cast<int>(m_minimum)) {
        return false;
    }
    return m_filter.isEmpty() || line.message.contains(m_filter, Qt::CaseInsensitive);
}

void LogView::rebuildVisible()
{
    m_visible.clear();
    m_visible.reserve(m_lines.size());
    for (int i = 0; i < m_lines.size(); ++i) {
        if (passes(m_lines.at(i))) {
            m_visible.append(i);
        }
    }
    rowsChanged(m_following);
}

void LogView::append(Level level, const QString& message, const QDateTime& when)
{
    Line line;
    line.level = level;
    line.message = message;
    line.when = when;
    m_lines.append(line);

    if (m_lines.size() > m_maximum) {
        // Dropping from the front shifts every index the visible list holds, so
        // it is rebuilt rather than patched. This happens once per line only
        // once the cap is reached, and the alternative is an index map that is
        // wrong exactly when the buffer is busiest.
        const int excess = m_lines.size() - m_maximum;
        m_lines.remove(0, excess);
        rebuildVisible();
        return;
    }

    if (passes(m_lines.last())) {
        m_visible.append(m_lines.size() - 1);
    }
    rowsChanged(m_following);
}

void LogView::appendAll(const QVector<QPair<Level, QString>>& lines)
{
    if (lines.isEmpty()) {
        return;
    }
    const QDateTime now = QDateTime::currentDateTime();
    for (const auto& entry : lines) {
        Line line;
        line.level = entry.first;
        line.message = entry.second;
        line.when = now;
        m_lines.append(line);
    }
    if (m_lines.size() > m_maximum) {
        m_lines.remove(0, m_lines.size() - m_maximum);
    }
    rebuildVisible();
}

void LogView::clear()
{
    m_lines.clear();
    m_visible.clear();
    rowsChanged(false);
}

int LogView::visibleLineCount() const
{
    return m_visible.size();
}

QString LogView::lineText(int index) const
{
    return index >= 0 && index < m_lines.size() ? m_lines.at(index).message : QString();
}

LogView::Level LogView::lineLevel(int index) const
{
    return index >= 0 && index < m_lines.size() ? m_lines.at(index).level : Level::Info;
}

void LogView::setMaximumLines(int lines)
{
    const int clamped = qMax(1, lines);
    if (m_maximum == clamped) {
        return;
    }
    m_maximum = clamped;
    if (m_lines.size() > m_maximum) {
        m_lines.remove(0, m_lines.size() - m_maximum);
    }
    rebuildVisible();
}

void LogView::setMinimumLevel(Level level)
{
    if (m_minimum == level) {
        return;
    }
    m_minimum = level;
    rebuildVisible();
}

void LogView::setFilter(const QString& text)
{
    if (m_filter == text) {
        return;
    }
    m_filter = text;
    rebuildVisible();
}

void LogView::setFollowing(bool following)
{
    if (m_following == following) {
        return;
    }
    m_following = following;
    if (following) {
        scrollToEnd();
    }
    emit followingChanged(following);
}

void LogView::setShowTimestamps(bool show)
{
    if (m_timestamps == show) {
        return;
    }
    m_timestamps = show;
    viewport()->update();
}

QString LogView::selectedText() const
{
    QStringList out;
    out.reserve(m_visible.size());
    for (const int index : m_visible) {
        const Line& line = m_lines.at(index);
        QString row;
        if (m_timestamps) {
            row += line.when.toString(QStringLiteral("HH:mm:ss")) + QLatin1Char(' ');
        }
        row += levelName(line.level).leftJustified(7) + QLatin1Char(' ') + line.message;
        out.append(row);
    }
    return out.join(QLatin1Char('\n'));
}

void LogView::scrollContentsBy(int dx, int dy)
{
    MonoLineView::scrollContentsBy(dx, dy);

    // Following is a consequence of where the user is, not a mode they set. A
    // log that jumps back to the bottom while someone is reading history is the
    // single most complained-about behaviour these consoles had.
    const bool atEnd = isAtEnd();
    if (atEnd != m_following) {
        m_following = atEnd;
        emit followingChanged(m_following);
    }
}

QColor LogView::levelColor(Level level) const
{
    const ThemeProfile& profile = themeFor(this);
    switch (level) {
    case Level::Debug:   return profile.colors.textDisabled;
    case Level::Info:    return profile.semantic.info.text;
    case Level::Warning: return profile.semantic.warning.text;
    case Level::Error:   return profile.semantic.danger.text;
    }
    return profile.colors.textSecondary;
}

void LogView::append(Level level, const QString& source, const QString& message,
                     const QDateTime& when)
{
    append(level, message, when);
    if (!m_lines.isEmpty()) {
        m_lines.last().source = source;
    }
    update();
}

void LogView::setSourceColumn(int characters)
{
    if (m_sourceColumn == characters) {
        return;
    }
    m_sourceColumn = qMax(0, characters);
    update();
}

void LogView::paintRow(QPainter* painter, int row, const QRect& box)
{
    const ThemeProfile& profile = themeFor(this);
    const QFontMetrics fm(rowFont());
    const Line& line = m_lines.at(m_visible.at(row));

    const int timestampWidth = m_timestamps
        ? fm.horizontalAdvance(QStringLiteral("00:00:00")) + kColumnGap
        : 0;

    int x = kPaddingH;
    if (m_timestamps) {
        painter->setPen(profile.colors.textDisabled);
        painter->drawText(QRect(x, box.top(), timestampWidth - kColumnGap, box.height()),
                          Qt::AlignLeft | Qt::AlignVCenter,
                          line.when.toString(QStringLiteral("HH:mm:ss")));
        x += timestampWidth;
    }

    // Who wrote it, before what it says: the design reads the column as an
    // address, and an address goes in front of the message.
    if (m_sourceColumn > 0) {
        const int width = m_sourceColumn * fm.horizontalAdvance(QLatin1Char('0'));
        painter->setPen(profile.colors.textSecondary);
        painter->drawText(QRect(x, box.top(), width, box.height()),
                          Qt::AlignLeft | Qt::AlignVCenter,
                          fm.elidedText(line.source, Qt::ElideRight, width));
        x += width + kColumnGap;
    }

    painter->setPen(levelColor(line.level));
    painter->drawText(QRect(x, box.top(), kLevelColumn, box.height()),
                      Qt::AlignLeft | Qt::AlignVCenter, levelName(line.level));
    x += kLevelColumn + kColumnGap;

    painter->setPen(profile.colors.textSecondary);
    const QRect text(x, box.top(), qMax(0, box.width() - x - kPaddingH), box.height());
    // Elided rather than wrapped. A wrapped log line breaks the level column
    // alignment that is the point of the layout, and a stack trace wrapped to
    // four lines each is a screen that shows four messages.
    painter->drawText(text, Qt::AlignLeft | Qt::AlignVCenter,
                      fm.elidedText(line.message, Qt::ElideRight, text.width()));
}

} // namespace novu::ui
