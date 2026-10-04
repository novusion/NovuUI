#include "novu/ui/FeedbackCenter.h"

#include "novu/ui/Button.h"
#include "novu/ui/Feedback.h"
#include "novu/ui/Fields.h"
#include "novu/ui/IconButton.h"
#include "novu/ui/Popover.h"
#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/Tooltip.h"
#include "novu/ui/ThemeManager.h"

#include <QAbstractListModel>
#include <QHBoxLayout>
#include <QListView>
#include <QScrollBar>
#include <QPainter>
#include <QResizeEvent>
#include <QStackedWidget>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

namespace novu::ui {

// ── status mapping ─────────────────────────────────────────────────────────

FeedbackStatusDisplay feedbackStatusDisplay(const FeedbackPresentationItem& item)
{
    // The two local states come first: neither is a server status, and
    // neither may borrow a server status's words.
    if (item.failed) {
        return {QObject::tr("Could not send"), Badge::Tone::Danger, Glyph::Warning};
    }
    if (item.pending) {
        return {QObject::tr("Pending upload"), Badge::Tone::Neutral, Glyph::Clock};
    }

    const QString status = item.publicStatus.toUpper();
    if (status == QLatin1String("RECEIVED")) {
        return {QObject::tr("Received"), Badge::Tone::Info, Glyph::Inbox};
    }
    if (status == QLatin1String("PROCESSING")) {
        return {QObject::tr("Reviewing"), Badge::Tone::Info, Glyph::Search};
    }
    if (status == QLatin1String("PLANNED")) {
        return {QObject::tr("Planned"), Badge::Tone::Info, Glyph::Calendar};
    }
    if (status == QLatin1String("IN_PROGRESS")) {
        return {QObject::tr("In progress"), Badge::Tone::Warning, Glyph::Play};
    }
    if (status == QLatin1String("RESOLVED")) {
        return {QObject::tr("Fixed"), Badge::Tone::Success, Glyph::Check};
    }
    if (status == QLatin1String("CLOSED")) {
        return {QObject::tr("Closed"), Badge::Tone::Neutral, Glyph::Check};
    }
    if (status.isEmpty()) {
        return {QObject::tr("Unknown"), Badge::Tone::Neutral, Glyph::Info};
    }
    // A status this build has never heard of. Showing it verbatim is the
    // honest answer: the server knows something we do not, and pretending
    // otherwise would hide a real state from the person it belongs to.
    return {item.publicStatus, Badge::Tone::Neutral, Glyph::Info};
}

// ── the list model ─────────────────────────────────────────────────────────

class FeedbackCenterView::Model : public QAbstractListModel
{
public:
    using QAbstractListModel::QAbstractListModel;

    int rowCount(const QModelIndex& parent = {}) const override
    {
        return parent.isValid() ? 0 : m_rows.size();
    }

    QVariant data(const QModelIndex& index, int role) const override
    {
        if (!index.isValid() || index.row() >= m_rows.size()) {
            return {};
        }
        if (role == Qt::UserRole) {
            return QVariant::fromValue(index.row());
        }
        if (role == Qt::DisplayRole) {
            return m_rows.at(index.row()).title;
        }
        return {};
    }

    // Replaces the contents without resetting when it does not have to: a
    // status change on one row is one dataChanged, so the view keeps its
    // selection and its scroll position.
    void setRows(const QVector<FeedbackPresentationItem>& rows)
    {
        const bool sameShape = rows.size() == m_rows.size()
            && [&] {
                for (int i = 0; i < rows.size(); ++i) {
                    if (rows.at(i).id != m_rows.at(i).id) {
                        return false;
                    }
                }
                return true;
            }();

        if (!sameShape) {
            beginResetModel();
            m_rows = rows;
            endResetModel();
            return;
        }

        for (int i = 0; i < rows.size(); ++i) {
            const FeedbackPresentationItem& next = rows.at(i);
            const FeedbackPresentationItem& current = m_rows.at(i);
            if (next.publicStatus != current.publicStatus
                || next.pending != current.pending
                || next.failed != current.failed
                || next.resolvedVersion != current.resolvedVersion
                || next.title != current.title) {
                m_rows[i] = next;
                emit dataChanged(index(i), index(i));
            }
        }
    }

