#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

namespace novu::playground {

// The profiles the gallery can preview, and the overlays that make theme
// editing a reload instead of a rebuild.
//
// This class holds no colours. Everything it offers comes out of the real
// novu::ui::ThemeManager — the built-in profiles are whatever the library
// registers for itself, and an overlay is applied by handing a ThemeProfile
// back to registerProfile(), which is the same call an application makes and
// the same call that repaints every live widget in the scope.
//
// An overlay file is a JSON patch on top of a base profile:
//
//     {
//       "id": "novuspace-contrast",
//       "displayName": "NovuSpace — high contrast",
//       "base": "novuspace",
//       "colors": { "accent": "#3DA9FC" },
//       "radii":  { "medium": 2 },
//       "spacing": { "md": 14 },
//       "type":   { "md": 13, "family": "Segoe UI" }
//     }
//
// Overlays may not take the id of a built-in profile. The gallery can be
// hosted in a process that also runs real product widgets, and an overlay
// that captured "novutrade" would restyle them. A refused overlay is
// reported, not applied.
class ThemeCatalog : public QObject
{
    Q_OBJECT

public:
    // An empty directory means paths::themeDir().
    explicit ThemeCatalog(QString directory = QString(), QObject* parent = nullptr);

    QString directory() const { return m_directory; }

    // Re-reads every overlay and re-registers it. Live: no rebuild, no
    // restart, no repopulation of the preview — registerProfile() sends a
    // ThemeChangeEvent to the widgets that are already on screen.
    bool reload();
    QStringList errors() const { return m_errors; }
    QStringList overlayIds() const { return m_overlayIds; }

    // Profile ids the gallery offers, built-ins and overlays together,
    // straight out of ThemeManager.
    static QStringList availableProfileIds();
    static QString displayNameFor(const QString& profileId);

    // Ids ThemeManager registers on its own, before any overlay.
    static QStringList builtinProfileIds();

private:
    QString m_directory;
    QStringList m_errors;
    QStringList m_overlayIds;
};

} // namespace novu::playground
