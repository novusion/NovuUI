#include "novu/ui/Keys.h"

#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>
#include <QPainter>
#include <QPen>

namespace novu::ui {

namespace {

// `.kb-r .k { border-radius:5px; padding:2px 7px }`.
constexpr int kCapPaddingH = 7;
constexpr int kCapPaddingV = 2;
constexpr int kCapGap = 4;
constexpr int kCapRadius = 5;
// A single-character cap is square-ish rather than narrow, so a row of
// "Ctrl  Shift  R" does not end in a sliver.
constexpr int kCapMinWidth = 20;

// `.kb-g { padding:14px 16px 6px }`, `.kb-r { padding:8px 16px; gap:14px }`.
constexpr int kListPaddingH = 16;
constexpr int kGroupPaddingTop = 14;
constexpr int kGroupPaddingBottom = 6;
constexpr int kRowPaddingV = 8;
constexpr int kRowGap = 14;

QFont capFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleMonoFont(context, TypeRole::Mono);
    font.setPixelSize(qMax(8, profile.type.xxs));
    return font;
}

QFont labelFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::Body);
    font.setPixelSize(profile.type.sm);
    return font;
}

QFont groupFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleMonoFont(context, TypeRole::Eyebrow);
    font.setPixelSize(qMax(8, profile.type.xxs - 1));
    font.setLetterSpacing(QFont::PercentageSpacing, 112);
    return font;
}

int capWidth(const QFontMetrics& fm, const QString& cap)
{
    return qMax(kCapMinWidth, fm.horizontalAdvance(cap) + 2 * kCapPaddingH);
}

int capsWidth(const QFontMetrics& fm, const QStringList& caps)
{
    int width = 0;
    for (const QString& cap : caps) {
        width += capWidth(fm, cap) + kCapGap;
    }
    return caps.isEmpty() ? 0 : width - kCapGap;
}

} // namespace

int paintKeyCaps(QPainter* painter, const QWidget* context, const QStringList& caps,
                 const QRect& box, int right)
{
    if (!painter || caps.isEmpty()) {
        return right;
    }
    const ThemeProfile& profile = themeFor(context);
    const QFont font = capFont(context);
    const QFontMetrics fm(font);
    const int height = fm.height() + 2 * kCapPaddingV;
    const double top = box.top() + (box.height() - height) / 2.0;

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setFont(font);

    // Laid out from the right, because that is the edge a chord is aligned to
    // and measuring forwards would need the total width first.
    int x = right;
    for (int i = caps.size() - 1; i >= 0; --i) {
        const int width = capWidth(fm, caps.at(i));
        const QRectF cap(x - width, top, width, height);
        painter->setPen(QPen(profile.colors.border, qMax(1, profile.border(BorderRole::Card))));
        painter->setBrush(profile.colors.windowBg);
        painter->drawRoundedRect(cap.adjusted(0.5, 0.5, -0.5, -0.5), kCapRadius, kCapRadius);
        painter->setPen(profile.colors.textSecondary);
        painter->drawText(cap, Qt::AlignCenter, caps.at(i));
        x -= width + kCapGap;
    }
    return x + kCapGap;
}

// == KeyCap =================================================================

KeyCap::KeyCap(const QString& keys, QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setKeys(keys);
}

KeyCap::KeyCap(const QKeySequence& sequence, QWidget* parent)
    : KeyCap(portableText(sequence), parent)
{
}

QStringList KeyCap::split(const QString& keys)
{
    const QString trimmed = keys.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }
    // A lone separator is a key: "+" on the numeric pad, and "Ctrl++" is a
    // real shortcut. Splitting on the character would produce an empty cap for
    // it, so single-character input is taken whole.
    if (trimmed.size() == 1) {
        return {trimmed};
    }

    QStringList caps;
    // A sequence ("Ctrl+K, Ctrl+S") is two chords. Each chord is drawn as its
    // own run of caps with the comma kept between them, because a reader needs
    // to see that the second half is pressed afterwards rather than together.
    const QStringList chords = trimmed.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (int chord = 0; chord < chords.size(); ++chord) {
        if (chord > 0) {
            caps.append(QStringLiteral(","));
        }
        for (const QString& part : chords.at(chord).split(QLatin1Char('+'))) {
            const QString piece = part.trimmed();
            if (!piece.isEmpty()) {
                caps.append(piece);
            }
        }
        // "Ctrl++" ends in an empty piece, which the loop above dropped.
        if (chords.at(chord).trimmed().endsWith(QLatin1Char('+'))) {
            caps.append(QStringLiteral("+"));
        }
    }
    return caps;
}

QString KeyCap::portableText(const QKeySequence& sequence)
{
    return sequence.toString(QKeySequence::NativeText);
}

void KeyCap::setKeys(const QString& keys)
{
    if (m_keys == keys) {
        return;
    }
    m_keys = keys;
    m_caps = split(keys);
    updateGeometry();
    update();
}

void KeyCap::setKeys(const QKeySequence& sequence)
{
    setKeys(portableText(sequence));
}

QSize KeyCap::sizeHint() const
{
    const QFontMetrics fm(capFont(this));
    return QSize(capsWidth(fm, m_caps), fm.height() + 2 * kCapPaddingV);
}

bool KeyCap::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        updateGeometry();
        update();
    }
    return QWidget::event(event);
}