    const FeedbackPresentationItem& at(int row) const { return m_rows.at(row); }
    bool isEmpty() const { return m_rows.isEmpty(); }

private:
    QVector<FeedbackPresentationItem> m_rows;
};

namespace {

// One line per report: its reference, its title, and its status as a badge.
//
// Three things, because three is what it takes to find a report again - which
// one it is, what it was about, and whether anything has happened to it. The
// row carried a second line of area, status and version as running text as
// well, and a list of those reads as a stack of paragraphs rather than as a
// list you can scan. Everything else now lives in the detail, which is one
// click away and has room for it.
//
// Painted rather than built from widgets, and the badge is painted too:
// paintBadge() is what Badge itself draws through, so the pill in a row and a
// Badge on a card are the same badge measured by the same code. A Badge
// instance per row is exactly what a painted list exists to avoid.
class RowDelegate : public QStyledItemDelegate
{
public:
    RowDelegate(FeedbackCenterView::Model* model, QWidget* view)
        : QStyledItemDelegate(view), m_model(model), m_view(view) {}

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override
    {
        Q_UNUSED(option)
        Q_UNUSED(index)
        // One line of text plus the plate's own breathing room. Was 58 for
        // two lines; a list of ten now fits where four did.
        return {0, 34};
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override
    {
        if (!index.isValid() || !m_model) {
            return;
        }
        const FeedbackPresentationItem& item = m_model->at(index.row());
        const ThemeProfile& profile = themeFor(m_view);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        const QRect box = option.rect.adjusted(4, 1, -4, -1);
        if (option.state & (QStyle::State_Selected | QStyle::State_MouseOver)) {
            painter->setBrush(profile.colors.surfaceHover);
            painter->setPen(Qt::NoPen);
            painter->drawRoundedRect(box, 8, 8);
        }

        QRect content = box.adjusted(10, 0, -10, 0);

        // The badge first, from the right: it is the only part whose width is
        // not negotiable, so the title gets whatever is left rather than the
        // badge being pushed off the end by a long one.
        const FeedbackStatusDisplay display = feedbackStatusDisplay(item);
        const QSize badge = badgeSizeFor(m_view, display.label,
                                         Badge::Size::Compact, true);
        const QRect badgeRect(content.right() - badge.width() + 1,
                              content.top() + (content.height() - badge.height()) / 2,
                              badge.width(), badge.height());
        paintBadge(painter, badgeRect, m_view, display.label, display.tone,
                   Badge::Size::Compact, true);
        content.setRight(badgeRect.left() - profile.space(SpacingRole::ToolbarGap));

        // The reference, in the mono face: it is an identifier, and a column
        // of them only lines up in a face where the digits do.
        //
        // The column is reserved whether or not this row has one. A report
        // queued on the device has no server reference yet, and letting its
        // title start where the references do would make the one row that is
        // not finished the one row that looks different - the eye reads the
        // ragged edge as the status, which is what the badge is for.
        QFont referenceFont(profile.type.monoFamily);
        referenceFont.setPixelSize(profile.type.sm);
        painter->setFont(referenceFont);
        const QFontMetrics referenceMetrics(referenceFont);
        const int referenceColumn =
            referenceMetrics.horizontalAdvance(QStringLiteral("FB-0000"))
            + profile.space(SpacingRole::ToolbarGap);
        if (!item.reference.isEmpty()) {
            painter->setPen(profile.colors.textTertiary);
            painter->drawText(QRect(content.left(), content.top(),
                                    referenceColumn, content.height()),
                              Qt::AlignLeft | Qt::AlignVCenter,
                              referenceMetrics.elidedText(item.reference, Qt::ElideRight,
                                                          referenceColumn));
        }
        content.setLeft(content.left() + referenceColumn);

        QFont titleFont(profile.type.family);
        titleFont.setPixelSize(profile.type.md);
        titleFont.setWeight(QFont::DemiBold);
        painter->setFont(titleFont);
        // A report the server has not seen is not the same thing as one it
        // has. The badge says so in words; this keeps the row from reading as
        // settled at a glance, without a second colour of its own.
        painter->setPen(item.pending ? profile.colors.textSecondary
                                     : profile.colors.textPrimary);
        if (content.width() > 0) {
            painter->drawText(content, Qt::AlignLeft | Qt::AlignVCenter,
                              painter->fontMetrics().elidedText(
                                  item.title, Qt::ElideRight, content.width()));
        }

        painter->restore();
    }

private:
    FeedbackCenterView::Model* m_model = nullptr;
    QWidget* m_view = nullptr;
};

}

// ── FeedbackButton ─────────────────────────────────────────────────────────

FeedbackButton::FeedbackButton(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_button = new IconButton(Glyph::Chat, IconButton::Variant::Chrome, this);
    // The shared progressive tip, not QToolTip: one hover gives the name, a
    // longer one explains what the panel is for. The host may replace this
    // with wording of its own through Tooltip::install().
    Tooltip::install(m_button, TipContent{
        tr("Feedback"), {},
        tr("Send feedback and follow your reports."), {},
        tr("Report a bug or suggest something, and see what happened to it - "
           "each report stays attached to this application and the area it "
           "came from.")});
    layout->addWidget(m_button);
    connect(m_button, &QAbstractButton::clicked, this, &FeedbackButton::clicked);

    m_badge = new Badge(QString(), Badge::Tone::Danger, this);
    m_badge->setSize(Badge::Size::Compact);
    // The badge is decoration over the button, so a click on the number is
    // still a click on the button rather than a dead spot.
    m_badge->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_badge->hide();

    setUnseenCount(0);
}

void FeedbackButton::setUnseenCount(int count)
{
    m_unseen = qMax(0, count);
    if (m_unseen == 0) {
        m_badge->hide();
        Tooltip::install(m_button, Tooltip::contentFor(m_button));
        return;
    }

    // Past 9 the exact number stops mattering and the badge stops fitting.
    m_badge->setText(m_unseen > 9 ? QStringLiteral("9+")
                                  : QString::number(m_unseen));
    m_badge->adjustSize();
    m_badge->show();
    // The count belongs in the status line, which is the field the design
    // reserves for what a control is doing right now.
    TipContent tip = Tooltip::contentFor(m_button);
    tip.status = tr("%n new update(s)", nullptr, m_unseen);
    Tooltip::install(m_button, tip);
    positionBadge();
}

void FeedbackButton::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    positionBadge();
}

void FeedbackButton::positionBadge()
{
    if (!m_badge->isVisible()) {
        return;
    }
    const QSize size = m_badge->sizeHint();
    m_badge->resize(size);
    m_badge->move(qMax(0, width() - size.width() - 1), 1);
    m_badge->raise();
}

// ── FeedbackCenterView ─────────────────────────────────────────────────────

FeedbackCenterView::FeedbackCenterView(QWidget* parent)
    : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_stack = new QStackedWidget(this);
    root->addWidget(m_stack, 1);

