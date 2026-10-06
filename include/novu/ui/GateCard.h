#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/AppMark.h"
#include "novu/ui/Icon.h"
#include "novu/ui/Layout.h"
#include "novu/ui/StyleSpec.h"

#include <QString>
#include <QWidget>

class QHBoxLayout;
class QVBoxLayout;

namespace novu::ui {

// The card a person signs in on.
//
// The approved NovuSpace design's `.gate` / `.gate-card` / `.gate-form` /
// `.gate-mark` / `.gate-foot`, and the approved NovuAdmin design's `.login` /
// `.login-card` / `.login-form` / `.login-mark` / `.login-foot`. Two names for
// one shape, drawn twice, and NovuAdmin built a third copy in Qt.
//
// ── what is shared and what is not ───────────────────────────────────────
//
// The *shape* is shared: a card centred in an otherwise empty window, a large
// application mark above a title and a line of explanation, a form, and a
// footer carrying the thing a person needs to check before they type a
// password — which host they are about to sign in to.
//
// The *form* is not, and this component does not have one. NovuAdmin signs in
// with an email and a password, NovuSpace with a workspace and a key, and a
// future one will use a device code. `form()` is a layout the caller fills;
// anything else would be a component that has to grow a field every time a
// product grows an authentication method.
//
// ── the footer is not decoration ─────────────────────────────────────────
//
// Both designs put the host in it, and that is a security property rather than
// a layout choice: the one thing a person must be able to check before typing
// a credential is where it is going. So `setHost()` exists and the footer is
// drawn whenever it is set, rather than being an optional slot a page may
// forget.
class NOVUUI_API GateCard : public QWidget
{
    Q_OBJECT

public:
    GateCard(const QString& title, QWidget* parent = nullptr);

    void setTitle(const QString& title);
    QString title() const { return m_title; }
    // The line under the title: what this sign-in is for.
    void setSubtitle(const QString& subtitle);
    QString subtitle() const { return m_subtitle; }
    void setGlyph(Glyph glyph);

    AppMark* mark() const { return m_mark; }

    // Where the credentials go. Fill this.
    QVBoxLayout* form() const;
    QWidget* formWidget() const { return m_form; }

    // The row the card's actions sit in, under the fields.
    //
    // Created on first call and always last in the form, so an action added
    // before a field still ends up below it. Right-aligned by default: the
    // primary action ends where the fields end, which is the edge the eye is
    // already following down the form.
    QHBoxLayout* actions();
    ActionAlignment actionAlignment() const { return m_actionAlignment; }
    void setActionAlignment(ActionAlignment alignment);

    // Where the credential is going. Shown in the footer in the monospace face,
    // because a hostname read in a proportional font is a hostname a person
    // does not check character by character.
    void setHost(const QString& host);
    QString host() const { return m_host; }
    // A second footer line — a version, an environment, a warning that this is
    // a staging server.
    void setFootnote(const QString& note);
    QString footnote() const { return m_footnote; }

    // The error a failed sign-in produced. Shown above the form, in the danger
    // tone. Empty clears it.
    void setError(const QString& message);
    QString error() const { return m_error; }
    bool hasError() const { return !m_error.isEmpty(); }

    // Disables the whole form while a sign-in is in flight, for the same
    // reason a Dialog does: a second press while the first request is out
    // creates a second session.
    bool isBusy() const { return m_busy; }
    void setBusy(bool busy);

    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool event(QEvent* event) override;

private:
    QHBoxLayout* m_actions = nullptr;
    ActionAlignment m_actionAlignment = ActionAlignment::Right;

    int headHeight() const;
    int footHeight() const;
    int errorHeight() const;
    void relayout();

    QString m_title;
    QString m_subtitle;
    QString m_host;
    QString m_footnote;
    QString m_error;
    bool m_busy = false;

    AppMark* m_mark = nullptr;
    QWidget* m_form = nullptr;
};

} // namespace novu::ui
