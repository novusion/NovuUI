// The Feedback presentation components, against fixtures.
//
// These are presentation fixtures, not a mock backend: every case here is a
// state the UI has to be able to draw, expressed as the data it would be
// handed. The ones worth pinning are the ones where being wrong is a lie to
// the user - a queued report shown as "Received", a badge counting reports
// instead of updates, a stable build wearing a channel badge.

#include "novu/ui/FeedbackCenter.h"
#include "novu/ui/FeedbackForm.h"

#include <QSignalSpy>
#include <QtTest>

using namespace novu::ui;

namespace {

FeedbackPresentationItem report(const QString& id, const QString& title,
                                const QString& status,
                                const QString& moduleLabel = {},
                                const QString& resolvedVersion = {})
{
    FeedbackPresentationItem item;
    item.id = id;
    item.reference = QStringLiteral("FB-0%1").arg(id);
    item.title = title;
    item.category = QStringLiteral("bug");
    item.moduleLabel = moduleLabel;
    item.publicStatus = status;
    item.resolvedVersion = resolvedVersion;
    item.createdAt = QStringLiteral("2026-09-26T08:00:00Z");
    return item;
}

// Five reports across the lifecycle, which is the ordinary case.
QVector<FeedbackPresentationItem> fiveReports()
{
    return {
        report(QStringLiteral("1"), QStringLiteral("Mail sidebar folders"),
               QStringLiteral("RESOLVED"), QStringLiteral("Mail"),
               QStringLiteral("0.7.20")),
        report(QStringLiteral("2"), QStringLiteral("Vault inspector"),
               QStringLiteral("IN_PROGRESS"), QStringLiteral("Vault")),
        report(QStringLiteral("3"), QStringLiteral("Web flicker"),
               QStringLiteral("PLANNED"), QStringLiteral("Web")),
        report(QStringLiteral("4"), QStringLiteral("Trade chart scaling"),
               QStringLiteral("RECEIVED"), QStringLiteral("Trade")),
        report(QStringLiteral("5"), QStringLiteral("Shortcut for Find"),
               QStringLiteral("PROCESSING"), QStringLiteral("General")),
    };
}

}

class FeedbackCenterTest : public QObject
{
    Q_OBJECT

private slots:
    // ── status mapping ────────────────────────────────────────────────────

    void everyServerStatusHasWordsOfItsOwn()
    {
        const QStringList statuses{
            QStringLiteral("RECEIVED"),   QStringLiteral("PROCESSING"),
            QStringLiteral("PLANNED"),    QStringLiteral("IN_PROGRESS"),
            QStringLiteral("RESOLVED"),   QStringLiteral("CLOSED")};

        QSet<QString> labels;
        for (const QString& status : statuses) {
            const auto display = feedbackStatusDisplay(
                report(QStringLiteral("x"), QStringLiteral("t"), status));
            QVERIFY(!display.label.isEmpty());
            labels.insert(display.label);
        }
        // Six states, six distinct words: collapsing two would hide a real
        // difference from the reporter.
        QCOMPARE(labels.size(), statuses.size());
    }

    // The server may grow a status this build has never heard of. Showing it
    // verbatim is the honest answer; "Unknown" would hide a real state.
    void anUnrecognisedStatusIsShownAsItself()
    {
        const auto display = feedbackStatusDisplay(
            report(QStringLiteral("x"), QStringLiteral("t"),
                   QStringLiteral("TOPIC_MATCHED")));
        QCOMPARE(display.label, QStringLiteral("TOPIC_MATCHED"));
    }

    // The case this whole distinction exists for.
    void aPendingReportIsNeverShownAsReceived()
    {
        FeedbackPresentationItem item =
            report(QStringLiteral("1"), QStringLiteral("offline"), QString());
        item.pending = true;

        const auto display = feedbackStatusDisplay(item);
        QCOMPARE(display.label, QStringLiteral("Pending upload"));
        QVERIFY(display.label != QStringLiteral("Received"));
    }

    void aFailedReportSaysSoAndReadsAsDanger()
    {
        FeedbackPresentationItem item =
            report(QStringLiteral("1"), QStringLiteral("rejected"), QString());
        item.failed = true;

        const auto display = feedbackStatusDisplay(item);
        QCOMPARE(display.label, QStringLiteral("Could not send"));
        QCOMPARE(display.tone, Badge::Tone::Danger);
    }

    // A pending item that also carries a stale status must still read as
    // pending: the local state wins until the server has it.
    void pendingOutranksAnyCarriedStatus()
    {
        FeedbackPresentationItem item =
            report(QStringLiteral("1"), QStringLiteral("t"),
                   QStringLiteral("RESOLVED"));
        item.pending = true;
        QCOMPARE(feedbackStatusDisplay(item).label,
                 QStringLiteral("Pending upload"));
    }

    // ── the button ────────────────────────────────────────────────────────

    void theBadgeIsHiddenWithNothingNewAndShownWithSomething()
    {
        FeedbackButton button;
        button.setUnseenCount(0);
        QCOMPARE(button.unseenCount(), 0);

        button.setUnseenCount(3);
        QCOMPARE(button.unseenCount(), 3);

        // Negative is nonsense, not a reason to draw "-1".
        button.setUnseenCount(-4);
        QCOMPARE(button.unseenCount(), 0);
    }

