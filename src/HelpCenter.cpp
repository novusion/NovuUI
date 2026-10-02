#include "novu/ui/HelpCenter.h"

#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/Chip.h"
#include "novu/ui/Feedback.h"
#include "novu/ui/Fields.h"
#include "novu/ui/FlowLayout.h"
#include "novu/ui/IconButton.h"
#include "novu/ui/Popover.h"
#include "novu/ui/Separator.h"
#include "novu/ui/Splitter.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSet>
#include <QTimer>
#include <QVBoxLayout>

namespace novu::ui {

namespace {

// The three columns, as the design fixes them. The reader takes whatever is
// left, which is the only one of the three whose width is a question.
constexpr int kListWidth = 300;
constexpr int kRailWidth = 238;
// Under this, the right-hand panel goes first: what is *in* the article matters
// less than the article. Under the second, the library goes too and the window
// is one column of prose. Measured against the window and not the screen, so a
// narrow window on a wide monitor behaves the same as a narrow monitor.
constexpr int kHideRailBelow = 820;
constexpr int kHideListBelow = 560;

// A row in the library. 44px against the reference's ~90: the same information
// in two tight lines instead of a card with a stack of its own inside it.
constexpr int kRowHeight = 44;

Card* surface(QWidget* parent, Card::Surface kind, QMargins padding = {0, 0, 0, 0},
              int gap = 0)
{
    auto* card = new Card(parent);
    card->setHeaderVisible(false);
    card->setSurface(kind);
    card->setBorderVisible(false);
    card->setRadiusStep(RadiusStep::None);
    card->setBodyMetrics(padding, gap);
    return card;
}

QScrollArea* scroller(QWidget* parent, QWidget* body)
{
    auto* area = new QScrollArea(parent);
    area->setFrameShape(QFrame::NoFrame);
    area->setWidgetResizable(true);
    area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    area->setWidget(body);
    // The viewport paints the window's own surface rather than QPalette::Base,
    // which is what put a plate behind every list that forgot to say so.
    area->viewport()->setAutoFillBackground(false);
    return area;
}

// The small mono line the design uses for metadata: category, minutes, date.
QLabel* meta(const QString& text, QWidget* parent)
{
    auto* line = label(text, parent, TypeRole::Mono, Ink::Muted);
    QFont font = line->font();
    font.setPointSizeF(10.0 * 72 / line->logicalDpiY());
    line->setFont(font);
    return line;
}

// What an article's metadata line says, skipping whatever the provider does
// not have. An article with none of it gets no line at all rather than a row
// of separators with nothing between them.
QString metaLine(const HelpCenterView::Article& article)
{
    QStringList parts;
    if (!article.category.isEmpty()) {
        parts << article.category;
    }
    if (article.minutes > 0) {
        parts << QStringLiteral("%1 min read").arg(article.minutes);
    }
    if (!article.author.isEmpty()) {
        parts << article.author;
    }
    if (!article.updated.isEmpty()) {
        parts << article.updated;
    }
    return parts.join(QString::fromUtf8(" · "));
}

bool matches(const HelpCenterView::Article& article, const QString& query)
{
    if (query.isEmpty()) {
        return true;
    }
    // Title, summary, category and tags - and the body, because the thing a
    // reader remembers is usually a phrase from inside an article rather than
    // its name.
    if (article.title.contains(query, Qt::CaseInsensitive)
        || article.summary.contains(query, Qt::CaseInsensitive)
        || article.category.contains(query, Qt::CaseInsensitive)) {
        return true;
    }
    for (const QString& tag : article.tags) {
        if (tag.contains(query, Qt::CaseInsensitive)) {
            return true;
        }
    }
    for (const HelpCenterView::Block& block : article.blocks) {
        if (block.text.contains(query, Qt::CaseInsensitive)) {
            return true;
        }
        for (const QString& item : block.items) {
            if (item.contains(query, Qt::CaseInsensitive)) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

HelpCenterView::HelpCenterView(QWidget* parent)
    : QWidget(parent)
    , m_category(QStringLiteral("all"))
{
    // A view of a whole window, not a widget on a page. Left at the default
    // policy it asks for the height its three columns' hints add up to and a
    // host layout without a stretch factor grants exactly that - which left
    // NovuAdmin's Help Center 180px tall in a 660px window, with the article
    // list cut off after two rows.
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    buildChrome();
}

void HelpCenterView::buildChrome()
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    m_columns = new Splitter(Qt::Horizontal, this);
    m_columns->setObjectName(QStringLiteral("help.columns"));
    m_columns->setHandleWidth(1);
    m_columns->setCollapseArrows(false);
    m_columns->setChildrenCollapsible(false);
    outer->addWidget(m_columns);

    // ── the library ──────────────────────────────────────────────────────
    auto* list = surface(m_columns, Card::Surface::Window);
    list->setObjectName(QStringLiteral("help.list"));
    m_listColumn = list;

    auto* searchWrap = surface(list, Card::Surface::Window, {9, 9, 9, 7});
    m_search = lineEdit(QStringLiteral("Search the documentation"), searchWrap);
    m_search->setFixedHeight(26);
    m_search->setObjectName(QStringLiteral("help.search"));
    searchWrap->body()->addWidget(m_search);
    list->body()->addWidget(searchWrap);
    connect(m_search, &QLineEdit::textChanged, this, [this](const QString& value) {
        m_query = value;
        rebuildList();
    });

    auto* filters = surface(list, Card::Surface::Window, {9, 0, 9, 8});
    m_categories = new SelectButton(filters);
    m_categories->setObjectName(QStringLiteral("help.category"));
    m_categories->setControlSize(ControlSize::Small);
    m_categories->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    filters->body()->addWidget(m_categories);
    list->body()->addWidget(filters);
    list->body()->addWidget(new Separator(Qt::Horizontal, list));
    connect(m_categories, &SelectButton::currentChanged, this,
            [this](const QString& key) {
                m_category = key;
                rebuildList();
            });

    auto* listBody = new QWidget(list);
    m_listBody = new QVBoxLayout(listBody);
    m_listBody->setContentsMargins(8, 7, 8, 10);
    m_listBody->setSpacing(2);
    m_listScroll = scroller(list, listBody);
    m_listScroll->setObjectName(QStringLiteral("help.articles"));
    list->body()->addWidget(m_listScroll, 1);
    m_columns->addWidget(list);

    // ── the article ──────────────────────────────────────────────────────
    auto* reader = surface(m_columns, Card::Surface::Plain);
    reader->setObjectName(QStringLiteral("help.reader"));
    m_readerColumn = reader;

    auto* readerBody = new QWidget(reader);
    m_readerBody = new QVBoxLayout(readerBody);
    m_readerBody->setContentsMargins(22, 16, 22, 24);
    m_readerBody->setSpacing(0);
    m_readerScroll = scroller(reader, readerBody);
    m_readerScroll->setObjectName(QStringLiteral("help.reader.body"));
    reader->body()->addWidget(m_readerScroll, 1);
    m_columns->addWidget(reader);

    // ── what is in the article ───────────────────────────────────────────
    auto* rail = surface(m_columns, Card::Surface::Window);
    rail->setObjectName(QStringLiteral("help.rail"));
    m_railColumn = rail;

    auto* railBody = new QWidget(rail);
    m_railBody = new QVBoxLayout(railBody);
    m_railBody->setContentsMargins(10, 10, 10, 10);
    m_railBody->setSpacing(10);
    rail->body()->addWidget(scroller(rail, railBody), 1);
    m_columns->addWidget(rail);

    list->setFixedWidth(kListWidth);
    rail->setFixedWidth(kRailWidth);
    m_columns->setStretchFactor(0, 0);
    m_columns->setStretchFactor(1, 1);
    m_columns->setStretchFactor(2, 0);
}

void HelpCenterView::setSearchPlaceholder(const QString& text)
{
    if (m_search) {
        m_search->setPlaceholderText(text);
    }
}

void HelpCenterView::setReadStateEnabled(bool enabled)
{
    if (m_readState == enabled) {
        return;
    }
    m_readState = enabled;
    rebuildList();
    rebuildReader();
}

void HelpCenterView::setArticleRead(const QString& id, bool read)
{
    if (read == m_read.contains(id)) {
        return;
    }
    if (read) {
        m_read.insert(id);
    } else {
        m_read.remove(id);
    }
    if (m_readState) {
        rebuildList();
        rebuildReader();
    }
}

QStringList HelpCenterView::readArticles() const
{
    QStringList out(m_read.cbegin(), m_read.cend());
    out.sort();
    return out;
}

void HelpCenterView::setArticles(const QList<Article>& articles)
{
    m_articles = articles;

    // The categories are the ones the content actually has. No application is
    // handed a fixed set to fill: NovuSpace's Web module and NovuExplorer have
    // nothing in common to group by, and pretending otherwise would leave both
    // with empty headings.
    const QString previousCategory = m_category;
    m_categories->clearOptions();
    m_categories->addOption(QStringLiteral("all"), QStringLiteral("All articles"));
    m_categories->menu()->setMeta(QStringLiteral("all"),
                                  QString::number(m_articles.size()));
    QStringList order;
    QHash<QString, int> counts;
    for (const Article& article : m_articles) {
        if (article.category.isEmpty()) {
            continue;
        }
        if (!order.contains(article.category)) {
            order.append(article.category);
        }
        ++counts[article.category];
    }
    for (const QString& category : order) {
        m_categories->addOption(category, category);
        m_categories->menu()->setMeta(category, QString::number(counts.value(category)));
    }
    m_category = order.contains(previousCategory) ? previousCategory
                                                  : QStringLiteral("all");
    m_categories->setCurrentKey(m_category);

    // Keep the open article when the new library still has it. A module switch
    // in NovuSpace replaces the whole library, so this normally falls through
    // to the first row - but a provider that merely refreshed itself should not
    // throw the reader back to the top.
    bool stillThere = false;
    for (const Article& article : m_articles) {
        if (article.id == m_currentId) {
            stillThere = true;
            break;
        }
    }
    if (!stillThere) {
        m_currentId.clear();
    }

    rebuildList();
    rebuildReader();
}

QList<HelpCenterView::Article> HelpCenterView::filtered() const
{
    QList<Article> rows;
    for (const Article& article : m_articles) {
        if (m_category != QLatin1String("all") && article.category != m_category) {
            continue;
        }
        if (!matches(article, m_query)) {
            continue;
        }
        rows.append(article);
    }
    return rows;
}

void HelpCenterView::setCurrentArticle(const QString& id)
{
    if (m_currentId == id) {
        return;
    }
    m_currentId = id;
    rebuildList();
    rebuildReader();
    if (!id.isEmpty()) {
        emit articleOpened(id);
    }
}

void HelpCenterView::rebuildList()
{
    while (QLayoutItem* item = m_listBody->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    const QList<Article> rows = filtered();
    m_categories->setMetaText(QString::number(rows.size()));

    if (rows.isEmpty()) {
        // Two different nothings, and they are not the same message. An empty
        // category is a fact about the library; an empty search is a fact about
        // what was typed, and only the second has anything to undo.
        auto* empty = m_query.isEmpty()
            ? new EmptyState(Glyph::Book, QStringLiteral("No articles here"),
                             QStringLiteral("This category has nothing in it yet."),
                             m_listBody->parentWidget())
            : new EmptyState(Glyph::Search, QStringLiteral("No results"),
                             QStringLiteral("No article mentions “") + m_query
                                 + QStringLiteral("”."),
                             m_listBody->parentWidget());
        empty->setTextMaximumWidth(kListWidth - 48);
        m_listBody->addWidget(empty);
        m_listBody->addStretch();
        return;
    }

    if (m_currentId.isEmpty()) {
        m_currentId = rows.first().id;
    }

    for (const Article& article : rows) {
        auto* row = new Card(m_listBody->parentWidget());
        row->setObjectName(QStringLiteral("help.article.") + article.id);
        row->setHeaderVisible(false);
        row->setSurface(Card::Surface::Window);
        row->setBodyMetrics({9, 0, 9, 0}, 0);
        row->setFixedHeight(kRowHeight);
        row->setInteractive(true);
        row->setHoverSurface(Card::Surface::Raised);
        row->setSelected(article.id == m_currentId);
        row->setSelectionOpacity(.11, .32);
        row->setBorderVisible(article.id == m_currentId);

        auto* line = new QHBoxLayout;
        line->setSpacing(9);
        line->setContentsMargins(0, 0, 0, 0);

        auto* text = new QVBoxLayout;
        text->setSpacing(1);
        text->addWidget(elidedLabel(article.title, row, TypeRole::Body,
                                    article.id == m_currentId ? Ink::Default
                                                              : Ink::Secondary,
                                    500));
        const QString second = article.summary.isEmpty() ? metaLine(article)
                                                         : article.summary;
        if (!second.isEmpty()) {
            text->addWidget(elidedLabel(second, row, TypeRole::Caption, Ink::Tertiary));
        }
        line->addLayout(text, 1);

        // The right edge carries only what the provider really has: minutes if
        // it counts them, and the read mark if the application remembers.
        if (article.minutes > 0) {
            line->addWidget(meta(QStringLiteral("%1m").arg(article.minutes), row), 0,
                            Qt::AlignVCenter);
        }
        if (m_readState && m_read.contains(article.id)) {
            auto* done = new IconButton(Glyph::Check, IconButton::Variant::Ghost, row);
            done->setFixedSize(16, 16);
            done->setGlyphSize(12);
            done->setAttribute(Qt::WA_TransparentForMouseEvents);
            done->setFocusPolicy(Qt::NoFocus);
            line->addWidget(done, 0, Qt::AlignVCenter);
        }
        row->body()->addLayout(line);

        const QString id = article.id;
        connect(row, &Card::clicked, this, [this, id] { setCurrentArticle(id); });
        m_listBody->addWidget(row);
    }
    m_listBody->addStretch();
}

void HelpCenterView::rebuildReader()
{
    while (QLayoutItem* item = m_readerBody->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    delete m_readerFoot;
    m_readerFoot = nullptr;

    const Article* current = nullptr;
    for (const Article& article : m_articles) {
        if (article.id == m_currentId) {
            current = &article;
            break;
        }
    }

    QWidget* host = m_readerBody->parentWidget();
    if (!current) {
        m_readerBody->addWidget(new EmptyState(
            Glyph::Book,
            m_articles.isEmpty() ? QStringLiteral("No documentation available")
                                 : QStringLiteral("Nothing open"),
            m_articles.isEmpty()
                ? QStringLiteral("This part of the application has not published any "
                                 "help yet.")
                : QStringLiteral("Pick an article on the left."),
            host));
        m_readerBody->addStretch();
        rebuildContents();
        return;
    }

    auto* title = paragraph(current->title, host, TypeRole::PageTitle, Ink::Default);
    setLineHeight(title, 1.35);
    m_readerBody->addWidget(title);

    const QString line = metaLine(*current);
    if (!line.isEmpty()) {
        m_readerBody->addSpacing(4);
        m_readerBody->addWidget(meta(line, host));
    }
    m_readerBody->addSpacing(10);
    m_readerBody->addWidget(new Separator(Qt::Horizontal, host));

    for (const Block& block : current->blocks) {
        switch (block.kind) {
        case Block::Kind::Heading:
        case Block::Kind::Subheading: {
            auto* heading = paragraph(block.text, host,
                                      block.kind == Block::Kind::Heading
                                          ? TypeRole::CardTitle
                                          : TypeRole::Body,
                                      Ink::Default);
            // Named so the Contents panel can scroll to it. The panel reads
            // these back out of the layout rather than keeping a second list
            // that could disagree with what is on screen.
            heading->setObjectName(QStringLiteral("help.heading.") + block.text);
            setLineHeight(heading, 1.45);
            m_readerBody->addSpacing(block.kind == Block::Kind::Heading ? 18 : 13);
            m_readerBody->addWidget(heading);
            break;
        }
        case Block::Kind::Paragraph: {
            auto* prose = paragraph(QString(), host, TypeRole::Body, Ink::Secondary);
            setLabelRichText(prose, block.text);
            setLineHeight(prose, 1.65);
            m_readerBody->addSpacing(9);
            m_readerBody->addWidget(prose);
            break;
        }
        case Block::Kind::Bullets: {
            m_readerBody->addSpacing(9);
            auto* items = new QVBoxLayout;
            items->setSpacing(5);
            items->setContentsMargins(0, 0, 0, 0);
            for (const QString& item : block.items) {
                auto* bulletLine = new QHBoxLayout;
                bulletLine->setSpacing(0);
                auto* bullet = label(QString::fromUtf8("•"), host, TypeRole::Body,
                                     Ink::Muted);
                bullet->setFixedWidth(18);
                bulletLine->addWidget(bullet, 0, Qt::AlignTop);
                auto* text = paragraph(QString(), host, TypeRole::Body, Ink::Secondary);
                setLabelRichText(text, item);
                setLineHeight(text, 1.6);
                bulletLine->addWidget(text, 1);
                items->addLayout(bulletLine);
            }
            m_readerBody->addLayout(items);
            break;
        }
        case Block::Kind::Shortcut: {
            // A key chord belongs with the thing it reaches, so it sits right
            // under the heading in the mono face rather than being collected
            // into a table at the end that nobody reads next to the prose.
            m_readerBody->addSpacing(6);
            auto* keys = label(block.text, host, TypeRole::Mono, Ink::Accent);
            m_readerBody->addWidget(keys);
            break;
        }
        case Block::Kind::Code: {
            m_readerBody->addSpacing(11);
            auto* code = new Card(host);
            code->setHeaderVisible(false);
            code->setSurface(Card::Surface::Raised);
            code->setBorderStyle(Qt::SolidLine, true);
            code->setBodyMetrics({12, 10, 12, 10}, 0);
            auto* text = paragraph(block.text, code, TypeRole::Mono, Ink::Secondary);
            setLineHeight(text, 1.6);
            code->body()->addWidget(text);
            m_readerBody->addWidget(code);
            break;
        }
        case Block::Kind::Callout: {
            m_readerBody->addSpacing(11);
            auto* callout = new Card(host);
            callout->setHeaderVisible(false);
            callout->setSurface(Card::Surface::Raised);
            callout->setBorderVisible(false);
            callout->setLeadingAccent(token(ColorRole::Accent, .5), 2);
            callout->setRadiusStep(RadiusStep::Small);
            callout->setBodyMetrics({13, 10, 12, 10}, 0);
            auto* text = paragraph(QString(), callout, TypeRole::Body, Ink::Secondary);
            setLabelRichText(text, block.text);
            setLineHeight(text, 1.6);
            callout->body()->addWidget(text);
            m_readerBody->addWidget(callout);
            break;
        }
        }
    }
    m_readerBody->addStretch();

    if (m_readState) {
        // Only with the capability. Without it there is no footer at all rather
        // than a disabled button saying the application cannot do this.
        auto* foot = surface(m_readerColumn, Card::Surface::Window, {15, 9, 15, 9});
        auto* row = new QHBoxLayout;
        row->setSpacing(10);
        const bool read = m_read.contains(current->id);
        row->addWidget(label(read ? QStringLiteral("You have read this.")
                                  : QStringLiteral("Not read yet."),
                             foot, TypeRole::Caption, Ink::Muted),
                       1);
        auto* mark = new Button(read ? QStringLiteral("Mark unread")
                                     : QStringLiteral("Mark as read"),
                                read ? Button::Variant::Outlined
                                     : Button::Variant::Primary,
                                foot);
        mark->setObjectName(QStringLiteral("help.mark"));
        mark->setControlSize(ControlSize::Small);
        mark->setGlyph(Glyph::Check);
        const QString id = current->id;
        connect(mark, &Button::clicked, this, [this, id, read] {
            setArticleRead(id, !read);
            emit readStateChanged(id, !read);
        });
        row->addWidget(mark);
        foot->body()->addLayout(row);

        auto* column = qobject_cast<QVBoxLayout*>(
            static_cast<Card*>(m_readerColumn)->body());
        column->addWidget(new Separator(Qt::Horizontal, m_readerColumn));
        column->addWidget(foot);
        m_readerFoot = foot;
    }

    rebuildContents();
    m_readerScroll->verticalScrollBar()->setValue(0);
}

void HelpCenterView::rebuildContents()
{
    while (QLayoutItem* item = m_railBody->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    const Article* current = nullptr;
    for (const Article& article : m_articles) {
        if (article.id == m_currentId) {
            current = &article;
            break;
        }
    }
    QWidget* host = m_railBody->parentWidget();
    if (!current) {
        m_railBody->addStretch();
        return;
    }

    // ── Contents ─────────────────────────────────────────────────────────
    //
    // From the article's own headings. An article with none - a short answer to
    // one question, which plenty of them are - gets no Contents panel rather
    // than an empty box with a title on it.
    QStringList headings;
    for (const Block& block : current->blocks) {
        if (block.kind == Block::Kind::Heading) {
            headings.append(block.text);
        }
    }
    if (!headings.isEmpty()) {
        auto* contents = new Card(QStringLiteral("Contents"), host);
        contents->setHeaderDivider(false);
        contents->setHeaderMetrics({14, 13, 14, 11}, 26);
        contents->setBodyMetrics({14, 0, 14, 13}, 2);
        int index = 0;
        for (const QString& heading : headings) {
            ++index;
            auto* row = surface(contents, Card::Surface::Plain, {6, 0, 6, 0});
            row->setObjectName(QStringLiteral("help.toc.") + QString::number(index));
            row->setFixedHeight(26);
            row->setInteractive(true);
            row->setHoverSurface(Card::Surface::Raised);
            auto* line = new QHBoxLayout;
            line->setSpacing(9);
            auto* number = meta(QString::number(index), row);
            number->setFixedWidth(16);
            line->addWidget(number);
            line->addWidget(elidedLabel(heading, row, TypeRole::Body, Ink::Tertiary), 1);
            row->body()->addLayout(line);
            // A real scroll, not a message saying it would be one. The heading
            // labels were named when the reader was built, so this finds the
            // widget actually on screen.
            const QString name = QStringLiteral("help.heading.") + heading;
            connect(row, &Card::clicked, this, [this, name] {
                QWidget* body = m_readerScroll->widget();
                if (!body) {
                    return;
                }
                if (QWidget* target = body->findChild<QWidget*>(name)) {
                    const int y = target->mapTo(body, QPoint(0, 0)).y();
                    m_readerScroll->verticalScrollBar()->setValue(qMax(0, y - 12));
                }
            });
            contents->body()->addWidget(row);
        }
        m_railBody->addWidget(contents, 0, Qt::AlignTop);
    }

    // ── Tags ─────────────────────────────────────────────────────────────
    //
    // Only what the provider supplies. Clicking one searches for it, which is
    // the one thing a tag can do here without a second filter model.
    if (!current->tags.isEmpty()) {
        auto* tags = new Card(QStringLiteral("Tags"), host);
        tags->setHeaderDivider(false);
        tags->setHeaderMetrics({14, 13, 14, 11}, 26);
        tags->setBodyMetrics({14, 0, 14, 13}, 6);
        auto* flowHost = new QWidget(tags);
        auto* flow = new FlowLayout(flowHost, 5);
        flow->setContentsMargins(0, 0, 0, 0);
        for (const QString& tag : current->tags) {
            auto* chip = new Chip(tag, flowHost, false);
            chip->setTagStyle(true);
            chip->setObjectName(QStringLiteral("help.tag.") + tag);
            connect(chip, &Chip::clicked, this, [this, tag] {
                m_search->setText(tag);
            });
            flow->addWidget(chip);
        }
        tags->body()->addWidget(flowHost);
        m_railBody->addWidget(tags, 0, Qt::AlignTop);
    }

    m_railBody->addStretch();
}

void HelpCenterView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    applyResponsiveLayout();
}

// Narrow windows drop panels in the order of what they are worth. The article
// is the window; the library is how you get to another one; the right-hand
// panel is a convenience inside the one you have. So the right goes first and
// the article never goes at all.
void HelpCenterView::applyResponsiveLayout()
{
    const int w = width();
    if (m_railColumn) {
        m_railColumn->setVisible(w >= kHideRailBelow);
    }
    if (m_listColumn) {
        m_listColumn->setVisible(w >= kHideListBelow);
    }
}

} // namespace novu::ui
