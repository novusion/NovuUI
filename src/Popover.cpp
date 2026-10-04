#include "novu/ui/Popover.h"

#include <QApplication>

#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QScreen>

#include <utility>

namespace novu::ui {

namespace {

// `.popover { min-width:230px; border-radius:12px; padding:6px }`.
constexpr int kMinWidth = 230;
constexpr int kPadding = 6;

// `.pitem { padding:7px 10px; gap:9px; border-radius:8px }`.
constexpr int kItemPaddingH = 10;
constexpr int kItemPaddingV = 7;
constexpr int kItemGap = 9;
constexpr int kItemRadius = 8;
constexpr int kItemGlyph = 14;

// `.popover .pgrp { padding:8px 10px 5px }`.
constexpr int kGroupTop = 8;
constexpr int kGroupBottom = 5;

// `.psep { height:1px; margin:5px 4px }`.
constexpr int kSeparatorMargin = 5;
constexpr int kSeparatorInset = 4;

constexpr double kDisabledOpacity = 0.42;

QFont itemFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::Body);
    // `.pitem{font-size:12.5px}` - the `md` step, not `sm`.
    font.setPixelSize(profile.type.md);
    return font;
}

QFont groupFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleMonoFont(context, TypeRole::Eyebrow);
    font.setPixelSize(qMax(8, profile.type.xxs - 1));
    font.setLetterSpacing(QFont::PercentageSpacing, 112);
    return font;
}

QFont metaFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleMonoFont(context, TypeRole::Mono);
    font.setPixelSize(qMax(8, profile.type.xxs));
    return font;
}

} // namespace

// ══════════════════════════════════════════════════════════════════════════
//  the control that opens one
// ══════════════════════════════════════════════════════════════════════════
namespace {

// `.select{height:26px; padding:0 11px; gap:8px}` and `.select.xs{height:23px;
// padding:0 9px; gap:6px}`; `.select .cv svg{11x11}`.
constexpr int kSelectChevron = 11;
constexpr int kSelectGlyph = 13;

int selectPadding(ControlSize size)
{
    return size == ControlSize::Small ? 9 : 11;
}

int selectGap(ControlSize size)
{
    return size == ControlSize::Small ? 6 : 8;
}

} // namespace

SelectButton::SelectButton(QWidget* parent)
    : QAbstractButton(parent)
{
    setCursor(Qt::PointingHandCursor);
    // Nothing is typed into it and nothing is tabbed into it: it is a menu
    // with its current answer written on the front.
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_menu = new Popover(this);
    connect(m_menu, &Popover::triggered, this, [this](const QString& key) {
        setCurrentKey(key);
        emit currentChanged(key);
    });
    connect(m_menu, &Popover::closed, this, [this] {
        m_open = false;
        update();
    });
    connect(this, &QAbstractButton::clicked, this, &SelectButton::openMenu);
    applyGeometry();
}

void SelectButton::setOptions(const QStringList& options)
{
    clearOptions();
    for (const QString& option : options) {
        addOption(option, option);
    }
}

int SelectButton::addOption(const QString& key, const QString& label, Glyph glyph)
{
    const int index = m_menu->addItem(key, label, glyph);
    if (m_currentKey.isEmpty()) {
        setCurrentKey(key);
    }
    return index;
}

void SelectButton::clearOptions()
{
    m_menu->clear();
    m_currentKey.clear();
    applyGeometry();
    update();
}

void SelectButton::setCurrentKey(const QString& key)
{
    if (m_currentKey == key) {
        return;
    }
    if (!m_currentKey.isEmpty()) {
        m_menu->setChecked(m_currentKey, false);
    }
    m_currentKey = key;
    // `.pitem.on` -- the menu remembers which one is in force, so opening it
    // again shows the answer rather than only the list.
    m_menu->setChecked(key, true);
    applyGeometry();
    update();
}

QString Popover::itemLabel(const QString& key) const
{
    for(const auto& item:m_items)if(item.key==key)return item.label;
    return key;
}

QString SelectButton::currentText() const
{
    return m_menu->itemLabel(m_currentKey);
}

void SelectButton::setStatus(StatusDot::Tone tone, bool live)
{
    if (!m_dot) {
        m_dot = new StatusDot(tone, this);
        m_dot->setAttribute(Qt::WA_TransparentForMouseEvents);
    }
    m_dot->setTone(tone);
    m_dot->setPulsing(live);
    m_dot->show();
    applyGeometry();
    update();
}

