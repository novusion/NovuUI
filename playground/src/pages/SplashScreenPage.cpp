#include "Pages.h"

#include "novu/playground/ExampleLayout.h"

#include "novu/ui/Card.h"
#include "novu/ui/SplashScreen.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/ThemeScope.h"

#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <iterator>

using namespace novu::ui;

namespace novu::playground::pages {

namespace {

// The mockup's own five stages - a startup sequence a caller would report as
// it actually happens, kept here only because a still Gallery page cannot
// otherwise show what the status line and the track are for.
struct Stage
{
    int percent;
    const char* status;
};

const Stage kStages[] = {
    {12, "Preparing workspace..."},
    {34, "Restoring cached state..."},
    {58, "Starting visible module..."},
    {78, "Connecting local services..."},
    {94, "Finalizing interface..."},
};

QWidget* build(const PreviewContext& ctx)
{
    auto* page = new QWidget(ctx.parent);
    QVBoxLayout* column = previewPage(page);

    {
        Card* card = exampleCard(page, QStringLiteral("NovuSpace"),
                                 QStringLiteral("What every Novu application opens with, "
                                                "before it opens."));
        QVBoxLayout* body = card->body();

        auto* splash = new SplashScreen(card);
        splash->setTitle(QStringLiteral("NovuSpace"));
        splash->setSubtitle(
            QStringLiteral("Your private workspace for web, mail, vault and trade."));
        splash->setVersionLine(QStringLiteral("Version 0.6"));
        splash->setBuildLine(QStringLiteral("Build 748"));

        auto* timer = new QTimer(splash);
        timer->setInterval(1250);
        QObject::connect(timer, &QTimer::timeout, splash, [splash, index = 0]() mutable {
            index = (index + 1) % static_cast<int>(std::size(kStages));
            const Stage& stage = kStages[index];
            splash->setStatusText(QString::fromLatin1(stage.status));
            splash->setPercent(stage.percent);
        });
        splash->setStatusText(QString::fromLatin1(kStages[0].status));
        splash->setPercent(kStages[0].percent);
        timer->start();

        body->addWidget(splash, 0, Qt::AlignHCenter);

        note(body, card,
             QStringLiteral("Cycling through the mockup's own five stages so the status "
                            "line and the track have something to show - a real launch "
                            "calls setStatusText()/setPercent() as it actually happens, "
                            "not on a timer this component runs itself."));
        column->addWidget(card);
    }

    {
        Card* card = exampleCard(page, QStringLiteral("A different application"),
                                 QStringLiteral("The name, the tagline and the version "
                                                "change; the shape and the mark's own "
                                                "accent do not."));
        QVBoxLayout* body = card->body();

        auto* splash = new SplashScreen(card);
        ThemeScope::apply(splash, QStringLiteral("novuai"));
        splash->setTitle(QStringLiteral("NovuAI"));
        splash->setSubtitle(QStringLiteral("Your local models, one console."));
        splash->setVersionLine(QStringLiteral("Version 1.2"));
        splash->setBuildLine(QStringLiteral("Build 214"));
        splash->setStatusText(QStringLiteral("Loading local model..."));
        splash->setPercent(61);

        body->addWidget(splash, 0, Qt::AlignHCenter);
        column->addWidget(card);
    }

    column->addStretch(1);
    return page;
}

} // namespace

ComponentSpec splashScreenComponent()
{
    ComponentSpec spec;
    spec.id = QStringLiteral("splash-screen");
    spec.name = QStringLiteral("SplashScreen");
    spec.category = Category::Foundation;
    spec.description = QStringLiteral(
        "The window every Novu application opens before it opens: the family mark, the "
        "product's own name and tagline, a version/build pair, and a status line over a "
        "progress track.");
    spec.headerPath = QStringLiteral("include/novu/ui/SplashScreen.h");
    spec.sourcePath = QStringLiteral("src/SplashScreen.cpp");
    spec.designedFor = {QStringLiteral("novuspace"), QStringLiteral("novuai"),
                        QStringLiteral("novuexplorer"), QStringLiteral("novuadmin")};
    spec.usedBy = {};
    spec.variants = {QStringLiteral("Normal")};
    spec.states = {QStringLiteral("0%"), QStringLiteral("Mid-progress"),
                   QStringLiteral("Complete")};
    spec.keywords = {QStringLiteral("splash"), QStringLiteral("splash screen"),
                     QStringLiteral("startup"), QStringLiteral("boot"),
                     QStringLiteral("loading"), QStringLiteral("progress")};
    spec.properties = {
        {QStringLiteral("title"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("The application's own name - changes per application.")},
        {QStringLiteral("subtitle"), QStringLiteral("QString"), {}, QString(),
         QStringLiteral("One line under the name.")},
        {QStringLiteral("percent"), QStringLiteral("int"), {}, QStringLiteral("0"),
         QStringLiteral("0-100. Drives the track's fill and the percent label together.")},
    };
    spec.build = build;
    return spec;
}

} // namespace novu::playground::pages
