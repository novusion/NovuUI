#pragma once

#include <QWidget>
#include <QStringList>

class QEvent;
class QResizeEvent;
class QTextBrowser;

namespace novu::ui {

// Canonical presentation for a Markdown document.
//
// This is deliberately a document surface rather than a general HTML view:
// author-owned HTML keeps its own CSS, while Markdown needs product typography,
// spacing and code treatment.  The component owns those decisions so previews,
// help surfaces and future document readers cannot grow separate themes.
class MarkdownDocumentView : public QWidget
{
public:
    explicit MarkdownDocumentView(QWidget* parent = nullptr);

    void setMarkdown(const QString& markdown);
    void setSearchPaths(const QStringList& paths);
    QString markdown() const { return m_markdown; }

protected:
    void changeEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void applyPresentation();
    void layoutDocument();

    QTextBrowser* m_browser = nullptr;
    QString m_markdown;
};

} // namespace novu::ui
