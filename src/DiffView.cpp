#include "novu/ui/DiffView.h"

#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"

#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QRegularExpression>

namespace novu::ui {

namespace {

// `.diff div { padding:0 12px }`.
constexpr int kTextPadding = 12;
constexpr int kGutterPadding = 8;
constexpr int kGutterGap = 6;

// The design tints a changed row at .10 of its hue and a hunk header at .07 of
// the accent. Named because the three are a set: change any one and a diff
// with many small hunks stops reading as a sequence.
constexpr double kChangeTint = 0.10;
constexpr double kHunkTint = 0.07;

QString marker(DiffView::Kind kind)
{
    switch (kind) {
    case DiffView::Kind::Added:   return QStringLiteral("+");
    case DiffView::Kind::Removed: return QStringLiteral("-");
    case DiffView::Kind::Context: return QStringLiteral(" ");
    case DiffView::Kind::Hunk:
    case DiffView::Kind::File:    break;
    }
    return QString();
}

} // namespace

DiffView::DiffView(QWidget* parent)
    : MonoLineView(parent)
{
}

void DiffView::setLines(const QVector<Line>& lines)
{
    m_lines = lines;
    m_added = 0;
    m_removed = 0;
    for (const Line& line : m_lines) {
        if (line.kind == Kind::Added) {
            ++m_added;
        } else if (line.kind == Kind::Removed) {
            ++m_removed;
        }
    }
    rowsChanged(false);
}

void DiffView::addLine(const Line& line)
{
    m_lines.append(line);
    if (line.kind == Kind::Added) {
        ++m_added;
    } else if (line.kind == Kind::Removed) {
        ++m_removed;
    }
    rowsChanged(false);
}

void DiffView::clear()
{
    m_lines.clear();
    m_added = 0;
    m_removed = 0;
    rowsChanged(false);
}

DiffView::Line DiffView::lineAt(int index) const
{
    return index >= 0 && index < m_lines.size() ? m_lines.at(index) : Line();
}

void DiffView::setShowLineNumbers(bool show)
{
    if (m_numbers == show) {
        return;
    }
    m_numbers = show;
    viewport()->update();
}

QVector<DiffView::Line> DiffView::parseUnified(const QString& text)
{
    // `@@ -12,7 +12,9 @@ optional trailing context`. The counts are optional
    // in the format - `@@ -1 +1 @@` is legal for a single-line hunk - so they
    // are matched as optional rather than required.
    static const QRegularExpression hunkHeader(
        QStringLiteral(R"(^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@)"));

    QVector<Line> lines;
    int oldNumber = 0;
    int newNumber = 0;

    const QStringList rows = text.split(QLatin1Char('\n'));
    for (const QString& raw : rows) {
        // A trailing \r from a file written on Windows would otherwise be drawn
        // as a box at the end of every line.
        const QString row = raw.endsWith(QLatin1Char('\r')) ? raw.chopped(1) : raw;

        const QRegularExpressionMatch hunk = hunkHeader.match(row);
        if (hunk.hasMatch()) {
            oldNumber = hunk.captured(1).toInt();
            newNumber = hunk.captured(3).toInt();
            lines.append({Kind::Hunk, row, -1, -1});
            continue;
        }

        if (row.startsWith(QLatin1String("diff --git"))
            || row.startsWith(QLatin1String("index "))
            || row.startsWith(QLatin1String("--- "))
            || row.startsWith(QLatin1String("+++ "))
            || row.startsWith(QLatin1String("new file"))
            || row.startsWith(QLatin1String("deleted file"))
            || row.startsWith(QLatin1String("similarity index"))
            || row.startsWith(QLatin1String("rename "))) {
            lines.append({Kind::File, row, -1, -1});
            continue;
        }

        if (row.startsWith(QLatin1Char('+'))) {
            lines.append({Kind::Added, row.mid(1), -1, newNumber++});
            continue;
        }
        if (row.startsWith(QLatin1Char('-'))) {
            lines.append({Kind::Removed, row.mid(1), oldNumber++, -1});
            continue;
        }
        if (row.startsWith(QLatin1String("\\ No newline"))) {
            lines.append({Kind::Context, row, -1, -1});
            continue;
        }

        // Everything else is context, including a body line whose leading space
        // a tool stripped. Showing it unstyled beats dropping it.
        const QString body = row.startsWith(QLatin1Char(' ')) ? row.mid(1) : row;
        lines.append({Kind::Context, body, oldNumber++, newNumber++});
    }

    // A trailing newline in the input produces one empty context line that was
    // never in the diff.
    if (!lines.isEmpty() && lines.last().kind == Kind::Context
        && lines.last().text.isEmpty()) {
        lines.removeLast();
    }
    return lines;
}

void DiffView::setUnifiedDiff(const QString& text)
{
    setLines(parseUnified(text));
}

int DiffView::gutterWidth() const
{
    if (!m_numbers) {
        return 0;
    }
    // Sized to the widest number either side reaches, so the body does not
    // shift when the view scrolls past line 1000.
    int widest = 1;
    for (const Line& line : m_lines) {
        widest = qMax(widest, qMax(line.oldNumber, line.newNumber));
    }
    const int digits = QString::number(qMax(1, widest)).size();
    const QFontMetrics fm(rowFont());
    const int column = fm.horizontalAdvance(QString(digits, QLatin1Char('0')));
    return 2 * column + kGutterGap + 2 * kGutterPadding;
}

void DiffView::mouseDoubleClickEvent(QMouseEvent* event)
{
    const int row = firstVisibleRow() + event->position().toPoint().y() / rowHeight();
    if (row >= 0 && row < m_lines.size()) {
        emit lineActivated(row);
    }
    MonoLineView::mouseDoubleClickEvent(event);
}

void DiffView::paintRow(QPainter* painter, int row, const QRect& box)
{
    const ThemeProfile& profile = themeFor(this);
    const Line& line = m_lines.at(row);
    const QFontMetrics fm(rowFont());

    QColor ink = profile.colors.textSecondary;
    QColor tint;

    switch (line.kind) {
    case Kind::Added:
        ink = profile.semantic.success.text;
        tint = withAlpha(profile.semantic.success.text, kChangeTint);
        break;
    case Kind::Removed:
        ink = profile.semantic.danger.text;
        tint = withAlpha(profile.semantic.danger.text, kChangeTint);
        break;
    case Kind::Hunk:
        ink = profile.colors.accentLight;
        tint = withAlpha(profile.colors.accent, kHunkTint);
        break;
    case Kind::File:
        ink = profile.colors.textPrimary;
        tint = profile.colors.panelBg;
        break;
    case Kind::Context:
        ink = profile.colors.textDisabled;
        break;
    }

    // Edge to edge: a tint that stopped at the text would leave the gutter
    // untinted and the row would read as two rows.
    if (tint.isValid() && tint.alpha() > 0) {
        painter->fillRect(box, tint);
    }

    int x = 0;
    const int gutter = gutterWidth();
    if (gutter > 0) {
        const int column = (gutter - kGutterGap - 2 * kGutterPadding) / 2;
        painter->setPen(withAlpha(profile.colors.textDisabled, 0.75));
        if (line.oldNumber > 0) {
            painter->drawText(QRect(kGutterPadding, box.top(), column, box.height()),
                              Qt::AlignRight | Qt::AlignVCenter,
                              QString::number(line.oldNumber));
        }
        if (line.newNumber > 0) {
            painter->drawText(QRect(kGutterPadding + column + kGutterGap, box.top(),
                                    column, box.height()),
                              Qt::AlignRight | Qt::AlignVCenter,
                              QString::number(line.newNumber));
        }
        painter->setPen(QPen(profile.colors.borderSubtle, 1));
        painter->drawLine(gutter - 1, box.top(), gutter - 1, box.bottom());
        x = gutter;
    }

    x += kTextPadding;

    const QString mark = marker(line.kind);
    if (!mark.isEmpty()) {
        painter->setPen(ink);
        painter->drawText(QRect(x, box.top(), fm.horizontalAdvance(QStringLiteral("0")),
                                box.height()),
                          Qt::AlignLeft | Qt::AlignVCenter, mark);
        x += fm.horizontalAdvance(QStringLiteral("0"));
    }

    painter->setPen(ink);
    // `white-space:pre` — a diff line is bytes, so tabs and runs of spaces stay
    // as they are and nothing is wrapped. A wrapped diff line is one whose
    // column no longer matches the line above it.
    const QRect text(x, box.top(), qMax(0, box.width() - x - kTextPadding), box.height());
    painter->drawText(text, Qt::AlignLeft | Qt::AlignVCenter,
                      fm.elidedText(line.text, Qt::ElideRight, text.width()));
}

} // namespace novu::ui
