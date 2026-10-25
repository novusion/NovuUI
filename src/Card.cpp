#include "novu/ui/Card.h"

#include "novu/ui/StyleSheet.h"
#include "novu/ui/Text.h"

#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>
#include <QVBoxLayout>

namespace novu::ui {

namespace {

// `.card-head{padding:13px 16px}`.
constexpr int kHeaderPaddingV = 13;

void applyTypeRole(QLabel* label, const ThemeProfile& profile, TypeRole role, const QColor& colour)
{
    // Through roleFont(), not by editing the label's inherited font. Editing it
    // never named a family, so the label kept whatever the application default
    // carried and only took size and weight from the profile - which is how a
    // card title ended up lighter than the same role elsewhere on the page.
    // One font-building path, or the two drift.
    const bool mono = profile.text(role).monospace;
    label->setFont(mono ? roleMonoFont(label, role) : roleFont(label, role));
    setInk(label, colour);
}

} // namespace

Card::Card(QWidget* parent)
    : Card(QString(), parent)
{
}

Card::Card(const QString& title, QWidget* parent)
    : QFrame(parent)
{
    setFrameShape(QFrame::NoFrame);
    // The card paints its own surface; letting Qt fill the background as well
    // would draw a square corner behind the rounded one.
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAutoFillBackground(false);

    // The card's own layout carries nothing: `.card` has no padding of its
    // own, `.card-head` has `13px 16px` and `.card-body` has `15px 16px`.
    // Two layouts, still one widget - the inner one is the body's padding,
    // which is what body() hands out.
    m_outer = new QVBoxLayout(this);
    m_outer->setContentsMargins(0, 0, 0, 0);
    m_outer->setSpacing(0);

    m_body = new QVBoxLayout();

    m_headerRow = new QHBoxLayout();

    // Title and subtitle share one column so the trailing widgets stay
    // vertically centred against the pair, not against the title alone.
    auto* titleColumn = new QVBoxLayout();
    // `.card-head .sub{margin-top:3px}`.
    titleColumn->setSpacing(3);
    m_title = new QLabel(title, this);
    m_title->setVisible(!title.isEmpty());
    titleColumn->addWidget(m_title);
    m_subtitle = new QLabel(this);
    m_subtitle->hide();
    // `.card-head .sub` is a block: it wraps under the title rather than
    // running past the card's own edge.
    //
    // What it must not do is wrap while there is room. `card()` builds the
    // head as `<div><h2>…</h2><div class="sub">…</div></div><div class=
    // "spacer"></div>`, and a wrapped QLabel reports a height-for-width hint
    // that makes its column's *width* hint collapse next to a stretch - so
    // with the spacer as a separate stretch item the sentence broke into
    // three lines in a card eight hundred pixels wide. The stretch is given
    // to the title column itself instead: the trailing controls still sit at
    // the right edge, and the subtitle wraps at the width it actually has.
    m_subtitle->setWordWrap(true);
    titleColumn->addWidget(m_subtitle);
    m_headerRow->addLayout(titleColumn, 1);

    // In a widget of its own, so setHeaderVisible(false) removes the gap the
    // body layout would otherwise keep around an empty row.
    m_header = new QWidget(this);
    m_header->setLayout(m_headerRow);
    // Content-first (`.card-head` is height:auto): the head is never handed
    // spare height. A card made taller than its content - stretched by a
    // tall neighbour - used to give it to the head when the body's rows
    // could not grow: the title sat at the top, the subtitle half way down
    // and the fields at the bottom. Spare height now stays below the body.
    m_header->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    m_outer->addWidget(m_header);
    m_outer->addLayout(m_body, 1);
    m_outer->addStretch(0);

    updateHeaderVisibility();
    applyTheme();
}

void Card::setTitle(const QString& title)
{
    m_title->setText(title);
    m_title->setVisible(!title.isEmpty());
    updateHeaderVisibility();
}

QString Card::title() const
{
    return m_title->text();
}

void Card::setSubtitle(const QString& subtitle)
{
    m_subtitle->setText(subtitle);
    m_subtitle->setVisible(!subtitle.isEmpty());
    updateHeaderVisibility();
    applyTheme();
}

QString Card::subtitle() const
{
    return m_subtitle->text();
}

void Card::setHeaderTrailing(QWidget* widget)
{
    setHeaderTrailing(widget ? QList<QWidget*>{widget} : QList<QWidget*>{});
}

void Card::setHeaderTrailing(const QList<QWidget*>& widgets)
{
    if (m_headerTrailing == widgets) {
        return;
    }
    for (QWidget* previous : m_headerTrailing) {
        if (!previous) {
            continue;
        }
        m_headerRow->removeWidget(previous);
        // Reparented away rather than deleted: the caller owns these.
        previous->setParent(nullptr);
    }
    m_headerTrailing.clear();

    for (QWidget* widget : widgets) {
        if (!widget) {
            continue;
        }
        widget->setParent(this);
        m_headerRow->addWidget(widget);
        m_headerTrailing.append(widget);
    }
    updateHeaderVisibility();
}

QVBoxLayout* Card::body() const
{
    return m_body;
}

void Card::setHeaderMetrics(const QMargins& margins, int minimumContentHeight)
{
    m_ownHeaderMetrics = true;
    m_headerMargins = margins;
    m_headerMinimumContentHeight = minimumContentHeight;
    applyTheme();
}

void Card::setHeaderVisible(bool visible)
{
    m_headerVisible = visible;
    m_title->setVisible(visible && !m_title->text().isEmpty());
    for (QWidget* trailing : m_headerTrailing) {
        if (trailing) {
            trailing->setVisible(visible);
        }
    }
    updateHeaderVisibility();
}

// The row is worth its own line in the layout only when it has something to
// show. Empty, it would still cost the body's gap - which is how a StatCard
// ended up a section gap taller than the metric inside it.
void Card::updateHeaderVisibility()
{
    const bool anyTrailing = std::any_of(m_headerTrailing.cbegin(), m_headerTrailing.cend(),
                                         [](QWidget* w) { return w != nullptr; });
    m_header->setVisible(m_headerVisible
                         && (!m_title->text().isEmpty() || !m_subtitle->text().isEmpty()
                             || anyTrailing));
}

void Card::setSurface(Surface surface)
{
    if (m_surface == surface) {
        return;
    }
    m_surface = surface;
    update();
}

void Card::setHeaderDivider(bool divider)
{
    if (m_headerDivider == divider) {
        return;
    }
    m_headerDivider = divider;
    update();
}

void Card::setBorderVisible(bool visible)
{
    if (m_borderVisible == visible) {
        return;
    }
    m_borderVisible = visible;
    applyTheme();
    update();
}

void Card::setBodyMetrics(const QMargins& margins, int spacing)
{
    m_ownMetrics = true;
    m_bodyMargins = margins;
    m_bodySpacing = spacing;
    applyTheme();
}

void Card::clearBodyMetrics()
{
    if (!m_ownMetrics) {
        return;
    }
    m_ownMetrics = false;
    applyTheme();
}

void Card::applyTheme()
{
    const ThemeProfile& profile = themeFor(this);
    // CSS lays a card's children inside its border box. With zero outer
    // margins, a flush Qt child can paint over that one-pixel outline and
    // square whichever pair of corners it reaches. Keep the content inside
    // the outline; the card remains the only owner of all four corners.
    const int edge = m_borderVisible ? profile.border(BorderRole::Card) : 0;
    m_outer->setContentsMargins(edge, edge, edge, edge);
    const int padding = profile.space(SpacingRole::CardPadding);
    // `.card-body{padding:15px 16px}` - the two are not the same number, and
    // a profile that never says so keeps the single padding it always had.
    const int paddingV = profile.roleSpacing.value(
        static_cast<int>(SpacingRole::CardPaddingV), padding);
    if (m_ownMetrics) {
        m_body->setContentsMargins(m_bodyMargins);
        m_body->setSpacing(m_bodySpacing);
    } else {
        m_body->setContentsMargins(padding, paddingV, padding, paddingV);
        m_body->setSpacing(profile.roleSpacing.value(
            static_cast<int>(SpacingRole::CardBodyGap),
            profile.space(SpacingRole::SectionGap)));
    }
    // `.card-head{gap:12px}` - the profile's own medium step.
    m_headerRow->setSpacing(profile.spacing.md);
    // `.card-head{padding:13px 16px}` - its own padding, because the body
    // below it may be flush against the card's edge (a table) while the
    // header never is.
    m_headerRow->setContentsMargins(profile.space(SpacingRole::CardPadding),
                                    kHeaderPaddingV,
                                    profile.space(SpacingRole::CardPadding),
                                    kHeaderPaddingV);
    if (m_ownHeaderMetrics) {
        m_headerRow->setContentsMargins(m_headerMargins);
        m_header->setMinimumHeight(m_headerMinimumContentHeight + m_headerMargins.top() + m_headerMargins.bottom());
    }
    // The approved design gives a card title the primary ink; secondary made it
    // recede into the surface and the card read as unlabelled.
    applyTypeRole(m_title, profile, TypeRole::CardTitle, profile.colors.textPrimary);
    applyTypeRole(m_subtitle, profile, TypeRole::Caption, profile.colors.textDisabled);
    updateGeometry();
}

void Card::setSelected(bool selected)
{
    if (m_selected == selected) return;
    m_selected = selected;
    update();
}

void Card::setInteractive(bool interactive)
{
    m_interactive = interactive;
    m_pressed = false;
    setFocusPolicy(interactive ? Qt::StrongFocus : Qt::NoFocus);
    setCursor(interactive ? Qt::PointingHandCursor : Qt::ArrowCursor);
    update();
}

void Card::mousePressEvent(QMouseEvent* event)
{
    if (m_interactive && isEnabled() && event->button() == Qt::LeftButton) {
        m_pressed = true;
        setFocus(Qt::MouseFocusReason);
        event->accept();
        update();
        return;
    }
    QFrame::mousePressEvent(event);
}

void Card::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_interactive && event->button() == Qt::LeftButton) {
        const bool activate = m_pressed && isEnabled() && rect().contains(event->position().toPoint());
        m_pressed = false;
        event->accept();
        update();
        if (activate) emit clicked();
        return;
    }
    QFrame::mouseReleaseEvent(event);
}