void SelectButton::clearStatus()
{
    delete m_dot;
    m_dot = nullptr;
    applyGeometry();
    update();
}

void SelectButton::setGlyph(Glyph glyph)
{
    if (m_glyph == glyph) {
        return;
    }
    m_glyph = glyph;
    applyGeometry();
    update();
}

void SelectButton::setControlSize(ControlSize size)
{
    if (m_size == size) {
        return;
    }
    m_size = size;
    applyGeometry();
    update();
}

static int selectHeight(const ThemeProfile& p,ControlSize size){
    const int explicitHeight=size==ControlSize::Small?p.controls.selectSmallHeight:size==ControlSize::Medium?p.controls.selectHeight:0;
    return explicitHeight>0?explicitHeight:p.controlHeight(size);
}
void SelectButton::applyGeometry()
{
    const ThemeProfile& profile = themeFor(this);
    setFixedHeight(selectHeight(profile,m_size));
    if (m_dot) {
        const QSize dot = m_dot->sizeHint();
        m_dot->setGeometry(selectPadding(m_size), (height() - dot.height()) / 2,
                           dot.width(), dot.height());
    }
    updateGeometry();
}

// `.select{font-size:var(--fs-md)}` against `.select.xs{font-size:var(--fs-sm)}`,
// both at weight 500.
QFont SelectButton::labelFont() const
{
    const TypeRole role =
        m_labelRole != TypeRole::Body
            ? m_labelRole
            : (m_size == ControlSize::Small ? TypeRole::Caption : TypeRole::Body);
    QFont font = role == TypeRole::Mono ? roleMonoFont(this, role)
                                        : roleFont(this, role);
    if (role != TypeRole::Mono) {
        font.setWeight(QFont::Medium);
    }
    return font;
}

void SelectButton::setLabelRole(TypeRole role)
{
    if (m_labelRole == role) {
        return;
    }
    m_labelRole = role;
    applyGeometry();
    update();
}

QSize SelectButton::sizeHint() const
{
    const ThemeProfile& profile = themeFor(this);
    const int pad = selectPadding(m_size);
    const int gap = selectGap(m_size);
    const QFont font = labelFont();
    int width = pad + QFontMetrics(font).horizontalAdvance(currentText())
        + gap + kSelectChevron + pad;
    if(!m_metaText.isEmpty())width+=QFontMetrics(roleMonoFont(this,TypeRole::Caption)).horizontalAdvance(m_metaText)+gap;
    if (m_dot) {
        width += m_dot->sizeHint().width() + gap;
    }
    if (m_glyph != Glyph::None) {
        width += kSelectGlyph + gap;
    }
    return QSize(width, selectHeight(profile,m_size));
}

void SelectButton::openMenu()
{
    if (m_menu->count() == 0) {
        return;
    }
    m_open = true;
    update();
    m_menu->popupUnder(this);
}

