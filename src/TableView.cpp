#include "novu/ui/TableView.h"

#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QApplication>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMouseEvent>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QEvent>
#include <QResizeEvent>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

namespace novu::ui {

namespace {

// The role a painted cell rides on. A cell either carries one of these or it
// is an ordinary text cell; the delegate checks and falls through.
constexpr int kCellContentRole = Qt::UserRole + 977;

constexpr int kContentGap = 7;
constexpr int kContentDot = 6;

} // namespace


namespace {

// The narrowest a Flexible column is allowed to become. Below this a
// sentence is an ellipsis with a letter in front of it, which says less
// than leaving the table to scroll.
constexpr int kFlexibleFloor = 90;

// A Text column stretches when there is room, so it can also be asked to stop
// when there is not - but a name column narrower than this is a list of first
// syllables, so it gives back before it gives up.
//
// The sheet draws a two-line identity cell - a name over what it is - at about
// 145 and lets the second line wrap below that, which is what the cell itself
// asks for through its own minimum. 120 is the point past which even the name
// starts to go.
constexpr int kTextFloor = 120;

// `th{padding:10px 13px}` - the header's own vertical padding, a step over
// the cells' 9 because the caption is smaller than what it labels.
constexpr int kHeaderPaddingV = 10;

// How much a group of columns can hand back, shared out in proportion to what
// each of them measured, with the floor as the stop. Returns what was
// actually recovered, which is less than `owed` when everything is at its
// floor and the table simply does not fit.
//
// `group` is a set of roles, each with its own floor, and not a single role
// because every column that wraps compresses *together*. Called once per role,
// the first group is drained to its floor before the second is asked for
// anything - and a sentence five hundred pixels long then loses the width a
// sixteen-character timestamp beside it keeps, because the sentence happened
// to be asked first. A browser's auto table layout takes from all of them at
// once, in proportion to how much each is holding over its own minimum, which
// is what puts the sentence in the wide column and the timestamp on two short
// lines.
//
// The floors are what still tells the roles apart: a name column gives back
// before it gives up, a sentence column may go further. Content, Numeric and
// Action are not in any group - they hold prices, badges and buttons, where
// losing a character is a wrong reading rather than a tighter one.
int giveBack(QVector<int>& widths,
             const QList<QPair<TableView::ColumnRole, int>>& group,
             int owed,
             const QVector<TableView::ColumnRole>& roles,
             const QVector<int>& captionWidths)
{
    if (owed <= 0) {
        return 0;
    }
    // -1 for a role the caller did not name: not in this pass at all.
    const auto floorOf = [&group](TableView::ColumnRole role) {
        for (const auto& entry : group) {
            if (entry.first == role) {
                return entry.second;
            }
        }
        return -1;
    };
    const auto inGroup = [&](TableView::ColumnRole role) {
        return floorOf(role) >= 0;
    };
    // A column is never squeezed below its own caption. `ENTRY TIME (UTC)` is
    // the widest thing in its column whatever the rows say, and compressing
    // past it draws the head as `ENTRY TIME (UT(` - a column narrower than
    // what it is called is not a column.
    const auto floorFor = [&](int column) {
        const int floor = floorOf(roles.at(column));
        return qMax(floor, captionWidths.value(column, floor));
    };
    int spare = 0;
    for (int column = 0; column < widths.size(); ++column) {
        if (inGroup(roles.at(column))) {
            spare += qMax(0, widths[column] - floorFor(column));
        }
    }
    if (spare <= 0) {
        return 0;
    }
    const int take = qMin(owed, spare);
    int recovered = 0;
    int last = -1;
    for (int column = 0; column < widths.size(); ++column) {
        if (inGroup(roles.at(column)) && widths[column] > floorFor(column)) {
            last = column;
        }
    }
    for (int column = 0; column < widths.size(); ++column) {
        if (!inGroup(roles.at(column))) {
            continue;
        }
        const int room = qMax(0, widths[column] - floorFor(column));
        if (room <= 0) {
            continue;
        }
        // The last one absorbs the rounding, so the columns still add up to
        // the viewport rather than to a pixel less than it.
        const int give = (column == last)
            ? take - recovered
            : static_cast<int>(static_cast<qint64>(take) * room / spare);
        widths[column] -= give;
        recovered += give;
    }
    return recovered;
}

} // namespace

namespace {

// One delegate for the whole table. It draws the selection and the focus the
// style would have drawn, then the cell's own content on top.
class CellContentDelegate : public QStyledItemDelegate
{
public:
    explicit CellContentDelegate(TableView* table)
        : QStyledItemDelegate(table)
        , m_table(table)
    {
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override
    {
        const QVariant data = index.data(kCellContentRole);
        if (!data.isValid()) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        QStyleOptionViewItem shell(option);
        initStyleOption(&shell, index);
        // The row's own background and selection, drawn by the style so a
        // painted cell and a text cell sit on the same plate. The text is
        // cleared out of it because this delegate draws that part itself.
        shell.text.clear();
        shell.features &= ~QStyleOptionViewItem::HasDisplay;
        const QWidget* widget = shell.widget;
        QStyle* style = widget ? widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &shell, painter, widget);

        const ThemeProfile& profile = themeFor(m_table);
        const int pad = profile.roleSpacing.value(
            static_cast<int>(SpacingRole::TableCellPaddingH),
            profile.space(SpacingRole::TableCellPadding));
        const QColor ink = (option.state & QStyle::State_Selected)
                               ? profile.colors.textPrimary
                               : profile.colors.textSecondary;
        paintCellContent(painter, option.rect.adjusted(pad, 0, -pad, 0), m_table,
                         data.value<TableView::CellContent>(), ink);
    }

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override
    {
        const QVariant data = index.data(kCellContentRole);
        if (!data.isValid()) {
            return QStyledItemDelegate::sizeHint(option, index);
        }
        return cellContentSize(m_table, data.value<TableView::CellContent>());
    }

private:
    TableView* m_table = nullptr;
};

} // namespace

TableView::TableView(QWidget* parent)
    : QTableWidget(parent)
{
    // Every cell goes through it; cells with no content of their own fall
    // straight back to the base delegate, so nothing that already worked
    // changes shape.
    setItemDelegate(new CellContentDelegate(this));

    const ThemeProfile& profile = themeFor(this);
    const ThemeColors& t = profile.colors;
    const int rowHeight = profile.table.rowHeight;

    // `class="selectable" data-open-*` on the row. Qt already tells us which
    // cell was clicked, and a control inside the row swallows the press before
    // it gets here, so this is the reference's rule with nothing added.
    connect(this, &QTableWidget::cellClicked, this,
            [this](int row, int) { emit rowActivated(row); });

    // `table.tbl tbody tr:hover{background:var(--panel)}` - mouseMoveEvent()
    // needs move events even with no button held, which Qt only delivers with
    // this on.
    setMouseTracking(true);
    viewport()->setMouseTracking(true);
    // The surrounding Card owns the surface and its four rounded corners.
    // An opaque viewport reaching a flush card edge would repaint that corner
    // as a square and cover the card's outline.
    setAttribute(Qt::WA_TranslucentBackground, true);
    viewport()->setAttribute(Qt::WA_TranslucentBackground, true);
    viewport()->setAutoFillBackground(false);

    verticalHeader()->setVisible(false);
    verticalHeader()->setDefaultSectionSize(rowHeight);
    verticalHeader()->setMinimumSectionSize(rowHeight);
    // Fixed, not merely defaulted: setStyleSheet() below hands the item
    // delegate a `padding` the platform style then feeds into its own
    // sizeFromContents(CT_ItemViewItem, ...), which silently recomputed
    // every row down to 24px regardless of what was just set here - the same
    // "the stylesheet decided the height instead of the token" class of bug
    // the header height fix above this already had to name explicitly.
    verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    // `th{font-family:'JetBrains Mono';font-size:10px;letter-spacing:.1em;
    // text-transform:uppercase}` - the caption is set in the mono face at the
    // smallest step and tracked out, which is what makes a column head read
    // as a label rather than as a short cell. setColumns() does the casing;
    // the face, the size and the tracking are here. Without this the header
    // inherited the table's own body font and every column head was drawn
    // three sizes too large in the wrong family.
    QFont headerFont = roleMonoFont(this, TypeRole::Eyebrow);
    headerFont.setPixelSize(qMax(8, profile.type.xxs));
    headerFont.setWeight(QFont::Medium);
    headerFont.setLetterSpacing(QFont::PercentageSpacing, 110);
    horizontalHeader()->setFont(headerFont);

    // Fixed, not a floor: the token exists so a column head is the same
    // height in every table and does not drift with the caption's font.
    horizontalHeader()->setFixedHeight(profile.table.headerHeight);
    horizontalHeader()->setHighlightSections(false);
    horizontalHeader()->setStretchLastSection(false);
    // Every width is assigned by relayoutColumns(); the automatic modes would
    // ignore those assignments.
    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    // A `td` states no `white-space`, so a cell that does not fit its column
    // runs onto a second line. Only `.num` and `.act` are nowrap, and those
    // are Content columns, which are measured to fit and never compressed -
    // so nothing in them ever reaches a second line anyway.
    setWordWrap(true);
    // Nothing elides. Text and Content are measured to fit; a compressed
    // Flexible column wraps, which is what the sheet does with the one
    // sentence it lets give way.
    setTextElideMode(Qt::ElideNone);
    // Neither bar exists in the design. A table in a card is markup with a
    // height: it is as tall as its rows and the *page* scrolls. The one
    // exception is `.tbl-wrap.cap`, which setHeightCap() turns back on.
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // Preferred, not the QAbstractScrollArea default of Expanding: a table
    // that claims every spare pixel turns a four-row card into a panel the
    // height of the window, with the rows stranded at the top of it.
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    // Alternating row colours are off by design: the alternate colour is drawn
    // by the item delegate and does not extend under cell widgets, so badge
    // and button cells showed the base colour as dark blocks.
    //
    // Qt's own grid draws every row's bottom edge AND every column's side
    // edge with the same pen; the design draws only a row's own bottom
    // border (`table.tbl td{border-bottom:1px solid var(--hair)}`, no
    // border-right anywhere). showGrid()/gridline-color cannot be told to
    // draw one axis and not the other, so it stays off and paintEvent()
    // draws the row lines itself - still ahead of the cell widgets'
    // compositing, for the same "stays continuous under a badge or a
    // button" reason the native grid was chosen for originally.
    setAlternatingRowColors(false);
    setShowGrid(false);

    // Only the header *sections* carry the header colour. Styling QHeaderView
    // itself would paint the empty area past the last column as a grey block.
    applySelectionStyle();
}

bool TableView::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type()
        || event->type() == QEvent::PolishRequest
        || event->type() == QEvent::ParentChange) {
        applyMetrics();
    }
    // An accented row says "this is the one open beside you" - Mail's list
    // beside its reading pane, a master list beside a detail panel. That stops
    // being true the moment focus leaves for a FloatingPanel opened over it
    // (Compose, an item dialog): the list is not what keyboard input reaches
    // any more, and painting its row in the same vivid accent it uses while
    // focused read as if the panel behind the floating window still had it.
    // `selection-background-color` has no `:focus` pseudo-state of its own to
    // lean on - only re-applying the sheet with the other colour does it.
    if (m_selectionAccented
        && (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut)) {
        applySelectionStyle();
        update();
    }
    return QTableWidget::event(event);
}

