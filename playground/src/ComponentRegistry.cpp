#include "novu/playground/ComponentRegistry.h"

#include <algorithm>

namespace novu::playground {

ComponentRegistry& ComponentRegistry::instance()
{
    static ComponentRegistry registry;
    return registry;
}

bool ComponentRegistry::add(ComponentSpec spec, QString* error)
{
    if (!spec.isValid()) {
        if (error) {
            *error = QStringLiteral("component \"%1\" is incomplete: id, name and a builder are required")
                         .arg(spec.id.isEmpty() ? spec.name : spec.id);
        }
        return false;
    }
    if (m_specs.contains(spec.id)) {
        // A duplicate id is not a merge. The second registration would win
        // silently and the first component would disappear from a gallery
        // that still listed the right number of entries.
        if (error) {
            *error = QStringLiteral("duplicate component id \"%1\"").arg(spec.id);
        }
        return false;
    }

    m_order.append(spec.id);
    m_specs.insert(spec.id, std::move(spec));
    return true;
}

void ComponentRegistry::clear()
{
    m_specs.clear();
    m_order.clear();
}

QVector<ComponentSpec> ComponentRegistry::all() const
{
    QVector<ComponentSpec> result;
    result.reserve(m_order.size());
    for (const QString& id : m_order) {
        result.append(m_specs.value(id));
    }

    const QVector<Category> order = allCategories();
    std::stable_sort(result.begin(), result.end(),
                     [&order](const ComponentSpec& a, const ComponentSpec& b) {
                         const int ia = order.indexOf(a.category);
                         const int ib = order.indexOf(b.category);
                         if (ia != ib) {
                             return ia < ib;
                         }
                         return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
                     });
    return result;
}

QVector<ComponentSpec> ComponentRegistry::inCategory(Category category) const
{
    QVector<ComponentSpec> result;
    for (const ComponentSpec& spec : all()) {
        if (spec.category == category) {
            result.append(spec);
        }
    }
    return result;
}

QVector<Category> ComponentRegistry::populatedCategories() const
{
    QVector<Category> result;
    for (const Category category : allCategories()) {
        for (const QString& id : m_order) {
            if (m_specs.value(id).category == category) {
                result.append(category);
                break;
            }
        }
    }
    return result;
}

QStringList ComponentRegistry::ids() const
{
    return m_order;
}

const ComponentSpec* ComponentRegistry::find(const QString& id) const
{
    const auto it = m_specs.constFind(id);
    return it == m_specs.constEnd() ? nullptr : &it.value();
}

QVector<ComponentSpec> ComponentRegistry::search(const QString& query,
                                                 const QString& applicationId) const
{
    QVector<ComponentSpec> result;
    for (const ComponentSpec& spec : all()) {
        if (!spec.matches(query)) {
            continue;
        }
        // Visibility only. The filter never touches how a component is built,
        // which scenario it gets or which profile it resolves against - a
        // filtered gallery and an unfiltered one render the same widget.
        // Applicability, not adoption. Filtering on measured usage would
        // empty the catalogue: almost nothing is consumed yet, and the point
        // of the filter is "show me what this application needs".
        if (!applicationId.isEmpty() && !spec.isDesignedFor(applicationId)) {
            continue;
        }
        result.append(spec);
    }
    return result;
}

} // namespace novu::playground