void SelectButton::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // `.select{border:1px solid var(--hair2); background:var(--panel)}`,
    // `:hover{border-color:var(--text4); background:var(--card)}`,
    // `.open{border-color:var(--acc)}`.
    const QRectF box = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const double radius =
        qMin(double(profile.radius(SurfaceRole::Select)), box.height() / 2.0);
    QColor border = profile.colors.borderHover;
    QColor fill = profile.colors.panelBgAlt;
    if (m_open) {
        border = profile.colors.accent;
    } else if (underMouse()) {
        border = profile.colors.textDisabled;
        fill = profile.colors.surfaceHover;
    }
    painter.setPen(QPen(border, 1));
    painter.setBrush(fill);
    painter.drawRoundedRect(box, radius, radius);

    const int pad = selectPadding(m_size);
    const int gap = selectGap(m_size);
    int x = pad;
    if (m_dot) {
        x += m_dot->width() + gap;
    }
    if (m_glyph != Glyph::None) {
        paintGlyph(&painter, m_glyph,
                   QRectF(x, (height() - kSelectGlyph) / 2.0, kSelectGlyph,
                          kSelectGlyph),
                   profile.colors.accentLight);
        x += kSelectGlyph + gap;
    }

    const QRectF chevron(width() - pad - kSelectChevron,
                         (height() - kSelectChevron) / 2.0, kSelectChevron,
                         kSelectChevron);
    paintGlyph(&painter, Glyph::ChevronDown, chevron, profile.colors.textDisabled,
               m_open ? 180.0 : 0.0);

    painter.setFont(labelFont());
    painter.setPen(m_open || underMouse() ? profile.colors.textPrimary
                                          : profile.colors.textSecondary);
    int textRight=int(chevron.left())-gap;
    if(!m_metaText.isEmpty()){
        auto metaFont=roleMonoFont(this,TypeRole::Caption);metaFont.setPointSizeF(profile.type.xs*72.0/logicalDpiY());
        const int metaWidth=QFontMetrics(metaFont).horizontalAdvance(m_metaText);
        painter.setFont(metaFont);painter.setPen(profile.colors.textDisabled);
        painter.drawText(QRect(textRight-metaWidth,0,metaWidth,height()),Qt::AlignVCenter|Qt::AlignRight,m_metaText);
        textRight-=metaWidth+gap;painter.setFont(labelFont());painter.setPen(m_open||underMouse()?profile.colors.textPrimary:profile.colors.textSecondary);
    }
    const QRect text(x, 0, qMax(0,textRight-x), height());
    painter.drawText(text, Qt::AlignVCenter | Qt::AlignLeft,
                     painter.fontMetrics().elidedText(currentText(), Qt::ElideRight,
                                                      text.width()));
}

void SelectButton::enterEvent(QEnterEvent* event)
{
    QAbstractButton::enterEvent(event);
    update();
}

void SelectButton::leaveEvent(QEvent* event)
{
    QAbstractButton::leaveEvent(event);
    update();
}

bool SelectButton::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type()) {
        applyGeometry();
        update();
    }
    return QAbstractButton::event(event);
}

Popover::Popover(QWidget* parent)
    : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
{
    // Qt::Popup is what closes the menu when the user clicks elsewhere and what
    // gives it the mouse grab. Without it a "menu" is a window that stays open
    // behind whatever the user does next.
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

int Popover::addItem(const QString& key, const QString& label, Glyph glyph)
{
    Entry entry;
    entry.kind = Kind::Item;
    entry.key = key;
    entry.label = label;
    entry.glyph = glyph;
    m_items.append(entry);
    updateGeometry();
    update();
    return m_items.size() - 1;
}

Popover* Popover::addSubmenu(const QString& key,const QString& label,Glyph glyph)
{
    const int index=addItem(key,label,glyph);auto* menu=new Popover(this);menu->m_parentMenu=this;m_items[index].submenu=menu;
    connect(menu,&Popover::triggered,this,&Popover::triggered);return menu;
}
void Popover::openSubmenu(int index,bool keyboard)
{
    for(int i=0;i<m_items.size();++i)if(i!=index&&m_items[i].submenu)m_items[i].submenu->hide();
    if(index<0||index>=m_items.size()||!m_items[index].enabled||!m_items[index].submenu)return;
    auto* menu=m_items[index].submenu.data();if(menu->isVisible()){if(keyboard)menu->setFocus();return;}
    const QRect row=itemRect(index);QPoint origin=mapToGlobal(QPoint(width()-2,row.top()-kPadding));
    if(auto* screen=QGuiApplication::screenAt(origin))if(origin.x()+menu->sizeHint().width()>screen->availableGeometry().right()-4)origin.setX(mapToGlobal(QPoint()).x()-menu->sizeHint().width()+2);
    menu->m_anchor=this;menu->popupAt(origin);
    if(keyboard){menu->m_hovered=menu->neighbour(-1,1);menu->update();}
}
int Popover::addGroup(const QString& title)
{
    Entry entry;
    entry.kind = Kind::Group;
    entry.label = title;
    m_items.append(entry);
    updateGeometry();
    update();
    return m_items.size() - 1;
}

int Popover::addSeparator()
{
    Entry entry;
    entry.kind = Kind::Separator;
    m_items.append(entry);
    updateGeometry();
    update();
    return m_items.size() - 1;
}

void Popover::clear()
{
    for(auto& entry:m_items)if(entry.submenu)delete entry.submenu.data();
    m_items.clear();
    m_hovered = -1;
    m_pressed = -1;
    updateGeometry();
    update();
}

int Popover::indexOfKey(const QString& key) const
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).kind == Kind::Item && m_items.at(i).key == key) {
            return i;
        }
    }
    return -1;
}

