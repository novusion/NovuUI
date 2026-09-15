#pragma once

#include "novu/ui/Badge.h"
#include "novu/ui/StyleSpec.h"
#include "novu/ui/ThemeProfile.h"

#include <QColor>
#include <QStringList>
#include <QVector>

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

    QString text() const { return m_text; }
    void setText(const QString& text);
    void setTone(Badge::Tone tone);

    // Width is a preference, never a measurement. A banner is handed the
    // messages nobody budgeted for - a linker error, a stack trace, a path
    // from a machine with deep folders - and one that sized itself to its
    // sentence would drag its window off the screen. Long text elides and the
    // whole of it stays on the tooltip.
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    // The sentence is painted rather than held in a child QLabel: it is one
    // line of prose nobody interacts with, and the label was both a second
    // widget and the thing whose sizeHint had to be fought.
    QString m_text;
    Badge::Tone m_tone = Badge::Tone::Info;
};

// A proportion: how much of something is done, or how much of it is used.
//
// The design's `.meter` and `.prog`. Two jobs, and they are the same drawing:
// a backup that is 62% complete and a disk that is 62% full are both a filled
// track. What differs is what the number *means*, which is what Tone says.
//
// ── coloured by what the proportion means ────────────────────────────────
//
// Ported from NovuAdmin's `UsageBar`, whose two ideas are worth keeping.
//
// The first is Tone::Threshold: a *usage* bar picks its own colour — amber past
// 75%, red past 90% — because a full disk has to read as full without anyone
// reading the number. A *progress* bar must not do this, or every download
// turns red as it finishes. So the behaviour is a tone the caller chooses
// rather than something the component decides for itself.
//
// The second is `setUnknown()`. An unmeasured reading draws an empty track
// rather than a zero-length fill, because "not measured" and "idle" must not
// look the same — and a value of 0 cannot say which one it is.
class ProgressBar : public QWidget
{
    Q_OBJECT

public:
    enum class Tone {
        Accent,     // work in progress: always the product accent
        Threshold,  // a proportion of a capacity: amber past 75%, red past 90%
        Success,
        Warning,
        Danger,
    };

    explicit ProgressBar(int value = 0, QWidget* parent = nullptr);
    ProgressBar(int value, Tone tone, QWidget* parent);

    // 0..100. Clamped, because a caller computing a percentage from two live
    // counters will hand over 101 sooner or later.
    int value() const { return m_value; }
    void setValue(int value);

    Tone tone() const { return m_tone; }
    void setTone(Tone tone);

    // Nothing has been measured. The track is drawn empty and no fill at all,
    // which is a different picture from a measured zero.
    bool isUnknown() const { return m_unknown; }
    void setUnknown(bool unknown);

    // The design's legend under the track: what is used on the left, what is
    // left on the right. Either may be empty; both empty draws no legend line
    // and the bar keeps its bare height.
    void setLegend(const QString& leading, const QString& trailing = {});
    QString leadingLegend() const { return m_leading; }
    QString trailingLegend() const { return m_trailing; }

    // Which tone a Threshold bar currently resolves to. Exposed so a test can
    // assert that 91% is a danger reading without sampling a pixel.
    SemanticRole effectiveRole() const;

    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    bool event(QEvent* event) override;

private:
    int m_value = 0;
    Tone m_tone = Tone::Accent;
    bool m_unknown = false;
    QString m_leading;
    QString m_trailing;
};

// A byte count in the units a person reads: "1.4 GB", "812 kB", "0 B".
//
// Shared because a size in a table, a size under a progress bar and a size in
// a tooltip have to be written the same way — NovuAdmin had two of these and
// they disagreed about whether a kilobyte was 1000 or 1024. This one is 1024,
// with the SI-style short names those tools actually print.
QString formatBytes(qint64 bytes);

// ── waiting ────────────────────────────────────────────────────────────────
//
// Three shapes for three different kinds of wait, and the difference between
// them is not decoration.
//
// Spinner says "something is happening and I cannot tell you how long".
// ProgressBar above says "this much of a known amount is done". Skeleton says
// "the shape of what is coming is already known, only the content is missing" —
// which is why it is the right answer for a table or a detail panel and the
// wrong one for a save.
//
// All three run on the shared Pulse clock. A window showing a spinner, four
// skeleton rows and a breathing status dot wakes the CPU once per frame, not
// six times.

// The design's `.sp`: a faint ring with one bright quarter turning round it.
//
// Sized from the profile's icon metrics rather than from a pixel count, so it
// matches the glyphs beside it at every control size.
class Spinner : public QWidget
{
    Q_OBJECT

public:
    explicit Spinner(QWidget* parent = nullptr);
    Spinner(ControlSize size, QWidget* parent);

    ControlSize controlSize() const { return m_size; }
    void setControlSize(ControlSize size);

    // Stops the animation and the subscription with it. A spinner left running
    // behind a finished operation is the most common reason a "quiet" window
    // still repaints sixty times a second.
    bool isSpinning() const { return m_spinning; }
    void setSpinning(bool spinning);

