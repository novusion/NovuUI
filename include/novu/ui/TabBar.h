#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/Icon.h"
#include "novu/ui/StatusDot.h"
#include "novu/ui/StyleSpec.h"

#include <QColor>
#include <QString>
#include <QVector>
#include <QWidget>

class QContextMenuEvent;
class QWheelEvent;

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
class NOVUUI_API TabBar : public QWidget
{
    Q_OBJECT

public:
    // What a tab is doing. Only one at a time — a tab that is both loading and
    // failed is a tab whose state machine has a bug, not a tab with two marks.
    // The two tab strips the approved design draws, which are not the same
    // control wearing different colours.
    //
    // Browser is `.btab`: a plate with rounded top corners, a close box, a
    // mark and a state light. It is a *document* - something you opened, that
    // can be closed, that can be loading or asleep.
    //
    // Underline is `.tab`: no plate and no close box, the accent ink and a
    // 2px accent rule under the current one, with a hairline along the whole
    // strip. It is a *face of one surface* - the four tabs of a settings card,
    // the four views of a market panel. Drawing those as documents said they
    // could be closed, and gave four faces of one card four separate plates.
    enum class Variant { Browser, Underline };

    Variant variant() const { return m_variant; }
    void setVariant(Variant variant);

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
    // A short mark drawn in a tile where the glyph would go: the page's own
    // initial, for documents that have no icon to give. A browser tab is the
    // case this exists for - `novuai.app` has no glyph in any icon set, but
    // the letter N tells the twelfth tab from the eleventh at a glance.
    //
    // A lettered mark is the document's identity, so it is not overwritten by
    // the document's state: a tab that has one shows Failed and Asleep as a
    // dot after the label instead of in place of its mark. Loading is the
    // exception, and stays a spinner in the mark's place - a page that is
    // still arriving has no identity to protect yet.
    void setMarkText(const QString& key, const QString& text);
    QString markText(const QString& key) const;
    void setState(const QString& key, State state);
    State state(const QString& key) const;
    // The design's per-tab underline. Held on the tab, so a reorder carries it.
    // An invalid colour removes it.
    void setColor(const QString& key, const QColor& color);
    QColor color(const QString& key) const;
    // A tab that cannot be closed — the one an application always keeps open.
    void setClosable(const QString& key, bool closable);

    // Whether a tab is making sound, and whether it has been silenced. Drawn
    // as a speaker after the label; pressing it asks for the opposite through
    // audioToggleRequested(). Held per key, so a reorder carries it like the
    // colour. The bar only shows it: muting is the owner's.
    enum class Audio { None, Playing, Muted };
    void setAudio(const QString& key, Audio audio);
    Audio audio(const QString& key) const;

    // ── scrolling ─────────────────────────────────────────────────────────
    //
    // Tabs shrink together down to their floor, and past it the strip scrolls
    // rather than clipping the tail away: the wheel moves it, the current tab
    // is kept in view, and a drag held near either edge carries the strip
    // with it so a tab can be moved to a place that is not on screen.
    int scrollOffset() const;
    int maximumScrollOffset() const;
    void setScrollOffset(int pixels);
    void ensureVisible(const QString& key);
    // The band the tabs are drawn in, between the leading inset and whatever
    // is pinned at the trailing end. Tabs outside it are not drawn or hit.
    QRect tabsViewport() const;

    // ── renaming a tab in place ───────────────────────────────────────────
    //
    // NavList's convention, on a tab: an editor over the tab's label, focused
    // with its text selected. Enter or focus leaving submits, Escape abandons.
    // The bar decides nothing about the text - inlineEditCommitted() hands it
    // over, and the handler may call rejectInlineEdit() to keep the editor
    // open with a reason (Enter) or abandon it (focus-out). The bar does not
    // change the label itself; the owner does, with setLabel(). The editor
    // follows its tab through scrolling and reordering and goes away with it.
    bool beginInlineEdit(const QString& key, const QString& text = QString());
    void rejectInlineEdit(const QString& message);
    void cancelInlineEdit();
    bool isInlineEditing() const;
    QString inlineEditKey() const;
    QRect audioRect(int index) const;

    // Whether tabs can be dragged into a different order.
    bool isReorderable() const { return m_reorderable; }
    void setReorderable(bool reorderable);

    // The trailing "+". Off by default: not every tab bar opens things.
    bool hasNewTabButton() const { return m_newTab; }
    void setNewTabButton(bool enabled);

    // `.btabs-row .bacts` - what belongs to the *set* of tabs rather than to
    // any one of them, pinned to the trailing edge of the strip.
    //
    // It rides here rather than on a bar of its own because a second
    // horizontal band between the tabs and the page is a layer that says
    // nothing. The tabs lay out in what is left, so they shrink around it
    // instead of sliding underneath it. Takes ownership; nullptr removes it.
    void setTrailingWidget(QWidget* widget);
    QWidget* trailingWidget() const { return m_trailing; }

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
    // A tab was asked for its own menu - right-clicked, or the context-menu
    // key pressed while it is current. The bar opens nothing itself, as
    // NavList does not: what a tab can do is the owner's.
    void contextMenuRequested(const QString& key, const QPoint& globalPosition);
    void inlineEditCommitted(const QString& key, const QString& text);
    void inlineEditCancelled(const QString& key);
    // The speaker on a tab was pressed.
    void audioToggleRequested(const QString& key);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    // The ring is the keyboard's, not the mouse's - see FocusRing in Theme.h.
    void focusInEvent(QFocusEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    bool event(QEvent* event) override;

private:
    FocusRing m_ring;

    struct Tab
    {
        QString key;
        QString label;
        Glyph glyph = Glyph::None;
        QString mark;
        State state = State::Idle;
        QColor color;
        bool closable = true;
    };

    int tabWidth(const Tab& tab) const;
    // What every tab is actually drawn at, in order.
    // Tabs shrink together when the bar runs out of room, down to a floor -
    // which is what a browser does, and what stops the thirtieth tab being
    // unreachable. Returns the width every tab is actually drawn at.
    QVector<int> laidOutWidths() const;
    // The first tab's width, for the callers that only need a scale.
    int laidOutWidth() const;
    void updateMotion();
    // The scroll offset actually in force: what was asked for, clamped to the
    // current content. Stored unclamped so a narrowing that is undone puts the
    // strip back where it was.
    int effectiveScroll() const;
    int contentWidth() const;
    void dragTo(const QPoint& position);
    void updateAutoScroll(const QPoint& position);
    void autoScrollStep();
    void placeInlineEditor();
    void finishInlineEdit(bool submit, bool fromFocusOut);
    bool handleWheel(QWheelEvent* event);
    bool handleContextMenu(QContextMenuEvent* event);
    // Scrolling, the inline editor and the audio marks keep their state in
    // TabBarExtras (TabBar.cpp), not in members here. Modules that were built
    // against the earlier header allocate a TabBar of the earlier size, so the
    // object layout below is frozen; new state hangs off the widget instead.
    friend struct TabBarExtras;
    const Tab* find(const QString& key) const;
    Tab* find(const QString& key);

    QVector<Tab> m_tabs;
    int m_current = -1;
    Variant m_variant = Variant::Browser;
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

    void placeTrailing();

    QWidget* m_trailing = nullptr;
    bool m_reorderable = true;
    bool m_newTab = false;
    // Whether any tab is loading, so the clock is held open only while one is.
    bool m_subscribed = false;
};

} // namespace novu::ui