// The header's own font, and the row height the profile states. Re-applied on
// every theme change rather than only at construction: a table is built
// before it is parented, so the scope that decides its profile is installed
// *after* it has already measured its columns - and a header font that grows
// on polish while the widths stay is a column head drawn with its first
// letter cut off.
void TableView::applyMetrics()
{
    const ThemeProfile& profile = themeFor(this);

    QFont headerFont = roleMonoFont(this, TypeRole::Eyebrow);
    headerFont.setPixelSize(qMax(8, profile.type.xxs));
    headerFont.setWeight(QFont::Medium);
    headerFont.setLetterSpacing(QFont::PercentageSpacing, 110);
    horizontalHeader()->setFont(headerFont);
    horizontalHeader()->setFixedHeight(profile.table.headerHeight);

    verticalHeader()->setMinimumSectionSize(profile.table.rowHeight);
    applySelectionStyle();
    relayoutColumns();
}

void TableView::applySelectionStyle()
{
    const ThemeProfile& profile = themeFor(this);
    const ThemeColors& t = profile.colors;
    // A translucent background belongs in QSS as `rgba(r, g, b, a)` - the
    // syntax every other stylesheet in this library already uses - not as
    // an 8-digit `#AARRGGBB` hex string, which Qt's QSS colour grammar does
    // not accept at all. `QColor::name(HexArgb)` produced exactly that
    // string, so the rule silently failed to parse and the row fell back to
    // the platform's own flat highlight - accented and unaccented alike
    // looked the same, because neither one was the colour this function
    // computed. The regression test only checked the separately painted
    // leading-edge accent bar, so the fallback went uncaught.
    //
    // Opaque now, not `rgba(...)`: this same colour is set on two different
    // properties below (`selection-background-color` for a setWidget() cell,
    // `::item:selected` for a plain one), and a selected row with any
    // item-based column repaints both -- two 13% washes of the same
    // translucent colour compositing to nearer 24%, not 13%. Blending once
    // here, against the table's own ground, keeps the result the single flat
    // colour the reference's own screenshot shows regardless of how many
    // layers Qt happens to paint it through.
    const auto blend = [](const QColor& fg, const QColor& bg, double alpha) {
        return QColor(qRound(fg.red() * alpha + bg.red() * (1.0 - alpha)),
                      qRound(fg.green() * alpha + bg.green() * (1.0 - alpha)),
                      qRound(fg.blue() * alpha + bg.blue() * (1.0 - alpha)));
    };
    // The vivid wash is what says "this row is open" while the list is what
    // keyboard input reaches; once focus moves elsewhere in the window (a
    // FloatingPanel opened over the list, most often) the row falls back to
    // the same neutral wash an unaccented table's own selection uses, rather
    // than keep claiming a focus it no longer has.
    const QString selectionBg = m_selectionAccented && hasFocus()
        ? blend(t.accent, t.panelBg, 0.13).name()
        : t.surfaceActive.name();
    setStyleSheet(QStringLiteral(
        // No border of its own. `table.tbl` states a font, a width and
        // `border-collapse`; the only borders in the design are `th` and
        // `td`'s own bottom hairlines. The card the table sits in owns the
        // outline, and drawing a second one inside it put a rounded box
        // fifteen pixels in from the card's edge in every table card in the
        // product.
        //
        // `selection-background-color`/`-color`, not only `::item:selected`:
        // the item pseudo-state only ever repaints an actual QTableWidgetItem,
        // and every row built from setCellWidget() (Documents, every other
        // table that composes a cell out of a badge or an icon plus a label
        // rather than plain text) has none. Without the palette property the
        // row region behind those widgets kept Qt's own default highlight -
        // a flat system blue identical whether setSelectionAccented() was on
        // or off, which is what made the accent wash look like it was never
        // applied even though this function was computing it correctly.
        "QTableWidget { background: transparent; color: %1; border: none;"
        " outline: none; selection-background-color: %3; selection-color: %1; }"
        "QTableWidget::item { padding: %6px %7px; border: none; }"
        "QTableWidget::item:selected { background: %3; color: %1; }"
        "QHeaderView { background: transparent; border: none; }"
        // `th{padding:10px 13px;font-weight:500;color:var(--text3)}` - the
        // same padding the cells under it use, the body weight plus one, and
        // the muted ink. It was 4px/8px, 600 and the secondary ink: a header
        // heavier and tighter than the sheet draws, over cells that were not.
        "QHeaderView::section { background: %4; color: %5; border: none;"
        " border-bottom: 1px solid %2; padding: %8px %7px; font-weight: 500; }")
        .arg(t.textPrimary.name(), t.border.name(), selectionBg,
             profile.table.headerBg.name(), t.textDisabled.name())
        .arg(profile.space(SpacingRole::TableCellPadding))
        .arg(profile.roleSpacing.value(static_cast<int>(SpacingRole::TableCellPaddingH),
                                       profile.space(SpacingRole::TableCellPadding)))
        .arg(kHeaderPaddingV));
}