    // Accent by default. A spinner inside a Primary button has to turn in the
    // ink of that button instead, and that is the only reason this exists.
    void setHeadColor(const QColor& color);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    bool event(QEvent* event) override;

private:
    void updateMotion();

    ControlSize m_size = ControlSize::Medium;
    bool m_spinning = true;
    bool m_subscribed = false;
    // Invalid means "the profile's accent", resolved at paint time. Held as an
    // explicit override rather than as a resolved default, so a theme change
    // moves the untouched case and leaves a deliberate one alone.
    QColor m_head;
};

// One placeholder plate with the design's `shimmer` sweeping across it.
//
// A plate, not a widget per line: `SkeletonList` below draws a whole table of
// them in a single widget, because forty placeholder rows that are each three
// QWidgets is a hundred and twenty widgets built to be thrown away.
class Skeleton : public QWidget
{
    Q_OBJECT

public:
    enum class Line {
        Text,   // the design's `.skel`  — one line of prose
        Block,  // the design's `.skel.tall` — a value, a heading, a control
    };

    explicit Skeleton(QWidget* parent = nullptr);
    Skeleton(Line line, QWidget* parent);

    Line line() const { return m_line; }
    void setLine(Line line);

    // 0..1 of the available width. Placeholder lines of identical length read
    // as a grid rather than as text, so callers vary them.
    double fill() const { return m_fill; }
    void setFill(double fraction);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    bool event(QEvent* event) override;

private:
    void updateMotion();

    Line m_line = Line::Text;
    double m_fill = 1.0;
    bool m_subscribed = false;
};

// The design's `.skelrow` repeated: what a table shows while its first page is
// in flight.
//
// One widget for the whole list. The row height and the separators are the
// profile's table tokens, so the placeholder and the table that replaces it are
// the same shape and the panel does not jump when the data lands.
class SkeletonList : public QWidget
{
    Q_OBJECT

public:
    // `columns` are the width fractions of the plates in each row; they are
    // normalised, so {3, 1, 1} and {0.6, 0.2, 0.2} draw the same thing.
    SkeletonList(int rows, const QVector<double>& columns, QWidget* parent = nullptr);
    explicit SkeletonList(QWidget* parent = nullptr);

    int rowCount() const { return m_rows; }
    void setRowCount(int rows);
    void setColumns(const QVector<double>& columns);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    bool event(QEvent* event) override;

private:
    void updateMotion();

    int m_rows = 5;
    QVector<double> m_columns{3.0, 1.0, 1.0};
    bool m_subscribed = false;
};

// The design's `.sline`: a sentence about one operation, with a mark in front
// of it saying where that operation is.
//
// The reason this is a component and not a QLabel with an icon: the three
// states are a sequence, and the transition between them is the whole point.
// A line that says "Checking credentials" with a turning mark and then says
// "Connected" with a tick is one row that reported on itself. Two labels swapped
// by a page is two rows that happened to occupy the same place.
class StatusLine : public QWidget
{
    Q_OBJECT

public:
    enum class State {
        Idle,     // not started; neutral ink, no mark
        Running,  // the spinner
        Done,     // a tick, in the success ink
        Failed,   // a cross, in the danger ink
    };

    StatusLine(const QString& text, State state = State::Idle,
               QWidget* parent = nullptr);

    QString text() const { return m_text; }
    void setText(const QString& text);

    State state() const { return m_state; }
    // Sets both at once, because in practice they always change together and
    // two calls means one repaint with the new mark against the old sentence.
    void setState(State state, const QString& text = {});

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent* event) override;
    bool event(QEvent* event) override;

private:
    QString m_text;
    State m_state = State::Idle;
    Spinner* m_spinner = nullptr;
};

// The design's `.steps`: where a multi-part operation has got to.
//
// A row of tracked, uppercase pills — done, current, still to come. It is the
// third member of the waiting family and the one that answers a question the
// other two cannot: ProgressBar says how far, Spinner says "still working", and
// this says *which part*. "Verifying" and "Uploading" are not 40% and 70% of
// the same bar, and a user watching a long import wants the name of the stage.
//
// Painted, one widget. Six pills that never change after construction have no
// business being six QLabels with three stylesheets between them.
class Steps : public QWidget
{
    Q_OBJECT

public:
    explicit Steps(QWidget* parent = nullptr);
    Steps(const QStringList& steps, QWidget* parent, int current = 0);

    int addStep(const QString& label);
    void clear();
    int count() const { return m_steps.size(); }
    QString stepText(int index) const;

    // Everything before `current` reads as done, `current` as in progress, and
    // everything after it as pending. One index rather than a state per step,
    // because a sequence that can be done at step four and pending at step two
    // is not a sequence.
    int currentIndex() const { return m_current; }
    void setCurrentIndex(int index);
    // Past the last step: every pill done, nothing current.
    void markComplete();
    bool isComplete() const;

    // Where a pill is, for a test that wants to assert on the layout rather
    // than photograph it.
    QRect stepRect(int index) const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent* event) override;
    bool event(QEvent* event) override;

private:
    int stepWidth(int index) const;

    QStringList m_steps;
    int m_current = 0;
};

} // namespace novu::ui
