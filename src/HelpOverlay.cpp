#include "novu/ui/HelpOverlay.h"

#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace novu::ui {

namespace {

// The gap between a bubble and the thing it points at.
//
// Deliberately wide - two of the largest spacing steps. At one step the bubble
// sits against its target and the leader degenerates into a join, which loses
// the one thing the leader is for: saying *which* control the note is about
// when several are close together. The line has to be long enough to be read
// as a line.
int leaderGap(const ThemeProfile& profile)
{
    return profile.spacing.xl + profile.spacing.lg;
}

// How far a bubble is kept from the edge of the window.
int frameMargin(const ThemeProfile& profile)
{
    return profile.spacing.md;
}

// The point on `rect` closest to `from` - where a leader should land.
QPointF anchorOn(const QRect& rect, const QPointF& from)
{
    return QPointF(qBound<qreal>(rect.left(), from.x(), rect.right()),
                   qBound<qreal>(rect.top(), from.y(), rect.bottom()));
}

} // namespace

HelpOverlay::HelpOverlay(QWidget* host)
    : QWidget(host)
    , m_host(host)
{
    Q_ASSERT_X(host, "HelpOverlay", "an overlay needs a window to annotate");

    // The whole point: the application underneath keeps working. A user reads
    // the note about the toolbar and then uses the toolbar, without dismissing
    // anything. An overlay that swallowed clicks would be a tour, and a tour
    // stops the application in order to explain it.
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    // No scrim. Dimming the window would make the thing being explained the
    // hardest thing on screen to look at.
    setAttribute(Qt::WA_TranslucentBackground, true);
    setFocusPolicy(Qt::NoFocus);

    setGeometry(host->rect());
    host->installEventFilter(this);
    raise();
}

HelpOverlay* HelpOverlay::attach(QWidget* host)
{
    return host ? new HelpOverlay(host) : nullptr;
}

void HelpOverlay::addNote(QWidget* target, const QString& title, const QString& body,
                          HelpPlacement placement, HelpTone tone)
{
    Note note;
    note.target = target;
    note.title = title;
    note.body = body;
    note.placement = placement;
    note.tone = tone;
    m_notes.append(note);
    update();
}

void HelpOverlay::clear()
{
    m_notes.clear();
    update();
}

int HelpOverlay::noteWidth() const
{
    if (m_noteWidth > 0) {
        return m_noteWidth;
    }
    // Derived rather than a constant: a bubble is about a dozen words to a
    // line, and what a dozen words measure depends on the profile's type scale.
    return themeFor(this).type.md * 14;
}

void HelpOverlay::setNoteWidth(int width)
{
    if (m_noteWidth == width) {
        return;
    }
    m_noteWidth = width;
    update();
}

QColor HelpOverlay::colourFor(HelpTone tone) const
{
    const ThemeProfile& profile = themeFor(this);
    switch (tone) {
    case HelpTone::Info:    return profile.semantic.info.text;
    case HelpTone::Success: return profile.semantic.success.text;
    case HelpTone::Warning: return profile.semantic.warning.text;
    case HelpTone::Danger:  return profile.semantic.danger.text;
    case HelpTone::Neutral: return profile.semantic.neutral.text;
    case HelpTone::Accent:  break;
    }
    return profile.colors.accent;
}

int HelpOverlay::bubbleHeightFor(const QString& title, const QString& body, int width) const
{
    const ThemeProfile& profile = themeFor(this);
    const int padding = profile.space(SpacingRole::CardPadding);
    const int inner = width - 2 * padding;

    const QFontMetrics titleMetrics(roleFont(this, TypeRole::CardTitle));
    const QFontMetrics bodyMetrics(roleFont(this, TypeRole::Caption));

    const int titleHeight =
        titleMetrics.boundingRect(QRect(0, 0, inner, 0), Qt::TextWordWrap, title).height();
    const int bodyHeight =
        body.isEmpty()
            ? 0
            : bodyMetrics.boundingRect(QRect(0, 0, inner, 0), Qt::TextWordWrap, body).height();
    const int gap = body.isEmpty() ? 0 : profile.spacing.xs;

    return padding + titleHeight + gap + bodyHeight + padding;
}