void TableView::setSelectionAccented(bool accented)
{
    if (m_selectionAccented == accented) {
        return;
    }
    m_selectionAccented = accented;
    applySelectionStyle();
    update();
}

void TableView::setStretchToFill(bool stretch)
{
    if (m_stretchToFill == stretch) {
        return;
    }
    m_stretchToFill = stretch;
    relayoutColumns();
}

void TableView::setColumns(const QVector<QPair<QString, ColumnRole>>& columns)
{
    setColumnCount(columns.size());
    m_roles.clear();
    m_roles.reserve(columns.size());

    QStringList captions;
    captions.reserve(columns.size());
    for (const auto& column : columns) {
        captions << column.first.toUpper();
        m_roles.append(column.second);
    }
    setHorizontalHeaderLabels(captions);

    // Content columns hold values - prices, counts, timestamps - and read
    // right-aligned against each other; text columns stay left.
    for (int i = 0; i < m_roles.size(); ++i) {
        if (auto* header = horizontalHeaderItem(i)) {
            header->setTextAlignment(alignmentFor(i) | Qt::AlignVCenter);
        }
    }
}

QSize cellContentSize(const QWidget* context, const TableView::CellContent& content)
{
    if (!context) {
        return QSize();
    }
    const QFontMetrics metrics(roleFont(context, TypeRole::Body));
    int width = 0;
    int height = metrics.height();

    if (content.hasStatus) {
        width += kContentDot + kContentGap;
    }
    if (!content.text.isEmpty()) {
        width += metrics.horizontalAdvance(content.text);
    }
    for (const TableView::CellBadge& badge : content.badges) {
        const QSize size = badgeSizeFor(context, badge.text, Badge::Size::Regular);
        width += kContentGap + size.width();
        height = qMax(height, size.height());
    }
    if (!content.trailing.isEmpty()) {
        width += kContentGap + metrics.horizontalAdvance(content.trailing);
    }
    return QSize(width, height);
}