void Card::keyPressEvent(QKeyEvent* event)
{
    if (m_interactive && isEnabled() && (event->key() == Qt::Key_Space ||
        event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)) {
        if (!event->isAutoRepeat()) m_pressed = true;
        event->accept();
        update();
        return;
    }
    QFrame::keyPressEvent(event);
}

void Card::keyReleaseEvent(QKeyEvent* event)
{
    if (m_interactive && (event->key() == Qt::Key_Space ||
        event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)) {
        const bool activate = m_pressed && isEnabled() && !event->isAutoRepeat();
        if (!event->isAutoRepeat()) m_pressed = false;
        event->accept();
        update();
        if (activate) emit clicked();
        return;
    }
    QFrame::keyReleaseEvent(event);
}

bool Card::event(QEvent* event)
{
    if (event->type() == QEvent::FocusOut || event->type() == QEvent::EnabledChange)
        m_pressed = false;
    if (m_interactive && (event->type() == QEvent::Enter || event->type() == QEvent::Leave ||
        event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut ||
        event->type() == QEvent::EnabledChange)) update();
    // Live retheming: the profile may have been edited or swapped under a
    // running window, so geometry and fonts are recomputed, not just repainted.
    //
    // PolishRequest is in here for a second reason. Applying the scope sheet to
    // the page re-polishes its widgets, and that drops the font this card set
    // on its title in the constructor - the title came out at the default size
    // and weight while every other label on the page kept its role. Re-applying
    // after the polish is what makes the title survive it.
    if (event->type() == ThemeChangeEvent::type()
        || event->type() == QEvent::PolishRequest
        || event->type() == QEvent::ParentChange) {
        applyTheme();
        update();
    }
    return QFrame::event(event);
}

