#include "novu/ui/ThemeManager.h"

#include "novu/ui/ThemeScope.h"

#include <QApplication>
#include <QEvent>
#include <QWidget>

namespace novu::ui {

QEvent::Type ThemeChangeEvent::type()
{
    static const QEvent::Type registered = static_cast<QEvent::Type>(QEvent::registerEventType());
    return registered;
}

ThemeChangeEvent::ThemeChangeEvent()
    : QEvent(type())
{
}

ThemeManager& ThemeManager::instance()
{
    static ThemeManager manager;
    return manager;
}

ThemeManager::ThemeManager()
{
    // The library is usable with no configuration: the canonical theme is
    // registered and selected before any consumer touches it.
    const ThemeProfile space = novuSpaceProfile();
    m_profiles.insert(space.id, space);
    const ThemeProfile trade = novuTradeProfile();
    m_profiles.insert(trade.id, trade);
    m_defaultProfileId = space.id;
}

void ThemeManager::registerProfile(const ThemeProfile& profile)
{
    if (profile.id.isEmpty()) {
        return;
    }
    m_profiles.insert(profile.id, profile);
    notifyThemeChanged(profile.id);
    emit profileChanged(profile.id);
}

bool ThemeManager::hasProfile(const QString& id) const
{
    return m_profiles.contains(id);
}

ThemeProfile ThemeManager::profile(const QString& id) const
{
    return m_profiles.value(id, m_profiles.value(m_defaultProfileId));
}

const ThemeProfile& ThemeManager::profileRef(const QString& id) const
{
    const auto it = m_profiles.constFind(id);
    return it == m_profiles.constEnd() ? defaultProfile() : *it;
}

QStringList ThemeManager::profileIds() const
{
    QStringList ids = m_profiles.keys();
    ids.sort();
    return ids;
}

void ThemeManager::setDefaultProfileId(const QString& id)
{
    if (!m_profiles.contains(id) || m_defaultProfileId == id) {
        return;
    }
    m_defaultProfileId = id;
    notifyThemeChanged();
    emit profileChanged(id);
}

QString ThemeManager::defaultProfileId() const
{
    return m_defaultProfileId;
}

const ThemeProfile& ThemeManager::defaultProfile() const
{
    // Guaranteed present: the constructor registers it and setDefaultProfileId
    // refuses ids that are not registered.
    return *m_profiles.constFind(m_defaultProfileId);
}

void ThemeManager::notifyThemeChanged(const QString& profileId)
{
    // Every live widget is asked to re-read its tokens. There is deliberately
    // no registry of themed components: a component that forgets to
    // unregister would crash the next repaint, and one that forgets to
    // register would silently stop following the theme.
    const auto widgets = QApplication::allWidgets();
    for (QWidget* widget : widgets) {
        if (!widget) {
            continue;
        }
        if (!profileId.isEmpty()) {
            const QString effective = ThemeScope::resolve(widget);
            const bool affected = effective == profileId
                || (effective.isEmpty() && m_defaultProfileId == profileId);
            if (!affected) {
                continue;
            }
        }
        ThemeChangeEvent event;
        QApplication::sendEvent(widget, &event);
        widget->update();
    }
}

const ThemeProfile& themeFor(const QWidget* widget)
{
    ThemeManager& manager = ThemeManager::instance();
    const QString scopeId = ThemeScope::resolve(widget);
    if (!scopeId.isEmpty() && manager.hasProfile(scopeId)) {
        // Straight out of the manager's storage. An earlier version returned a
        // reference to a thread_local copy, which would have made two nested
        // themeFor() calls alias the same object.
        return manager.profileRef(scopeId);
    }
    return manager.defaultProfile();
}

} // namespace novu::ui