    // ── list page ──
    m_listPage = new QWidget(m_stack);
    const ThemeProfile& profile = ThemeManager::instance().defaultProfile();
    auto* listLayout = new QVBoxLayout(m_listPage);
    listLayout->setContentsMargins(14, 12, 14, 12);
    listLayout->setSpacing(0);

    // The heading and the primary action share one line. Putting Send at the
    // top is what makes it read as the thing to do here rather than as a
    // footnote under an empty list - and it means the action sits in the same
    // place whether there are no reports or two hundred.
    auto* headerRow = new QWidget(m_listPage);
    auto* headerLayout = new QHBoxLayout(headerRow);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(profile.space(SpacingRole::ToolbarGap));
    headerLayout->addWidget(label(tr("Your reports"), headerRow,
                                  TypeRole::Eyebrow, Ink::Muted));
    headerLayout->addStretch(1);

    m_send = primaryButton(tr("Send feedback"), headerRow);
    m_send->setGlyph(Glyph::Plus);
    m_send->setControlSize(ControlSize::Small);
    headerLayout->addWidget(m_send);
    connect(m_send, &QAbstractButton::clicked, this,
            &FeedbackCenterView::sendFeedbackRequested);

    listLayout->addWidget(headerRow);
    listLayout->addSpacing(profile.space(SpacingRole::SectionGap));

