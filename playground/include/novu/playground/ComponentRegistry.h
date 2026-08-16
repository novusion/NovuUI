#pragma once

#include "novu/playground/ComponentSpec.h"

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

namespace novu::playground {

// The catalog of previewable components.
//
// Registration is explicit — registerBuiltinComponents() calls one function
// per page — rather than a static registrar object per translation unit. In a
// static library the linker is free to drop an object file nothing references,
// and a self-registering page would then vanish from the gallery in exactly
// the build configuration nobody checks. One line in Components.cpp is cheap
// and cannot fail silently.
//
// Adding a component is: write pages/FooPage.cpp returning a ComponentSpec,
// declare it in pages/Pages.h, add one line to Components.cpp, add the file
// to playground/CMakeLists.txt. No navigation code changes.
class ComponentRegistry
{
public:
    // The registry the running gallery uses. Tests construct their own.
    static ComponentRegistry& instance();

    // Refuses an invalid spec and a duplicate id, and says which — a gallery
    // that silently drops a component is worse than one that will not start.
    bool add(ComponentSpec spec, QString* error = nullptr);

    void clear();
    int count() const { return m_order.size(); }
    bool isEmpty() const { return m_order.isEmpty(); }

    // Category order first, then name. The navigation renders this directly.
    QVector<ComponentSpec> all() const;
    QVector<ComponentSpec> inCategory(Category category) const;
    QVector<Category> populatedCategories() const;

    QStringList ids() const;
    const ComponentSpec* find(const QString& id) const;

    // Empty query returns everything, in the same order as all(). Matches on
    // id, name, category, keywords, variants and states.
    QVector<ComponentSpec> search(const QString& query) const;

private:
    QHash<QString, ComponentSpec> m_specs;
    QStringList m_order;   // insertion order, before sorting
};

// Registers every component this build of NovuUI exposes. Called by main();
// tests call it against a registry of their own.
void registerBuiltinComponents(ComponentRegistry& registry);

} // namespace novu::playground