void paintCellContent(QPainter* painter, const QRect& box, const QWidget* context,
                      const TableView::CellContent& content, const QColor& textInk)
{
    if (!painter || !context) {
        return;
    }
    const ThemeProfile& profile = themeFor(context);
    int x = box.left();

    if (content.hasStatus) {
        const QColor dot = statusDotToken(content.status).resolve(profile);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(Qt::NoPen);
        painter->setBrush(dot);
        painter->drawEllipse(QPointF(x + kContentDot / 2.0, box.center().y()),
                             kContentDot / 2.0, kContentDot / 2.0);
        painter->restore();
        x += kContentDot + kContentGap;
    }

    // The badges and the trailing note are measured first and reserved at the
    // right of what is left, so the name is the only thing that ever elides.
    int reserved = 0;
    QVector<QSize> badgeSizes;
    badgeSizes.reserve(content.badges.size());
    for (const TableView::CellBadge& badge : content.badges) {
        const QSize size = badgeSizeFor(context, badge.text, Badge::Size::Regular);
        badgeSizes.append(size);
        reserved += kContentGap + size.width();
    }
    const QFontMetrics trailingMetrics(roleMonoFont(context, TypeRole::Mono));
    if (!content.trailing.isEmpty()) {
        reserved += kContentGap + trailingMetrics.horizontalAdvance(content.trailing);
    }

    if (!content.text.isEmpty()) {
        QFont font = roleFont(context, TypeRole::Body);
        if (content.strongText) {
            font.setWeight(QFont::Medium);
        }
        const QFontMetrics metrics(font);
        const int room = qMax(0, box.right() - x - reserved);
        painter->save();
        painter->setFont(font);
        painter->setPen(textInk);
        painter->drawText(QRect(x, box.top(), room, box.height()),
                          Qt::AlignVCenter | Qt::AlignLeft,
                          metrics.elidedText(content.text, Qt::ElideRight, room));
        painter->restore();
        x += qMin(room, metrics.horizontalAdvance(content.text));
    }

    for (int i = 0; i < content.badges.size(); ++i) {
        const QSize size = badgeSizes.at(i);
        x += kContentGap;
        if (x + size.width() > box.right()) {
            break;
        }
        paintBadge(painter, QRectF(x, box.center().y() - size.height() / 2.0,
                                   size.width(), size.height()),
                   context, content.badges.at(i).text, content.badges.at(i).tone,
                   Badge::Size::Regular);
        x += size.width();
    }

    if (!content.trailing.isEmpty()) {
        x += kContentGap;
        painter->save();
        painter->setFont(roleMonoFont(context, TypeRole::Mono));
        painter->setPen(profile.colors.textDisabled);
        painter->drawText(QRect(x, box.top(), qMax(0, box.right() - x), box.height()),
                          Qt::AlignVCenter | Qt::AlignLeft, content.trailing);
        painter->restore();
    }
}

void TableView::setContent(int row, int column, const CellContent& content)
{
    QTableWidgetItem* cell = item(row, column);
    if (!cell) {
        cell = new QTableWidgetItem;
        setItem(row, column, cell);
    }
    cell->setData(kCellContentRole, QVariant::fromValue(content));
    // The text is still the item's, so sorting, keyboard search and anything
    // that reads a table by its model keep working on a painted row.
    cell->setText(content.text);
    if (!content.text.isEmpty()) {
        cell->setToolTip(content.text);
    }
}

void TableView::setWidget(int row, int column, QWidget* widget)
{
    // The item carries the widget's size hint. QHeaderView and our own
    // measurement both look at items; without this a column holding only
    // widgets measures as empty and collapses.
    auto* item = new QTableWidgetItem();
    item->setSizeHint(widget->sizeHint());
    setItem(row, column, item);
    setCellWidget(row, column, widget);
}

void TableView::setWidgets(int row, int column, const QVector<QWidget*>& widgets)
{
    auto* container = new QWidget(this);
    // Transparent: a plain container would paint over the row background.
    container->setObjectName(QStringLiteral("NovuTableCell"));
    container->setStyleSheet(
        QStringLiteral("QWidget#NovuTableCell { background: transparent; border: none; }"));

    // No margins of its own: Qt already insets a cell *widget* by the
    // `QTableWidget::item` padding the constructor sets from the profile
    // (`td{padding:9px 13px}`), so a container that padded itself as well
    // left an inch of room in the middle of the row - a 47px row with 11px
    // of usable height in it.
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(metrics::spacingSmall);
    layout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    for (QWidget* widget : widgets) {
        layout->addWidget(widget);
    }
    layout->activate();

    setWidget(row, column, container);
}

