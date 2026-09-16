#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/Form.h"
#include "novu/ui/Layout.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/ThemeScope.h"
#include "novu/ui/TitleBar.h"
#include "novu/ui/WindowShell.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

// A title bar shown inside the page rather than on a window.
//
// The bar is the same component a real window gets - it is placed in a layout
// instead of in a frame, which is the whole reason it is a widget and not
// something the shell paints inline. What this preview cannot show is the
// resize band and the DWM corner radius, so the page also opens real windows
// below.
QWidget* barSample(QWidget* parent, const QString& title, bool minimizable,
                   bool maximizable, bool active)
{
    auto* bar = new TitleBar(parent);
    bar->setTitle(title);
    bar->setMinimizable(minimizable);
    bar->setMaximizable(maximizable);
    bar->setWindowActive(active);
    return bar;
}

// The reference window. One per page instance, reused: opening a second copy
// of the same reference is noise, and a raise() says more about window
// management than a new window does.
QPointer<AppWindow> s_referenceWindow;
QPointer<ToolWindow> s_referenceTool;

void openReferenceWindow(const QString& profileId)
{
    if (s_referenceWindow) {
        s_referenceWindow->raise();
        s_referenceWindow->activateWindow();
        return;
    }

    auto* window = new AppWindow;
    window->setAttribute(Qt::WA_DeleteOnClose);
    // The window is its own scope root: it is a top-level widget, so it has no
    // parent to inherit the gallery's profile from.
    ThemeScope::apply(window, profileId);
    window->setTitle(QStringLiteral("Novu reference window"));

    auto* body = new QWidget(window->content());
    QVBoxLayout* column = pageLayout(body);

    column->addWidget(label(QStringLiteral("A main application window"), body,
                            TypeRole::PageTitle));
    QLabel* intro = label(
        QStringLiteral("Everything above this line is NovuUI: the bar, the caption, the three "
                       "controls, the border and the corner radius. Drag the bar, double-click "
                       "it, drag an edge, maximize and restore - none of that is code this "
                       "window contains."),
        body, TypeRole::Body, Ink::Secondary);
    intro->setWordWrap(true);
    column->addWidget(intro);

    Card* card = new Card(QStringLiteral("Content"), body);
    card->setSubtitle(QStringLiteral("An ordinary page. It draws no title of its own."));
    card->body()->addWidget(label(
        QStringLiteral("A page inside a Novu window never builds a title bar. If it needs a "
                       "caption, the window already has one."),
        card, TypeRole::Body, Ink::Secondary));
    column->addWidget(card);
    column->addStretch(1);

    window->setContentWidget(body);
    window->shell()->setContentSize(QSize(720, 460));
    window->show();
    s_referenceWindow = window;
}

void openReferenceTool(const QString& profileId)
{
    if (s_referenceTool) {
        s_referenceTool->raise();
        s_referenceTool->activateWindow();
        return;
    }

    auto* window = new ToolWindow;
    window->setAttribute(Qt::WA_DeleteOnClose);
    ThemeScope::apply(window, profileId);
    window->setTitle(QStringLiteral("Detached panel"));

    auto* body = new QWidget(window->content());
    QVBoxLayout* column = pageLayout(body);
    QLabel* intro = label(
        QStringLiteral("A tool window resizes but neither minimizes nor maximizes. The bar is "
                       "the same height and the Close box is the same size as the main "
                       "window's - only two controls are absent."),
        body, TypeRole::Body, Ink::Secondary);
    intro->setWordWrap(true);
    column->addWidget(intro);
    column->addStretch(1);

    window->setContentWidget(body);
    window->shell()->setContentSize(QSize(420, 260));
    window->show();
    s_referenceTool = window;
}

