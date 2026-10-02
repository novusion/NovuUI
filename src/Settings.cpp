#include "novu/ui/Settings.h"

#include "novu/ui/Form.h"
#include "novu/ui/Layout.h"
#include "novu/ui/NavList.h"
#include "novu/ui/PageHeader.h"
#include "novu/ui/Separator.h"
#include "novu/ui/SettingRow.h"
#include "novu/ui/StyleSheet.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPainter>
#include <QScrollArea>
#include <QVector>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace novu::ui {

namespace {

// Between the heading and the first row, and between the heading and the
// sentence under it.
constexpr int kHeadingGap = 10;
constexpr int kDescriptionGap = 4;

} // namespace

// ── SettingsSection ───────────────────────────────────────────────────────

SettingsSection::SettingsSection(const QString& title, QWidget* parent)
    : QWidget(parent)
    , m_title(title)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_body = new QVBoxLayout(this);
    m_body->setSpacing(0);
    relayout();
}

SettingsSection::SettingsSection(const QString& title, const QString& description,
                                 QWidget* parent)
    : SettingsSection(title, parent)
{
    m_description = description;
    relayout();
}

void SettingsSection::setTitle(const QString& title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    relayout();
    update();
}

void SettingsSection::setDescription(const QString& description)
{
    if (m_description == description) {
        return;
    }
    m_description = description;
    relayout();
    update();
}

int SettingsSection::headingHeight() const
{
    if (m_title.isEmpty() && m_description.isEmpty()) {
        return 0;
    }
    int h = 0;
    if (!m_title.isEmpty()) {
        h += QFontMetrics(roleFont(this, TypeRole::CardTitle)).height();
    }
    if (!m_description.isEmpty()) {
        const QFontMetrics fm(roleFont(this, TypeRole::Caption));
        const QRect bounds = fm.boundingRect(QRect(0, 0, qMax(1, width()), 0),
                                             Qt::TextWordWrap, m_description);
        h += kDescriptionGap + qMax(fm.height(), bounds.height());
    }
    return h + kHeadingGap;
}

void SettingsSection::relayout()
{
    // The heading is painted into the top margin, so the layout holds only the
    // rows. A QLabel for the title and another for the sentence would be two
    // more widgets per section and twenty more in a settings window.
    m_body->setContentsMargins(0, headingHeight(), 0, 0);
    alignControls();
    updateGeometry();
}

// One control column for the whole section.
//
// SettingRow has always said "the rows in one panel must agree, and a
// measured column makes every row as wide as its own control" - and nothing
// measured it, so every row WAS as wide as its own control. A column of
// switches then stepped left and right down the page according to how long
// each label's own control happened to be. The widest control in the section
// sets the column and every row in it uses that, which is what puts them on
// one axis without anyone writing a pixel width down.
void SettingsSection::alignControls()
{
    int widest = 0;
    QVector<SettingRow*> rows;
    for (int i = 0; i < m_body->count(); ++i) {
        auto* row = qobject_cast<SettingRow*>(m_body->itemAt(i)->widget());
        if (!row || !row->control()) {
            continue;
        }
        rows.append(row);
        widest = qMax(widest, row->control()->sizeHint().width());
    }
    if (rows.isEmpty() || widest <= 0) {
        return;
    }
    // Never more than half the section: a long control must not squeeze the
    // label it belongs to off the row when the window is narrow.
    const int ceiling = qMax(96, width() / 2);
    const int column = qMin(widest, ceiling);
    for (SettingRow* row : rows) {
        row->setControlColumn(column);
    }
}

SettingRow* SettingsSection::addRow(const QString& name, const QString& hint)
{
    auto* row = new SettingRow(name, hint, this);
    // Settings uses the frozen `.set-row` grid: compact vertical padding and
    // a fixed label column. Standalone `.kv` rows retain their own geometry.
    row->setProperty("novuSettingsRow", true);
    addRow(row);
    return row;
}

void SettingsSection::addRow(SettingRow* row)
{
    if (!row) {
        return;
    }
    // The divider belongs to the section, not the row: only the *last* row is
    // without one, and a caller cannot know which row is last while it is
    // adding them.
    if (auto* previous = qobject_cast<SettingRow*>(m_lastRow)) {
        previous->setDivider(true);
    }
    row->setDivider(false);
    m_lastRow = row;
    m_body->addWidget(row);
}

void SettingsSection::addWidget(QWidget* widget)
{
    if (!widget) {
        return;
    }
    m_lastRow = nullptr;
    m_body->addWidget(widget);
}

