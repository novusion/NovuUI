#include "novu/ui/MonoLineView.h"

#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>
#include <QPainter>
#include <QResizeEvent>
#include <QScrollBar>

namespace novu::ui {

MonoLineView::MonoLineView(QWidget* parent)
    : QAbstractScrollArea(parent)
{
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    viewport()->setAutoFillBackground(false);
}

QFont MonoLineView::rowFont() const
{
    const ThemeProfile& profile = themeFor(this);
    QFont font = roleMonoFont(this, TypeRole::Mono);
    font.setPixelSize(profile.type.xs);
    return font;
}

int MonoLineView::rowHeight() const
{
    // A line-height is a multiple of the *font size*, which is what the
    // design states: 1.65 for a diff, 1.75 for a log. Multiplying the font's
    // metric height instead -- ascent plus descent plus leading -- adds about
    // a third to every row, so both views came out that much airier than the
    // design they were measured from, in every application at once.
    const QFont font = rowFont();
    const int line = qRound(font.pixelSize() * lineLeading());
    // Never shorter than the glyphs themselves: a face with tall ascenders
    // would otherwise clip against the row above it.
    return qMax(QFontMetrics(font).height(), qMax(1, line));
}

int MonoLineView::contentHeight() const
{
    return rowCount() * rowHeight() + 2 * contentPaddingV();
}

void MonoLineView::setFitToContent(bool fit, int maxRows)
{
    if (m_fitToContent == fit && m_maxRows == maxRows) {
        return;
    }
    m_fitToContent = fit;
    m_maxRows = qMax(0, maxRows);
    // A view that sizes to its rows must not also be stretched by the column
    // it sits in, or the hint is the one thing the layout ignores.
    setSizePolicy(sizePolicy().horizontalPolicy(),
                  fit ? QSizePolicy::Fixed : QSizePolicy::Expanding);
    updateGeometry();
}

QSize MonoLineView::sizeHint() const
{
    if (!m_fitToContent) {
        return QAbstractScrollArea::sizeHint();
    }
    QSize hint = QAbstractScrollArea::sizeHint();
    int height = contentHeight();
    if (m_maxRows > 0) {
        height = qMin(height, m_maxRows * rowHeight() + 2 * contentPaddingV());
    }
    hint.setHeight(height);
    return hint;
}

QSize MonoLineView::minimumSizeHint() const
{
    if (!m_fitToContent) {
        return QAbstractScrollArea::minimumSizeHint();
    }
    // The width stays the base class's -- a diff may be scrolled sideways --
    // but the height is the one it will be given, so nothing can stretch it.
    QSize hint = QAbstractScrollArea::minimumSizeHint();
    hint.setHeight(sizeHint().height());
    return hint;
}

int MonoLineView::firstVisibleRow() const
{
    return verticalScrollBar()->value();
}

int MonoLineView::visibleRowCount() const
{
    const int usable = viewport()->height() - 2 * contentPaddingV();
    // One extra, because a row is usually clipped at the bottom edge and a
    // view that stopped at the last whole row would leave a strip of ground.
    return qMax(0, qMin(rowCount() - firstVisibleRow(), usable / rowHeight() + 2));
}

void MonoLineView::updateScrollRange(bool keepAtEnd)
{
    const int perPage =
        qMax(1, (viewport()->height() - 2 * contentPaddingV()) / rowHeight());

    QScrollBar* bar = verticalScrollBar();
    const bool wasAtEnd = bar->value() >= bar->maximum();

    bar->setPageStep(perPage);
    bar->setSingleStep(1);
    bar->setRange(0, qMax(0, rowCount() - perPage));

    if (keepAtEnd || wasAtEnd) {
        bar->setValue(bar->maximum());
    }
}

void MonoLineView::rowsChanged(bool keepAtEnd)
{
    if (m_fitToContent) {
        updateGeometry();
    }
    updateScrollRange(keepAtEnd);
    viewport()->update();
}

bool MonoLineView::isAtEnd() const
{
    const QScrollBar* bar = verticalScrollBar();
    return bar->value() >= bar->maximum();
}

void MonoLineView::scrollToEnd()
{
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

void MonoLineView::scrollToRow(int row)
{
    if (row < 0 || row >= rowCount()) {
        return;
    }
    QScrollBar* bar = verticalScrollBar();
    if (row < bar->value()) {
        bar->setValue(row);
        return;
    }
    const int last = bar->value() + bar->pageStep() - 1;
    if (row > last) {
        bar->setValue(qMin(bar->maximum(), row - bar->pageStep() + 1));
    }
}

void MonoLineView::resizeEvent(QResizeEvent* event)
{
    QAbstractScrollArea::resizeEvent(event);
    updateScrollRange(false);
}

void MonoLineView::scrollContentsBy(int dx, int dy)
{
    QAbstractScrollArea::scrollContentsBy(dx, dy);
    viewport()->update();
}

bool MonoLineView::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        // The row height is a function of the profile's type scale, so a theme
        // change moves it and the scroll range with it.
        updateScrollRange(false);
        viewport()->update();
    }
    return QAbstractScrollArea::event(event);
}

StyleSpec MonoLineView::styleSpec() const
{
    StyleSpec spec;
    spec.background = token(groundRole());
    spec.foreground = token(ColorRole::TextSecondary);
    spec.border = token(ColorRole::Transparent);
    spec.radius = SurfaceRole::Table;
    spec.borderWidth = BorderRole::Table;
    spec.type = TypeRole::Mono;
    return spec;
}

ResolvedStyle MonoLineView::resolvedStyle() const
{
    return resolve(styleSpec(), this, 0);
}

void MonoLineView::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);

    QPainter painter(viewport());
    painter.fillRect(viewport()->rect(), token(groundRole()).resolve(profile));
    painter.setFont(rowFont());

    const int height = rowHeight();
    const int first = firstVisibleRow();
    const int rows = visibleRowCount();

    int y = contentPaddingV();
    for (int i = 0; i < rows; ++i) {
        const int row = first + i;
        if (row < 0 || row >= rowCount()) {
            break;
        }
        // The full viewport width: a subclass that tints a whole row - a diff
        // does - needs the tint to run edge to edge, and one that does not can
        // simply inset by contentPaddingH() itself.
        paintRow(&painter, row, QRect(0, y, viewport()->width(), height));
        y += height;
    }
}

} // namespace novu::ui