void TableView::setIdentity(int row, int column, const QString& title,
                            const QString& subtitle, QWidget* leading)
{
    auto* container = new QWidget(this);
    container->setObjectName(QStringLiteral("NovuTableIdentity"));
    container->setStyleSheet(QStringLiteral(
        "QWidget#NovuTableIdentity { background: transparent; border: none; }"));

    auto* line = new QHBoxLayout(container);
    line->setContentsMargins(0, 0, 0, 0);
    line->setSpacing(10);

    if (leading) {
        leading->setParent(container);
        line->addWidget(leading, 0, Qt::AlignVCenter);
    }

    auto* names = new QWidget(container);
    auto* stack = new QVBoxLayout(names);
    stack->setContentsMargins(0, 0, 0, 0);
    stack->setSpacing(2);

    auto* primary = new ElidedLabel(title, names);
    QFont primaryFont = roleFont(this, TypeRole::Body);
    primaryFont.setWeight(QFont::Medium);
    primary->setFont(primaryFont);
    setInk(primary, Ink::Default);
    primary->setToolTip(title);
    stack->addWidget(primary);

    if (!subtitle.isEmpty()) {
        auto* secondary = new ElidedLabel(subtitle, names);
        QFont secondaryFont = roleMonoFont(this, TypeRole::Mono);
        secondaryFont.setPixelSize(qMax(8, themeFor(this).type.xxs));
        secondary->setFont(secondaryFont);
        setInk(secondary, Ink::Muted);
        secondary->setToolTip(subtitle);
        stack->addWidget(secondary);
    }

    line->addWidget(names, 1, Qt::AlignVCenter);
    line->activate();
    setWidget(row, column, container);
}

void TableView::setText(int row, int column, const QString& text, const QString& tooltip)
{
    auto* item = new QTableWidgetItem(text.isEmpty() ? QStringLiteral("—") : text);
    const QString tip = tooltip.isEmpty() ? text : tooltip;
    if (!tip.isEmpty()) {
        item->setToolTip(tip);
    }
    item->setTextAlignment(alignmentFor(column) | Qt::AlignVCenter);
    // The column states the face and the ink, so a page writes the value and
    // nothing else. A page that wants something different still can: the
    // roles below are only defaults on the item.
    if (column >= 0 && column < m_roles.size()) {
        const ColumnRole role = m_roles.at(column);
        if (role == ColumnRole::Numeric || role == ColumnRole::Mono) {
            item->setData(Qt::FontRole, columnFont(column));
        }
        item->setForeground(columnInk(column));
    }
    setItem(row, column, item);
}

// A column's alignment follows from its role, so a page never states it and
// two tables cannot disagree about how a price is aligned.
Qt::Alignment TableView::alignmentFor(int column) const
{
    if (column < 0 || column >= m_roles.size()) {
        return Qt::AlignLeft;
    }
    switch (m_roles.at(column)) {
    // `td.num{text-align:right}` and `td.act{text-align:right}` — and those
    // two only. Content used to right-align as well, on the reading that it
    // holds "values"; the sheet says otherwise, and a badge column pushed to
    // the right edge of its own cell put every column head one letter off the
    // left of its box, which is how `COLLECTION` came out drawn as
    // `OLLECTION`. Numeric now carries the meaning Content was overloaded
    // with.
    case ColumnRole::Numeric:
    case ColumnRole::Action:
        return Qt::AlignRight;
    case ColumnRole::Content:
    case ColumnRole::Text:
    case ColumnRole::Flexible:
    case ColumnRole::Mono:
        break;
    }
    return Qt::AlignLeft;
}

// `td.mono{font-family:'JetBrains Mono';font-size:11.5px}` and `td.num
// {font-family:'JetBrains Mono';font-variant-numeric:tabular-nums}` — the two
// places the sheet changes a cell's face, said once here rather than by every
// page putting a styled QLabel in the cell.
QFont TableView::columnFont(int column) const
{
    if (column < 0 || column >= m_roles.size()) {
        return font();
    }
    switch (m_roles.at(column)) {
    case ColumnRole::Numeric: {
        QFont f = roleMonoFont(this, TypeRole::Mono);
        f.setPixelSize(font().pixelSize() > 0 ? font().pixelSize()
                                              : themeFor(this).type.md);
        return f;
    }
    case ColumnRole::Mono: {
        QFont f = roleMonoFont(this, TypeRole::Mono);
        f.setPixelSize(qMax(8, themeFor(this).type.xs));
        return f;
    }
    default:
        break;
    }
    return font();
}

// `td{color:var(--text2)}` with `td.mono{color:var(--text3)}`.
QColor TableView::columnInk(int column) const
{
    const ThemeProfile& profile = themeFor(this);
    if (column >= 0 && column < m_roles.size()
        && m_roles.at(column) == ColumnRole::Mono) {
        return profile.colors.textDisabled;
    }
    return profile.colors.textSecondary;
}

// What a column's own head needs, padding included. Split out of
// measureColumn() because it is also the floor a column may never be
// compressed past.
int TableView::measureHeader(int column) const
{
    const QFontMetrics headerMetrics(horizontalHeader()->font());
    const QTableWidgetItem* header = horizontalHeaderItem(column);
    // The caption is tracked out by 10%, so the ink bounds and the advance
    // disagree; the wider of the two is what is actually drawn.
    //
    // And then the header's own margin on top. QHeaderView deflates the label
    // rect by PM_HeaderMargin on each side *in addition* to the padding the
    // stylesheet states, so a caption measured to the pixel is drawn four
    // pixels short at each end - which is how `WIN RATE` came out as
    // `IN RATE` and `OPENED` as `PENED`, in every numeric column in the
    // product. Read from the style rather than guessed, because it is scaled
    // by the display.
    int width = 0;
    if (header) {
        const QFont headerFont = horizontalHeader()->font();
        width = qMax(headerMetrics.horizontalAdvance(header->text()),
                     headerMetrics.boundingRect(header->text()).width());
        // The tracking, added by hand. `th{letter-spacing:.1em}` puts a tenth
        // of the size after every glyph, and QFontMetrics does not report it
        // - so a ten-letter caption came out six pixels short and `UNREALISED`
        // was drawn as `NREALISED`, in every wide column head in the product.
        if (headerFont.letterSpacingType() == QFont::PercentageSpacing) {
            const double extra = (headerFont.letterSpacing() - 100.0) / 100.0
                * headerFont.pixelSize();
            width += qRound(extra * header->text().length());
        }
        width += 2 * style()->pixelMetric(QStyle::PM_HeaderMargin, nullptr,
                                          horizontalHeader());
        // Plus the glyph's own bearing at each end. A capital `W` in the mono
        // face hangs a pixel outside its advance, and a caption measured to
        // the advance alone loses that pixel to the clip.
        width += 2;
    }
    const ThemeProfile& headerProfile = themeFor(this);
    const int headerPadV = headerProfile.space(SpacingRole::TableCellPadding);
    return width + 2 * headerProfile.roleSpacing.value(
        static_cast<int>(SpacingRole::TableCellPaddingH), headerPadV);
}