const Popover::Entry* Popover::find(const QString& key) const
{
    const int index = indexOfKey(key);
    return index < 0 ? nullptr : &m_items.at(index);
}

Popover::Entry* Popover::find(const QString& key)
{
    return const_cast<Entry*>(std::as_const(*this).find(key));
}

void Popover::setMeta(const QString& key, const QString& meta)
{
    Entry* entry = find(key);
    if (!entry || entry->meta == meta) {
        return;
    }
    entry->meta = meta;
    updateGeometry();
    update();
}

void Popover::setChecked(const QString& key, bool checked)
{
    Entry* entry = find(key);
    if (!entry || entry->checked == checked) {
        return;
    }
    entry->checked = checked;
    update();
}

bool Popover::isChecked(const QString& key) const
{
    const Entry* entry = find(key);
    return entry && entry->checked;
}

void Popover::setDangerous(const QString& key, bool dangerous)
{
    Entry* entry = find(key);
    if (!entry || entry->dangerous == dangerous) {
        return;
    }
    entry->dangerous = dangerous;
    update();
}

void Popover::setItemEnabled(const QString& key, bool enabled)
{
    Entry* entry = find(key);
    if (!entry || entry->enabled == enabled) {
        return;
    }
    entry->enabled = enabled;
    update();
}

int Popover::entryHeight(const Entry& entry) const
{
    switch (entry.kind) {
    case Kind::Separator:
        return 1 + 2 * kSeparatorMargin;
    case Kind::Group:
        return QFontMetrics(groupFont(this)).height() + kGroupTop + kGroupBottom;
    case Kind::Item:
        break;
    }
    return qMax(QFontMetrics(itemFont(this)).height(), kItemGlyph) + 2 * kItemPaddingV;
}

QRect Popover::itemRect(int index) const
{
    if (index < 0 || index >= m_items.size()) {
        return QRect();
    }
    int y = kPadding;
    for (int i = 0; i < index; ++i) {
        y += entryHeight(m_items.at(i));
    }
    return QRect(kPadding, y, qMax(0, width() - 2 * kPadding),
                 entryHeight(m_items.at(index)));
}

int Popover::indexAt(const QPoint& position) const
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).kind == Kind::Item && itemRect(i).contains(position)) {
            return i;
        }
    }
    return -1;
}

int Popover::neighbour(int from, int step) const
{
    for (int i = from + step; i >= 0 && i < m_items.size(); i += step) {
        if (m_items.at(i).kind == Kind::Item && m_items.at(i).enabled) {
            return i;
        }
    }
    return -1;
}

QSize Popover::sizeHint() const
{
    const QFontMetrics items(itemFont(this));
    const QFontMetrics metas(metaFont(this));
    const QFontMetrics groups(groupFont(this));

    int height = 2 * kPadding;
    int width = 0;
    for (const Entry& entry : m_items) {
        height += entryHeight(entry);
        switch (entry.kind) {
        case Kind::Separator:
            break;
        case Kind::Group:
            width = qMax(width, groups.horizontalAdvance(entry.label.toUpper())
                                    + 2 * kItemPaddingH);
            break;
        case Kind::Item: {
            int own = 2 * kItemPaddingH + items.horizontalAdvance(entry.label);
            if (entry.glyph != Glyph::None) {
                own += kItemGlyph + kItemGap;
            }
            if (!entry.meta.isEmpty()) {
                own += kItemGap * 2 + metas.horizontalAdvance(entry.meta);
            }
            if(entry.submenu)own+=kItemGlyph+kItemGap;
            width = qMax(width, own);
            break;
        }
        }
    }
    return QSize(qMax(kMinWidth, width + 2 * kPadding), height);
}

Popover* Popover::chainRoot()
{
    Popover* root = this;
    while (root->m_parentMenu)
        root = root->m_parentMenu.data();
    return root;
}

