#include "novu/playground/ThemeCatalog.h"

#include "novu/playground/Paths.h"

#include "novu/ui/ThemeManager.h"
#include "novu/ui/ThemeProfile.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using novu::ui::ThemeManager;
using novu::ui::ThemeProfile;

namespace novu::playground {

namespace {

void applyColors(ThemeProfile& profile, const QJsonObject& colors)
{
    const auto set = [&colors](const QString& key, QColor& target) {
        const QString value = colors.value(key).toString();
        if (!value.isEmpty()) {
            const QColor parsed(value);
            if (parsed.isValid()) {
                target = parsed;
            }
        }
    };

    novu::ui::ThemeColors& c = profile.colors;
    set(QStringLiteral("windowBg"), c.windowBg);
    set(QStringLiteral("panelBg"), c.panelBg);
    set(QStringLiteral("panelBgAlt"), c.panelBgAlt);
    set(QStringLiteral("headerBg"), c.headerBg);
    set(QStringLiteral("surfaceHover"), c.surfaceHover);
    set(QStringLiteral("surfaceActive"), c.surfaceActive);
    set(QStringLiteral("border"), c.border);
    set(QStringLiteral("borderSubtle"), c.borderSubtle);
    set(QStringLiteral("borderHover"), c.borderHover);
    set(QStringLiteral("textPrimary"), c.textPrimary);
    set(QStringLiteral("textSecondary"), c.textSecondary);
    set(QStringLiteral("textDisabled"), c.textDisabled);
    set(QStringLiteral("textOnAccent"), c.textOnAccent);
    set(QStringLiteral("accent"), c.accent);
    set(QStringLiteral("accentLight"), c.accentLight);
    set(QStringLiteral("success"), c.success);
    set(QStringLiteral("warning"), c.warning);
    set(QStringLiteral("danger"), c.danger);
}

void applyInts(const QJsonObject& source, std::initializer_list<QPair<const char*, int*>> fields)
{
    for (const auto& field : fields) {
        const QJsonValue value = source.value(QLatin1String(field.first));
        if (value.isDouble()) {
            *field.second = value.toInt();
        }
    }
}

} // namespace

ThemeCatalog::ThemeCatalog(QString directory, QObject* parent)
    : QObject(parent)
    , m_directory(directory.isEmpty() ? paths::themeDir() : std::move(directory))
{
}

QStringList ThemeCatalog::builtinProfileIds()
{
    // Asked of the library rather than listed here. A profile NovuUI adds
    // appears in the gallery with no change to this file, and — more
    // importantly — an overlay can never be mistaken for one.
    QStringList ids;
    for (const novu::ui::ThemeProfile& profile : novu::ui::builtinProfiles()) {
        ids.append(profile.id);
    }
    return ids;
}

bool ThemeCatalog::reload()
{
    m_errors.clear();
    m_overlayIds.clear();

    QDir dir(m_directory);
    if (!dir.exists()) {
        // Not a failure worth reporting: overlays are optional and the
        // built-in profiles are always available.
        return true;
    }

    ThemeManager& manager = ThemeManager::instance();
    const QStringList builtins = builtinProfileIds();
    const QStringList files = dir.entryList({QStringLiteral("*.json")}, QDir::Files, QDir::Name);

    for (const QString& fileName : files) {
        QFile file(dir.filePath(fileName));
        if (!file.open(QIODevice::ReadOnly)) {
            m_errors.append(QStringLiteral("%1: cannot be opened").arg(fileName));
            continue;
        }

        QJsonParseError parseError{};
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            m_errors.append(QStringLiteral("%1: %2").arg(fileName, parseError.errorString()));
            continue;
        }
        const QJsonObject object = document.object();

        const QString id = object.value(QStringLiteral("id")).toString();
        if (id.isEmpty()) {
            m_errors.append(QStringLiteral("%1: no \"id\"").arg(fileName));
            continue;
        }
        if (builtins.contains(id)) {
            // The gallery can run in a process that also hosts real product
            // widgets. An overlay that captured "novutrade" would restyle
            // them, and preview data would have become production data.
            m_errors.append(QStringLiteral("%1: refusing to overwrite the built-in profile \"%2\"")
                                .arg(fileName, id));
            continue;
        }

        const QString baseId = object.value(QStringLiteral("base")).toString(
            manager.defaultProfileId());
        if (!manager.hasProfile(baseId)) {
            m_errors.append(QStringLiteral("%1: unknown base profile \"%2\"").arg(fileName, baseId));
            continue;
        }

        ThemeProfile profile = manager.profile(baseId);
        profile.id = id;
        profile.displayName = object.value(QStringLiteral("displayName")).toString(id);

        applyColors(profile, object.value(QStringLiteral("colors")).toObject());

        const QJsonObject radii = object.value(QStringLiteral("radii")).toObject();
        applyInts(radii, {{"none", &profile.radii.none},
                          {"xs", &profile.radii.xs},
                          {"small", &profile.radii.small},
                          {"medium", &profile.radii.medium},
                          {"large", &profile.radii.large},
                          {"xl", &profile.radii.xl}});

        const QJsonObject spacing = object.value(QStringLiteral("spacing")).toObject();
        applyInts(spacing, {{"xs", &profile.spacing.xs},
                            {"sm", &profile.spacing.sm},
                            {"md", &profile.spacing.md},
                            {"lg", &profile.spacing.lg},
                            {"xl", &profile.spacing.xl}});

        const QJsonObject type = object.value(QStringLiteral("type")).toObject();
        applyInts(type, {{"xs", &profile.type.xs},
                         {"sm", &profile.type.sm},
                         {"md", &profile.type.md},
                         {"lg", &profile.type.lg},
                         {"xl", &profile.type.xl}});
        const QString family = type.value(QStringLiteral("family")).toString();
        if (!family.isEmpty()) {
            profile.type.family = family;
        }
        const QString monoFamily = type.value(QStringLiteral("monoFamily")).toString();
        if (!monoFamily.isEmpty()) {
            profile.type.monoFamily = monoFamily;
        }

        const QJsonObject controls = object.value(QStringLiteral("controls")).toObject();
        applyInts(controls, {{"height", &profile.controls.height},
                             {"heightSmall", &profile.controls.heightSmall},
                             {"rowHeight", &profile.controls.rowHeight}});

        // The real call, on the real manager. Every live widget scoped to
        // this profile is repainted by it — which is what makes editing the
        // JSON a reload rather than a rebuild.
        manager.registerProfile(profile);
        m_overlayIds.append(id);
    }

    return m_errors.isEmpty();
}

QStringList ThemeCatalog::availableProfileIds()
{
    return ThemeManager::instance().profileIds();
}

QString ThemeCatalog::displayNameFor(const QString& profileId)
{
    const ThemeProfile profile = ThemeManager::instance().profile(profileId);
    return profile.displayName.isEmpty() ? profileId : profile.displayName;
}

} // namespace novu::playground
