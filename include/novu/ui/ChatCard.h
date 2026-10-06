#pragma once

#include "novu/ui/Export.h"

#include <QWidget>

class QLabel;
class QVBoxLayout;

namespace novu::ui {

class Badge;

// One turn in a conversation: who spoke, on what, when, and a body.
//
// WHO IS SPEAKING DECIDES THE WHOLE SHAPE, which is why it is a constructor
// argument rather than a setter. A person's turn is a card held against the
// right edge — short, quotable, visibly theirs. An agent's runs the full width
// of the column with the product's mark beside it and no plate at all, because
// an agent's turn is usually long and a bubble around four paragraphs of prose
// reads as a wall rather than as a remark. Giving both the same treatment is
// the single thing that makes a transcript hard to skim.
//
// WHAT THIS IS NOT. Streaming, tool calls, fenced code blocks and retries stay
// in the application: they are what each product's agent can actually do, and
// they differ between products. What repeats is the shell around a turn — the
// surface, the author line, the model badge, the timestamp — and that is this.
// Put the said content in body().
class NOVUUI_API ChatCard : public QWidget
{
    Q_OBJECT

public:
    enum class Author { User, Agent };

    explicit ChatCard(Author author, QWidget* parent = nullptr);
    ChatCard(Author author, const QString& name, QWidget* parent = nullptr);

    Author author() const { return m_author; }

    void setName(const QString& name);
    // The model the turn was produced on. Empty hides the badge — a person's
    // turn has no model, and an agent's does not always name one.
    void setModel(const QString& model);
    // Empty hides it rather than leaving a gap where a time used to be.
    void setTime(const QString& when);

    // Where the application puts what was actually said. Widgets added here
    // keep the card's own spacing, so a caller never sets margins by hand.
    QVBoxLayout* body() const { return m_body; }
    void addContent(QWidget* widget);

private:
    void build(const QString& name);

    Author m_author;
    QVBoxLayout* m_body = nullptr;
    QLabel* m_name = nullptr;
    Badge* m_model = nullptr;
    QLabel* m_time = nullptr;
};

} // namespace novu::ui
