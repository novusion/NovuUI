#pragma once

#include "novu/ui/Export.h"

#include "novu/ui/Icon.h"
#include "novu/ui/StyleSpec.h"

#include <QString>
#include <QWidget>

namespace novu::ui {

// The address of the thing on screen, and whether it can be trusted.
//
// The design's `.omni`. It is not a QLineEdit with a stylesheet: three of the
// four things in it are not editable text. The lock or the warning in front of
// the address is a statement about the connection, the host is emphasised
// inside the address because that is the part that says who you are talking
// to, and the chip at the trailing edge is a real control.
//
// ── why the host is bold and the path is not ─────────────────────────────
//
// `urlMarkup()` in the approved design splits the address at its first slash
// and weights only what comes before it. That is the whole anti-phishing
// affordance a browser chrome has: `novuai.app/console` and
// `novuai.app.evil.example/console` differ in the part a reader skims past
// unless something makes it the loudest thing in the field.
//
// ── the trailing widget ──────────────────────────────────────────────────
//
// Painted chrome, one real control. NovuSpace puts the tab's session chip
// there — which identity this page is loaded under — and that is a button
// because it opens a panel. Anything a caller hands over is laid out inside
// the field's own padding and never overlaps the address.
class NOVUUI_API AddressBar : public QWidget
{
    Q_OBJECT

public:
    // What the leading mark says about the connection.
    enum class Security {
        None,       // nothing loaded — no claim is made
        Secure,     // `.tls` — encrypted
        Insecure,   // plain transport
        Failed,     // `.tls.bad` — the host was not reached at all
    };

    explicit AddressBar(QWidget* parent = nullptr);

    QString url() const { return m_url; }
    void setUrl(const QString& url);

    // What is drawn when there is no address: "No tab open", not an empty
    // field that reads as a field waiting to be typed into.
    QString placeholder() const { return m_placeholder; }
    void setPlaceholder(const QString& placeholder);

    Security security() const { return m_security; }
    void setSecurity(Security security);

    // Takes ownership. A second one deletes the first rather than leaving it
    // parented and unplaced.
    void setTrailing(QWidget* trailing);
    QWidget* trailing() const { return m_trailing; }

    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    bool event(QEvent* event) override;

private:
    // Where the address may be drawn: inside the padding, after the mark and
    // before whatever the caller put at the trailing edge.
    QRect textRect() const;
    void placeTrailing();

    QString m_url;
    QString m_placeholder;
    Security m_security = Security::None;
    bool m_hovered = false;
    QWidget* m_trailing = nullptr;
};

} // namespace novu::ui
