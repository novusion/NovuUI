#include "novu/ui/ChatCard.h"

#include "novu/ui/AppMark.h"
#include "novu/ui/Badge.h"
#include "novu/ui/Card.h"
#include "novu/ui/Icon.h"
#include "novu/ui/Text.h"

#include <QBoxLayout>
#include <QLabel>

namespace novu::ui {

namespace {

// The air under a turn. Without it the next one arrives stuck to the bottom of
// this one and the transcript reads as one block rather than as an exchange.
constexpr int kTurnGap = 16;
// Between the mark and the turn it belongs to.
constexpr int kMarkGap = 11;
// The agent's mark: the product, not a party avatar. A turn by the application
// is marked by the application.
constexpr int kMarkSide = 22;
// A person's bubble is padded; an agent's column is not, because there is no
// plate to pad away from.
constexpr int kBubblePadH = 14;
constexpr int kBubblePadV = 10;

} // namespace

ChatCard::ChatCard(Author author, QWidget* parent)
    : ChatCard(author, QString(), parent)
{
}

ChatCard::ChatCard(Author author, const QString& name, QWidget* parent)
    : QWidget(parent)
    , m_author(author)
{
    build(name);
}

void ChatCard::build(const QString& name)
{
    const bool fromUser = m_author == Author::User;

    auto* outer = new QHBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, kTurnGap);
    outer->setSpacing(kMarkGap);

    if (fromUser) {
        // Held against the right edge rather than stretched: a person's turn
        // is as wide as what they said.
        outer->addStretch(1);
    } else {
        auto* mark = new AppMark(Glyph::None, this, AppMark::Size::Small);
        mark->setShape(AppMark::Shape::Gem);
        mark->setSideOverride(kMarkSide);
        // Pinned to the top of the turn, so a long answer does not leave its
        // mark floating halfway down the column.
        auto* hold = new QWidget(this);
        auto* holdColumn = new QVBoxLayout(hold);
        holdColumn->setContentsMargins(0, 2, 0, 0);
        holdColumn->addWidget(mark);
        holdColumn->addStretch(1);
        outer->addWidget(hold, 0, Qt::AlignTop);
    }

    QWidget* bubble = nullptr;
    if (fromUser) {
        auto* card = new Card(this);
        card->setSurface(Card::Surface::Raised);
        card->setBodyMetrics(QMargins(kBubblePadH, kBubblePadV, kBubblePadH, kBubblePadV), 6);
        m_body = card->body();
        bubble = card;
    } else {
        bubble = new QWidget(this);
        m_body = new QVBoxLayout(bubble);
        m_body->setContentsMargins(0, 0, 0, 0);
        m_body->setSpacing(6);
    }

    // The author line: name, what it ran on, when. One row for all three,
    // because they answer one question between them.
    auto* head = new QWidget(bubble);
    auto* row = new QHBoxLayout(head);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(8);

    m_name = label(name, head, TypeRole::Body, Ink::Default, true);
    row->addWidget(m_name);

    m_model = new Badge(QString(), Badge::Tone::Info, head);
    m_model->setSize(Badge::Size::Regular);
    m_model->hide();
    row->addWidget(m_model);

    m_time = label(QString(), head, TypeRole::Mono, Ink::Muted);
    m_time->hide();
    row->addWidget(m_time);

    row->addStretch(1);
    m_body->addWidget(head);

    outer->addWidget(bubble, fromUser ? 0 : 1);
}

void ChatCard::setName(const QString& name)
{
    setLabelText(m_name, name);
}

void ChatCard::setModel(const QString& model)
{
    m_model->setText(model);
    m_model->setVisible(!model.isEmpty());
}

void ChatCard::setTime(const QString& when)
{
    setLabelText(m_time, when);
    m_time->setVisible(!when.isEmpty());
}

void ChatCard::addContent(QWidget* widget)
{
    if (!widget) {
        return;
    }
    widget->setParent(m_body->parentWidget());
    m_body->addWidget(widget);
}

} // namespace novu::ui
