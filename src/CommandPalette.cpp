#include "novu/ui/CommandPalette.h"

#include "novu/ui/Fields.h"
#include "novu/ui/Keys.h"
#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QRegularExpression>
#include <QResizeEvent>

#include <algorithm>

namespace novu::ui {

namespace {

// `.palette { width:min(620px, 92%) }`, opened a fifth of the way down.
constexpr int kWidth = 620;
constexpr double kWidthFraction = 0.92;
constexpr double kTopFraction = 0.16;

constexpr int kInputHeight = 46;
constexpr int kRowHeight = 34;
constexpr int kHeadingHeight = 26;
constexpr int kListPadding = 6;
constexpr int kListMaxHeight = 380;

constexpr int kPaddingH = 14;
constexpr int kRowPaddingH = 10;
constexpr int kGlyph = 15;
constexpr int kGap = 10;

constexpr double kScrimAlpha = 0.5;
constexpr double kDisabledOpacity = 0.42;

// Ranking. A prefix beats a word start beats a scattered subsequence; without
// an order the top result - the one Enter runs - is whichever command happened
// to be registered first.
constexpr int kScorePrefix = 300;
constexpr int kScoreWordStart = 200;
constexpr int kScoreSubsequence = 100;
constexpr int kScoreGroupOnly = 20;

QFont rowFont(const QWidget* context, bool current)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::Body);
    font.setPixelSize(profile.type.sm);
    font.setWeight(current ? QFont::Medium : QFont::Normal);
    return font;
}

QFont headingFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleMonoFont(context, TypeRole::Eyebrow);
    font.setPixelSize(qMax(8, profile.type.xxs - 1));
    font.setLetterSpacing(QFont::PercentageSpacing, 112);
    return font;
}

// The scrim. Its own class so it can dim and swallow the click that dismisses.
class PaletteScrim : public QWidget
{
public:
    PaletteScrim(QWidget* host, CommandPalette* palette)
        : QWidget(host)
        , m_palette(palette)
    {
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QColor wash = themeFor(m_palette).colors.windowBg;
        wash.setAlphaF(kScrimAlpha);
        QPainter(this).fillRect(rect(), wash);
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        event->accept();
        if (m_palette) {
            m_palette->dismiss();
        }
    }

private:
    QPointer<CommandPalette> m_palette;
};

} // namespace

CommandPalette::CommandPalette(QWidget* parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAutoFillBackground(false);

    m_input = lineEdit(QObject::tr("Type a command…"), this);
    m_input->setFrame(false);
    connect(m_input, &QLineEdit::textChanged, this, [this] { refilter(); });
    // Return in the field runs the top match, which is the whole point of
    // typing three letters rather than opening a menu.
    connect(m_input, &QLineEdit::returnPressed, this, [this] { run(m_current); });
    // The field has the focus, so it sees the arrows first. Forwarding them
    // here is what keeps the caret in the box while the list moves.
    m_input->installEventFilter(this);

    hide();
}

void CommandPalette::setCommands(const QVector<Command>& commands)
{
    m_commands = commands;
    refilter();
}

void CommandPalette::addCommand(const Command& command)
{
    m_commands.append(command);
    refilter();
}

void CommandPalette::clear()
{
    m_commands.clear();
    refilter();
}

QString CommandPalette::filter() const
{
    return m_input ? m_input->text() : QString();
}

void CommandPalette::setFilter(const QString& text)
{
    if (m_input) {
        m_input->setText(text);   // triggers refilter through textChanged
    }
}

QString CommandPalette::currentName() const
{
    if (m_current < 0 || m_current >= m_matches.size()) {
        return QString();
    }
    return m_commands.at(m_matches.at(m_current)).name;
}

// == matching ===============================================================

bool CommandPalette::subsequenceMatch(const QString& needle, const QString& haystack)
{
    if (needle.isEmpty()) {
        return true;
    }
    int at = 0;
    for (const QChar character : haystack) {
        if (character.toLower() == needle.at(at).toLower()) {
            if (++at == needle.size()) {
                return true;
            }
        }
    }
    return false;
}