// The reference dialog: a Settings-shaped window, modal, Close only.
void openReferenceDialog(QWidget* parent, const QString& profileId)
{
    Dialog dialog(QStringLiteral("Settings"), parent);
    ThemeScope::apply(&dialog, profileId);
    dialog.setModal(true);

    const ThemeProfile& profile = themeFor(&dialog);
    const int padding = profile.space(SpacingRole::CardPadding);
    dialog.contentLayout()->setContentsMargins(padding, padding, padding, padding);
    dialog.contentLayout()->setSpacing(profile.space(SpacingRole::SectionGap));

    QLabel* intro = label(
        QStringLiteral("A dialog is the same shell with Minimize and Maximize hidden. The bar "
                       "height, the caption baseline and the Close box are unchanged, which is "
                       "what makes a dialog read as part of the window behind it rather than "
                       "as a window from another product."),
        dialog.content(), TypeRole::Body, Ink::Secondary);
    intro->setWordWrap(true);
    dialog.contentLayout()->addWidget(intro);

    Card* card = new Card(QStringLiteral("A settings group"), dialog.content());
    card->body()->addWidget(label(QStringLiteral("Application content goes here."),
                                  card, TypeRole::Body, Ink::Secondary));
    dialog.contentLayout()->addWidget(card);
    dialog.contentLayout()->addStretch(1);

    Button* cancel = secondaryButton(QStringLiteral("Cancel"), dialog.content());
    Button* save = primaryButton(QStringLiteral("Save"), dialog.content());
    dialog.buttonRow()->addWidget(cancel);
    dialog.buttonRow()->addWidget(save);
    QObject::connect(cancel, &QAbstractButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(save, &QAbstractButton::clicked, &dialog, &QDialog::accept);

    dialog.shell()->setContentSize(QSize(460, 320));
    dialog.exec();
}

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    const ThemeProfile& profile = themeFor(page);
    const WindowTokens& tokens = profile.window;
    const QString profileId = ctx.profileId;

    // -- the three controls ------------------------------------------------
    {
        Card* card = exampleCard(page, QStringLiteral("Window controls"),
                                 QStringLiteral("One implementation, one bounding box. Close is "
                                                "neutral until the pointer is on it."));
        QVBoxLayout* body = card->body();
        QGridLayout* grid = matrix(body, card,
                                   {QStringLiteral("Minimize"), QStringLiteral("Maximize"),
                                    QStringLiteral("Restore"), QStringLiteral("Close")},
                                   {QStringLiteral("Active"), QStringLiteral("Inactive window")});

        const WindowControlButton::Control controls[] = {
            WindowControlButton::Control::Minimize,
            WindowControlButton::Control::Maximize,
            WindowControlButton::Control::Restore,
            WindowControlButton::Control::Close,
        };
        for (int row = 0; row < 2; ++row) {
            for (int col = 0; col < 4; ++col) {
                auto* button = new WindowControlButton(controls[col], card);
                button->setWindowActive(row == 0);
                matrixCell(grid, row, col, button);
            }
        }
        note(body, card,
             QStringLiteral("Hover Close: the plate turns danger and the cross turns white. "
                            "Hover the other two: the neutral surface-hover token. All four "
                            "are %1x%2 - the same box, always.")
                 .arg(tokens.controlWidth)
                 .arg(tokens.controlHeight));
        column->addWidget(card);
    }

    // -- capabilities ------------------------------------------------------
    {
        Card* card = exampleCard(page, QStringLiteral("Capabilities"),
                                 QStringLiteral("Hiding a control changes nothing else. This is "
                                                "the assertion the whole task rests on."));
        QVBoxLayout* body = card->body();

        body->addWidget(barSample(card, QStringLiteral("Main window - all three"), true, true, true));
        body->addWidget(barSample(card, QStringLiteral("Tool window - close only"), false, false, true));
        body->addWidget(barSample(card, QStringLiteral("Dialog - close only"), false, false, true));
        body->addWidget(barSample(card, QStringLiteral("Inactive window"), true, true, false));

        note(body, card,
             QStringLiteral("Every bar above is %1px tall with a %2px leading inset. The Close "
                            "box does not move or change size between them.")
                 .arg(tokens.titleBarHeight)
                 .arg(tokens.titleBarPadding));
        column->addWidget(card);
    }

    // -- long captions -----------------------------------------------------
    if (ctx.scenario == Scenario::LongText || ctx.scenario == Scenario::LongIdentifier) {
        Card* card = exampleCard(page, QStringLiteral("A caption that does not fit"),
                                 QStringLiteral("It elides. It never pushes a control off the "
                                                "frame."));
        const QString caption = ctx.scenario == Scenario::LongIdentifier
            ? QStringLiteral("J:/Github/NovuExplorer/src/ui/dialogs/"
                             "a4f9c1e2b7d84e0fa1c3d5e7b9081234.novuproject")
            : QStringLiteral("Import from a remote workspace - choose the source folder and the "
                             "destination category before continuing");
        card->body()->addWidget(barSample(card, caption, true, true, true));
        column->addWidget(card);
    }

    // -- real windows ------------------------------------------------------
    {
        Card* card = exampleCard(page, QStringLiteral("The real thing"),
                                 QStringLiteral("A bar in a page cannot show resizing, snapping "
                                                "or the corner radius. These open actual "
                                                "windows in the profile selected above."));
        QVBoxLayout* body = card->body();
        QHBoxLayout* row = exampleRow(body, card, QString());

        Button* windowButton = primaryButton(QStringLiteral("Open reference window"), card);
        QObject::connect(windowButton, &QAbstractButton::clicked, card,
                         [profileId] { openReferenceWindow(profileId); });
        row->addWidget(windowButton);

        Button* toolButton = secondaryButton(QStringLiteral("Open tool window"), card);
        QObject::connect(toolButton, &QAbstractButton::clicked, card,
                         [profileId] { openReferenceTool(profileId); });
        row->addWidget(toolButton);

        Button* dialogButton = secondaryButton(QStringLiteral("Open reference dialog"), card);
        QObject::connect(dialogButton, &QAbstractButton::clicked, card,
                         [card, profileId] { openReferenceDialog(card, profileId); });
        row->addWidget(dialogButton);

        Button* confirmButton = secondaryButton(QStringLiteral("Confirm (destructive)"), card);
        QObject::connect(confirmButton, &QAbstractButton::clicked, card, [card] {
            Dialog::confirm(card, QObject::tr("Delete workspace"),
                            QObject::tr("The workspace and its 12 saved layouts are removed. "
                                        "Files on disk are untouched."),
                            QObject::tr("Delete"), QObject::tr("Keep"), true);
        });
        row->addWidget(confirmButton);

        row->addStretch(1);
        note(body, card,
             QStringLiteral("Dialog::confirm() and Dialog::inform() exist so the last native "
                            "QMessageBox calls in an application have somewhere to go."));
        column->addWidget(card);
    }

    // -- tokens ------------------------------------------------------------
    {
        Card* card = exampleCard(page, QStringLiteral("Tokens in force"),
                                 QStringLiteral("Read from the selected profile. Nothing on this "
                                                "page names a number of its own."));
        QVBoxLayout* body = card->body();
        const QVector<QPair<QString, QString>> rows = {
            {QStringLiteral("titleBarHeight"), QString::number(tokens.titleBarHeight)},
            {QStringLiteral("controlWidth x controlHeight"),
             QStringLiteral("%1 x %2").arg(tokens.controlWidth).arg(tokens.controlHeight)},
            {QStringLiteral("titleBarPadding / titleBarGap"),
             QStringLiteral("%1 / %2").arg(tokens.titleBarPadding).arg(tokens.titleBarGap)},
            {QStringLiteral("borderWidth / resizeMargin"),
             QStringLiteral("%1 / %2").arg(tokens.borderWidth).arg(tokens.resizeMargin)},
            {QStringLiteral("iconSize / glyphSize"),
             QStringLiteral("%1 / %2").arg(tokens.iconSize).arg(tokens.glyphSize)},
            {QStringLiteral("radius(SurfaceRole::Window)"),
             QString::number(profile.radius(SurfaceRole::Window))},
            {QStringLiteral("radius(SurfaceRole::Dialog)"),
             QString::number(profile.radius(SurfaceRole::Dialog))},
            {QStringLiteral("titleBarBg"), tokens.titleBarBg.name()},
            {QStringLiteral("titleText / inactive"),
             QStringLiteral("%1 / %2").arg(tokens.titleText.name(),
                                           tokens.titleTextInactive.name())},
            {QStringLiteral("closeHoverBg / closePressedBg"),
             QStringLiteral("%1 / %2").arg(tokens.closeHoverBg.name(),
                                           tokens.closePressedBg.name())},
        };
        for (const auto& entry : rows) {
            propertyRow(body, card, entry.first, entry.second, Ink::Secondary, true);
        }
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec windowShellComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("window-shell");
    spec.name = QStringLiteral("WindowShell");
    spec.category = Category::TitleBar;
    spec.description = QStringLiteral(
        "The frame every Novu window and dialog wears: title bar, three controls, border, "
        "radius, drag and resize. Applications choose semantics, never chrome.");
    spec.headerPath = QStringLiteral("include/novu/ui/WindowShell.h");
    spec.sourcePath = QStringLiteral("src/WindowShell.cpp");
    spec.designedFor = {QStringLiteral("novuspace"),
                   QStringLiteral("novuai"),
                   QStringLiteral("novuexplorer"),
                   QStringLiteral("novuhub"),
                   QStringLiteral("novuvoice"),
                   QStringLiteral("novutrade")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("AppWindow"), QStringLiteral("ToolWindow"),
                     QStringLiteral("Dialog")};
    spec.states = {QStringLiteral("Active"), QStringLiteral("Inactive"),
                   QStringLiteral("Maximized"), QStringLiteral("Restored"),
                   QStringLiteral("Close hover")};
    spec.keywords = {QStringLiteral("window"), QStringLiteral("titlebar"),
                     QStringLiteral("chrome"), QStringLiteral("frame"),
                     QStringLiteral("dialog"), QStringLiteral("popup"),
                     QStringLiteral("minimize"), QStringLiteral("maximize"),
                     QStringLiteral("close"), QStringLiteral("frameless")};
    spec.properties = {
        {QStringLiteral("title"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("Caption and the OS window title together. Elides; never widens the "
                        "window.")},
        {QStringLiteral("icon"), QStringLiteral("QIcon"), {}, QString(),
         QStringLiteral("Application mark, drawn at the profile's icon size and the display's "
                        "pixel ratio.")},
        {QStringLiteral("resizable"), QStringLiteral("bool"), {}, QStringLiteral("true"),
         QStringLiteral("false for a fixed dialog. Also clears the OS maximize box, so Aero "
                        "Snap agrees with the bar.")},
        {QStringLiteral("minimizable"), QStringLiteral("bool"), {}, QStringLiteral("true"),
         QStringLiteral("Hides Minimize without moving anything else.")},
        {QStringLiteral("maximizable"), QStringLiteral("bool"), {}, QStringLiteral("true"),
         QStringLiteral("Hides Maximize and disables double-click on the bar.")},
        {QStringLiteral("kind"), QStringLiteral("enum"),
         {QStringLiteral("Main"), QStringLiteral("Tool"), QStringLiteral("Dialog")},
         QStringLiteral("Main"),
         QStringLiteral("Sets the default capabilities and the radius role.")},
    };
    spec.scenarios = {Scenario::Normal, Scenario::LongText, Scenario::LongIdentifier};
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