void Card::paintEvent(QPaintEvent* event)
{
    const ThemeProfile& profile = themeFor(this);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF surface = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const int radius = qMin(m_ownRadius ? profile.radii.value(m_radiusStep) : profile.radius(SurfaceRole::Card),
                            static_cast<int>(surface.height() / 2));

    const bool raised = m_surface == Surface::Raised;

    QPainterPath path;
    path.addRoundedRect(surface, radius, radius);
    const bool active = m_interactive && isEnabled();
    const QColor hover = !m_ownHover ? profile.colors.surfaceHover
        : m_hoverSurface == Surface::Window ? profile.colors.windowBg
        : m_hoverSurface == Surface::Elevated ? profile.colors.surfaceHover
        : m_hoverSurface == Surface::Raised ? profile.colors.panelBgAlt : profile.colors.panelBg;
    QColor hoverBorder = profile.colors.borderHover;
    if (m_hoverAccentBorder) { hoverBorder = profile.colors.accent; hoverBorder.setAlphaF(0.4); }
    painter.fillPath(path, active && m_pressed ? profile.colors.surfaceActive
        : active && underMouse() ? hover
        : m_surface == Surface::Elevated ? profile.colors.surfaceHover
        : raised ? profile.colors.panelBgAlt
        : m_surface == Surface::Window ? profile.colors.windowBg : profile.colors.panelBg);
    if (m_selected) {
        QColor tint = profile.colors.accent;
        tint.setAlphaF(m_selectionFill);
        painter.fillPath(path, tint);
    }

    if(m_leadingWidth>0&&!m_leadingAccent.isTransparent()){
        painter.save();painter.setClipPath(path);painter.fillRect(QRectF(0,0,m_leadingWidth,height()),m_leadingAccent.resolve(profile));painter.restore();
    }
    QColor selectedBorder=profile.colors.accent;selectedBorder.setAlphaF(m_selectionBorder);
    const int borderWidth = profile.border(BorderRole::Card);
    if (borderWidth > 0 && m_borderVisible) {
        painter.setPen(QPen((m_selected || (active && hasFocus())) ? selectedBorder
            : active && underMouse() ? hoverBorder : m_borderEmphasized?profile.colors.borderHover:profile.colors.border, borderWidth,m_borderStyle));
        painter.drawPath(path);
    }

    // `.card-head{border-bottom:1px solid var(--hair)}` - drawn edge to edge,
    // the way a border on a full-width row is, not inset to the body padding.
    if (m_headerDivider && m_header && m_header->isVisible()) {
        const int y = m_header->geometry().bottom() + 1;
        paintHairline(&painter, QPoint(1, y), QPoint(width() - 2, y),
                      profile.colors.border,
                      qMax(1, profile.border(BorderRole::Divider)));
    }

    QFrame::paintEvent(event);
}

QLabel* section(QVBoxLayout* into, const QString& title, QWidget* parent)
{
    auto* label = new QLabel(title, parent);
    const ThemeProfile& profile = themeFor(parent);
    applyTypeRole(label, profile, TypeRole::SectionTitle, profile.colors.textPrimary);
    if (into) {
        into->addWidget(label);
    }
    return label;
}

} // namespace novu::ui
