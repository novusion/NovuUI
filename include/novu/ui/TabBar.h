#pragma once

#include "novu/ui/Icon.h"
#include "novu/ui/StatusDot.h"
#include "novu/ui/StyleSpec.h"

#include <QColor>
#include <QString>
#include <QVector>
#include <QWidget>

namespace novu::ui {

// The tabs across the top of a window: open documents, open folders, open
// conversations.
//
// ── not TabStrip ─────────────────────────────────────────────────────────
//
// TabStrip is a row of buttons where exactly one is active: a section switcher,
// a period selector, a preset picker. Its items are a fixed set decided by the
// page, none of them can be closed, and they never move.
//
// These are *documents*. They are opened and closed by the user, they can be
// dragged into a different order, there can be thirty of them, each one has a
// state of its own — loading, failed, asleep, modified — and the set is never
// known in advance. That is a different control with a different model, and
// trying to make TabStrip do both is what produced three tab bars in three
// applications.
//
// ── what it takes from the three that existed ────────────────────────────
//
// NovuExplorer's `ColoredTabBar` contributed the per-tab colour and, more
// usefully, *where it is kept*: on the tab itself, so a drag-reorder carries it
// without any external bookkeeping. Every implementation that kept colours in a
// parallel list got them out of step the first time a tab moved.
//
// NovuSpace's `.btab` contributed the states: a status dot, a spinner while a
// tab is loading, an error mark, and the "asleep" treatment for a tab whose
// content has been discarded to save memory but which is still open.
//
// NovuAI's `.utabs` contributed the compact geometry and the new-tab button.
//
// ── one widget ───────────────────────────────────────────────────────────
//
// Thirty tabs, each with a mark, a label, a status dot and a close box, is a
// hundred and twenty widgets for a strip 32 pixels tall. Tabs are painted and
// hit-tested per zone: the close box is a separate target from the tab, because
// closing a document and switching to it are very different mistakes to make.
class TabBar : public QWidget
{
    Q_OBJECT

public:
    // What a tab is doing. Only one at a time — a tab that is both loading and
    // failed is a tab whose state machine has a bug, not a tab with two marks.
    enum class State {
        Idle,
        Loading,   // a spinner in place of the mark
        Failed,    // the danger dot
        Modified,  // unsaved changes: the accent dot
        Asleep,    // still open, content discarded — drawn faded
    };

    explicit TabBar(QWidget* parent = nullptr);

    // `key` is the caller's identifier and what every signal carries.
    int addTab(const QString& key, const QString& label, Glyph glyph = Glyph::None);
    // Inserts at `index`, for a tab opened next to the current one rather than
    // at the end.
    int insertTab(int index, const QString& key, const QString& label,
                  Glyph glyph = Glyph::None);
    void removeTab(const QString& key);
    void clear();

    int count() const { return m_tabs.size(); }
    int indexOfKey(const QString& key) const;
    QString keyAt(int index) const;
    QStringList keys() const;

    QString currentKey() const;
    int currentIndex() const { return m_current; }
    void setCurrentKey(const QString& key);
    void setCurrentIndex(int index);

    void setLabel(const QString& key, const QString& label);
    QString label(const QString& key) const;
    void setGlyph(const QString& key, Glyph glyph);
    void setState(const QString& key, State state);
    State state(const QString& key) const;
    // The design's per-tab underline. Held on the tab, so a reorder carries it.
    // An invalid colour removes it.
    void setColor(const QString& key, const QColor& color);
    QColor color(const QString& key) const;
    // A tab that cannot be closed — the one an application always keeps open.
    void setClosable(const QString& key, bool closable);

    // Whether tabs can be dragged into a different order.
    bool isReorderable() const { return m_reorderable; }
    void setReorderable(bool reorderable);

    // The trailing "+". Off by default: not every tab bar opens things.
    bool hasNewTabButton() const { return m_newTab; }
    void setNewTabButton(bool enabled);

    // Geometry, so a test can click a tab or its close box without guessing.
    QRect tabRect(int index) const;
    QRect closeRect(int index) const;
    QRect newTabRect() const;
    int indexAt(const QPoint& position) const;

    StyleSpec styleSpec() const;
    StyleSpec tabStyleSpec(int index) const;
    ResolvedStyle resolvedStyle() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void currentChanged(const QString& key);
    // A tab's close box was pressed. The bar does not remove the tab itself:
    // a document with unsaved changes has to be able to ask first, and a bar
    // that had already closed it would be asking about something gone.
    void closeRequested(const QString& key);
    void newTabRequested();
    void tabMoved(const QString& key, int from, int to);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    bool event(QEvent* event) override;

private:
    struct Tab
    {
        QString key;
        QString label;
        Glyph glyph = Glyph::None;
        State state = State::Idle;
        QColor color;
        bool closable = true;
    };

    int tabWidth(const Tab& tab) const;
    // Tabs shrink together when the bar runs out of room, down to a floor -
    // which is what a browser does, and what stops the thirtieth tab being
    // unreachable. Returns the width every tab is actually drawn at.
    int laidOutWidth() const;
    void updateMotion();
    const Tab* find(const QString& key) const;
    Tab* find(const QString& key);

    QVector<Tab> m_tabs;
    int m_current = -1;
    int m_hovered = -1;
    bool m_hoveredClose = false;
    bool m_hoveredNew = false;
    int m_pressed = -1;
    bool m_pressedClose = false;
    bool m_pressedNew = false;

    // Where a drag started and where it is now, in tab indices. -1 when no drag
    // is in progress.
    int m_dragIndex = -1;
    QPoint m_dragOrigin;

    bool m_reorderable = true;
    bool m_newTab = false;
    // Whether any tab is loading, so the clock is held open only while one is.
    bool m_subscribed = false;
};

} // namespace novu::ui
