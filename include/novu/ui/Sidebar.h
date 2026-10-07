#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/AppMark.h"
#include "novu/ui/Icon.h"
#include "novu/ui/NavList.h"
#include "novu/ui/StyleSpec.h"

#include <QString>
#include <QWidget>

class QScrollArea;
class QVBoxLayout;

namespace novu::ui {

class Splitter;

// The design's `.rail-conn`: where you are connected, and how well.
//
// *The runtime line, never an identity* — the design says so, and the
// distinction is the reason this is a component rather than a label. A rail
// already says which application it is; this says whether that application can
// currently reach anything. Putting an account name here is what turns a
// transport indicator into a second, worse identity badge.
//
// A light, a host, a latency figure, and one optional action — the design's
// `.lockb`, which locks the credential the connection belongs to.
class NOVUUI_API ConnectionStrip : public QWidget
{
    Q_OBJECT

public:
    enum class State {
        Online,    // the light breathes
        Degraded,  // amber, still lit
        Offline,   // danger, and the breathing stops
        // Nothing has been established yet, or the module does not track a
        // link. Neutral rather than Offline: "not connected" and "we have not
        // looked" are different things, and a red light for the second is a
        // fault the person then goes looking for.
        Unknown,
    };

    explicit ConnectionStrip(QWidget* parent = nullptr);

    QString host() const { return m_host; }
    void setHost(const QString& host);

    // Free text rather than a number, because "52 ms", "—" and "timing out"
    // all belong in the same place and only the first is a measurement.
    //
    // Drawn on its own line under the host. It shared a line with it and took
    // at most half the width, so a host long enough to matter and a detail
    // worth reading could not both be there: "trade.novuhub.com" elided to
    // "trade...com" to make room for "Connection to..." which elided as well,
    // and two half-sentences side by side read as neither.
    QString detail() const { return m_detail; }
    void setDetail(const QString& detail);

    State state() const { return m_state; }
    void setState(State state);

    void setAction(Glyph glyph, const QString& tooltip = {});
    void clearAction();

    // Icon only, centred — what the strip becomes on a collapsed rail.
    bool isCompact() const { return m_compact; }
    void setCompact(bool compact);

    QRect actionRect() const;

    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void actionTriggered();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool event(QEvent* event) override;

private:
    void updateMotion();

    //! Width available to the host and detail lines, in pixels.
    //!
    //! The same band paintEvent() lays them out in. Shared so that "does this
    //! text fit" is answered by the measurement that actually draws it rather
    //! than by a second guess that can disagree with it.
    int textBandWidth() const;

    //! Puts the complete host and detail in the tooltip when either is elided,
    //! and takes the tooltip away when both fit.
    //!
    //! A status line is where a diagnostic appears, and a rail is narrow: a
    //! transport error elided to "NovuTrade private transport is unav..." is a
    //! message nobody can act on, and the shell offers no other place to read
    //! it. Generic on purpose - every module's strip gets this, because every
    //! module can produce a sentence longer than 238 pixels.
    void refreshElisionTooltip();

    QString m_host;
    QString m_detail;
    State m_state = State::Online;
    Glyph m_action = Glyph::None;
    QString m_actionTip;
    bool m_compact = false;
    bool m_actionHovered = false;
    bool m_actionPressed = false;
    bool m_subscribed = false;
};

// The whole left rail: identity, transport, navigation and actions.
//
// The design is explicit about what this owns, and the boundary is worth
// keeping: *it belongs entirely to the active module — its identity, its
// context, its navigation and its actions. Nothing in here switches modules.*
// Module switching is the title bar's job. A sidebar that also switches
// modules is the design every one of these applications started with, and the
// reason a user could never tell which of two lists was the important one.
//
// ── shape ─────────────────────────────────────────────────────────────────
//
//     head         an AppMark, a name, a version or context line
//     connection   the strip above, optional
//     navigation   a scrolling region the caller fills — usually one NavList,
//                  or several inside Disclosure blocks
//     actions      a pinned column at the bottom, for the one or two buttons
//                  that belong to the module rather than to a page
//
// Only the navigation scrolls. That is deliberate and it is stated in the
// design: identity and status are pinned, however many workspaces there are.
//
// ── collapsed ─────────────────────────────────────────────────────────────
//
// One call folds the rail to a strip of icons: the head centres and loses its
// text, the connection strip becomes its light alone, and every NavList inside
// the navigation region is told to collapse too. A caller that put its own
// widgets in there is responsible for those, which is why `collapsed()` is
// readable and the signal exists.
class NOVUUI_API Sidebar : public QWidget
{
    Q_OBJECT

public:
    explicit Sidebar(QWidget* parent = nullptr);