QVector<HelpOverlay::Placed> HelpOverlay::layoutNotes() const
{
    const ThemeProfile& profile = themeFor(this);
    const int gap = leaderGap(profile);
    const int margin = frameMargin(profile);
    const int width = noteWidth();

    QVector<Placed> placed;
    QVector<QRect> taken;

    for (const Note& note : m_notes) {
        // A target that has gone away takes its note with it. A leader pointing
        // at nothing is worse than a missing note.
        if (!note.target || !note.target->isVisible()) {
            continue;
        }

        Placed entry;
        entry.note = &note;
        entry.colour = colourFor(note.tone);

        const QPoint origin = note.target == m_host
            ? QPoint(0, 0)
            : note.target->mapTo(m_host, QPoint(0, 0));
        entry.target = QRect(origin, note.target->size());

        const int height = bubbleHeightFor(note.title, note.body, width);

        // Auto picks the side with the most room. Measured rather than
        // guessed, so a note on a widget at the right edge does not put its
        // bubble off-screen and then get clamped back on top of it.
        HelpPlacement placement = note.placement;
        if (placement == HelpPlacement::Auto) {
            const int right = this->width() - entry.target.right();
            const int left = entry.target.left();
            const int below = this->height() - entry.target.bottom();
            const int above = entry.target.top();
            const int best = qMax(qMax(right, left), qMax(below, above));
            placement = best == right  ? HelpPlacement::Right
                      : best == left   ? HelpPlacement::Left
                      : best == below  ? HelpPlacement::Below
                                       : HelpPlacement::Above;
        }

        QRect bubble(0, 0, width, height);
        switch (placement) {
        case HelpPlacement::Right:
            bubble.moveTo(entry.target.right() + gap,
                          entry.target.center().y() - height / 2);
            break;
        case HelpPlacement::Left:
            bubble.moveTo(entry.target.left() - gap - width,
                          entry.target.center().y() - height / 2);
            break;
        case HelpPlacement::Below:
            bubble.moveTo(entry.target.center().x() - width / 2,
                          entry.target.bottom() + gap);
            break;
        case HelpPlacement::Above:
        case HelpPlacement::Auto:
            bubble.moveTo(entry.target.center().x() - width / 2,
                          entry.target.top() - gap - height);
            break;
        }

        // Kept inside the window, then nudged clear of the bubbles already
        // placed. Greedy and in declaration order: a caller that cares about
        // which note wins a crowded corner controls it by ordering, which is
        // simpler to reason about than any packing this could do for them.
        const QRect frame = rect().adjusted(margin, margin, -margin, -margin);
        const auto clampInto = [&frame](QRect& r) {
            r.moveLeft(qBound(frame.left(), r.left(), frame.right() - r.width()));
            r.moveTop(qBound(frame.top(), r.top(), frame.bottom() - r.height()));
        };
        clampInto(bubble);

        const bool vertical = placement == HelpPlacement::Left
                           || placement == HelpPlacement::Right;
        for (int attempt = 0; attempt < 24; ++attempt) {
            bool clash = false;
            for (const QRect& other : taken) {
                if (bubble.intersects(other.adjusted(-4, -4, 4, 4))) {
                    clash = true;
                    break;
                }
            }
            if (!clash) {
                break;
            }
            if (vertical) {
                bubble.moveTop(bubble.top() + height / 2 + 8);
            } else {
                bubble.moveLeft(bubble.left() + width / 2 + 8);
            }
            clampInto(bubble);
        }

        taken.append(bubble);
        entry.bubble = bubble;
        entry.body = note.body;
        placed.append(entry);
    }

    return placed;
}

