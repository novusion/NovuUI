#pragma once

#include "novu/ui/Icon.h"
#include "novu/ui/ItemStrip.h"
#include "novu/ui/StyleSpec.h"

#include <QString>
#include <QStringList>
#include <QVector>
#include <QWidget>

namespace novu::ui {

// Two or more mutually exclusive choices in one tray, one of them in force.
//
// The design's `.chipset`: a well with a raised plate sliding between its
// segments. It is a *picker*, not a row of commands — whatever is chosen stays
// chosen and stays visible, which is what separates it from a toolbar.
//
// One component for both shapes the designs use. NovuSpace sets its segments in
// words (the density presets: Basic / Extended / Pro / MAX); NovuHub sets them
// as glyphs (a view mode). Those are the same control with different content,
// so they are one class with `addSegment()` overloaded rather than two classes
// that will drift.
//
// ── on painting the segments ──────────────────────────────────────────────
//
// The segments are painted, not child buttons. A tray of four is four more
// widgets, four more polish passes and four more places for a stray stylesheet
// to land, for four labels nobody can select, focus or drag.
//
// The hit testing used to live here, with a note saying it would move out the
// moment a second component needed it. ModuleBar is that second component, so
// it has: ItemStrip now owns where an item is, which one the press landed in,
// that a release elsewhere cancels, and the arrow keys. What is left here is
// the tray, the sliding plate and the type — which is all this class was ever
// about.
class Segmented : public ItemStrip
{
    Q_OBJECT

public:
    enum class Variant { Natural, RailSwitcher };

    explicit Segmented(QWidget* parent = nullptr);
    // The common case in one line, since a picker almost always knows its
    // options up front.
    Segmented(const QStringList& labels, QWidget* parent = nullptr, int current = 0);

    int addSegment(const QString& text);
    int addSegment(Glyph glyph, const QString& tooltip = {});
    int count() const { return m_segments.size(); }
    void clear();

    QString segmentText(int index) const;

    // Marks one segment as the one that needs attention even while another is
    // current — the design does this to the widest density preset, because
    // choosing it has a cost the others do not.
    void setSegmentAttention(int index, bool attention);

    // currentIndex(), setCurrentIndex(), choose(), segmentRect() and indexAt()
    // are ItemStrip's. They kept their names: this control's contract did not
    // change when its machinery moved.

    ControlSize controlSize() const { return m_size; }
    void setControlSize(ControlSize size);

    // A panel-face switcher in a rail/header. This is the shared visual used
    // by Properties | Screen: full available width, medium metrics and equal
    // segments. Keeping it as a variant prevents each product screen from
    // rebuilding the same contract with three independent setter calls.
    Variant variant() const { return m_variant; }
    void setVariant(Variant variant);

    // The segments divide the tray between them instead of each taking the
    // width of its own label -- `.seg button{flex:1}`, which is what a rail
    // head wants: two faces of one panel read as two halves of one control,
    // not as a short word beside a long one. Off by default, so a toolbar
    // picker still sizes to its labels.
    void setFill(bool fill);
    bool fills() const { return m_fill; }

    // The inspection seam: the tray's own contract, and one segment's.
    StyleSpec styleSpec() const;
    StyleSpec segmentStyleSpec(int index) const;
    ResolvedStyle resolvedStyle() const;

    // Where a segment is, for a test that wants to click one without guessing.
    // A name of its own over ItemStrip::itemRect(), because a caller reading a
    // picker's code should see segments rather than items.
    QRect segmentRect(int index) const { return itemRect(index); }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

signals:
    void currentIndexChanged(int index);

protected:
    void paintEvent(QPaintEvent* event) override;
    bool event(QEvent* event) override;

    // What ItemStrip asks about the items it is driving.
    int stripCount() const override { return m_segments.size(); }
    int stripItemWidth(int index) const override;
    void stripItemChosen(int index) override;
    QString stripItemTooltip(int index) const override;
    int stripLead() const override;
    int stripGap() const override;
    int stripTop() const override;
    int stripItemHeight() const override;

private:
    struct Segment
    {
        QString text;
        Glyph glyph = Glyph::None;
        QString tooltip;
        bool attention = false;
    };

    int segmentWidth(const Segment& segment) const;
    void refresh();

    QVector<Segment> m_segments;
    ControlSize m_size = ControlSize::Medium;
    bool m_fill = false;
    Variant m_variant = Variant::Natural;
};

} // namespace novu::ui
