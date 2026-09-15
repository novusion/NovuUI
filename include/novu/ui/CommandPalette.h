#pragma once

#include "novu/ui/Icon.h"
#include "novu/ui/StyleSpec.h"

#include <QPointer>
#include <QString>
#include <QVariant>
#include <QVector>
#include <QWidget>

class QLineEdit;

namespace novu::ui {

// One thing the application can do, by the name a person would search for.
struct Command
{
    QString name;
    QString group;      // "View", "File" — shown as a heading in the list
    QString shortcut;
    Glyph glyph = Glyph::None;
    QVariant data;      // what `chosen` carries back
    bool enabled = true;
};

// Type what you want.
//
// The same set of actions the menus offer, reached by name instead of by place.
// The commands are given to it rather than discovered by it, because the
// application already knows what it can do — and a palette that scraped a menu
// bar would list every separator and every submenu title.
//
// ── what this is a port of ───────────────────────────────────────────────
//
// NovusionUI's `CommandPalette`, which the approved NovuExplorer and NovuVoice
// designs both draw as `.palette` / `.palette-list` / `.pcmd`. Three
// descriptions of one control, and the existing implementation was the best of
// them: grouped headings, a painted list, and the whole thing dismissed by
// Escape or by running something.
//
// Two things changed in the port. The colours are tokens rather than
// `Token::Panel` constants, so a palette inside a NovuTrade scope is a NovuTrade
// palette. And the match is a *subsequence* rather than a substring: typing
// "ndp" finds "New device peer", which is how anyone who has used one of these
// expects it to behave and is most of why they are faster than a menu.
//
// ── ranking ──────────────────────────────────────────────────────────────
//
// A prefix beats a word start beats a scattered subsequence, and a shorter name
// beats a longer one at the same rank. Without an order the first match is
// whichever command happened to be registered first, which makes the top result
// — the one Enter runs — effectively random.
class CommandPalette : public QWidget
{
    Q_OBJECT

public:
    explicit CommandPalette(QWidget* parent = nullptr);

    void setCommands(const QVector<Command>& commands);
    void addCommand(const Command& command);
    void clear();
    int commandCount() const { return m_commands.size(); }

    // Opens over `host`, near the top where the eye already is. The palette
    // parents itself to the host's window and covers it with a scrim.
    void present(QWidget* host);
    void dismiss();
    bool isPresented() const { return isVisible(); }

    QString filter() const;
    void setFilter(const QString& text);

    // The matches in display order, by index into the command list. For a test
    // that wants to assert on the ranking without reading pixels.
    QVector<int> matches() const { return m_matches; }
    int currentMatch() const { return m_current; }
    QString currentName() const;

    // True when `needle`'s characters appear in `haystack` in order. Public
    // because the ranking is the interesting part of this component and a test
    // should be able to reach it directly.
    static bool subsequenceMatch(const QString& needle, const QString& haystack);
    static int matchScore(const QString& needle, const Command& command);

    QRect rowRect(int visualRow) const;

    StyleSpec styleSpec() const;
    ResolvedStyle resolvedStyle() const;

    QSize sizeHint() const override;

signals:
    void chosen(const QVariant& data);
    void dismissed();

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool event(QEvent* event) override;
    // The field has the focus, so it sees the arrows first. Forwarding them
    // here is what keeps the caret in the box while the list moves.
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    // One line of the list: a group heading, or a command.
    struct Row
    {
        bool heading = false;
        QString group;
        int match = -1;   // index into m_matches, for a command row
    };

    void refilter();
    void run(int matchIndex);
    void place();
    int rowAt(const QPoint& position) const;

    QLineEdit* m_input = nullptr;
    QVector<Command> m_commands;
    QVector<int> m_matches;
    QVector<Row> m_rows;

    int m_current = -1;
    int m_hovered = -1;
    int m_pressed = -1;

    QPointer<QWidget> m_scrim;
    QPointer<QWidget> m_host;
};

} // namespace novu::ui
