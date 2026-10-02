#pragma once

// Showing a person what happened to the reports they filed.
//
// These components know nothing about where feedback comes from. They are
// handed a list of presentation items and hand back intents - open this one,
// send a new one, filter to that area - and everything else is the host
// application's business. That boundary is the point: the same components sit
// in NovuSpace, which has modules, and in an application that has none.
//
// The lifecycle vocabulary belongs to the server. This file maps its values
// to words and a tone for display, and does not invent a second set: a status
// this library has never heard of is shown as itself rather than as "Unknown",
// because the server adding a state must not make the client lie about it.

#include "novu/ui/Badge.h"
#include "novu/ui/Card.h"
#include "novu/ui/Icon.h"

#include <QAbstractListModel>
#include <QString>
#include <QVector>
#include <QWidget>

class QListView;
class QStackedWidget;
class QVBoxLayout;

namespace novu::ui {

class Button;
class ComboBox;
class EmptyState;
class IconButton;

// One report, as the UI needs it. A flat struct rather than the Hub's own
// record: this library must not include anything from the Hub, and an
// adapter in the application maps between them.
struct FeedbackPresentationItem
{
    QString id;
    //! The server's human reference, "FB-0184". Shown as given.
    QString reference;
    QString title;
    QString category;
    //! Already resolved to words by the host - this library has no module
    //! registry and no way to turn an id into a name.
    QString moduleLabel;
    QString contextLabel;
    //! The server's own value: RECEIVED, PROCESSING, PLANNED, IN_PROGRESS,
    //! RESOLVED, CLOSED. Stored, not translated, until it is drawn.
    QString publicStatus;
    QString resolutionMessage;
    QString resolvedVersion;
    QString createdAt;
    QString updatedAt;
    QString description;

    //! Queued on this device and not yet delivered. Must never be shown as
    //! "Received": the server has not seen it.
    bool pending = false;
    //! The server refused it, and retrying will not help.
    bool failed = false;
};

// How a status is drawn. The words and the tone, and nothing else - no
// percentage, no invented ordering.
struct FeedbackStatusDisplay
{
    QString label;
    Badge::Tone tone = Badge::Tone::Neutral;
    Glyph glyph = Glyph::Info;
};

// Maps a server status to display. An unrecognised value comes back as
// itself, so a status added on the server appears in the UI as that status
// rather than as a lie about it.
FeedbackStatusDisplay feedbackStatusDisplay(const FeedbackPresentationItem& item);

// The titlebar entry point: an icon, and a count when there is something new.
//
// The count is updates the person has not seen, not how many reports they
// have filed - a number that only ever grows is not news.
class FeedbackButton : public QWidget
{
    Q_OBJECT

public:
    explicit FeedbackButton(QWidget* parent = nullptr);

    // Zero hides the badge entirely rather than drawing a "0", which reads as
    // a fault rather than as nothing to report.
    void setUnseenCount(int count);
    int unseenCount() const { return m_unseen; }

signals:
    void clicked();

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void positionBadge();

    IconButton* m_button = nullptr;
    Badge* m_badge = nullptr;
    int m_unseen = 0;
};

// The list of reports, and the detail behind one of them.
//
// A model/view rather than a widget per report: a person who files feedback
// often has hundreds of them, and a stack of cards that deep is slow to build
// and slower to update. A single status change repaints one row.
class FeedbackCenterView : public QWidget
{
    Q_OBJECT

public:
    explicit FeedbackCenterView(QWidget* parent = nullptr);

    // Replaces the list. Cheap to call: the model diffs by id, so a repeated
    // set with one changed status updates one row instead of resetting.
    void setItems(const QVector<FeedbackPresentationItem>& items);

    // The optional area filter. Fewer than two entries means the host has no
    // module model worth filtering by, and the control stays hidden. The
    // first entry is the "everything" one, whatever the host calls it.
    void setFilterOptions(const QVector<QPair<QString, QString>>& options);
    QString activeFilterId() const;

    void showList();

    //! The list model. Public only so the row delegate in the implementation
    //! can read a row; nothing outside this library should construct one.
    class Model;

signals:
    void sendFeedbackRequested();
    void itemOpened(const QString& id);
    void retryRequested(const QString& id);
    void filterChanged(const QString& moduleId);

private:
    void showDetail(const FeedbackPresentationItem& item);
    void applyFilter();

    Model* m_model = nullptr;
    QListView* m_list = nullptr;
    QStackedWidget* m_stack = nullptr;
    QWidget* m_listPage = nullptr;
    QWidget* m_detailPage = nullptr;
    QVBoxLayout* m_detailLayout = nullptr;
    EmptyState* m_empty = nullptr;
    ComboBox* m_filter = nullptr;
    QWidget* m_filterRow = nullptr;
    Button* m_send = nullptr;
    QVector<QPair<QString, QString>> m_filterOptions;
    QVector<FeedbackPresentationItem> m_items;
    //! Which report the detail page is showing, empty while the list is up.
    //! It is what lets a refresh update an open detail in place instead of
    //! dropping the reader back to the list.
    QString m_openDetailId;
    //! Where the list was when the detail was opened, so Back returns there.
    int m_listScroll = 0;
};

// The release channel, when it is worth saying.
//
// Stable says nothing: every user is on stable, and a badge that is always
// there stops being read. The value is the host's - this draws it and does
// not decide what channel anything is on.
class ReleaseChannelBadge : public QWidget
{
    Q_OBJECT

public:
    explicit ReleaseChannelBadge(QWidget* parent = nullptr);

    // "stable" hides it; "beta", "dev", "preview" and anything else is shown
    // uppercased. Not an enum, because the set belongs to whoever ships the
    // releases and a new channel should not need a library release.
    void setChannel(const QString& channel);
    QString channel() const { return m_channel; }

private:
    Badge* m_badge = nullptr;
    QString m_channel;
};

} // namespace novu::ui