bool Popover::chainContains(const QWidget* widget) const
{
    if (!widget) {
        return false;
    }
    // Walk the whole chain from its root: the widget may be an ancestor menu,
    // this menu, or any submenu opened from it.
    const Popover* root = this;
    while (root->m_parentMenu) {
        root = root->m_parentMenu.data();
    }
    QList<const Popover*> pending{root};
    while (!pending.isEmpty()) {
        const Popover* menu = pending.takeLast();
        if (menu == widget || menu->isAncestorOf(widget)) {
            return true;
        }
        for (const Entry& entry : menu->m_items) {
            if (entry.submenu) {
                pending.append(entry.submenu.data());
            }
        }
    }
    return false;
}

void Popover::closeChain()
{
    Popover* root = chainRoot();
    QList<Popover*> pending{root};
    while (!pending.isEmpty()) {
        Popover* menu = pending.takeLast();
        for (const Entry& entry : menu->m_items) {
            if (entry.submenu) {
                pending.append(entry.submenu.data());
            }
        }
        menu->hide();
    }
}

void Popover::dismissForeignPopup(QWidget* anchor)
{
    QWidget* active = QApplication::activePopupWidget();
    // A popover opened by a control inside an existing PopupSurface is a
    // child interaction, not a competing top-level popup. Keep the owning
    // surface alive while this menu temporarily takes the popup grab.
    const bool ownedByActivePopup = active && anchor
        && (active == anchor || active->isAncestorOf(anchor));
    if (!active || active == this || chainContains(active) || ownedByActivePopup) {
        return;
    }
    // Qt closes the previous popup through the platform grab. Where that grab
    // is absent the old menu simply stays on screen, so say it explicitly.
    if (auto* other = qobject_cast<Popover*>(active)) {
        other->closeChain();
    } else {
        active->close();
    }
}

void Popover::popupAt(const QPoint& globalPosition)
{
    dismissForeignPopup();
    const QSize size = sizeHint();
    setFixedSize(size);

    const QScreen* screen = QGuiApplication::screenAt(globalPosition);
    const QRect bounds = (screen ? screen : QGuiApplication::primaryScreen())
                             ->availableGeometry();

    int x = qBound(bounds.left() + 4, globalPosition.x(),
                   qMax(bounds.left() + 4, bounds.right() - size.width() - 4));
    int y = globalPosition.y();
    if (y + size.height() > bounds.bottom() - 4) {
        // No room below: flip above the point rather than pushing the menu up
        // over the thing that opened it.
        y = qMax(bounds.top() + 4, globalPosition.y() - size.height());
    }

    move(x, y);
    m_hovered = -1;
    m_pressed = -1;
    show();
    raise();
    setFocus(Qt::PopupFocusReason);
}

void Popover::popupUnder(QWidget* anchor)
{
    if (!anchor) {
        return;
    }
    popupUnder(anchor, anchor->rect());
}

void Popover::popupUnder(QWidget* anchor, const QRect& anchorRect)
{
    if (!anchor) {
        return;
    }
    dismissForeignPopup(anchor);
    m_anchor = anchor;
    const QRect box(anchor->mapToGlobal(anchorRect.topLeft()), anchorRect.size());

    const QSize size = sizeHint();
    setFixedSize(size);

    const QScreen* screen = anchor->screen();
    const QRect bounds = (screen ? screen : QGuiApplication::primaryScreen())
                             ->availableGeometry();

    int x = qBound(bounds.left() + 4, box.left(),
                   qMax(bounds.left() + 4, bounds.right() - size.width() - 4));
    int y = box.bottom() + 4;
    if (y + size.height() > bounds.bottom() - 4) {
        y = qMax(bounds.top() + 4, box.top() - size.height() - 4);
    }

    move(x, y);
    m_hovered = -1;
    m_pressed = -1;
    show();
    raise();
    setFocus(Qt::PopupFocusReason);
}

void Popover::trigger(int index)
{
    if (index < 0 || index >= m_items.size()) {
        return;
    }
    const Entry& entry = m_items.at(index);
    if (entry.kind != Kind::Item || !entry.enabled) {
        return;
    }
    if(entry.submenu){openSubmenu(index,true);return;}
    // Closed before the signal, not after: a handler that opens a dialog would
    // otherwise do so with a menu still on screen above it.
    //
    // Unless this is a menu of choices rather than of commands, in which case
    // it stays up and the caller keeps picking - see setClosesOnTrigger().
    const QString key = entry.key;
    if (m_closesOnTrigger) {
        for(auto* menu=this;menu;menu=menu->m_parentMenu.data())menu->hide();
    }
    emit triggered(key);
}