int CommandPalette::matchScore(const QString& needle, const Command& command)
{
    const QString trimmed = needle.trimmed();
    if (trimmed.isEmpty()) {
        return 1;
    }
    const QString name = command.name;
    const QString lowerName = name.toLower();
    const QString lowerNeedle = trimmed.toLower();

    // Shorter names win at the same rank: "New tab" should beat "New tab to the
    // right" for "new tab", and without the length term the order is arbitrary.
    const int brevity = qMax(0, 60 - name.size());

    if (lowerName.startsWith(lowerNeedle)) {
        return kScorePrefix + brevity;
    }

    // A word start: "device peer" for "dp", "New device" for "dev".
    for (const QString& word : name.split(QRegularExpression(QStringLiteral("[\\s\\-_/]+")),
                                          Qt::SkipEmptyParts)) {
        if (word.toLower().startsWith(lowerNeedle)) {
            return kScoreWordStart + brevity;
        }
    }

    // Initials: "ndp" for "New device peer". Checked before the general
    // subsequence so it outranks an accidental scatter.
    QString initials;
    for (const QString& word : name.split(QRegularExpression(QStringLiteral("[\\s\\-_/]+")),
                                          Qt::SkipEmptyParts)) {
        if (!word.isEmpty()) {
            initials.append(word.at(0));
        }
    }
    if (initials.toLower().startsWith(lowerNeedle)) {
        return kScoreWordStart + brevity;
    }

    if (subsequenceMatch(lowerNeedle, lowerName)) {
        return kScoreSubsequence + brevity;
    }
    // The group is searchable too - "view" should find everything under View -
    // but it ranks below anything the name itself matched.
    if (command.group.toLower().contains(lowerNeedle)) {
        return kScoreGroupOnly + brevity;
    }
    return 0;
}

void CommandPalette::refilter()
{
    const QString needle = filter().trimmed();

    QVector<QPair<int, int>> scored;   // (score, index)
    scored.reserve(m_commands.size());
    for (int i = 0; i < m_commands.size(); ++i) {
        const int score = matchScore(needle, m_commands.at(i));
        if (score > 0) {
            scored.append({score, i});
        }
    }

    // Stable, so commands of equal score keep the order the application gave
    // them - which is usually the order they appear in its menus.
    std::stable_sort(scored.begin(), scored.end(),
                     [](const QPair<int, int>& a, const QPair<int, int>& b) {
                         return a.first > b.first;
                     });

    m_matches.clear();
    m_rows.clear();
    for (const auto& entry : scored) {
        m_matches.append(entry.second);
    }

    // Headings only while nothing is typed. Once the list is ranked by
    // relevance, grouping it by menu would scatter the best matches down the
    // page under headings that no longer mean anything.
    QString group;
    for (int i = 0; i < m_matches.size(); ++i) {
        const Command& command = m_commands.at(m_matches.at(i));
        if (needle.isEmpty() && command.group != group) {
            group = command.group;
            Row heading;
            heading.heading = true;
            heading.group = group;
            m_rows.append(heading);
        }
        Row row;
        row.match = i;
        m_rows.append(row);
    }

    m_current = m_matches.isEmpty() ? -1 : 0;
    m_hovered = -1;

    int height = kInputHeight + 2 * kListPadding;
    for (const Row& row : m_rows) {
        height += row.heading ? kHeadingHeight : kRowHeight;
    }
    setFixedHeight(qMin(kInputHeight + kListMaxHeight, height));
    update();
}

// == presentation ===========================================================

void CommandPalette::present(QWidget* host)
{
    QWidget* window = host ? host->window() : nullptr;
    if (!window) {
        return;
    }
    m_host = window;
    setParent(window);

    if (!m_scrim) {
        m_scrim = new PaletteScrim(window, this);
    }
    m_scrim->setGeometry(window->rect());
    m_scrim->show();
    m_scrim->raise();

    if (m_input) {
        m_input->clear();   // a palette always opens empty
    }
    refilter();
    place();
    show();
    raise();
    if (m_input) {
        m_input->setFocus(Qt::OtherFocusReason);
    }
}

void CommandPalette::dismiss()
{
    if (!isVisible()) {
        return;
    }
    if (m_scrim) {
        m_scrim->hide();
    }
    hide();
    emit dismissed();
}