StyleSpec SettingsSection::styleSpec() const
{
    StyleSpec spec;
    spec.type = TypeRole::CardTitle;
    spec.foreground = token(ColorRole::TextPrimary);
    spec.hoverForeground = token(ColorRole::TextDisabled);   // the description
    return spec;
}

ResolvedStyle SettingsSection::resolvedStyle() const
{
    return resolve(styleSpec(), this, height());
}

bool SettingsSection::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type()
        || event->type() == QEvent::ParentChange
        || event->type() == QEvent::Resize) {
        relayout();
        update();
    }
    return QWidget::event(event);
}

void SettingsSection::paintEvent(QPaintEvent*)
{
    if (m_title.isEmpty() && m_description.isEmpty()) {
        return;
    }
    const ResolvedStyle style = resolvedStyle();

    QPainter painter(this);
    int y = 0;
    if (!m_title.isEmpty()) {
        const QFont font = roleFont(this, TypeRole::CardTitle);
        painter.setFont(font);
        painter.setPen(style.foreground);
        const int h = QFontMetrics(font).height();
        painter.drawText(QRect(0, y, width(), h), Qt::AlignLeft | Qt::AlignVCenter, m_title);
        y += h;
    }
    if (!m_description.isEmpty()) {
        y += kDescriptionGap;
        const QFont font = roleFont(this, TypeRole::Caption);
        painter.setFont(font);
        painter.setPen(style.hoverForeground);
        painter.drawText(QRect(0, y, width(), headingHeight() - y),
                         Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, m_description);
    }
}

// ── SettingsPage ──────────────────────────────────────────────────────────

SettingsPage::SettingsPage(const QString& title, QWidget* parent)
    : SettingsPage(title, QString(), parent)
{
}

SettingsPage::SettingsPage(const QString& title, const QString& description,
                           QWidget* parent)
    : QWidget(parent)
{
    const ThemeProfile& profile = themeFor(this);

    auto* column = new QVBoxLayout(this);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(0);

    m_header = description.isEmpty() ? new PageHeader(title, this)
                                     : new PageHeader(title, description, this);
    column->addWidget(m_header);
    column->addWidget(new Separator(Qt::Horizontal, this));

    // scrollPage() is the library's own answer to "a page that scrolls", so a
    // settings page and every other scrolling page behave the same.
    auto* host = new QWidget(this);
    m_body = scrollPage(host, &m_scroll);
    m_body->setContentsMargins(profile.space(SpacingRole::PagePadding),
                               profile.space(SpacingRole::PagePadding),
                               profile.space(SpacingRole::PagePadding),
                               profile.space(SpacingRole::PagePadding));
    m_body->setSpacing(profile.space(SpacingRole::CardGap) * 2);
    // Keep short settings pages pinned to the top. Without a trailing stretch,
    // QVBoxLayout distributes spare viewport height between fixed sections.
    m_body->addStretch(1);
    column->addWidget(host, 1);
}

SettingsSection* SettingsPage::addSection(const QString& title, const QString& description)
{
    auto* section = description.isEmpty() ? new SettingsSection(title)
                                          : new SettingsSection(title, description);
    addSection(section);
    return section;
}

namespace {

// Where the next thing goes: before a trailing stretch if the column has one,
// at the end if it does not.
//
// Assuming the stretch is always there reverses the order - with none present,
// count() - 1 is the position *before* what was added last, so every section
// landed above its predecessor and a settings page read bottom to top.
int appendIndex(QVBoxLayout* column)
{
    const int last = column->count() - 1;
    if (last >= 0) {
        if (QLayoutItem* item = column->itemAt(last); item && item->spacerItem()) {
            return last;
        }
    }
    return column->count();
}

} // namespace

void SettingsPage::addSection(SettingsSection* section)
{
    if (!section) {
        return;
    }
    m_body->insertWidget(appendIndex(m_body), section);
}

void SettingsPage::addWidget(QWidget* widget)
{
    if (!widget) {
        return;
    }
    m_body->insertWidget(appendIndex(m_body), widget);
}

// ── SettingsShell ─────────────────────────────────────────────────────────

SettingsShell::SettingsShell(QWidget* parent)
    : QWidget(parent)
{
    const ThemeProfile& profile = themeFor(this);

    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    m_navColumn = new QWidget(this);
    m_navColumn->setFixedWidth(m_navWidth);
    auto* navLayout = new QVBoxLayout(m_navColumn);
    navLayout->setContentsMargins(profile.spacing.sm, profile.spacing.sm,
                                  profile.spacing.sm, profile.spacing.sm);
    navLayout->setSpacing(profile.spacing.sm);

    m_nav = new NavList(m_navColumn);
    navLayout->addWidget(m_nav, 1);

    row->addWidget(m_navColumn);
    row->addWidget(new Separator(Qt::Vertical, Separator::Weight::Strong, this));

    m_pages = new QStackedWidget(this);
    row->addWidget(m_pages, 1);

    connect(m_nav, &NavList::currentChanged, this, [this](const QString& key) {
        if (QWidget* widget = page(key)) {
            m_pages->setCurrentWidget(widget);
        }
        emit currentChanged(key);
    });
}

