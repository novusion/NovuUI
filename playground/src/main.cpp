// NovuUI Component Gallery — development entry point.
//
// This executable exists only in a developer's build tree. It is guarded by
// NOVUUI_BUILD_PLAYGROUND, which defaults to "NovuUI is the project being
// built", so an application that consumes NovuUI with add_subdirectory()
// never configures it, never compiles it, and never links it.

#include "novu/playground/ComponentRegistry.h"
#include "novu/playground/GalleryWindow.h"
#include "novu/playground/Paths.h"
#include "novu/playground/SampleData.h"
#include "novu/playground/ShadowRun.h"
#include "novu/playground/ThemeCatalog.h"

#include "novu/ui/ThemeManager.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDebug>

using namespace novu::playground;

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("NovuUI Component Gallery"));
    app.setApplicationVersion(QStringLiteral("0.1"));
    app.setOrganizationName(QStringLiteral("Novusion"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Live preview of the real NovuUI components."));
    parser.addHelpOption();
    parser.addVersionOption();

    // Written by the gallery itself when it restarts after a build, so a
    // rebuild lands back on the component the developer was looking at.
    QCommandLineOption componentOption({QStringLiteral("c"), QStringLiteral("component")},
                                       QStringLiteral("Component id to open."),
                                       QStringLiteral("id"));
    QCommandLineOption scenarioOption({QStringLiteral("s"), QStringLiteral("scenario")},
                                      QStringLiteral("Scenario key to select."),
                                      QStringLiteral("key"));
    QCommandLineOption profileOption({QStringLiteral("p"), QStringLiteral("profile")},
                                     QStringLiteral("Theme profile id for the preview."),
                                     QStringLiteral("id"));
    QCommandLineOption listOption(QStringLiteral("list"),
                                  QStringLiteral("Print the registry and exit."));
    QCommandLineOption profilesOption(QStringLiteral("profiles"),
                                      QStringLiteral("Print the theme profiles and exit."));
    // Set by the gallery on the copy it starts of itself; see ShadowRun.h.
    QCommandLineOption originOption(QStringLiteral("origin"),
                                    QStringLiteral("Build-tree executable a reload restarts."),
                                    QStringLiteral("path"));
    parser.addOption(componentOption);
    parser.addOption(scenarioOption);
    parser.addOption(profileOption);
    parser.addOption(listOption);
    parser.addOption(profilesOption);
    parser.addOption(originOption);
    parser.process(app);

    const bool headless = parser.isSet(listOption) || parser.isSet(profilesOption);

    // Hand over to a private copy before touching anything else, so the build
    // tree's executable is not locked by the process that wants to relink it.
    // The headless queries have nothing to rebuild, so they stay put.
    if (!headless) {
        if (parser.isSet(originOption)) {
            shadow::setOriginExecutable(parser.value(originOption));
        } else {
            QString error;
            if (shadow::relaunchFromCopy(app.arguments().mid(1), &error)) {
                return 0;
            }
            if (!error.isEmpty()) {
                // Not fatal: the gallery works, only Rebuild & reload will
                // fail to relink, and it reports that itself.
                qWarning() << "NovuUI gallery: running from the build tree —" << error;
            }
        }
    }

    SampleData data;
    data.reload();

    ThemeCatalog themes;
    themes.reload();

    ComponentRegistry& registry = ComponentRegistry::instance();
    registerBuiltinComponents(registry);

    if (parser.isSet(profilesOption)) {
        // The same list the Theme selector offers, so "it is not in the
        // dropdown" can be checked without opening a window.
        const QStringList builtins = ThemeCatalog::builtinProfileIds();
        for (const QString& id : ThemeCatalog::availableProfileIds()) {
            fprintf(stdout, "%-24s %-12s %s\n", qUtf8Printable(id),
                    builtins.contains(id) ? "built-in" : "overlay",
                    qUtf8Printable(ThemeCatalog::displayNameFor(id)));
        }
        for (const QString& error : themes.errors()) {
            fprintf(stderr, "overlay error: %s\n", qUtf8Printable(error));
        }
        return themes.errors().isEmpty() ? 0 : 1;
    }

    if (parser.isSet(listOption)) {
        // A headless view of the same registry the window renders, useful in a
        // terminal and in CI.
        for (const ComponentSpec& spec : registry.all()) {
            fprintf(stdout, "%-14s %-18s %-14s %s\n",
                    qUtf8Printable(spec.id), qUtf8Printable(spec.name),
                    qUtf8Printable(categoryLabel(spec.category)),
                    qUtf8Printable(spec.sourcePath));
        }
        return 0;
    }

    GalleryWindow window(registry, data, themes);
    window.restore(parser.value(componentOption), parser.value(scenarioOption),
                   parser.value(profileOption));
    window.show();

    return app.exec();
}
