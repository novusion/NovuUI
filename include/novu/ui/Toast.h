#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/Badge.h"
#include "novu/ui/Icon.h"
#include "novu/ui/StyleSpec.h"

#include <QPointer>
#include <QString>
#include <QVector>
#include <QWidget>

class QTimer;

namespace novu::ui {

// The line that says something happened, and then goes away.
//
// The approved NovuAI design's `.toast`: a pill on the card-hover surface,
// centred near the bottom of the window, with a mark, a sentence and at most
// one action. It rises into place and fades out on its own.
//
// ── what it is for, and what it is not ───────────────────────────────────
//
// A toast confirms. "Copied", "Workspace archived", "Sent to the agent" — an
// outcome the user already expected, acknowledged without interrupting them.
//
// It is not for anything the user has to act on, and the component enforces
// that as far as a component can: there is no modal toast, no toast that stays
// until dismissed by default, and no stacking beyond a small cap. An error that
// needs a decision is a dialog; a condition that persists is an InfoBanner in
// the panel it belongs to. A toast that has to be read is a toast that will be
// missed, because by the time the user looks up it has gone.
//
// ── one at a time, mostly ────────────────────────────────────────────────
//
// Toasts queue rather than overlap. Three actions in quick succession produce
// three messages in sequence, not three pills on top of each other — and a
// fourth replaces the oldest rather than growing the stack, because a column of
// six toasts is a notification centre nobody asked for.
class NOVUUI_API Toast : public QWidget
{
    Q_OBJECT

public:
    // The tone of the mark. Neutral is the default and the common case: most
    // toasts confirm something ordinary and do not need a colour.
    using Tone = Badge::Tone;

    // The usual call, and the only one most callers need. `host` is the window
    // the toast appears over; the layer is created on it once and reused.
    // Named `post` rather than `show`: `show` is QWidget's, and a static
    // factory that hides it makes `toast->show()` a compile error inside the
    // component itself. Posting is also what a caller is doing.
    static Toast* post(QWidget* host, const QString& message,
                       Tone tone = Tone::Neutral, int milliseconds = 0);
    // With an action — "Undo", "Show me". The action closes the toast when it
    // is pressed, because a toast whose action has been taken is a toast about
    // something that is no longer true.
    static Toast* postWithAction(QWidget* host, const QString& message,
                                 const QString& actionText, Tone tone = Tone::Neutral,
                                 int milliseconds = 0);
    // Everything currently queued or on screen over `host`, for a test and for
    // a caller that wants to clear them on a view change.
    static QVector<Toast*> shownOn(QWidget* host);
    static void dismissAll(QWidget* host);

    QString message() const { return m_message; }
    void setMessage(const QString& message);

    Tone tone() const { return m_tone; }
    void setTone(Tone tone);

    QString actionText() const { return m_actionText; }
    void setActionText(const QString& text);

    // How long it stays. 0 asks the component, which scales the default with
    // the length of the message — a nine-word sentence needs longer on screen
    // than "Copied", and a fixed duration is wrong for one of them.
    int duration() const { return m_duration; }
    void setDuration(int milliseconds);

    // Cancels the timer. For a toast a caller wants to keep while something is
    // in flight; it still has no close button, so the caller must dismiss it.
    void hold();
    void dismiss();

    QRect actionRect() const;

    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

signals:
    void actionTriggered();
    void dismissed();

protected:
    explicit Toast(QWidget* parent);

    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    bool event(QEvent* event) override;

private:
    friend class ToastLayer;

    int defaultDuration() const;
    void startTimer();

    QString m_message;
    QString m_actionText;
    Tone m_tone = Tone::Neutral;
    int m_duration = 0;
    QTimer* m_timer = nullptr;
    bool m_actionHovered = false;
    bool m_actionPressed = false;
    // 0 while rising, 1 in place. The design's `slideup`.
    double m_entry = 0.0;
};

} // namespace novu::ui
