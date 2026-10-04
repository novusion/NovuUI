#include "novu/ui/ReleaseNotes.h"

#include "novu/ui/Feedback.h"
#include "novu/ui/Icon.h"
#include "novu/ui/Layout.h"
#include "novu/ui/Separator.h"
#include "novu/ui/Text.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace novu::ui {

namespace {

QWidget* plain(QWidget* parent = nullptr)
{
    return new QWidget(parent);
}

} // namespace

QWidget* documentationItem(const QString& name, const QString& shortcut,
                           const QString& body, Ink nameInk, QWidget* parent)
{
    auto* wrap = plain(parent);
    auto* wv = new QVBoxLayout(wrap);
    wv->setContentsMargins(0, 0, 0, 0);
    wv->setSpacing(0);
    wv->addWidget(new Separator(Qt::Horizontal, wrap));

    auto* item = plain(wrap);
    auto* v = new QVBoxLayout(item);
    v->setContentsMargins(0, 11, 0, 11);
    v->setSpacing(5);

    auto* head = plain(item);
    auto* h = new QHBoxLayout(head);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(9);
    h->addWidget(label(name, head, TypeRole::Body, nameInk, true));
    if (!shortcut.isEmpty()) {
        h->addWidget(label(shortcut, head, TypeRole::Mono, Ink::Muted));
    }
    h->addStretch(1);
    v->addWidget(head);

    auto* text = label(body, item, TypeRole::Caption, Ink::Muted);
    text->setWordWrap(true);
    v->addWidget(text);

    wv->addWidget(item);
    return wrap;
}

QWidget* windowFoot(QWidget* leading, const QList<QWidget*>& actions,
                    QWidget* parent)
{
    auto* wrap = plain(parent);
    auto* wv = new QVBoxLayout(wrap);
    wv->setContentsMargins(0, 0, 0, 0);
    wv->setSpacing(0);
    wv->addWidget(new Separator(Qt::Horizontal, wrap));

    auto* row = plain(wrap);
    auto* h = new QHBoxLayout(row);
    h->setContentsMargins(16, 11, 16, 11);
    h->setSpacing(8);
    if (leading) {
        leading->setParent(row);
        h->addWidget(leading);
    }
    h->addStretch(1);
    for (QWidget* action : actions) {
        if (!action) {
            continue;
        }
        action->setParent(row);
        h->addWidget(action);
    }
    wv->addWidget(row);
    return wrap;
}

QWidget* releaseNotesPage(QWidget* parent, const QString& intro,
                          const QList<ReleaseNote>& releases, QWidget* lead)
{
    auto* host = plain(parent);
    QVBoxLayout* column = scrollPage(host);
    column->setContentsMargins(16, 14, 16, 16);

    if (lead) {
        lead->setParent(host);
        column->addWidget(lead);
    }

    if (releases.isEmpty()) {
        // Nothing to say, said once. A changelog is a record; a context that
        // has not made a release does not get one written for it.
        column->addWidget(new EmptyState(
            Glyph::WhatsNew, QStringLiteral("No release notes available yet."),
            QStringLiteral("Nothing has been recorded for this context in this build."),
            host));
        column->addStretch(1);
        return host;
    }

    if (!intro.isEmpty()) {
        // Whose version numbers these are. A module that ships inside a host
        // application has no release line of its own, and a page that leaves
        // that unsaid invites the reader to assume it has one.
        auto* note = label(intro, host, TypeRole::Caption, Ink::Muted);
        note->setWordWrap(true);
        note->setContentsMargins(0, 0, 0, 6);
        column->addWidget(note);
    }

    // The three changelog headings are semantic and deliberately not the
    // product accent: a release note must not read as branding.
    const QList<QPair<QString, Ink>> sections = {
        {QStringLiteral("New"), Ink::Info},
        {QStringLiteral("Improved"), Ink::Success},
        {QStringLiteral("Fixed"), Ink::Warning},
    };

    for (const ReleaseNote& release : releases) {
        auto* head = plain();
        auto* hh = new QHBoxLayout(head);
        hh->setContentsMargins(0, 14, 0, 0);
        hh->setSpacing(9);
        hh->addWidget(label(release.version, head, TypeRole::SectionTitle,
                            Ink::Default, true));
        hh->addStretch(1);
        if (!release.date.isEmpty()) {
            hh->addWidget(label(release.date, head, TypeRole::Mono, Ink::Muted));
        }
        column->addWidget(head);

        for (int i = 0; i < sections.size(); ++i) {
            const QStringList& lines = i == 0   ? release.added
                                       : i == 1 ? release.improved
                                                : release.fixed;
            if (lines.isEmpty()) {
                continue;
            }
            QStringList body;
            for (const QString& line : lines) {
                body << QStringLiteral("- ") + line;
            }
            column->addWidget(documentationItem(sections.at(i).first, {},
                                                body.join(QLatin1Char('\n')),
                                                sections.at(i).second));
        }
    }
    column->addStretch(1);
    return host;
}

} // namespace novu::ui