void KeyCap::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    paintKeyCaps(&painter, this, m_caps, rect(), width());
}

// == ShortcutList ===========================================================

ShortcutList::ShortcutList(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

void ShortcutList::addGroup(const QString& title)
{
    Entry entry;
    entry.group = true;
    entry.label = title;
    m_entries.append(entry);
    updateGeometry();
    update();
}

void ShortcutList::addShortcut(const QString& label, const QString& keys)
{
    Entry entry;
    entry.label = label;
    entry.keys = keys;
    entry.caps = KeyCap::split(keys);
    m_entries.append(entry);
    updateGeometry();
    update();
}

void ShortcutList::addShortcut(const QString& label, const QKeySequence& sequence)
{
    addShortcut(label, KeyCap::portableText(sequence));
}

void ShortcutList::clear()
{
    m_entries.clear();
    updateGeometry();
    update();
}

int ShortcutList::rowCount() const
{
    int count = 0;
    for (const Entry& entry : m_entries) {
        if (!entry.group) {
            ++count;
        }
    }
    return count;
}

int ShortcutList::groupCount() const
{
    return m_entries.size() - rowCount();
}

void ShortcutList::setFilter(const QString& query)
{
    if (m_filter == query) {
        return;
    }
    m_filter = query;
    updateGeometry();
    update();
}

QVector<ShortcutList::Entry> ShortcutList::visibleEntries() const
{
    if (m_filter.trimmed().isEmpty()) {
        return m_entries;
    }
    const QString needle = m_filter.trimmed();

    QVector<Entry> result;
    for (const Entry& entry : m_entries) {
        if (entry.group) {
            // Held back rather than appended: a caption with nothing under it
            // is the thing that makes a filtered sheet look broken.
            result.append(entry);
            continue;
        }
        if (entry.label.contains(needle, Qt::CaseInsensitive)
            || entry.keys.contains(needle, Qt::CaseInsensitive)) {
            result.append(entry);
        }
    }

    // Drop every caption that no longer has a row after it.
    QVector<Entry> pruned;
    for (int i = 0; i < result.size(); ++i) {
        if (result.at(i).group
            && (i + 1 >= result.size() || result.at(i + 1).group)) {
            continue;
        }
        pruned.append(result.at(i));
    }
    return pruned;
}

int ShortcutList::entryHeight(const Entry& entry) const
{
    if (entry.group) {
        return QFontMetrics(groupFont(this)).height() + kGroupPaddingTop + kGroupPaddingBottom;
    }
    const QFontMetrics labels(labelFont(this));
    const QFontMetrics caps(capFont(this));
    return qMax(labels.height(), caps.height() + 2 * kCapPaddingV) + 2 * kRowPaddingV;
}

QSize ShortcutList::sizeHint() const
{
    const QVector<Entry> entries = visibleEntries();
    const QFontMetrics labels(labelFont(this));
    const QFontMetrics caps(capFont(this));

    int height = 0;
    int width = 0;
    for (const Entry& entry : entries) {
        height += entryHeight(entry);
        const int own = entry.group
            ? QFontMetrics(groupFont(this)).horizontalAdvance(entry.label.toUpper())
            : labels.horizontalAdvance(entry.label) + kRowGap + capsWidth(caps, entry.caps);
        width = qMax(width, own);
    }
    return QSize(width + 2 * kListPaddingH, height);
}

QSize ShortcutList::minimumSizeHint() const
{
    return QSize(220, sizeHint().height());
}

bool ShortcutList::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        updateGeometry();
        update();
    }
    return QWidget::event(event);
}

void ShortcutList::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const QVector<Entry> entries = visibleEntries();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QFont labels = labelFont(this);
    const QFont groups = groupFont(this);
    const QFontMetrics labelMetrics(labels);

    int y = 0;
    bool afterGroup = true;
    for (const Entry& entry : entries) {
        const int height = entryHeight(entry);
        const QRect box(kListPaddingH, y, width() - 2 * kListPaddingH, height);

        if (entry.group) {
            painter.setFont(groups);
            painter.setPen(profile.colors.textDisabled);
            painter.drawText(box.adjusted(0, kGroupPaddingTop, 0, -kGroupPaddingBottom),
                             Qt::AlignLeft | Qt::AlignVCenter, entry.label.toUpper());
            afterGroup = true;
            y += height;
            continue;
        }

        // `.kb-r { border-top:1px solid hair }` - a rule above every row except
        // the first one under a caption, so a group reads as a block.
        if (!afterGroup) {
            painter.setPen(QPen(profile.colors.borderSubtle,
                                qMax(1, profile.border(BorderRole::Divider))));
            painter.drawLine(kListPaddingH, y, width() - kListPaddingH, y);
        }
        afterGroup = false;

        const int capsLeft = paintKeyCaps(&painter, this, entry.caps, box, box.right());

        painter.setFont(labels);
        painter.setPen(profile.colors.textPrimary);
        const QRect textBox(box.left(), box.top(),
                            qMax(0, capsLeft - kRowGap - box.left()), box.height());
        painter.drawText(textBox, Qt::AlignLeft | Qt::AlignVCenter,
                         labelMetrics.elidedText(entry.label, Qt::ElideRight, textBox.width()));
        y += height;
    }
}

} // namespace novu::ui
