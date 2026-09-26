#pragma once

#include "novu/ui/ThemeProfile.h"

#include <QKeySequence>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QWidget>

namespace novu::ui {

// A key, drawn as a key.
//
// The design's `.kb-r .k` and `.tip-t .sc` — a mono chip with a hairline
// outline over the window ground. Every Novusion application had its own: a
// QLabel with a stylesheet in one, a bracketed string in another, and in a
// third the shortcut simply written into the sentence.
//
// A chord is several caps, not one wide chip: `Ctrl+Shift+R` is drawn as three,
// because that is what the design does and because a single chip containing
// three key names is unreadable at 10px. Splitting happens here so no caller
// has to know that a chord uses `+` and a *sequence* uses a space.
class KeyCap : public QWidget
{
    Q_OBJECT

public:
    explicit KeyCap(const QString& keys, QWidget* parent = nullptr);
    KeyCap(const QKeySequence& sequence, QWidget* parent);

    QString keys() const { return m_keys; }
    void setKeys(const QString& keys);
    void setKeys(const QKeySequence& sequence);

    // The individual caps, in order. Exposed because a test that wants to know
    // a chord was split correctly should not have to measure pixels.
    QStringList caps() const { return m_caps; }

    // "Ctrl+Shift+R" -> {"Ctrl", "Shift", "R"}. A separator that stands alone
    // ("+" pressed on its own) survives, which a naive split does not.
    static QStringList split(const QString& keys);

    // The platform's own spelling. QKeySequence renders Ctrl as the command
    // glyph on macOS, and a chip that says "Ctrl" on a Mac is a chip that is
    // simply wrong.
    static QString portableText(const QKeySequence& sequence);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent* event) override;
    bool event(QEvent* event) override;

private:
    QString m_keys;
    QStringList m_caps;
};

// The design's `.kb`: a keyboard reference, as groups of labelled shortcuts.
//
// One widget for the whole sheet. A shortcut list is forty rows of a caption
// and two or three caps; built out of widgets that is four hundred of them, all
// of them inert, in a window that is read once and closed.
//
// The list is *content*, so it is given rather than composed: a caller adds
// groups and rows and the component decides what a row looks like. That is what
// stops the NovuAdmin keyboard window and the NovuSpace one from being two
// different drawings of the same table, which is what they were.
class ShortcutList : public QWidget
{
    Q_OBJECT

public:
    explicit ShortcutList(QWidget* parent = nullptr);

    // `.kb-g` — the tracked uppercase caption over a set of rows.
    void addGroup(const QString& title);
    // `.kb-r` — a description and the keys that reach it.
    void addShortcut(const QString& label, const QString& keys,const QString& note = {});
    void addShortcut(const QString& label, const QKeySequence& sequence);
    void clear();

    int rowCount() const;
    int visibleRowCount() const;
    void setCompact(bool compact) { m_compact=compact;updateGeometry();update(); }
    void setEmptyText(const QString& text) { m_emptyText=text;updateGeometry();update(); }
    int groupCount() const;

    // Only the rows whose label or keys contain `query`, groups that end up
    // empty dropped. Empty restores everything. Here rather than in a caller
    // because a keyboard sheet without a filter is unusable past thirty rows,
    // and every application would otherwise write this loop.
    void setFilter(const QString& query);
    QString filter() const { return m_filter; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    bool event(QEvent* event) override;

private:
    struct Entry
    {
        bool group = false;
        QString label;
        QStringList caps;
        QString keys;
        QString note;
    };

    QVector<Entry> visibleEntries() const;
    int entryHeight(const Entry& entry) const;

    QVector<Entry> m_entries;
    QString m_filter;
    QString m_emptyText;
    bool m_compact=false;
};

// Draws one chord at `right`, right-aligned, vertically centred in `box`, and
// returns the leftmost pixel it used.
//
// Public because three things draw a key chip — KeyCap, ShortcutList and the
// tooltip's `.sc` — and a fourth will. One drawing, so a chip in a tooltip and
// a chip in the keyboard sheet cannot end up different shapes.
int paintKeyCaps(QPainter* painter, const QWidget* context, const QStringList& caps,
                 const QRect& box, int right);

} // namespace novu::ui
