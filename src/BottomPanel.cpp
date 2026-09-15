#include "novu/ui/BottomPanel.h"

#include "novu/ui/IconButton.h"
#include "novu/ui/StyleSheet.h"
#include "novu/ui/TabBar.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QPainter>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace novu::ui {

namespace {

// The strip is a control-height row with the profile's toolbar gap, so it lines
// up with the status bar under it and the toolbars above it.
constexpr int kStripPaddingH = 8;

} // namespace

BottomPanel::BottomPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* column = new QVBoxLayout(this);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(0);

    buildStrip();
    column->addWidget(m_strip);

    m_pages = new QStackedWidget(this);
    column->addWidget(m_pages, 1);

    applyExpanded();
}

void BottomPanel::buildStrip()
{
    const ThemeProfile& profile = themeFor(this);

    m_strip = new QWidget(this);
    m_strip->setFixedHeight(stripHeight());

    m_stripRow = new QHBoxLayout(m_strip);
    m_stripRow->setContentsMargins(kStripPaddingH, 0, kStripPaddingH, 0);
    m_stripRow->setSpacing(profile.space(SpacingRole::ToolbarGap));

    m_tabs = new TabBar(m_strip);
    m_stripRow->addWidget(m_tabs, 0);
    m_stripRow->addStretch(1);

    m_trailing = new QHBoxLayout;
    m_trailing->setSpacing(profile.space(SpacingRole::ToolbarGap));
    m_stripRow->addLayout(m_trailing);

    // The one control whose only job is to ask. It shows the state it would
    // move *to*, which is why it is a chevron and not a switch.
    auto* toggle = new IconButton(Glyph::ChevronDown, IconButton::Variant::Ghost, m_strip);
    toggle->setControlSize(ControlSize::Small);
    toggle->setToolTip(tr("Show or hide the panel"));
    m_toggle = toggle;
    m_stripRow->addWidget(toggle);
    connect(toggle, &IconButton::clicked, this,
            [this] { emit expandRequested(!m_expanded); });

    // A tab is also a request to open: clicking one while the panel is shut is
    // the commonest way anybody opens a console, and making them click twice is
    // the kind of friction nobody reports and everybody feels.
    connect(m_tabs, &TabBar::currentChanged, this, [this](const QString& key) {
        if (m_pages) {
            if (QWidget* widget = page(key)) {
                m_pages->setCurrentWidget(widget);
            }
        }
        emit currentChanged(key);
        if (!m_expanded) {
            emit expandRequested(true);
        }
    });
    connect(m_tabs, &TabBar::closeRequested, this, &BottomPanel::pageCloseRequested);
    connect(m_tabs, &TabBar::newTabRequested, this, &BottomPanel::newPageRequested);
}

int BottomPanel::stripHeight() const
{
    const ThemeProfile& profile = themeFor(this);
    return profile.controlHeight(ControlSize::Medium) + profile.spacing.xs;
}

int BottomPanel::addPage(const QString& key, const QString& title, QWidget* page,
                         Glyph glyph)
{
    if (!page) {
        return -1;
    }
    page->setProperty("novuPanelKey", key);
    m_pages->addWidget(page);
    const int index = m_tabs->addTab(key, title, glyph);
    if (m_pages->count() == 1) {
        m_pages->setCurrentWidget(page);
    }
    return index;
}

void BottomPanel::removePage(const QString& key)
{
    if (QWidget* widget = page(key)) {
        m_pages->removeWidget(widget);
        widget->deleteLater();
    }
    m_tabs->removeTab(key);
}

QWidget* BottomPanel::page(const QString& key) const
{
    for (int i = 0; i < m_pages->count(); ++i) {
        QWidget* widget = m_pages->widget(i);
        if (widget && widget->property("novuPanelKey").toString() == key) {
            return widget;
        }
    }
    return nullptr;
}

int BottomPanel::pageCount() const
{
    return m_pages ? m_pages->count() : 0;
}

QString BottomPanel::currentKey() const
{
    return m_tabs ? m_tabs->currentKey() : QString();
}

void BottomPanel::setCurrentKey(const QString& key)
{
    if (m_tabs) {
        m_tabs->setCurrentKey(key);
    }
}

void BottomPanel::setExpanded(bool expanded)
{
    if (m_expanded == expanded) {
        return;
    }
    m_expanded = expanded;
    applyExpanded();
}

void BottomPanel::applyExpanded()
{
    if (m_pages) {
        m_pages->setVisible(m_expanded);
    }
    if (auto* toggle = qobject_cast<IconButton*>(m_toggle)) {
        // Points the way it would go: down to put the panel away, up to bring
        // it back.
        toggle->setGlyph(m_expanded ? Glyph::ChevronDown : Glyph::Back);
    }
    updateGeometry();
    update();
}

void BottomPanel::setExpandedHeight(int height)
{
    height = qMax(0, height);
    if (m_expandedHeight == height) {
        return;
    }
    m_expandedHeight = height;
    if (m_expanded) {
        updateGeometry();
    }
}

StyleSpec BottomPanel::styleSpec() const
{
    StyleSpec spec;
    spec.radius = SurfaceRole::Toolbar;
    spec.borderWidth = BorderRole::Divider;
    // The panel is a surface of its own under the content, so it takes the
    // window ground and the between-surfaces rule rather than the card one.
    spec.background = token(ColorRole::WindowBg);
    spec.border = token(ColorRole::BorderHover);
    spec.foreground = token(ColorRole::TextSecondary);
    return spec;
}

ResolvedStyle BottomPanel::resolvedStyle() const
{
    return resolve(styleSpec(), this, height());
}

QSize BottomPanel::sizeHint() const
{
    const int strip = stripHeight();
    return QSize(320, m_expanded ? strip + m_expandedHeight : strip);
}

QSize BottomPanel::minimumSizeHint() const
{
    return QSize(160, stripHeight());
}

bool BottomPanel::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type()
        || event->type() == QEvent::ParentChange) {
        if (m_strip) {
            m_strip->setFixedHeight(stripHeight());
        }
        updateGeometry();
        update();
    }
    return QWidget::event(event);
}

void BottomPanel::paintEvent(QPaintEvent*)
{
    const ResolvedStyle style = resolvedStyle();

    QPainter painter(this);
    painter.fillRect(rect(), style.background);

    // The rule along the top, which is what separates the panel from the
    // content above it. Between surfaces, so the stronger of the two weights.
    const int line = qMax(1, style.borderWidth);
    painter.fillRect(QRect(0, 0, width(), line), style.border);
}

} // namespace novu::ui
