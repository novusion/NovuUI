#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/StyleSpec.h"

#include <QColor>
#include <QList>
#include <QWidget>

namespace novu::ui {

// A row of colours, one of them chosen — or several, when it is a filter.
//
// NovuExplorer tags folders with colours and then filters by them, which is the
// same row asked two questions: "what colour is this?" takes one answer, "show
// me any of these" takes many. The design draws them identically, so they are
// one component with a flag rather than two that will drift.
//
// Ported from NovusionUI's `SwatchRow`, keeping the three things it got right:
// the "no colour" swatch is part of the row rather than a button beside it, the
// custom swatch asks rather than picking, and the row reports its height for a
// given width — without that a layout asks for a hint at width zero, gets one
// column and eleven rows, and leaves a hole the size of the whole palette.
//
// Painted, with hit testing. Eleven colours is eleven paint calls; as eleven
// checkable buttons it is eleven widgets, eleven polish passes and eleven
// stylesheets for something with no text in it.
class NOVUUI_API Swatch : public QWidget
{
    Q_OBJECT

public:
    explicit Swatch(QWidget* parent = nullptr);

    // `withNone` puts the "no colour" swatch first; `withCustom` adds the
    // multi-hued one that opens a picker.
    void setColors(const QList<QColor>& colors, bool withNone = true,
                   bool withCustom = false);
    QList<QColor> colors() const { return m_colors; }

    // Single select. An invalid colour means none.
    QColor current() const { return m_current; }
    void setCurrent(const QColor& color);

    // A filter asks "any of these", so its swatches toggle rather than
    // replacing one another.
    bool isMultiSelect() const { return m_multi; }
    void setMultiSelect(bool multi);
    QList<QColor> chosen() const { return m_chosen; }
    void setChosen(const QList<QColor>& colors);

    int swatchSize() const { return m_size; }
    void setSwatchSize(int px);

    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

    // Where the swatches are, so a test clicks one rather than a coordinate.
    QRect swatchRect(int index) const;
    int indexAt(const QPoint& position) const;
    int count() const;

    QSize sizeHint() const override;
    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override;

signals:
    // Which swatch was pressed. Emitted for every pick, single or multi.
    void picked(const QColor& color);
    // The row now holds these. Multi-select only.
    void chosenChanged(const QList<QColor>& colors);
    // The custom swatch. It asks; it does not open anything itself, because a
    // colour dialog is the application's to choose.
    void customRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    bool event(QEvent* event) override;

private:
    int columns(int width) const;
    bool isNoneAt(int index) const;
    bool isCustomAt(int index) const;
    QColor colorAt(int index) const;

    QList<QColor> m_colors;
    QList<QColor> m_chosen;
    QColor m_current;
    int m_size = 20;
    int m_hovered = -1;
    int m_pressed = -1;
    bool m_multi = false;
    bool m_withNone = true;
    bool m_withCustom = false;
};

// The colours the design tags with. A palette rather than a free picker: a
// folder list where every colour is slightly different is a folder list with no
// colours in it.
QList<QColor> tagPalette(const QWidget* context);

} // namespace novu::ui