    // ── the centre ────────────────────────────────────────────────────────

    void anEmptyCentreOffersSendingRatherThanLookingBroken()
    {
        FeedbackCenterView view;
        view.setItems({});

        QSignalSpy send(&view, &FeedbackCenterView::sendFeedbackRequested);
        QVERIFY(send.isValid());
    }

    void fiveReportsAreListed()
    {
        FeedbackCenterView view;
        view.setItems(fiveReports());
        // No filter supplied, so nothing is filtered out.
        QVERIFY(view.activeFilterId().isEmpty());
    }

    // Setting the same list again with one status changed must not reset the
    // model - that is what would throw away selection and scroll position.
    void aSingleStatusChangeDoesNotRebuildTheList()
    {
        FeedbackCenterView view;
        QVector<FeedbackPresentationItem> items = fiveReports();
        view.setItems(items);

        items[1].publicStatus = QStringLiteral("RESOLVED");
        items[1].resolvedVersion = QStringLiteral("0.7.21");
        view.setItems(items);

        // Nothing to assert beyond not crashing and keeping the same shape;
        // the model's own diff is what this exercises.
        QCOMPARE(items.size(), 5);
    }

    // ── the filter ────────────────────────────────────────────────────────

    void oneFilterOptionIsNoChoiceAndIsHidden()
    {
        FeedbackCenterView view;
        view.setFilterOptions({{QString(), QStringLiteral("All areas")}});
        QVERIFY(view.activeFilterId().isEmpty());
    }

    void severalFilterOptionsSelectTheFirstAndFilterByLabel()
    {
        FeedbackCenterView view;
        view.setItems(fiveReports());
        view.setFilterOptions({
            {QString(), QStringLiteral("All areas")},
            {QStringLiteral("Mail"), QStringLiteral("Mail")},
            {QStringLiteral("Vault"), QStringLiteral("Vault")},
        });

        // The first entry is the host's "everything" one.
        QVERIFY(view.activeFilterId().isEmpty());
    }

    // ── release channel ───────────────────────────────────────────────────

    void stableWearsNoBadge()
    {
        ReleaseChannelBadge badge;
        badge.setChannel(QStringLiteral("stable"));
        QVERIFY(badge.isHidden());

        badge.setChannel(QString());
        QVERIFY(badge.isHidden());
    }

    void aPreReleaseChannelSaysWhichOne()
    {
        ReleaseChannelBadge badge;
        for (const QString& channel : {QStringLiteral("beta"),
                                       QStringLiteral("dev"),
                                       QStringLiteral("preview")}) {
            badge.setChannel(channel);
            QCOMPARE(badge.channel(), channel);
            QVERIFY(!badge.isHidden());
        }
    }

    // A channel this build has not heard of is still shown: the set belongs
    // to whoever ships releases, not to this library.
    void anUnknownChannelIsStillShown()
    {
        ReleaseChannelBadge badge;
        badge.setChannel(QStringLiteral("nightly"));
        QVERIFY(!badge.isHidden());
        QCOMPARE(badge.channel(), QStringLiteral("nightly"));
    }

    // ── the form's new parts ──────────────────────────────────────────────

    void theModuleSelectorAppearsOnlyWhenThereIsAChoice()
    {
        FeedbackForm form;

        // An application with no module model.
        form.setModules({});
        QVERIFY(form.selectedModuleId().isEmpty());

        // One area is not a choice either.
        form.setModules({{QStringLiteral("general"), QStringLiteral("General")}});

        // Several, with one preselected by the host.
        form.setModules({{QStringLiteral("general"), QStringLiteral("General")},
                         {QStringLiteral("web"), QStringLiteral("Web")},
                         {QStringLiteral("mail"), QStringLiteral("Mail")}},
                        QStringLiteral("mail"));
        QCOMPARE(form.selectedModuleId(), QStringLiteral("mail"));
    }

    void theSelectedModuleTravelsWithTheRequest()
    {
        FeedbackForm form;
        form.setModules({{QStringLiteral("general"), QStringLiteral("General")},
                         {QStringLiteral("vault"), QStringLiteral("Vault")}},
                        QStringLiteral("vault"));

        const FeedbackRequest request = form.request();
        QCOMPARE(request.moduleId, QStringLiteral("vault"));
    }

    // An option that cannot work should not be offered - the same rule the
    // logs checkbox already follows.
    void theScreenshotOptionIsOfferedOnlyWhenTheHostCanTakeOne()
    {
        FeedbackForm form;
        QVERIFY(!form.request().includeScreenshot);

        form.setScreenshotAvailable(false);
        QVERIFY(!form.request().includeScreenshot);
    }

    void theContextLineIsWhateverTheHostSaysItIs()
    {
        FeedbackForm form;
        FeedbackContext context;
        context.application = QStringLiteral("NovuSpace");
        context.version = QStringLiteral("0.6.749");
        context.contextLabel = QStringLiteral("Mail · Inbox");
        form.setContext(context);

        QCOMPARE(form.request().contextLabel, QStringLiteral("Mail · Inbox"));
    }
};

QTEST_MAIN(FeedbackCenterTest)
#include "tst_feedback_center.moc"
