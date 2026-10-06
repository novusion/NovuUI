#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/Icon.h"
#include "novu/ui/StyleSpec.h"

#include <QString>
#include <QWidget>

class QVBoxLayout;

namespace novu::ui {

// A header that puts its content away.
//
// Two shapes in the approved design, and they are the same control:
//
//   `.disclose`   an inline row with an outline — "Advanced" inside a form.
//                 The design's note is the reason it exists: *Advanced belongs
//                 to the window it is in, not to a second dialog. The window
//                 grows to fit rather than scrolling.*
//
//   `.sb-block` / `.sb-bhead` / `.sb-bbody`
//                 a sidebar group — a tracked uppercase caption, a count of
//                 what is hidden, and an optional action that must *not*
//                 collapse the block it sits in.
//
// They differ in typography, in whether there is an outline, and in what sits
// on the right. That is a variant, not a second class — and writing it as two
// was how NovuAI's sidebar block and NovuSpace's Advanced row ended up with two
// different chevron angles.
//
// ── what a caller owns ────────────────────────────────────────────────────
//
// The body. `body()` is a layout to fill; everything above it — the row, the
// chevron, the count, the rotation — belongs here. The header is painted, so a
// sidebar with nine blocks costs nine widgets plus their contents rather than
// nine headers of four labels each.
//
// Collapsing is *not* the same state as whatever is expanded inside the body.
// The design says so outright: collapsing WORKSPACES does not forget which
// workspaces were open inside it. Hiding the body leaves it intact.
class NOVUUI_API Disclosure : public QWidget
{
    Q_OBJECT

public:
    enum class Variant {
        Inline,   // `.disclose` — outlined row, sentence case, hint on the right
        Block,    // `.sb-block` — sidebar caption, count on the right
    };

    explicit Disclosure(const QString& title, QWidget* parent = nullptr,
                        Variant variant = Variant::Inline);

    QString title() const { return m_title; }
    void setTitle(const QString& title);

    Variant variant() const { return m_variant; }
    void setVariant(Variant variant);

    bool isOpen() const { return m_open; }
    // Silent, like every other setter in the library.
    void setOpen(bool open);
    // Asks, and says so. What a click goes through.
    void toggle();

    // `.disclose .hintx` — a word on the right of an inline row saying what is
    // inside without opening it ("3 changed", "default").
    QString hint() const { return m_hint; }
    void setHint(const QString& hint);

    // `.sb-bhead .bn` — the count, in a pill. -1 draws nothing, which is
    // different from 0: a block with no children says "0", a block that does
    // not count says nothing.
    int count() const { return m_count; }
    void setCount(int count);

    // `.sb-add` — the action beside the caption. It is hit-tested before the
    // header is, because the design requires that creating something must not
    // collapse the block it is created in.
    void setAction(Glyph glyph, const QString& tooltip = {});
    void clearAction();
    bool hasAction() const { return m_action != Glyph::None; }

    // The body. Fill this; never reparent it.
    QWidget* bodyWidget() const { return m_body; }
    QVBoxLayout* body() const;
    void setBodyWidget(QWidget* widget);

    // Where the header and the action are, for a test that wants to click one
    // without guessing at the padding.
    QRect headerRect() const;
    QRect actionRect() const;

    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void toggled(bool open);
    void actionTriggered();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool event(QEvent* event) override;

private:
    int headerHeight() const;
    void relayout();
    void animateChevron();

    QString m_title;
    QString m_hint;
    Variant m_variant = Variant::Inline;
    bool m_open = true;
    int m_count = -1;
    Glyph m_action = Glyph::None;
    QString m_actionTip;

    bool m_hovered = false;
    bool m_actionHovered = false;
    // Which part the press landed in, so a press on the action that releases
    // on the caption does not collapse the block - the same press/release
    // ownership rule Segmented follows.
    int m_pressed = -1;

    // 0 shut, 1 open. Interpolated rather than switched, because the design
    // rotates the chevron over 180ms and a chevron that snaps is the one part
    // of a sidebar a person notices.
    double m_chevron = 1.0;

    QWidget* m_body = nullptr;
};

} // namespace novu::ui