    // Directly under the header, with no spacer between them: the filter
    // belongs to the heading above it, not to the list below.
    m_filterRow = new QWidget(m_listPage);
    auto* filterLayout = new QVBoxLayout(m_filterRow);
    filterLayout->setContentsMargins(0, 0, 0, 0);
    // Match Help Center's category selector exactly: SelectButton owns the
    // canonical NovuUI Popover, checked option, geometry and open state.
    m_filter = new SelectButton(m_filterRow);
    m_filter->setControlSize(ControlSize::Small);
    m_filter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    filterLayout->addWidget(m_filter);
    m_filterRow->setVisible(false);
    listLayout->addWidget(m_filterRow);
    connect(m_filter, &SelectButton::currentChanged, this, [this](const QString& key) {
        applyFilter();
        emit filterChanged(key);
    });

    listLayout->addSpacing(profile.space(SpacingRole::CardGap));

    // One content area holding either the list or the empty state, so the
    // header and the filter keep their position however much is in it.
    auto* content = new QWidget(m_listPage);
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    m_model = new Model(this);
    m_list = new QListView(content);
    m_list->setModel(m_model);
    m_list->setItemDelegate(new RowDelegate(m_model, m_list));
    m_list->setSpacing(0);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setMouseTracking(true);
    // The list draws on whatever is behind it.
    //
    // QListView fills its viewport with QPalette::Base, and that palette is
    // the host application's, not this popup's. NovuExplorer sets Base to its
    // panel colour application-wide, so the list blended into the surface;
    // NovuSpace sets no application palette at all, so Qt's own default Base
    // painted a pale rectangle behind the rows - a card inside the card, and
    // the whole of the difference between the two.
    //
    // Transparent rather than "the right colour": the right colour is
    // whatever PopupSurface is painting, and a component that had to know
    // that would be wrong again in the next host.
    QPalette listPalette = m_list->palette();
    listPalette.setColor(QPalette::Base, Qt::transparent);
    listPalette.setColor(QPalette::Window, Qt::transparent);
    m_list->setPalette(listPalette);
    m_list->viewport()->setAutoFillBackground(false);
    m_list->setUniformItemSizes(true);
    contentLayout->addWidget(m_list, 1);

    // Enter opens, which is what a keyboard user expects of a list.
    connect(m_list, &QListView::activated, this, [this](const QModelIndex& index) {
        if (index.isValid()) {
            const FeedbackPresentationItem item = m_model->at(index.row());
            showDetail(item);
            emit itemOpened(item.id);
        }
    });
    // A single click opens. activated() alone is a double click on this
    // platform, and a list whose rows open on the second click reads as
    // broken when every row is obviously a link.
    connect(m_list, &QListView::clicked, this, [this](const QModelIndex& index) {
        if (index.isValid()) {
            const FeedbackPresentationItem item = m_model->at(index.row());
            showDetail(item);
            emit itemOpened(item.id);
        }
    });

