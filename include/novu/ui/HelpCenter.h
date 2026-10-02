#pragma once

#include "novu/ui/Icon.h"

#include <QList>
#include <QString>
#include <QStringList>
#include <QWidget>

class QLabel;
class QLineEdit;
class QScrollArea;
class QVBoxLayout;

namespace novu::ui {

class Card;
class EmptyState;
class SelectButton;
class Splitter;

// The documentation window every Novusion application wears.
//
// Three columns, which is the shape NovuAI's Help Center settled on and the one
// this is lifted from: the library on the left (search, a category, the
// articles), the article in the middle, and what is in the article on the right
// (its contents, its tags). The reader learns one window and then knows all of
// them - which was not true while NovuAI had three panes, NovuSpace had a rail
// and a page, and NovuAdmin had a third arrangement of the same two things.
//
// ── what this class is not ───────────────────────────────────────────────
//
// It is not a documentation system. It holds no text of its own, reads no file
// and knows no application: it is handed articles and draws them. Where those
// come from - a provider per module in NovuSpace, one list in NovuExplorer, a
// backend in NovuAI - stays with the application, because that is the part
// where the applications genuinely differ.
//
// ── optional by default ──────────────────────────────────────────────────
//
// Read state and notes are capabilities and both start off. An application that
// cannot remember what you have read must not draw a progress bar that resets
// every time the window opens, and one with nowhere to put a note must not draw
// an empty Notes panel - a control that only ever says "not implemented" is
// worse than its absence. Tags and contents are different: they come from the
// article itself, so they appear exactly when an article has them.
class HelpCenterView : public QWidget
{
    Q_OBJECT

public:
    // One piece of an article's body. Enough shapes for documentation and
    // deliberately not more: this is prose with headings, lists and the
    // occasional command, not a document format.
    struct Block
    {
        enum class Kind {
            Heading,      // a section. These are what the Contents panel lists.
            Subheading,   // a step inside a section
            Paragraph,    // prose; limited rich text is allowed
            Bullets,      // `items`, one line each
            Code,         // a command or a path, in the mono face
            Shortcut,     // the keys that reach the thing just described
            Callout,      // one thing that matters more than the prose around it
        };

        Kind kind = Kind::Paragraph;
        QString text;
        QStringList items;
    };

    struct Article
    {
        QString id;
        QString title;
        QString summary;    // one line, under the title in the list
        QString category;   // what the category filter groups by
        QList<Block> blocks;
        QStringList tags;

        // Metadata, all optional. Each is drawn only when it is there, because
        // an application that does not record who wrote an article must not be
        // made to invent an author to fill a slot.
        int minutes = 0;    // 0: unknown
        QString updated;    // empty: unknown
        QString author;     // empty: unknown
    };

    explicit HelpCenterView(QWidget* parent = nullptr);

    // The library. Replacing it keeps the selected article when that article is
    // still in the new list, which is what makes a module switch in NovuSpace
    // land somewhere sensible instead of always on the first row.
    void setArticles(const QList<Article>& articles);
    QList<Article> articles() const { return m_articles; }

    QString currentArticleId() const { return m_currentId; }
    void setCurrentArticle(const QString& id);

    // Whether the application can remember what has been read. Off by default:
    // see the note above. With it on the list carries a state per row, the
    // reader carries "Mark as read", and readStateChanged() is emitted for the
    // application to store - this class keeps no file of its own.
    void setReadStateEnabled(bool enabled);
    bool isReadStateEnabled() const { return m_readState; }
    void setArticleRead(const QString& id, bool read);
    QStringList readArticles() const;

    // What the search field says it searches. The default names the library;
    // an application with one may say so.
    void setSearchPlaceholder(const QString& text);

signals:
    void articleOpened(const QString& id);
    void readStateChanged(const QString& id, bool read);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void buildChrome();
    void rebuildList();
    void rebuildReader();
    void rebuildContents();
    QList<Article> filtered() const;
    void applyResponsiveLayout();

    QList<Article> m_articles;
    QSet<QString> m_read;
    QString m_currentId;
    QString m_query;
    QString m_category;
    bool m_readState = false;

    Splitter* m_columns = nullptr;
    QWidget* m_listColumn = nullptr;
    QWidget* m_readerColumn = nullptr;
    QWidget* m_railColumn = nullptr;

    QLineEdit* m_search = nullptr;
    SelectButton* m_categories = nullptr;
    QVBoxLayout* m_listBody = nullptr;
    QScrollArea* m_listScroll = nullptr;
    QVBoxLayout* m_readerBody = nullptr;
    QScrollArea* m_readerScroll = nullptr;
    QVBoxLayout* m_railBody = nullptr;
    QWidget* m_readerFoot = nullptr;
};

} // namespace novu::ui