void Popover::mousePressEvent(QMouseEvent* event)
{
    // A popup with a mouse grab is handed presses that land anywhere, not just
    // on itself. Accepting those unconditionally is what kept this menu open
    // over the application: the click had nowhere left to go. Outside the
    // chain, the press means dismiss.
    if (!rect().contains(event->position().toPoint())) {
        closeChain();
        event->ignore();
        return;
    }
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    const int index = indexAt(event->position().toPoint());
    m_pressed = (index >= 0 && m_items.at(index).enabled) ? index : -1;
    update();
    event->accept();
}

void Popover::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    const int released = indexAt(event->position().toPoint());
    const int pressed = m_pressed;
    m_pressed = -1;
    if (pressed >= 0 && released == pressed) {
        trigger(released);
        return;
    }
    update();
    event->accept();
}

void Popover::mouseMoveEvent(QMouseEvent* event)
{
    const int index = indexAt(event->position().toPoint());
    const int hovered = (index >= 0 && m_items.at(index).enabled) ? index : -1;
    if (hovered != m_hovered) {
        m_hovered = hovered;
        openSubmenu(hovered);
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void Popover::leaveEvent(QEvent* event)
{
    if(m_hovered>=0&&m_items[m_hovered].submenu&&m_items[m_hovered].submenu->isVisible()){QWidget::leaveEvent(event);return;}
    if (m_hovered != -1) {
        m_hovered = -1;
        update();
    }
    QWidget::leaveEvent(event);
}

void Popover::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Right:openSubmenu(m_hovered,true);return;
    case Qt::Key_Left:if(m_parentMenu){hide();m_parentMenu->setFocus(Qt::PopupFocusReason);return;}break;
    case Qt::Key_Down: {
        const int next = neighbour(m_hovered, 1);
        if (next >= 0) {
            m_hovered = next;
            update();
        }
        return;
    }
    case Qt::Key_Up: {
        const int previous = neighbour(m_hovered < 0 ? m_items.size() : m_hovered, -1);
        if (previous >= 0) {
            m_hovered = previous;
            update();
        }
        return;
    }
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space:
        trigger(m_hovered);
        return;
    case Qt::Key_Escape:
        for(auto* menu=this;menu;menu=menu->m_parentMenu.data())menu->hide();
        return;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void Popover::hideEvent(QHideEvent* event)
{
    for(auto& entry:m_items)if(entry.submenu)entry.submenu->hide();
    m_hovered = -1;
    m_pressed = -1;
    QWidget::hideEvent(event);
    emit closed();
}

bool Popover::event(QEvent* event)
{
    if (event->type() == QEvent::WindowDeactivate) {
        // The last signal that still arrives when the platform grab does not
        // reach the click: a native child surface such as a web view taking
        // focus, or another application being raised. A transient menu must
        // not hang over either. Moving between a parent menu and its submenu
        // deactivates too, so the chain itself is exempt.
        QWidget* active = QApplication::activePopupWidget();
        if (active == this || !chainContains(active)) {
            closeChain();
        }
    }
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        updateGeometry();
        update();
    }
    return QWidget::event(event);
}

StyleSpec Popover::styleSpec() const
{
    StyleSpec spec;
    spec.background = token(ColorRole::PanelBg);
    spec.border = token(ColorRole::Border);
    spec.foreground = token(ColorRole::TextSecondary);
    spec.radius = SurfaceRole::Overlay;
    spec.borderWidth = BorderRole::Card;
    spec.type = TypeRole::Body;
    return spec;
}

StyleSpec Popover::itemStyleSpec(int index) const
{
    StyleSpec spec = styleSpec();
    spec.radius = SurfaceRole::Button;
    spec.background = token(ColorRole::Transparent);
    spec.border = token(ColorRole::Transparent);
    spec.hoverBackground = token(ColorRole::PanelBgAlt);
    spec.hoverForeground = token(ColorRole::TextPrimary);
    spec.checkedForeground = token(ColorRole::AccentLight);
    spec.disabledForeground = token(ColorRole::TextDisabled, kDisabledOpacity);

    if (index >= 0 && index < m_items.size() && m_items.at(index).dangerous) {
        spec.foreground = statusToken(SemanticRole::Danger, ColorRole::StatusText);
        spec.hoverForeground = statusToken(SemanticRole::Danger, ColorRole::StatusText);
        spec.hoverBackground = statusToken(SemanticRole::Danger, ColorRole::StatusFill);
    }
    return spec;
}

ResolvedStyle Popover::resolvedStyle() const
{
    return resolve(styleSpec(), this, height());
}

void Popover::paintEvent(QPaintEvent*)
{
    // Resolved against the anchor where there is one. A popover is a top-level
    // window and so belongs to no ThemeScope; resolving against itself would
    // give a NovuTrade menu the default product's colours.
    const QWidget* context = m_anchor ? m_anchor.data() : static_cast<const QWidget*>(this);
    const ThemeProfile& profile = themeFor(context);
    const ResolvedStyle surface = resolve(styleSpec(), context, height());

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF card = QRectF(rect()).adjusted(surface.borderWidth / 2.0,
                                                surface.borderWidth / 2.0,
                                                -surface.borderWidth / 2.0,
                                                -surface.borderWidth / 2.0);
    painter.setPen(QPen(surface.border, surface.borderWidth));
    painter.setBrush(surface.background);
    painter.drawRoundedRect(card, surface.radius, surface.radius);

    const QFont items = itemFont(context);
    const QFont groups = groupFont(context);
    const QFont metas = metaFont(context);
    const QFontMetrics itemMetrics(items);
    const QFontMetrics metaMetrics(metas);

    for (int index = 0; index < m_items.size(); ++index) {
        const Entry& entry = m_items.at(index);
        const QRect box = itemRect(index);

        if (entry.kind == Kind::Separator) {
            painter.setPen(QPen(profile.colors.borderSubtle, 1));
            painter.drawLine(box.left() + kSeparatorInset, box.center().y(),
                             box.right() - kSeparatorInset, box.center().y());
            continue;
        }

        if (entry.kind == Kind::Group) {
            painter.setFont(groups);
            painter.setPen(profile.colors.textDisabled);
            painter.drawText(box.adjusted(kItemPaddingH, kGroupTop, -kItemPaddingH,
                                          -kGroupBottom),
                             Qt::AlignLeft | Qt::AlignVCenter, entry.label.toUpper());
            continue;
        }

        const ResolvedStyle style = resolve(itemStyleSpec(index), context, box.height());
        const bool hot = index == m_hovered || index == m_pressed;

        if (hot && entry.enabled) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(style.hoverBackground);
            painter.drawRoundedRect(QRectF(box), kItemRadius, kItemRadius);
        }

        QColor ink = entry.checked ? style.checkedForeground
                                   : (hot ? style.hoverForeground : style.foreground);
        if (!entry.enabled) {
            ink = withAlpha(ink, kDisabledOpacity);
        }

        int left = box.left() + kItemPaddingH;
        if (entry.glyph != Glyph::None) {
            paintGlyph(&painter, entry.glyph,
                       QRectF(left, box.center().y() - kItemGlyph / 2.0,
                              kItemGlyph, kItemGlyph),
                       ink);
            left += kItemGlyph + kItemGap;
        }

        int rightEdge = box.right() - kItemPaddingH;
        if(entry.submenu){
            paintGlyph(&painter,Glyph::Forward,QRectF(rightEdge-kItemGlyph,box.center().y()-kItemGlyph/2,kItemGlyph,kItemGlyph),ink);
            rightEdge-=kItemGlyph+kItemGap;
        }
        if (!entry.meta.isEmpty()) {
            const int metaWidth = metaMetrics.horizontalAdvance(entry.meta);
            painter.setFont(metas);
            painter.setPen(entry.enabled ? profile.colors.textDisabled
                                         : withAlpha(profile.colors.textDisabled,
                                                     kDisabledOpacity));
            painter.drawText(QRect(rightEdge - metaWidth, box.top(), metaWidth, box.height()),
                             Qt::AlignRight | Qt::AlignVCenter, entry.meta);
            rightEdge -= metaWidth + kItemGap;
        }

        painter.setFont(items);
        painter.setPen(ink);
        const QRect textBox(left, box.top(), qMax(0, rightEdge - left), box.height());
        painter.drawText(textBox, Qt::AlignLeft | Qt::AlignVCenter,
                         itemMetrics.elidedText(entry.label, Qt::ElideRight, textBox.width()));
    }
}

} // namespace novu::ui