    // Centred in the content area, and carrying no action of its own: the
    // only Send button is the one in the header.
    m_empty = new EmptyState(Glyph::Chat, tr("No reports yet"),
                             tr("Send feedback and it will appear here."),
                             content);
    m_empty->hide();
    contentLayout->addWidget(m_empty, 1);

    listLayout->addWidget(content, 1);

    m_stack->addWidget(m_listPage);

    // ── detail page ──
    m_detailPage = new QWidget(m_stack);
    m_detailLayout = new QVBoxLayout(m_detailPage);
    m_detailLayout->setContentsMargins(14, 12, 14, 12);
    m_detailLayout->setSpacing(8);
    m_stack->addWidget(m_detailPage);

    m_stack->setCurrentWidget(m_listPage);
}

void FeedbackCenterView::setItems(const QVector<FeedbackPresentationItem>& items)
{
    m_items = items;
    applyFilter();
}

void FeedbackCenterView::applyFilter()
{
    const QString filter = activeFilterId();
    QVector<FeedbackPresentationItem> visible;
    visible.reserve(m_items.size());
    for (const FeedbackPresentationItem& item : m_items) {
        // An empty filter id is the host's "everything" entry.
        if (filter.isEmpty() || item.moduleLabel == filter
            || item.category == filter) {
            visible.append(item);
        }
    }

    m_model->setRows(visible);
    const bool empty = visible.isEmpty();
    m_empty->setVisible(empty);
    m_list->setVisible(!empty);

    // A refresh arriving while somebody is reading a report must not throw
    // them back to the list. The open detail is re-rendered from the new data
    // instead, so a status that changed underneath is simply shown; and if
    // the report is gone from the set entirely, what is on screen is left
    // alone rather than replaced by an empty page.
    if (!m_openDetailId.isEmpty()) {
        for (const FeedbackPresentationItem& item : m_items) {
            if (item.id == m_openDetailId) {
                showDetail(item);
                break;
            }
        }
    }
}

void FeedbackCenterView::setFilterOptions(const QVector<QPair<QString, QString>>& options)
{
    m_filterOptions = options;
    m_filter->clearOptions();
    for (const auto& option : options) {
        m_filter->addOption(option.first, option.second);
    }
    // One option is not a choice; an application without modules shows no
    // filter rather than a filter that cannot filter.
    m_filterRow->setVisible(options.size() > 1);
}

QString FeedbackCenterView::activeFilterId() const
{
    return m_filter->currentKey();
}

void FeedbackCenterView::showList()
{
    m_openDetailId.clear();
    m_stack->setCurrentWidget(m_listPage);
    // The list widget was never destroyed, so its filter and its selection
    // are still what they were; the scroll offset is restored explicitly
    // because a refresh that changed the set of rows while the detail was
    // open would have reset it underneath us.
    if (m_list->verticalScrollBar()) {
        m_list->verticalScrollBar()->setValue(m_listScroll);
    }
}

