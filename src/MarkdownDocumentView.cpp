#include "novu/ui/MarkdownDocumentView.h"

#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QPalette>
#include <QResizeEvent>
#include <QTextBrowser>
#include <QTextDocument>

namespace novu::ui {

namespace {

QString css(const QColor& color)
{
    return color.name(QColor::HexRgb);
}

QString documentStyle(const ThemeProfile& profile)
{
    const ThemeColors& c = profile.colors;
    const int page = profile.space(SpacingRole::PagePadding);
    const int paragraph = profile.space(SpacingRole::ToolbarGap);
    const int inputRadius = profile.radius(SurfaceRole::Input);

    return QStringLiteral(
        "body { color:%1; background:transparent; font-family:'%2';"
        " font-size:%3px; line-height:1.58; }"
        "h1,h2,h3,h4 { color:%1; font-family:'%4'; font-weight:600; }"
        "h1 { font-size:%5px; margin:0 0 %6px 0; }"
        "h2 { font-size:%7px; margin:%8px 0 %6px 0; }"
        "h3 { font-size:%9px; margin:%8px 0 %6px 0; }"
        "h4 { font-size:%3px; margin:%8px 0 %6px 0; }"
        "p { margin:0 0 %10px 0; }"
        "ul,ol { margin:0 0 %10px %11px; padding-left:%12px; }"
        "li { margin:0 0 4px 0; }"
        "a { color:%13; text-decoration:none; }"
        "code { color:%14; background:%15; font-family:'%16';"
        " font-size:%17px; border-radius:4px; padding:2px 4px; }"
        "pre { color:%14; background:%18; border:1px solid %19;"
        " border-radius:%20px; padding:12px; margin:4px 0 %10px 0;"
        " white-space:pre-wrap; }"
        "pre code { background:transparent; padding:0; }"
        "blockquote { color:%21; background:%18; border-left:3px solid %13;"
        " margin:4px 0 %10px 0; padding:8px 12px; }"
        "hr { border:none; border-top:1px solid %19; margin:%8px 0; }"
        "table { border-collapse:collapse; margin:4px 0 %10px 0; }"
        "th,td { border:1px solid %19; padding:6px 9px; }"
        "th { color:%1; background:%18; font-weight:600; }")
        .arg(css(c.textPrimary), profile.type.family)
        .arg(profile.type.md)
        .arg(profile.type.displayFamily)
        .arg(profile.text(TypeRole::PageTitle).size)
        .arg(qMax(8, paragraph))
        .arg(profile.text(TypeRole::SectionTitle).size)
        .arg(qMax(14, page))
        .arg(profile.text(TypeRole::CardTitle).size)
        .arg(qMax(10, paragraph))
        .arg(qMax(16, page))
        .arg(qMax(14, page))
        .arg(css(c.accentLight), css(c.textSecondary), css(c.surfaceHover),
             profile.type.monoFamily)
        .arg(profile.type.sm)
        .arg(css(c.panelBgAlt), css(c.border), QString::number(inputRadius),
             css(c.textSecondary));
}

} // namespace

MarkdownDocumentView::MarkdownDocumentView(QWidget* parent)
    : QWidget(parent)
    , m_browser(new QTextBrowser(this))
{
    // A readable document measure. It still contracts with a resized window.
    m_browser->setFrameShape(QFrame::NoFrame);
    m_browser->setOpenExternalLinks(true);
    m_browser->viewport()->setAutoFillBackground(false);
    applyPresentation();
    layoutDocument();
}

void MarkdownDocumentView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    layoutDocument();
}

void MarkdownDocumentView::setMarkdown(const QString& markdown)
{
    m_markdown = markdown;
    applyPresentation();
}

void MarkdownDocumentView::setSearchPaths(const QStringList& paths)
{
    m_browser->setSearchPaths(paths);
}

void MarkdownDocumentView::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::StyleChange
        || event->type() == QEvent::PaletteChange
        || event->type() == QEvent::FontChange) {
        applyPresentation();
        layoutDocument();
    }
}

void MarkdownDocumentView::layoutDocument()
{
    const int padding = themeFor(this).space(SpacingRole::PagePadding);
    const int available = qMax(0, width() - 2 * padding);
    const int documentWidth = qMin(760, available);
    m_browser->setGeometry((width() - documentWidth) / 2, padding,
                           documentWidth, qMax(0, height() - 2 * padding));
}

void MarkdownDocumentView::applyPresentation()
{
    const ThemeProfile& profile = themeFor(this);
    const QFont bodyFont = roleFont(this, TypeRole::Body);
    QPalette palette = m_browser->palette();
    palette.setColor(QPalette::Text, profile.colors.textPrimary);
    palette.setColor(QPalette::WindowText, profile.colors.textPrimary);
    palette.setColor(QPalette::Link, profile.colors.accentLight);
    palette.setColor(QPalette::LinkVisited, profile.colors.accent);
    m_browser->setPalette(palette);
    m_browser->setFont(bodyFont);
    m_browser->document()->setDefaultFont(bodyFont);
    m_browser->document()->setDocumentMargin(0);
    m_browser->document()->setDefaultStyleSheet(documentStyle(profile));
    m_browser->setMarkdown(m_markdown);
}

} // namespace novu::ui
