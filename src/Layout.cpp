#include "novu/ui/Layout.h"
#include "novu/ui/Text.h"

#include "novu/ui/StyleSheet.h"
#include "novu/ui/StyleSpec.h"
#include "novu/ui/ThemeManager.h"

#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

namespace novu::ui {

namespace {

QLabel* roleLabel(QWidget* parent, const QString& text, TypeRole role, const QColor& colour)
{
    const ThemeProfile& profile = themeFor(parent);
    const TypeStyle style = profile.text(role);
    auto* label = new QLabel(text, parent);
    label->setFont(roleFont(label, role));
    QPalette palette = label->palette();
    palette.setColor(QPalette::WindowText, colour);
    label->setPalette(palette);
    return label;
}

} // namespace

namespace {

// The stretches applyActionAlignment() owns, marked so it can tell them from a
// stretch the caller added itself.
constexpr int kAlignmentStretch = 1;

bool isAlignmentStretch(QLayoutItem* item)
{
    auto* spacer = item ? item->spacerItem() : nullptr;
    return spacer && spacer->expandingDirections().testFlag(Qt::Horizontal);
}

} // namespace

void applyActionAlignment(QHBoxLayout* row, ActionAlignment alignment)
{
    if (!row) {
        return;
    }
    // Take out the ones we put in last time, so repeated calls do not stack
    // stretch upon stretch and squeeze the actions into nothing.
    for (int i = row->count() - 1; i >= 0; --i) {
        if (isAlignmentStretch(row->itemAt(i))) {
            delete row->takeAt(i);
        }
    }

    switch (alignment) {
    case ActionAlignment::Left:
        row->addStretch(kAlignmentStretch);
        break;
    case ActionAlignment::Center:
        row->insertStretch(0, kAlignmentStretch);
        row->addStretch(kAlignmentStretch);
        break;
    case ActionAlignment::Right:
        row->insertStretch(0, kAlignmentStretch);
        break;
    }
}

QHBoxLayout* toolbar(QBoxLayout* into)
{
    auto* row = new QHBoxLayout();
    if (into) {
        const ThemeProfile& profile = themeFor(into->parentWidget());
        row->setSpacing(profile.space(SpacingRole::ToolbarGap));
        row->setContentsMargins(0, 0, 0, 0);
        into->addLayout(row);
    }
    return row;
}

QVBoxLayout* pageLayout(QWidget* page)
{
    const ThemeProfile& profile = themeFor(page);
    auto* layout = new QVBoxLayout(page);
    const int padding = profile.space(SpacingRole::PagePadding);
    // `.page-body{padding:16px 20px 22px}` - three numbers, not one. Tighter
    // at the top because the page header is already above it; looser at the
    // bottom so the last card does not sit on the status bar. A profile that
    // states neither keeps the single padding it always had.
    const int top = profile.roleSpacing.value(
        static_cast<int>(SpacingRole::PagePaddingTop), profile.spacing.lg);
    const int bottom = profile.roleSpacing.value(
        static_cast<int>(SpacingRole::PagePaddingBottom), padding);
    layout->setContentsMargins(padding, top, padding, bottom);
    layout->setSpacing(profile.space(SpacingRole::CardGap));
    return layout;
}

void fillScrollViewport(QScrollArea* area, ColorRole role)
{
    if (!area) {
        return;
    }
    QWidget* viewport = area->viewport();
    QPalette palette = viewport->palette();
    palette.setColor(QPalette::Window, token(role).resolve(themeFor(area)));
    viewport->setPalette(palette);
    viewport->setAutoFillBackground(true);
}

QVBoxLayout* scrollPage(QWidget* page, QScrollArea** areaOut)
{
    const ThemeProfile& profile = themeFor(page);

    auto* outer = new QVBoxLayout(page);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // The only widget this file creates. A QScrollArea cannot scroll without
    // a viewport widget, so this container is mechanism rather than decoration
    // — and it is kept invisible so it adds no surface of its own.
    auto* area = new QScrollArea(page);
    // Named, so the scope stylesheet's QScrollArea rule reaches it. Without
    // this the area matched nothing, kept Qt's default palette and painted a
    // grey block inside every card that scrolls — in every application, not
    // just here. The library must not hand out a widget its own sheet cannot
    // address.
    area->setObjectName(QLatin1String(name::scrollArea));
    area->setWidgetResizable(true);
    area->setFrameShape(QFrame::NoFrame);
    // The page ground, painted: see fillScrollViewport().
    fillScrollViewport(area);

    auto* content = new QWidget(area);
    content->setAutoFillBackground(false);
    auto* layout = new QVBoxLayout(content);
    const int padding = profile.space(SpacingRole::PagePadding);
    layout->setContentsMargins(padding, padding, padding, padding);
    layout->setSpacing(profile.space(SpacingRole::CardGap));

    area->setWidget(content);
    outer->addWidget(area);
    if (areaOut) {
        *areaOut = area;
    }
    return layout;
}

QHBoxLayout* splitHeader(QBoxLayout* into, QWidget* parent, const QString& title,
                         const QString& subtitle, const QString& eyebrow)
{
    const ThemeProfile& profile = themeFor(parent);

    auto* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(profile.space(SpacingRole::ToolbarGap));

    auto* titles = new QVBoxLayout();
    titles->setContentsMargins(0, 0, 0, 0);
    titles->setSpacing(2);
    if (!eyebrow.isEmpty()) {
        // The one place the type scale's smallest step is used, in the accent
        // and uppercase. Bold because at 10px a normal weight in the accent
        // colour reads as an artefact rather than a label.
        QLabel* label = roleLabel(parent, eyebrow.toUpper(), TypeRole::Caption,
                                  profile.colors.accent);
        QFont font = label->font();
        font.setPixelSize(profile.type.xxs);
        font.setWeight(QFont::Bold);
        label->setFont(font);
        titles->addWidget(label);
    }
    titles->addWidget(roleLabel(parent, title, TypeRole::PageTitle, profile.colors.textPrimary));
    if (!subtitle.isEmpty()) {
        titles->addWidget(roleLabel(parent, subtitle, TypeRole::Caption, profile.colors.textDisabled));
    }
    row->addLayout(titles);
    row->addStretch(1);

    auto* actions = new QHBoxLayout();
    actions->setContentsMargins(0, 0, 0, 0);
    actions->setSpacing(profile.space(SpacingRole::ToolbarGap));
    row->addLayout(actions);

    if (into) {
        into->addLayout(row);
    }
    return actions;
}

} // namespace novu::ui
