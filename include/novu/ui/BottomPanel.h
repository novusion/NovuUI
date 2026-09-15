#pragma once

#include "novu/ui/Icon.h"
#include "novu/ui/StyleSpec.h"

#include <QString>
#include <QWidget>

class QHBoxLayout;
class QStackedWidget;

namespace novu::ui {

class TabBar;

// The panel across the bottom of a window: a console, a log, a task list.
//
// A strip that is always there, and above it — when it is open — one of several
// pages. Collapsed, the strip is still a working surface: it keeps the tabs, it
// keeps whatever the application put on its right, and clicking a tab opens the
// panel at that tab.
//
// ── what this deliberately does not own ──────────────────────────────────
//
// **Not whether it is open.** `setExpanded()` is a setter and nothing else; the
// panel never decides for itself. Every gesture that would change the state
// emits `expandRequested()` and waits.
//
// That is the whole reason this component is careful. NovuExplorer's console is
// four classes carrying session lifecycle, persistence and an `isOpen` of its
// own, and a shared panel that also kept an `isOpen` would give the window two
// answers to one question — which one is right after a restore, and which one
// the user's last click changed. So the application keeps the answer, and this
// draws it.
//
// **Not which pages exist**, beyond holding them. Sessions are created, named
// and destroyed by whatever owns them; `addPage()` takes a widget somebody else
// made.
//
// **Not what is written to disk.** Nothing here is persisted. A host that
// restores a panel's height and open tab does so from its own settings and
// calls the setters.
class BottomPanel : public QWidget
{
    Q_OBJECT

public:
    explicit BottomPanel(QWidget* parent = nullptr);

    // The tab bar in the strip. Exposed rather than wrapped: it is the shared
    // TabBar, it already does closeable, reorderable and coloured tabs, and a
    // second API over it would be a second place for those to drift.
    TabBar* tabs() const { return m_tabs; }

    // `key` is the caller's identifier and is what every signal carries back,
    // so a host that renames a session does not lose track of it.
    int addPage(const QString& key, const QString& title, QWidget* page,
                Glyph glyph = Glyph::None);
    void removePage(const QString& key);
    QWidget* page(const QString& key) const;
    int pageCount() const;

    QString currentKey() const;
    void setCurrentKey(const QString& key);

    // The right-hand end of the strip: a status line, a filter, a clear button.
    // A layout, because whatever goes there belongs to the application.
    QHBoxLayout* stripTrailing() const { return m_trailing; }

    // ── state, as an input ───────────────────────────────────────────────

    bool isExpanded() const { return m_expanded; }
    // Silent. Applies the state; asks nobody and tells nobody.
    void setExpanded(bool expanded);

    // How tall the panel is when open, not counting the strip. The host owns
    // this number too — it is the thing a splitter drag changes and the thing a
    // session restores.
    int expandedHeight() const { return m_expandedHeight; }
    void setExpandedHeight(int height);

    int stripHeight() const;

    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    // Every gesture that wants the panel open or shut. The host decides.
    void expandRequested(bool expanded);
    // The tab's close affordance. The panel does not remove the page itself —
    // a session may need to ask before it dies.
    void pageCloseRequested(const QString& key);
    void currentChanged(const QString& key);
    void newPageRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    bool event(QEvent* event) override;

private:
    void buildStrip();
    void applyExpanded();

    QWidget* m_strip = nullptr;
    QHBoxLayout* m_stripRow = nullptr;
    QHBoxLayout* m_trailing = nullptr;
    TabBar* m_tabs = nullptr;
    QStackedWidget* m_pages = nullptr;
    QWidget* m_toggle = nullptr;

    bool m_expanded = false;
    int m_expandedHeight = 220;
};

} // namespace novu::ui
