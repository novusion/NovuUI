#include "novu/ui/FeedbackCenter.h"

#include "novu/ui/Button.h"
#include "novu/ui/Feedback.h"
#include "novu/ui/Fields.h"
#include "novu/ui/IconButton.h"
#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"

#include <QAbstractListModel>
#include <QHBoxLayout>
#include <QListView>
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

// One compact row: the title, where it came from, and where it got to.
// Deliberately not a card - a card per report in a list of two hundred is a
// lot of chrome for very little information.
class RowDelegate : public QStyledItemDelegate
{
public:
    RowDelegate(FeedbackCenterView::Model* model, QObject* parent)
        : QStyledItemDelegate(parent), m_model(model) {}

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override
    {
        Q_UNUSED(option)
        Q_UNUSED(index)
        return {0, 58};
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override
    {
        if (!index.isValid() || !m_model) {
            return;
        }
        const FeedbackPresentationItem& item = m_model->at(index.row());
        const ThemeProfile& profile = ThemeManager::instance().defaultProfile();

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        QRect box = option.rect.adjusted(6, 3, -6, -3);
        if (option.state & QStyle::State_Selected) {
            painter->setBrush(profile.colors.surfaceHover);
            painter->setPen(Qt::NoPen);
            painter->drawRoundedRect(box, 8, 8);
        } else if (option.state & QStyle::State_MouseOver) {
            painter->setBrush(profile.colors.surfaceHover);
            painter->setPen(Qt::NoPen);
            painter->drawRoundedRect(box, 8, 8);
        }

        const QRect text = box.adjusted(10, 8, -10, -8);

        QFont titleFont(profile.type.family);
        titleFont.setPixelSize(profile.type.md);
        titleFont.setWeight(QFont::DemiBold);
        painter->setFont(titleFont);
        painter->setPen(profile.colors.textPrimary);
        painter->drawText(QRect(text.left(), text.top(), text.width(), 18),
                          Qt::AlignLeft | Qt::AlignVCenter,
                          painter->fontMetrics().elidedText(
                              item.title, Qt::ElideRight, text.width()));

        // Second line: area, status, and the version it shipped in when there
        // is one. Assembled rather than fixed, so a report with no module and
        // no version still reads as a sentence.
        QStringList parts;
        if (!item.moduleLabel.isEmpty()) {
            parts << item.moduleLabel;
        }
        const FeedbackStatusDisplay display = feedbackStatusDisplay(item);
        parts << display.label;
        if (!item.resolvedVersion.isEmpty() && !item.pending && !item.failed) {
            parts << QObject::tr("Available in %1").arg(item.resolvedVersion);
        }

        QFont subFont(profile.type.family);
        subFont.setPixelSize(profile.type.sm);
        painter->setFont(subFont);
        painter->setPen(item.failed ? profile.semantic.danger.text
                                    : profile.colors.textSecondary);
        painter->drawText(QRect(text.left(), text.top() + 20, text.width(), 16),
                          Qt::AlignLeft | Qt::AlignVCenter,
                          painter->fontMetrics().elidedText(
                              parts.join(QStringLiteral(" · ")),
                              Qt::ElideRight, text.width()));

        painter->restore();
    }

private:
    FeedbackCenterView::Model* m_model = nullptr;
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
    m_button->setToolTip(tr("Feedback"));
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
        m_button->setToolTip(tr("Feedback"));
        return;
    }

    // Past 9 the exact number stops mattering and the badge stops fitting.
    m_badge->setText(m_unseen > 9 ? QStringLiteral("9+")
                                  : QString::number(m_unseen));
    m_badge->adjustSize();
    m_badge->show();
    m_button->setToolTip(tr("Feedback - %n update(s)", nullptr, m_unseen));
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
    m_filter = new ComboBox(m_filterRow);
    filterLayout->addWidget(m_filter);
    m_filterRow->setVisible(false);
    listLayout->addWidget(m_filterRow);
    connect(m_filter, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index >= 0 && index < m_filterOptions.size()) {
            applyFilter();
            emit filterChanged(m_filterOptions.at(index).first);
        }
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
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setMouseTracking(true);
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
}

void FeedbackCenterView::setFilterOptions(const QVector<QPair<QString, QString>>& options)
{
    m_filterOptions = options;
    m_filter->clear();
    for (const auto& option : options) {
        m_filter->addItem(option.second);
    }
    // One option is not a choice; an application without modules shows no
    // filter rather than a filter that cannot filter.
    m_filterRow->setVisible(options.size() > 1);
}

QString FeedbackCenterView::activeFilterId() const
{
    const int index = m_filter->currentIndex();
    if (index < 0 || index >= m_filterOptions.size()) {
        return {};
    }
    return m_filterOptions.at(index).first;
}

void FeedbackCenterView::showList()
{
    m_stack->setCurrentWidget(m_listPage);
}

void FeedbackCenterView::showDetail(const FeedbackPresentationItem& item)
{
    while (QLayoutItem* child = m_detailLayout->takeAt(0)) {
        delete child->widget();
        delete child;
    }

    // No back-arrow glyph in this set; the word carries it on its own.
    auto* back = secondaryButton(tr("Back"), m_detailPage);
    m_detailLayout->addWidget(back);
    connect(back, &QAbstractButton::clicked, this, &FeedbackCenterView::showList);

    if (!item.reference.isEmpty()) {
        m_detailLayout->addWidget(label(item.reference, m_detailPage,
                                        TypeRole::Mono, Ink::Muted));
    }

    auto* title = label(item.title, m_detailPage, TypeRole::CardTitle, Ink::Default);
    title->setWordWrap(true);
    m_detailLayout->addWidget(title);

    const FeedbackStatusDisplay display = feedbackStatusDisplay(item);
    auto* status = new Badge(display.label, display.tone, m_detailPage);
    m_detailLayout->addWidget(status, 0, Qt::AlignLeft);

    // Only what the model actually carries. A field the host did not supply
    // is left out rather than shown empty or invented.
    const auto addLine = [this](const QString& caption, const QString& value) {
        if (value.isEmpty()) {
            return;
        }
        m_detailLayout->addWidget(label(caption, m_detailPage,
                                        TypeRole::Eyebrow, Ink::Muted));
        auto* text = label(value, m_detailPage, TypeRole::Body, Ink::Secondary);
        text->setWordWrap(true);
        m_detailLayout->addWidget(text);
    };

    addLine(tr("Area"), item.moduleLabel);
    addLine(tr("Context"), item.contextLabel);
    addLine(tr("Type"), item.category);
    addLine(tr("Reported"), item.createdAt);
    addLine(tr("Description"), item.description);
    addLine(tr("Resolution"), item.resolutionMessage);
    addLine(tr("Available in"), item.resolvedVersion);

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
