#pragma once

#include "novu/ui/ThemeProfile.h"

#include <QEvent>
#include <QHash>
#include <QObject>
#include <QString>

class QWidget;

namespace novu::ui {

// Sent to every widget in a scope when its profile changes or is edited.
// Components react by re-reading tokens: the ones that paint themselves just
// update(), the ones that cache a stylesheet rebuild it.
//
// A custom event rather than a signal because there is no registry of live
// components to connect to — and there must not be one, or every component
// would have to remember to unregister.
class ThemeChangeEvent : public QEvent
{
public:
    static QEvent::Type type();
    ThemeChangeEvent();
};

// Owns the profiles and decides which one a widget gets.
//
// Replaces the old single global theme. That global could not work once one
// process hosts several products at once — NovuSpace loads NovuTrade, NovuMail
// and NovuVault as modules into its own process, so setTheme() would repaint
// all of them together.
class ThemeManager : public QObject
{
    Q_OBJECT

public:
    static ThemeManager& instance();

    // Registers or replaces a profile. Replacing one that is in use
    // repaints every widget currently scoped to it — this is what makes live
    // token editing possible.
    void registerProfile(const ThemeProfile& profile);
    bool hasProfile(const QString& id) const;
    ThemeProfile profile(const QString& id) const;
    // Reference into the manager's own storage, so a caller may hold it for
    // the duration of a paint without copying a profile. Falls back to the
    // default profile for an unknown id.
    const ThemeProfile& profileRef(const QString& id) const;
    QStringList profileIds() const;

    // Used by widgets that are not inside any ThemeScope.
    void setDefaultProfileId(const QString& id);
    QString defaultProfileId() const;
    const ThemeProfile& defaultProfile() const;

    // Repaints and re-lays out every live widget scoped to `profileId`
    // (or everything, when empty). Called automatically by registerProfile();
    // exposed for tools that mutate a profile in place.
    void notifyThemeChanged(const QString& profileId = {});

signals:
    void profileChanged(const QString& profileId);

private:
    ThemeManager();

    QHash<QString, ThemeProfile> m_profiles;
    QString m_defaultProfileId;
};

// Resolves the profile that applies to `widget` by walking up its parents to
// the nearest ThemeScope. Falls back to the default profile.
//
// Resolution happens on every read rather than being cached in the widget:
// a widget can be reparented into another scope at any time, and a cache
// would then be silently wrong. The walk is a handful of pointer hops.
const ThemeProfile& themeFor(const QWidget* widget);

} // namespace novu::ui
