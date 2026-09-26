#pragma once

#include "novu/ui/StyleSpec.h"

#include <QString>
#include <QStringList>
#include <QWidget>

class QHBoxLayout;
class QPainter;

namespace novu::ui {

struct ThemeProfile;

// Where you are, as a trail of places.
//
// The design's `.crumb`, and its two rules are the interesting part: the trail
// sits *in front of* the title rather than on a line of its own, and the last
// segment is dropped — *because the title next to it already says that*. A
// breadcrumb that repeats the heading immediately below it is a line of text
// doing no work.
//
// So this takes the full path and draws all but the last of it. Callers pass
// what they have; the component decides what is redundant.
class Breadcrumb : public QWidget
{
    Q_OBJECT

public:
    explicit Breadcrumb(QWidget* parent = nullptr);
    Breadcrumb(const QStringList& path, QWidget* parent);

    QStringList path() const { return m_path; }
    void setPath(const QStringList& path);

    // What is actually drawn: the path without its last segment.
    QStringList visiblePath() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    bool event(QEvent* event) override;

private:
    QStringList m_path;
};

// The band across the top of a page: trail, title, description, actions.
//
// The design's `.page-head`. Every application had one and no two agreed —
// NovuExplorer set its title at 18px semibold with no description, NovuAdmin at
// 15px with the description on the same line, NovuSpace at 16px with it after a
// gap. The heading of a page is the single most repeated shape in the system
// and it was the least consistent.
//
// ── what is painted and what is not ──────────────────────────────────────
//
// The trail, the title and the description are painted: they are prose nobody
// interacts with. The actions are real widgets, because they are buttons.
// That split is the whole of the design here — `actions()` is a layout to fill
// and everything else is a string to set.
//
// The description wraps, and the header grows to fit it. A page heading that
// elides its own explanation is a heading that explains nothing, which is what
// a fixed-height header forces.
class PageHeader : public QWidget
{
    Q_OBJECT

public:
    // How the trail, the title and the description are arranged.
    //
    // The two are both in the approved designs and neither is a preference:
    // NovuExplorer stacks them, and NovuSpace's own `.page-head` is one flex
    // row - `.ttl{display:flex;align-items:baseline;gap:10px}` puts the trail,
    // the heading and the description on a single baseline, and the band is
    // `padding:10px 20px` tall because of it. A stacked header in that design
    // is three rows where the reference draws one, which moves every pixel of
    // the page below it.
    enum class Layout {
        Stacked,  // trail above the title, description under it
        Inline,   // all three on one baseline, wrapping only when out of room
    };

    explicit PageHeader(const QString& title, QWidget* parent = nullptr);
    PageHeader(const QString& title, const QString& description, QWidget* parent);

    Layout layout() const { return m_layout; }
    void setLayout(Layout layout);

    QString title() const { return m_title; }
    void setTitle(const QString& title);

    QString description() const { return m_description; }
    void setDescription(const QString& description);

    // Empty hides the trail entirely rather than leaving its line height.
    void setPath(const QStringList& path);
    QStringList path() const { return m_path; }

    // Optional compact context shown above the title. The caller supplies
    // the shared widget (typically a Badge); PageHeader owns its placement.
    void setEyebrowWidget(QWidget* widget);
    QWidget* eyebrowWidget() const { return m_eyebrow; }

    // The trailing controls. Empty by default, and takes no width until
    // something is put in it.
    QHBoxLayout* actions() const;
    QWidget* actionsWidget() const { return m_actions; }

    // A rule under the header, as the design draws it. Off for a header inside
    // a card, which already has an edge of its own.
    bool hasDivider() const { return m_divider; }
    void setDivider(bool divider);

    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    int heightForWidth(int width) const override;
    bool hasHeightForWidth() const override { return true; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool event(QEvent* event) override;

private:
    // The one-row arrangement: trail, title and description on a shared
    // baseline. `top` is where the row's box starts and `available` the room
    // the three of them have to share before the actions.
    void paintInline(QPainter* painter, const ThemeProfile& profile, int top,
                     int available) const;
    bool inlineDescriptionWraps(int width) const;

    // The text block's height at `width`, which is what heightForWidth() and
    // the paint pass both need and must not compute differently.
    int textHeight(int width) const;
    int textWidth(int width) const;
    void relayout();

    QString m_title;
    QString m_description;
    QStringList m_path;
    Layout m_layout = Layout::Stacked;
    bool m_divider = true;
    QWidget* m_eyebrow = nullptr;
    QWidget* m_actions = nullptr;
};

} // namespace novu::ui