void CommandPalette::place()
{
    if (!m_host) {
        return;
    }
    const QRect host = m_host->rect();
    const int paletteWidth = qMin(kWidth, int(host.width() * kWidthFraction));
    // Near the top rather than centred: a palette is read from its first line
    // down, and a centred one puts that line in the middle of the screen while
    // the list grows downwards past the edge.
    move((host.width() - paletteWidth) / 2, int(host.height() * kTopFraction));
    resize(paletteWidth, height());
}

void CommandPalette::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (m_input) {
        m_input->setGeometry(kPaddingH, 0, qMax(0, width() - 2 * kPaddingH), kInputHeight);
    }
}

// == interaction ============================================================

QRect CommandPalette::rowRect(int visualRow) const
{
    if (visualRow < 0 || visualRow >= m_rows.size()) {
        return QRect();
    }
    int y = kInputHeight + kListPadding;
    for (int i = 0; i < visualRow; ++i) {
        y += m_rows.at(i).heading ? kHeadingHeight : kRowHeight;
    }
    const int rowHeight = m_rows.at(visualRow).heading ? kHeadingHeight : kRowHeight;
    return QRect(kListPadding, y, qMax(0, width() - 2 * kListPadding), rowHeight);
}

int CommandPalette::rowAt(const QPoint& position) const
{
    for (int i = 0; i < m_rows.size(); ++i) {
        if (!m_rows.at(i).heading && rowRect(i).contains(position)) {
            return i;
        }
    }
    return -1;
}

void CommandPalette::run(int matchIndex)
{
    if (matchIndex < 0 || matchIndex >= m_matches.size()) {
        return;
    }
    const Command& command = m_commands.at(m_matches.at(matchIndex));
    if (!command.enabled) {
        return;
    }
    // Dismissed before the signal: a handler that opens a dialog would
    // otherwise do so with the palette still over it.
    const QVariant data = command.data;
    dismiss();
    emit chosen(data);
}

bool CommandPalette::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        update();
    }
    return QWidget::event(event);
}