void FeedbackCenterView::showDetail(const FeedbackPresentationItem& item)
{
    // Where the list was, so Back returns to it rather than to the top. Taken
    // before the page changes and only when we are coming from the list - a
    // re-render of an open detail must not overwrite it with whatever the
    // hidden list reads now.
    if (m_openDetailId.isEmpty() && m_list->verticalScrollBar()) {
        m_listScroll = m_list->verticalScrollBar()->value();
    }
    m_openDetailId = item.id;

    while (QLayoutItem* child = m_detailLayout->takeAt(0)) {
        delete child->widget();
        delete child;
    }

    const ThemeProfile& profile = themeFor(this);

    // The header is the way back and says where back goes. One row, so the
    // detail opens where the list's own heading was and the panel does not
    // appear to jump.
    auto* header = new QWidget(m_detailPage);
    auto* headerRow = new QHBoxLayout(header);
    headerRow->setContentsMargins(0, 0, 0, 0);
    headerRow->setSpacing(profile.space(SpacingRole::ToolbarGap));
    auto* back = new IconButton(Glyph::Back, header);
    back->setControlSize(ControlSize::Small);
    back->setToolTip(tr("Back to your reports"));
    headerRow->addWidget(back);
    headerRow->addWidget(label(tr("Your reports"), header,
                               TypeRole::Eyebrow, Ink::Muted));
    headerRow->addStretch(1);
    m_detailLayout->addWidget(header);
    connect(back, &QAbstractButton::clicked, this, &FeedbackCenterView::showList);

    m_detailLayout->addSpacing(profile.space(SpacingRole::SectionGap));

    if (!item.reference.isEmpty()) {
        m_detailLayout->addWidget(label(item.reference, m_detailPage,
                                        TypeRole::Mono, Ink::Muted));
    }

    auto* title = label(item.title, m_detailPage, TypeRole::CardTitle, Ink::Default);
    title->setWordWrap(true);
    m_detailLayout->addWidget(title);

    const FeedbackStatusDisplay display = feedbackStatusDisplay(item);
    auto* status = statusBadge(display.label, display.tone, m_detailPage);
    m_detailLayout->addWidget(status, 0, Qt::AlignLeft);

    m_detailLayout->addSpacing(profile.space(SpacingRole::SectionGap));

    // Only what the model actually carries. A field the host did not supply
    // is left out rather than shown empty or filled with something invented -
    // an "Application" line reading "unknown" would be worse than no line.
    const auto addLine = [this, &profile](const QString& caption, const QString& value) {
        if (value.isEmpty()) {
            return;
        }
        m_detailLayout->addWidget(label(caption, m_detailPage,
                                        TypeRole::Eyebrow, Ink::Muted));
        auto* text = label(value, m_detailPage, TypeRole::Body, Ink::Secondary);
        text->setWordWrap(true);
        m_detailLayout->addWidget(text);
        m_detailLayout->addSpacing(profile.space(SpacingRole::CardGap));
    };

    addLine(tr("Area"), item.moduleLabel);
    addLine(tr("Type"), item.category);
    addLine(tr("Description"), item.description);
    addLine(tr("Submitted"), item.createdAt);
    addLine(tr("Application"), item.contextLabel);
    addLine(tr("Resolved in"), item.resolvedVersion);
    addLine(tr("Resolution"), item.resolutionMessage);

    if (item.failed) {
        auto* retry = secondaryButton(tr("Try again"), m_detailPage);
        retry->setGlyph(Glyph::Refresh);
        m_detailLayout->addWidget(retry, 0, Qt::AlignLeft);
        const QString id = item.id;
        connect(retry, &QAbstractButton::clicked, this, [this, id] {
            emit retryRequested(id);
        });
    }

    m_detailLayout->addStretch(1);
    m_stack->setCurrentWidget(m_detailPage);
}

// ── ReleaseChannelBadge ────────────────────────────────────────────────────

ReleaseChannelBadge::ReleaseChannelBadge(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Info, not warning: a pre-release build is a fact about the build, not
    // a problem with it, and red would say otherwise.
    m_badge = new Badge(QString(), Badge::Tone::Info, this);
    m_badge->setSize(Badge::Size::Compact);
    layout->addWidget(m_badge);

    setChannel(QStringLiteral("stable"));
}

void ReleaseChannelBadge::setChannel(const QString& channel)
{
    m_channel = channel.trimmed();
    const QString normalized = m_channel.toLower();

    // Stable is the absence of a badge. Everyone is on stable; a badge that
    // is always there is one nobody reads.
    const bool hidden = normalized.isEmpty()
        || normalized == QLatin1String("stable");
    setVisible(!hidden);
    if (hidden) {
        return;
    }

    m_badge->setText(m_channel.toUpper());
    m_badge->adjustSize();
}

} // namespace novu::ui