// The narrowest this column may ever be drawn: its own caption, and the widest
// minimum any widget in it states.
//
// A cell widget is not text the view can simply wrap - it is a laid-out thing
// with a minimum of its own, and a column compressed past that does not make
// the cell narrower, it clips it. The overview's identity cell is the case: a
// 29px plaque, a 10px gap and a name that says it needs 120 to stay a name.
// Told only about the caption, the column went to `WHAT`'s own 66 and drew
// `Breakout H` with the rest of the name cut off - the one thing wrapping was
// supposed to prevent.
int TableView::measureColumnFloor(int column) const
{
    int minimum = 0;
    for (int row = 0; row < rowCount(); ++row) {
        if (const QWidget* widget = cellWidget(row, column)) {
            minimum = qMax(minimum, widget->minimumSizeHint().width());
            minimum = qMax(minimum, widget->minimumWidth());
        }
    }
    if (minimum > 0) {
        const ThemeProfile& profile = themeFor(this);
        const int padV = profile.space(SpacingRole::TableCellPadding);
        minimum += 2 * profile.roleSpacing.value(
            static_cast<int>(SpacingRole::TableCellPaddingH), padV);
    }
    return qMax(measureHeader(column), minimum);
}

int TableView::measureColumn(int column) const
{
    // Step 1 + 2: the widest of the header caption and every cell, each with
    // its own padding. The two are measured apart because the header's
    // measurement is also the floor the column may never be compressed past.
    int cells = 0;

    for (int row = 0; row < rowCount(); ++row) {
        if (QWidget* widget = cellWidget(row, column)) {
            cells = qMax(cells, widget->sizeHint().width());
            continue;
        }
        const QTableWidgetItem* item = this->item(row, column);
        if (!item) {
            continue;
        }
        const QVariant content = item->data(kCellContentRole);
        if (content.isValid()) {
            // A painted cell measures itself, by the same function that draws
            // it - so a column can never come out narrower than its content.
            cells = qMax(cells, cellContentSize(this, content.value<CellContent>()).width());
            continue;
        }
        const QVariant fontData = item->data(Qt::FontRole);
        const QFontMetrics metrics(fontData.isValid() ? fontData.value<QFont>() : font());
        cells = qMax(cells, metrics.horizontalAdvance(item->text()));
    }
    // The cell's own padding, from the profile - not a constant of its own.
    // They were two different numbers, and the column came out narrower than
    // the cell it had just measured: a Type badge sized to its own text was
    // laid out in a box ten pixels tighter and painted its caption clipped.
    const ThemeProfile& profile = themeFor(this);
    const int padV = profile.space(SpacingRole::TableCellPadding);
    const int padH = profile.roleSpacing.value(
        static_cast<int>(SpacingRole::TableCellPaddingH), padV);
    // The header value already carries its padding, so only the cells' own
    // measurement gains it here.
    return qMax(measureHeader(column), cells + 2 * padH);
}