bool CommandPalette::eventFilter(QObject* watched, QEvent* event)
{
    // The line edit has the focus and sees the arrows first. Taking them here
    // is what keeps the caret in the box while the list moves - the behaviour
    // every palette has and none of them explains. Return is connected to
    // returnPressed instead, which the field delivers on its own.
    if (watched == m_input && event->type() == QEvent::KeyPress) {
        auto* key = static_cast<QKeyEvent*>(event);
        switch (key->key()) {
        case Qt::Key_Down:
        case Qt::Key_Up:
        case Qt::Key_Escape:
            keyPressEvent(key);
            return true;
        default:
            break;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void CommandPalette::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Down:
        if (!m_matches.isEmpty()) {
            m_current = (m_current + 1) % m_matches.size();
            update();
        }
        return;
    case Qt::Key_Up:
        if (!m_matches.isEmpty()) {
            m_current = (m_current - 1 + m_matches.size()) % m_matches.size();
            update();
        }
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        run(m_current);
        return;
    case Qt::Key_Escape:
        dismiss();
        return;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void CommandPalette::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    m_pressed = rowAt(event->position().toPoint());
    update();
}

void CommandPalette::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    const int pressed = m_pressed;
    m_pressed = -1;
    const int released = rowAt(event->position().toPoint());
    if (pressed >= 0 && released == pressed) {
        run(m_rows.at(pressed).match);
        return;
    }
    update();
}

void CommandPalette::mouseMoveEvent(QMouseEvent* event)
{
    const int row = rowAt(event->position().toPoint());
    if (row != m_hovered) {
        m_hovered = row;
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void CommandPalette::leaveEvent(QEvent* event)
{
    if (m_hovered != -1) {
        m_hovered = -1;
        update();
    }
    QWidget::leaveEvent(event);
}

// == appearance =============================================================

StyleSpec CommandPalette::styleSpec() const
{
    StyleSpec spec;
    spec.background = token(ColorRole::PanelBg);
    spec.border = token(ColorRole::Border);
    spec.foreground = token(ColorRole::TextSecondary);
    spec.hoverBackground = token(ColorRole::PanelBgAlt);
    spec.checkedBackground = token(ColorRole::Accent, 0.16);
    spec.checkedForeground = token(ColorRole::TextPrimary);
    spec.radius = SurfaceRole::Overlay;
    spec.borderWidth = BorderRole::Card;
    spec.type = TypeRole::Body;
    return spec;
}

ResolvedStyle CommandPalette::resolvedStyle() const
{
    return resolve(styleSpec(), this, height());
}

QSize CommandPalette::sizeHint() const
{
    return QSize(kWidth, height());
}

void CommandPalette::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);
    const ResolvedStyle style = resolvedStyle();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF card = QRectF(rect()).adjusted(style.borderWidth / 2.0,
                                                style.borderWidth / 2.0,
                                                -style.borderWidth / 2.0,
                                                -style.borderWidth / 2.0);
    painter.setPen(QPen(style.border, style.borderWidth));
    painter.setBrush(style.background);
    painter.drawRoundedRect(card, style.radius, style.radius);

    painter.setPen(QPen(profile.colors.borderSubtle, 1));
    painter.drawLine(0, kInputHeight, width(), kInputHeight);

    if (m_matches.isEmpty()) {
        QFont font = rowFont(this, false);
        painter.setFont(font);
        painter.setPen(profile.colors.textDisabled);
        painter.drawText(QRect(0, kInputHeight, width(), height() - kInputHeight),
                         Qt::AlignCenter, QObject::tr("No matching command"));
        return;
    }

    const QFont headings = headingFont(this);
    const QFontMetrics headingMetrics(headings);

    for (int i = 0; i < m_rows.size(); ++i) {
        const Row& row = m_rows.at(i);
        const QRect box = rowRect(i);
        if (box.bottom() > height()) {
            break;   // past the cap; the rest is not drawn
        }

        if (row.heading) {
            painter.setFont(headings);
            painter.setPen(profile.colors.textDisabled);
            painter.drawText(box.adjusted(kRowPaddingH, 0, 0, 0),
                             Qt::AlignLeft | Qt::AlignVCenter, row.group.toUpper());
            continue;
        }

        const Command& command = m_commands.at(m_matches.at(row.match));
        const bool current = row.match == m_current;
        const bool hot = i == m_hovered && !current;

        if (current || hot || i == m_pressed) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(current ? style.checkedBackground : style.hoverBackground);
            painter.drawRoundedRect(QRectF(box), profile.radii.small + 2,
                                    profile.radii.small + 2);
        }

        QColor ink = current ? style.checkedForeground : style.foreground;
        if (!command.enabled) {
            ink = withAlpha(ink, kDisabledOpacity);
        }

        int left = box.left() + kRowPaddingH;
        if (command.glyph != Glyph::None) {
            paintGlyph(&painter, command.glyph,
                       QRectF(left, box.center().y() - kGlyph / 2.0, kGlyph, kGlyph), ink);
            left += kGlyph + kGap;
        }

        int rightEdge = box.right() - kRowPaddingH;
        if (!command.shortcut.isEmpty()) {
            // The same chips the keyboard sheet and the tooltip draw: one
            // drawing, so a shortcut here and in the reference are the same
            // shape.
            rightEdge = paintKeyCaps(&painter, this, KeyCap::split(command.shortcut), box,
                                     rightEdge)
                - kGap;
        }

        // The group, on the right of the name, once the list is ranked and the
        // headings are gone - or a filtered list loses all sense of where its
        // commands came from.
        if (!filter().trimmed().isEmpty() && !command.group.isEmpty()) {
            painter.setFont(headings);
            const int groupWidth = headingMetrics.horizontalAdvance(command.group.toUpper());
            painter.setPen(profile.colors.textDisabled);
            painter.drawText(QRect(rightEdge - groupWidth, box.top(), groupWidth,
                                   box.height()),
                             Qt::AlignRight | Qt::AlignVCenter, command.group.toUpper());
            rightEdge -= groupWidth + kGap;
        }

        const QFont font = rowFont(this, current);
        const QFontMetrics fm(font);
        painter.setFont(font);
        painter.setPen(ink);
        const QRect textBox(left, box.top(), qMax(0, rightEdge - left), box.height());
        painter.drawText(textBox, Qt::AlignLeft | Qt::AlignVCenter,
                         fm.elidedText(command.name, Qt::ElideRight, textBox.width()));
    }
}

} // namespace novu::ui
