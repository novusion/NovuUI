#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/StyleSpec.h"

#include <QString>
#include <QWidget>

class QHBoxLayout;

namespace novu::ui {

// What appears under a list when things in it are selected.
//
// The approved NovuAI design's `.sel-bar`: a tinted strip with a count on the
// left and the actions that apply to the selection on the right, hidden
// entirely when nothing is selected.
//
// ── why this is a component ──────────────────────────────────────────────
//
// Every list that supports multi-select grows one of these, and every one of
// them was written from scratch: NovuExplorer's transfer list, NovuAdmin's
// table toolbars, NovuAI's tree. They disagreed on where the count went, on
// whether the bar pushed the list up or floated over it, and on whether it
// stayed visible with a count of zero — which is the one that matters, because
// a bar reading "0 selected" with a Delete button in it is a bar that invites
// exactly the wrong click.
//
// So it hides itself. `setCount(0)` is not a state a caller has to handle.
//
// The count is painted; the actions are real buttons, because they are buttons.
class NOVUUI_API SelectionBar : public QWidget
{
    Q_OBJECT

public:
    explicit SelectionBar(QWidget* parent = nullptr);

    int count() const { return m_count; }
    // Hides the bar at zero. The singular and the plural are both produced
    // here, so twelve call sites do not each write their own "1 item" branch.
    void setCount(int count);

    // What the things are called: "file", "workspace", "peer". The bar builds
    // "3 files selected" from it. A caller with an irregular plural sets the
    // whole sentence with setText() instead.
    QString noun() const { return m_noun; }
    void setNoun(const QString& singular, const QString& plural = {});

    // Overrides the built sentence entirely — for "3 of 40 rows", or a summary
    // a count alone cannot express.
    void setText(const QString& text);
    QString text() const;

    // The trailing controls.
    QHBoxLayout* actions() const;
    QWidget* actionsWidget() const { return m_actions; }

    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    // Emitted when the bar appears or disappears, so a caller can adjust the
    // space above it without watching visibility itself.
    void shownChanged(bool shown);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool event(QEvent* event) override;

private:
    void relayout();
    QString displayText() const;

    int m_count = 0;
    QString m_noun;
    QString m_plural;
    QString m_override;
    QWidget* m_actions = nullptr;
};

} // namespace novu::ui