    // The head. Passing an empty subtitle drops the second line rather than
    // reserving space for it.
    void setIdentity(Glyph glyph, const QString& name, const QString& subtitle = {});
    // Not every rail is headed by an identity. NovuAI names itself in the
    // title bar and starts its sidebar at the first section, so the head
    // is a choice rather than a fixture.
    bool isHeadVisible() const { return m_headVisible; }
    void setHeadVisible(bool visible);
    void setName(const QString& name);
    void setSubtitle(const QString& subtitle);
    QString name() const { return m_name; }
    QString subtitle() const { return m_subtitle; }

    AppMark* mark() const { return m_mark; }

    // Created on first call. A rail with nothing to connect to never builds it.
    ConnectionStrip* connection();
    ConnectionStrip* existingConnection() const { return m_connection; }

    // The scrolling region. Fill this with NavLists, Disclosures, anything.
    QVBoxLayout* navigation() const;
    QWidget* navigationWidget() const { return m_navContent; }
    // The convenience: one NavList, created on first call and placed for you,
    // which is what most rails need and all of them start as.
    NavList* nav();
    NavList* existingNav() const { return m_nav; }

    // A second, independent scrolling region, below the first by default -
    // the design's own Workspaces tree sharing a rail with a Hub block and a
    // Quick access list. One shared scroll area meant a long tree crowded
    // out Quick access's own "+" rather than the other way of it, with no
    // way for anyone to say which mattered more on their own rail. Splitting
    // the two into a Splitter the user can drag settles it per rail rather
    // than by whichever grew first.
    //
    // Created on first call - a rail that never asks for one pays nothing
    // for it, and the single-region behaviour above is exactly what it was
    // before this existed.
    QVBoxLayout* secondaryNavigation();
    QWidget* secondaryNavigationWidget() const { return m_navContent2; }
    bool hasSecondaryNavigation() const { return m_navContent2 != nullptr; }

    // Which region is on top. Swapping changes only the stacking order -
    // each region keeps its own content, its own scroll position and its
    // own share of the height.
    bool isNavigationOrderSwapped() const { return m_navSwapped; }
    void setNavigationOrderSwapped(bool swapped);

    // The handle between the two regions, once secondaryNavigation() has
    // made one - for a caller that wants to read or restore the user's own
    // split (QSplitter::saveState()/restoreState()) the way it would any
    // other splitter's position.
    Splitter* navigationSplitter() const { return m_navSplitter; }

    // The pinned column at the bottom — `.rail-acts`. Empty by default and it
    // takes no height until something is put in it.
    QVBoxLayout* actions() const;

    bool isCollapsed() const { return m_collapsed; }
    void setCollapsed(bool collapsed);
    void toggleCollapsed();

    int expandedWidth() const { return m_expandedWidth; }
    void setExpandedWidth(int px);

    // ── user resizing ────────────────────────────────────────────────────
    //
    // Off by default, because a sidebar pinned to its design width is what
    // every consumer has had and a width that moves on its own is a surprise.
    //
    // On, the rail stops fixing its own width and takes a range instead, so a
    // QSplitter above it can drive the edge - Qt's own primitive, rather than a
    // second mouse-drag implementation living in a widget. The width the user
    // settles on becomes the expanded width, so collapsing and expanding
    // returns to theirs rather than to the design's.
    //
    // The reason this exists: real content is wider than a mockup's. Workspace
    // names that fit the approved 236px as "NovuAI" arrive from production as
    // "Esanti_Unity_HEngine" and elide to nothing.
    bool isUserResizable() const { return m_userResizable; }
    void setUserResizable(bool resizable);

    // The range a dragged edge is held to. Narrower than the minimum is a rail
    // that cannot show a name; wider is a rail that has become the page.
    static constexpr int kMinExpandedWidth = 180;
    static constexpr int kMaxExpandedWidth = 560;
    int collapsedWidth() const;

    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void collapsedChanged(bool collapsed);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void applyWidthPolicy();

protected:
    bool event(QEvent* event) override;

private:
    int headHeight() const;
    void relayout();
    void applyCollapsed();

    QString m_name;
    QString m_subtitle;
    bool m_collapsed = false;
    bool m_userResizable = false;
    bool m_headVisible = true;
    int m_expandedWidth = 236;

    AppMark* m_mark = nullptr;
    ConnectionStrip* m_connection = nullptr;
    QWidget* m_edge = nullptr;
    QScrollArea* m_scroll = nullptr;
    QWidget* m_navContent = nullptr;
    NavList* m_nav = nullptr;
    QScrollArea* m_scroll2 = nullptr;
    QWidget* m_navContent2 = nullptr;
    Splitter* m_navSplitter = nullptr;
    bool m_navSwapped = false;
    QWidget* m_actions = nullptr;
};

} // namespace novu::ui