void SettingsShell::addGroup(const QString& caption)
{
    m_nav->addSection(caption);
}

SettingsPage* SettingsShell::addPage(const QString& key, const QString& title,
                                     Glyph glyph, const QString& description)
{
    auto* page = new SettingsPage(title, description, this);
    addPage(key, title, page, glyph);
    return page;
}

void SettingsShell::addPage(const QString& key, const QString& title,
                            SettingsPage* page, Glyph glyph)
{
    if (!page) {
        return;
    }
    page->setProperty("novuSettingsKey", key);
    m_pages->addWidget(page);
    m_nav->addItem(key, title, glyph);
    if (m_pages->count() == 1) {
        m_nav->setCurrentKey(key);
        m_pages->setCurrentWidget(page);
    }
}

SettingsPage* SettingsShell::page(const QString& key) const
{
    for (int i = 0; i < m_pages->count(); ++i) {
        QWidget* widget = m_pages->widget(i);
        if (widget && widget->property("novuSettingsKey").toString() == key) {
            return qobject_cast<SettingsPage*>(widget);
        }
    }
    return nullptr;
}

int SettingsShell::pageCount() const
{
    return m_pages ? m_pages->count() : 0;
}

QStringList SettingsShell::keys() const
{
    QStringList out;
    for (int i = 0; i < m_pages->count(); ++i) {
        const QString key = m_pages->widget(i)->property("novuSettingsKey").toString();
        if (!key.isEmpty()) {
            out.append(key);
        }
    }
    return out;
}

QString SettingsShell::currentKey() const
{
    return m_nav ? m_nav->currentKey() : QString();
}

void SettingsShell::setCurrentKey(const QString& key)
{
    if (m_nav) {
        m_nav->setCurrentKey(key);
    }
    if (QWidget* widget = page(key)) {
        m_pages->setCurrentWidget(widget);
    }
}

void SettingsShell::setSearchEnabled(bool enabled)
{
    if (enabled == (m_search != nullptr)) {
        return;
    }
    if (!enabled) {
        delete m_search;
        m_search = nullptr;
        applyFilter(QString());
        return;
    }
    m_search = searchBar(m_navColumn, tr("Find a setting"));
    // Above the list, inside the navigation column: a filter over the
    // navigation belongs to the navigation.
    if (auto* column = qobject_cast<QVBoxLayout*>(m_navColumn->layout())) {
        column->insertWidget(0, m_search);
    }
    connect(m_search, &QLineEdit::textChanged, this, &SettingsShell::applyFilter);
}

void SettingsShell::applyFilter(const QString& query)
{
    // Hides rows rather than rebuilding the list, so the current page does not
    // move while somebody is typing.
    const QString needle = query.trimmed();
    for (const QString& key : m_nav->keys()) {
        const bool match = needle.isEmpty()
            || m_nav->label(key).contains(needle, Qt::CaseInsensitive);
        m_nav->setRowVisible(key, match);
    }
}

void SettingsShell::setNavigationWidth(int px)
{
    px = qMax(120, px);
    if (m_navWidth == px) {
        return;
    }
    m_navWidth = px;
    if (!m_navCollapsed) {
        m_navColumn->setFixedWidth(px);
    }
}

void SettingsShell::setNavigationCollapsed(bool collapsed)
{
    if (m_navCollapsed == collapsed) {
        return;
    }
    m_navCollapsed = collapsed;
    // Hides labels, section captions, counts and twisties and centres the
    // glyphs - the list's own contract, not this shell's to re-implement.
    m_nav->setCollapsed(collapsed);
    // Not NavList's: a text field has no icon-rail form to shrink to, so it
    // goes with the rest of what a rail hides rather than staying behind as
    // a lone control with nothing to filter into view.
    if (m_search) {
        m_search->setVisible(!collapsed);
    }
    m_navColumn->setFixedWidth(collapsed
        ? m_nav->sizeHint().width() + 2 * themeFor(this).spacing.sm
        : m_navWidth);
    emit navigationCollapsedChanged(collapsed);
}

bool SettingsShell::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type()) {
        updateGeometry();
        update();
    }
    return QWidget::event(event);
}

} // namespace novu::ui
