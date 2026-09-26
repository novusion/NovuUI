#pragma once

#include "novu/ui/Icon.h"
#include "novu/ui/StyleSpec.h"

#include <QString>
#include <QWidget>

class QLineEdit;
class QScrollArea;
class QStackedWidget;
class QVBoxLayout;

namespace novu::ui {

class NavList;
class PageHeader;
class SettingRow;

// The settings window, as a shape rather than as a set of settings.
//
// Nineteen settings pages across NovuAI and NovuExplorer, and they disagreed on
// the width of the navigation, the gap between a section heading and its first
// row, whether a page had a description, and whether the rows were separated.
// None of that is a decision any single page should be making, so the shape
// lives here and the pages bring their content.
//
// ── the boundary ─────────────────────────────────────────────────────────
//
// This owns the presentation and the interaction: the navigation, the page
// host, the headings, the grouping, the spacing, the scrolling, and which page
// is showing.
//
// It owns none of the settings. There is no value in here, nothing is read or
// written, nothing is validated and no page is defined. An application builds
// its pages out of `SettingRow`, `Field`, `Toggle`, `Slider` and whatever else
// it needs, hands them over, and keeps its own model.
//
// Which page is showing *is* kept here, and that is deliberate — unlike
// BottomPanel's open state it is pure view state, nothing else in the window
// has an opinion about it, and a host that wants to restore it calls
// setCurrentKey().

// A heading, an optional sentence under it, and the rows that belong to them.
//
// The unit every settings page is built from. It is a plain surface, not a
// Card: the approved designs put settings on the page itself, and a column of
// cards inside a scrolling pane reads as a list of unrelated boxes.
class SettingsSection : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsSection(const QString& title, QWidget* parent = nullptr);
    SettingsSection(const QString& title, const QString& description,
                    QWidget* parent = nullptr);

    QString title() const { return m_title; }
    void setTitle(const QString& title);

    QString description() const { return m_description; }
    void setDescription(const QString& description);

    // Creates a row, adds it, and hands it back so the caller can give it a
    // control. The divider between rows is the section's business, not the
    // caller's — the last row never has one.
    SettingRow* addRow(const QString& name, const QString& hint = {});
    void addRow(SettingRow* row);

    // Anything that is not a SettingRow: a Field, a Swatch, a note, a button
    // row. Settings are not all name-and-control.
    void addWidget(QWidget* widget);

    // For a caller that wants to place things itself.
    QVBoxLayout* body() const { return m_body; }

    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

protected:
    void paintEvent(QPaintEvent* event) override;
    bool event(QEvent* event) override;

private:
    void relayout();
    int headingHeight() const;

    QString m_title;
    QString m_description;
    QVBoxLayout* m_body = nullptr;
    QWidget* m_lastRow = nullptr;
};

// One page: a header, and a column of sections that scrolls.
class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(const QString& title, QWidget* parent = nullptr);
    SettingsPage(const QString& title, const QString& description,
                 QWidget* parent = nullptr);

    PageHeader* header() const { return m_header; }

    SettingsSection* addSection(const QString& title, const QString& description = {});
    void addSection(SettingsSection* section);
    void addWidget(QWidget* widget);

    QVBoxLayout* body() const { return m_body; }
    QScrollArea* scrollArea() const { return m_scroll; }

private:
    PageHeader* m_header = nullptr;
    QScrollArea* m_scroll = nullptr;
    QVBoxLayout* m_body = nullptr;
};

// Navigation on the left, one page on the right.
class SettingsShell : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsShell(QWidget* parent = nullptr);

    // A caption in the navigation. Everything added after it belongs to it,
    // the way NavList already groups.
    void addGroup(const QString& caption);

    // Creates the page and hands it back, which is what almost every caller
    // wants — a page with nothing in it is not useful to hold on to.
    SettingsPage* addPage(const QString& key, const QString& title,
                          Glyph glyph = Glyph::None, const QString& description = {});
    // For a page the application built itself.
    void addPage(const QString& key, const QString& title, SettingsPage* page,
                 Glyph glyph = Glyph::None);

    SettingsPage* page(const QString& key) const;
    int pageCount() const;
    QStringList keys() const;

    QString currentKey() const;
    void setCurrentKey(const QString& key);

    NavList* navigation() const { return m_nav; }

    // A filter over the navigation by page title. Every settings window grows
    // one once it passes a dozen pages, and each one grew a different one.
    bool isSearchEnabled() const { return m_search != nullptr; }
    void setSearchEnabled(bool enabled);
    QLineEdit* searchField() const { return m_search; }

    int navigationWidth() const { return m_navWidth; }
    void setNavigationWidth(int px);

    // The same rail every NovuUI sidebar collapses by: NavList's own
    // setCollapsed() is what actually hides labels, captions and counts and
    // centres the glyphs. This decides only what is not the list's to
    // decide - the column's width, and the search field, which has no icon
    // to shrink to and hides with the rest. Expanding restores exactly the
    // hierarchy and selection NavList already keeps through its own
    // collapse - see NavList::setCollapsed().
    bool isNavigationCollapsed() const { return m_navCollapsed; }
    void setNavigationCollapsed(bool collapsed);

signals:
    void currentChanged(const QString& key);
    void navigationCollapsedChanged(bool collapsed);

protected:
    bool event(QEvent* event) override;

private:
    void applyFilter(const QString& query);

    NavList* m_nav = nullptr;
    QWidget* m_navColumn = nullptr;
    QLineEdit* m_search = nullptr;
    QStackedWidget* m_pages = nullptr;
    int m_navWidth = 200;
    bool m_navCollapsed = false;
};

} // namespace novu::ui