void TableView::relayoutColumns()
{
    const int columns = columnCount();
    if (columns <= 0 || m_roles.size() != columns || m_inRelayout) {
        return;
    }
    // Assigning widths can toggle the scrollbar, which resizes the viewport
    // and would re-enter through resizeEvent().
    m_inRelayout = true;

    QVector<int> widths(columns);
    // What each column asked for before anything was taken off it. The spare
    // left over after compression is shared out in these proportions.
    QVector<int> measured(columns);
    // The hard floor per column: its own head, and the widest minimum stated
    // by anything laid out inside it.
    QVector<int> captionWidths(columns);
    for (int column = 0; column < columns; ++column) {
        captionWidths[column] = measureColumnFloor(column);
    }
    int contentTotal = 0;
    int textMeasured = 0;
    int lastTextColumn = -1;

    int flexibleMeasured = 0;
    for (int column = 0; column < columns; ++column) {
        widths[column] = measureColumn(column);
        measured[column] = widths[column];
        switch (m_roles[column]) {
        case ColumnRole::Text:
            textMeasured += widths[column];
            lastTextColumn = column;
            break;
        // Mono compresses like Flexible: the sheet states no `white-space`
        // on it, so a timestamp column runs onto a second line rather than
        // pushing the price columns off the card.
        case ColumnRole::Flexible:
        case ColumnRole::Mono:
            flexibleMeasured += widths[column];
            break;
        case ColumnRole::Content:
        case ColumnRole::Numeric:
        case ColumnRole::Action:
            contentTotal += widths[column];
            break;
        }
    }

    // Step 4: leftover width goes to the columns that stretch - Text, and
    // Flexible, which is Text that is also allowed to give the width back.
    const int stretchMeasured = textMeasured + flexibleMeasured;
    int lastStretchColumn = lastTextColumn;
    for (int column = 0; column < columns; ++column) {
        if (m_roles[column] == ColumnRole::Flexible
            || m_roles[column] == ColumnRole::Mono) {
            lastStretchColumn = column;
        }
    }
    const int remaining = viewport()->width() - contentTotal - stretchMeasured;
    if (m_stretchToFill && remaining > 0 && stretchMeasured > 0) {
        int handedOut = 0;
        for (int column = 0; column < columns; ++column) {
            if (m_roles[column] == ColumnRole::Content
                || m_roles[column] == ColumnRole::Numeric
                || m_roles[column] == ColumnRole::Action) {
                continue;
            }
            const int share = (column == lastStretchColumn)
                ? remaining - handedOut
                : static_cast<int>(static_cast<qint64>(remaining) * widths[column]
                                   / stretchMeasured);
            handedOut += share;
            widths[column] += share;
        }
    } else if (remaining < 0) {
        // Step 4b: the table is wider than the card it is in, and something
        // has to give the difference back.
        //
        // In proportion to what each column asked for, not one column at a
        // time. Draining them in order meant the first Flexible column fell
        // to the floor while the second kept everything it had measured -
        // so a two-sentence table came out with one column five lines deep
        // beside one on a single line, which is not what an auto table
        // layout does and not what the sheet shows.
        // Every column that wraps, in one pass. The three roles differ in how
        // far each may be taken - a name gives back before it gives up, a
        // sentence may go further - and not in whether it is asked at all.
        //
        // Asked in turn instead, the sentence column was drained to ninety
        // pixels while the name column beside it kept the whole of a width it
        // was using to fit `Breakout Hunter` on one line - which the sheet
        // itself wraps onto two. This is also exactly how the spare width is
        // shared when the table is *wider* than its content: the same three
        // roles, in the same proportions.
        int owed = -remaining;
        owed -= giveBack(widths,
                         {{ColumnRole::Flexible, kFlexibleFloor},
                          {ColumnRole::Mono, kFlexibleFloor},
                          {ColumnRole::Text, kTextFloor}},
                         owed, m_roles, captionWidths);
        // Still over. Better a bar the person can move than a column head
        // silently cut in half, which is what hiding the overflow did to
        // `OPENED` in the overview's own table.
        setHorizontalScrollBarPolicy(owed > 0 ? Qt::ScrollBarAsNeeded
                                              : Qt::ScrollBarAlwaysOff);
    } else {
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    // Nothing is left on the floor. Compression works to a floor per column
    // and rounds down, so the assigned widths can add up to less than the
    // viewport - which draws a strip of bare plate past the last column while
    // a sentence two columns to the left is wrapping for want of it.
    int assigned = 0;
    for (int width : widths) {
        assigned += width;
    }
    int spare = viewport()->width() - assigned;
    if (m_stretchToFill && spare > 0) {
        // Shared out among the columns that stretch, in proportion to what
        // they asked for - not handed to whichever happened to be last. Given
        // to the last one, a timestamp column ended up with sixty pixels it
        // had no use for while the sentence beside it wrapped for want of
        // them.
        int wanted = 0;
        for (int column = 0; column < columns; ++column) {
            if (m_roles[column] != ColumnRole::Content
                && m_roles[column] != ColumnRole::Numeric
                && m_roles[column] != ColumnRole::Action) {
                wanted += measured[column];
            }
        }
        if (wanted > 0) {
            int handedOut = 0;
            int last = -1;
            for (int column = 0; column < columns; ++column) {
                if (m_roles[column] != ColumnRole::Content
                    && m_roles[column] != ColumnRole::Numeric
                    && m_roles[column] != ColumnRole::Action) {
                    last = column;
                }
            }
            for (int column = 0; column < columns; ++column) {
                if (m_roles[column] == ColumnRole::Content
                    || m_roles[column] == ColumnRole::Numeric
                    || m_roles[column] == ColumnRole::Action) {
                    continue;
                }
                const int share = (column == last)
                    ? spare - handedOut
                    : static_cast<int>(static_cast<qint64>(spare) * measured[column]
                                       / wanted);
                handedOut += share;
                widths[column] += share;
            }
        }
    }

    for (int column = 0; column < columns; ++column) {
        setColumnWidth(column, widths[column]);
    }

    fitRows();

    m_inRelayout = false;
    updateGeometry();
}

// The height of a table is the height of what is in it.
//
// QAbstractScrollArea's own hint is a fixed 256x192 viewport plus the frame,
// which is why every table in the product had to be given a minimum height by
// hand from the page that built it - a number computed from a row height the
// page could not see, in six different places, each of them wrong as soon as
// a cell grew. The rows are right here.
QSize TableView::sizeHint() const
{
    // isHidden(), not !isVisible(): a widget that has simply not been shown
    // yet is not visible, and measuring the table before its window is on
    // screen would leave the header out of every height it reports.
    int height = horizontalHeader()->isHidden() ? 0 : horizontalHeader()->height();
    for (int row = 0; row < rowCount(); ++row) {
        if (!isRowHidden(row)) {
            height += rowHeight(row);
        }
    }
    // The frame is `border:none`, so this is normally zero - it is read
    // rather than assumed so a profile that gives the table a frame back
    // still adds up.
    height += 2 * frameWidth();
    if (m_heightCap > 0) {
        height = qMin(height, m_heightCap);
    }
    // Width stays the base class's claim: a table is as wide as the card it
    // is in, and the columns divide that width between them.
    return QSize(QTableWidget::sizeHint().width(), qMax(height, 1));
}

QSize TableView::minimumSizeHint() const
{
    // Not the rows: a table with forty of them would make its card
    // unshrinkable. The header plus one row is the least that is still a
    // table rather than a line.
    const int header = horizontalHeader()->isHidden() ? 0 : horizontalHeader()->height();
    const int first = rowCount() > 0 ? rowHeight(0) : 0;
    return QSize(QTableWidget::minimumSizeHint().width(),
                 qMax(header + first + 2 * frameWidth(), 1));
}

void TableView::setHeightCap(int pixels)
{
    if (m_heightCap == pixels) {
        return;
    }
    m_heightCap = pixels;
    setVerticalScrollBarPolicy(pixels > 0 ? Qt::ScrollBarAsNeeded
                                          : Qt::ScrollBarAlwaysOff);
    updateGeometry();
}


// A row is exactly as tall as the tallest thing in it.
//
// Exactly, not at least. The two things that decide it - the tallest cell
// widget and the wrapped height of a Flexible or Mono cell - both depend on
// the width the column ended up with, and a table is laid out more than once:
// first at whatever size it has before it is put in a card, then again at the
// real one. Raising the height on each pass and never lowering it meant the
// rows kept the height they needed while the table was three hundred pixels
// narrower than it would end up, which is how a two-line sentence came out as
// four lines in a card wide enough for one.
void TableView::fitRows()
{
    const int rows = rowCount();
    if (rows <= 0) {
        return;
    }
    const ThemeProfile& profile = themeFor(this);
    const int padV = profile.space(SpacingRole::TableCellPadding);
    const int padH = profile.roleSpacing.value(
        static_cast<int>(SpacingRole::TableCellPaddingH), padV);
    const int floor = profile.table.rowHeight;

    // One height for every row: the design draws a table, not a list of boxes,
    // and rows of different heights read as a broken grid. The tallest row is
    // what the rest follow.
    int tallest = 0;
    for (int row = 0; row < rows; ++row) {
        int content = 0;
        for (int column = 0; column < columnCount(); ++column) {
            if (QWidget* widget = cellWidget(row, column)) {
                // A cell holding wrapped text is as tall as the width its
                // column ended up with, and a widget says so through
                // heightForWidth. Taking sizeHint().height() instead measured
                // it at one line and cut the second one off - which is what a
                // two-line subject looked like after `.cellmain` learned to
                // wrap.
                const int available = columnWidth(column) - 2 * padH;
                content = qMax(content,
                               widget->hasHeightForWidth() && available > 0
                                   ? widget->heightForWidth(available)
                                   : widget->sizeHint().height());
                continue;
            }
            if (column >= m_roles.size()
                || (m_roles.at(column) != ColumnRole::Flexible
                    && m_roles.at(column) != ColumnRole::Mono)) {
                continue;
            }
            const QTableWidgetItem* item = this->item(row, column);
            if (!item || item->text().isEmpty()) {
                continue;
            }
            const QVariant fontData = item->data(Qt::FontRole);
            const QFontMetrics metrics(fontData.isValid() ? fontData.value<QFont>() : font());
            const int available = columnWidth(column) - 2 * padH;
            if (available <= 0) {
                continue;
            }
            const QRect bounds = metrics.boundingRect(
                QRect(0, 0, available, 0), Qt::TextWordWrap | Qt::AlignLeft, item->text());
            content = qMax(content, bounds.height());
        }
        tallest = qMax(tallest, content);
    }

    const int height = qMax(floor, tallest + 2 * padV);
    verticalHeader()->setDefaultSectionSize(height);
    for (int row = 0; row < rows; ++row) {
        setRowHeight(row, height);
    }
    updateGeometry();
}


void TableView::resizeEvent(QResizeEvent* event)
{
    QTableWidget::resizeEvent(event);
    relayoutColumns();
}

void TableView::paintEvent(QPaintEvent* event)
{
    const int rows = rowCount();
    if (rows > 0) {
        // `table.tbl tbody tr:hover{background:var(--panel)}`,
        // `tr.sel{background:rgba(var(--rl),.13)}`,
        // `tr.sel:hover{background:rgba(var(--rl),.17)}` - the three washes,
        // painted before QTableWidget's own paintEvent() so a plain-text cell
        // (setText(), no cell widget) draws its item on top of the fill
        // instead of the fill covering it. A cell *widget* is a separate
        // child of the viewport either way and always composites after this
        // whole function returns, so the ordering only matters for text.
        const ThemeProfile& profile = themeFor(this);
        QPainter under(viewport());
        for (int row = 0; row < rows; ++row) {
            const bool selected = m_selectionAccented && row == currentRow();
            const bool hovered = row == m_hoveredRow;
            if (!selected && !hovered) {
                continue;
            }
            // `--panel` is `panelBgAlt` in this palette, not `panelBg` -
            // `panelBg` is `--bg`, the card's own base the table already sits
            // on, and filling a row with its own background is invisible.
            QColor fill = selected ? profile.colors.accent : profile.colors.panelBgAlt;
            if (selected) {
                fill.setAlphaF(hovered ? 0.17 : 0.13);
            }
            under.fillRect(QRect(0, rowViewportPosition(row), viewport()->width(),
                                 rowHeight(row)),
                           fill);
        }
    }

    QTableWidget::paintEvent(event);

    if (rows == 0) {
        return;
    }
    const ThemeProfile& profile = themeFor(this);
    QPainter painter(viewport());
    painter.setPen(QPen(profile.table.gridLine, 1));
    // Every row but the last - `table.tbl tr:last-child td{border-bottom:none}`.
    for (int row = 0; row < rows - 1; ++row) {
        const int y = rowViewportPosition(row) + rowHeight(row) - 1;
        paintHairline(&painter, QPoint(0, y), QPoint(viewport()->width() - 1, y),
                      profile.table.gridLine);
    }

    // `tr.sel td:first-child{box-shadow:inset 2.5px 0 0 var(--acc)}` - a
    // hairline bar on the selected row's own leading edge, painted rather
    // than a border on the cell so it reads as the row's, not the first
    // column's.
    if (m_selectionAccented && currentRow() >= 0) {
        const int row = currentRow();
        const int top = rowViewportPosition(row);
        constexpr double kBarWidth = 2.5;
        // Accent while this list is what keyboard input reaches; the same
        // muted tone the row's own wash falls back to once focus has moved to
        // a FloatingPanel opened over it - the bar and the wash agree about
        // whether this is still "the focused list", not just about which row.
        const QColor barColor = hasFocus() ? profile.colors.accent
                                           : profile.colors.surfaceActive;
        painter.fillRect(QRectF(0, top, kBarWidth, rowHeight(row)), barColor);
    }
}

void TableView::mouseMoveEvent(QMouseEvent* event)
{
    QTableWidget::mouseMoveEvent(event);
    const int row = rowAt(event->position().toPoint().y());
    if (row != m_hoveredRow) {
        m_hoveredRow = row;
        viewport()->update();
    }
}

void TableView::leaveEvent(QEvent* event)
{
    QTableWidget::leaveEvent(event);
    if (m_hoveredRow != -1) {
        m_hoveredRow = -1;
        viewport()->update();
    }
}

} // namespace novu::ui