void HelpOverlay::paintLeader(QPainter& painter, const Placed& placed) const
{
    const QPointF bubbleCentre = QRectF(placed.bubble).center();
    const QPointF targetCentre = QRectF(placed.target).center();

    const QPointF start = anchorOn(placed.bubble, targetCentre);
    const QPointF end = anchorOn(placed.target, bubbleCentre);

    const QPointF delta = end - start;
    const qreal length = std::hypot(delta.x(), delta.y());
    if (length < 1.0) {
        return;
    }

    // A gentle curve rather than a straight rule. Eight leaders crossing a
    // screen as straight lines read as a diagram of the window; curved, they
    // read as annotation over it.
    const QPointF normal(-delta.y() / length, delta.x() / length);
    const QPointF control = (start + end) / 2.0 + normal * (length * 0.12);

    QPainterPath path(start);
    path.quadTo(control, end);

    painter.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(placed.colour);
    pen.setWidthF(1.0);
    pen.setStyle(Qt::DashLine);
    pen.setDashPattern({4.0, 3.0});
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    // The arrowhead points along the curve's tangent at the target, not along
    // the straight line between the two - otherwise it sits at a visible angle
    // to the line it terminates.
    const QPointF tangent = end - control;
    const qreal tangentLength = std::hypot(tangent.x(), tangent.y());
    if (tangentLength < 0.5) {
        return;
    }
    const QPointF direction = tangent / tangentLength;
    const QPointF side(-direction.y(), direction.x());
    const qreal head = 5.0;

    QPainterPath arrow(end);
    arrow.lineTo(end - direction * head + side * (head * 0.5));
    arrow.lineTo(end - direction * head - side * (head * 0.5));
    arrow.closeSubpath();

    painter.setPen(Qt::NoPen);
    painter.setBrush(placed.colour);
    painter.drawPath(arrow);
}

void HelpOverlay::paintNote(QPainter& painter, const Placed& placed) const
{
    const ThemeProfile& profile = themeFor(this);
    const int padding = profile.space(SpacingRole::CardPadding);
    const int radius = profile.radius(SurfaceRole::Overlay);

    const QRectF surface = QRectF(placed.bubble).adjusted(0.5, 0.5, -0.5, -0.5);

    painter.setRenderHint(QPainter::Antialiasing, true);
    QPainterPath path;
    path.addRoundedRect(surface, radius, radius);

    // Opaque, not tinted: a bubble has to be readable over whatever it happens
    // to land on, and what it lands on is a live application.
    painter.fillPath(path, profile.colors.panelBg);
    painter.setPen(QPen(placed.colour, profile.border(BorderRole::Card)));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    const QRect inner = placed.bubble.adjusted(padding, padding, -padding, -padding);

    painter.setFont(roleFont(this, TypeRole::CardTitle));
    const QFontMetrics titleMetrics(painter.font());
    const QRect titleRect =
        titleMetrics.boundingRect(QRect(inner.x(), inner.y(), inner.width(), 0),
                                  Qt::TextWordWrap, placed.note->title);
    // The title takes the note's own colour. It is what tells eight bubbles
    // apart at a glance, which is the job the colour is doing here.
    painter.setPen(placed.colour);
    painter.drawText(QRect(inner.x(), inner.y(), inner.width(), titleRect.height()),
                     Qt::TextWordWrap, placed.note->title);

    if (placed.body.isEmpty()) {
        return;
    }
    painter.setFont(roleFont(this, TypeRole::Caption));
    painter.setPen(profile.colors.textSecondary);
    const int bodyTop = inner.y() + titleRect.height() + profile.spacing.xs;
    painter.drawText(QRect(inner.x(), bodyTop, inner.width(), inner.bottom() - bodyTop + 1),
                     Qt::TextWordWrap, placed.body);
}

void HelpOverlay::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    const QVector<Placed> placed = layoutNotes();
    // Leaders first, so a line that passes under a neighbouring bubble is
    // covered by it rather than drawn across it.
    for (const Placed& entry : placed) {
        paintLeader(painter, entry);
    }
    for (const Placed& entry : placed) {
        paintNote(painter, entry);
    }
}

bool HelpOverlay::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_host) {
        switch (event->type()) {
        case QEvent::Resize:
        // A host resized while it is hidden defers its resize event until it is
        // shown, so an overlay attached before the window appears would sit at
        // the old size until something else moved.
        case QEvent::Show:
            setGeometry(m_host->rect());
            break;
        case QEvent::ChildAdded:
            // A widget added after the overlay would otherwise be stacked on
            // top of it and hide the annotation of its own neighbours.
            raise();
            break;
        case QEvent::LayoutRequest:
            // Anchors are read at paint time, so a re-layout only has to ask
            // for a repaint - the notes follow their targets by themselves.
            update();
            break;
        default:
            break;
        }
    }
    return QWidget::eventFilter(watched, event);
}

bool HelpOverlay::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type()) {
        update();
    }
    if (event->type() == QEvent::Show) {
        raise();
    }
    return QWidget::event(event);
}

} // namespace novu::ui
