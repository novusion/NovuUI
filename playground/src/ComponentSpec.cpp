#include "novu/playground/ComponentSpec.h"

namespace novu::playground {

QString categoryLabel(Category category)
{
    switch (category) {
    case Category::Foundation:  return QStringLiteral("Foundation");
    case Category::TitleBar:    return QStringLiteral("Window title bar");
    case Category::Navigation:  return QStringLiteral("Navigation");
    case Category::Footer:      return QStringLiteral("Footer");
    case Category::Input:       return QStringLiteral("Input");
    case Category::DataDisplay: return QStringLiteral("Data display");
    case Category::Feedback:    return QStringLiteral("Feedback");
    }
    return QStringLiteral("Foundation");
}

QString categoryKey(Category category)
{
    switch (category) {
    case Category::Foundation:  return QStringLiteral("foundation");
    case Category::TitleBar:    return QStringLiteral("title-bar");
    case Category::Navigation:  return QStringLiteral("navigation");
    case Category::Footer:      return QStringLiteral("footer");
    case Category::Input:       return QStringLiteral("input");
    case Category::DataDisplay: return QStringLiteral("data-display");
    case Category::Feedback:    return QStringLiteral("feedback");
    }
    return QStringLiteral("foundation");
}

QVector<Category> allCategories()
{
    // Navigation order. Foundation first because a developer checking a token
    // change looks there before anything else. Title bar and Footer sit next
    // to Navigation because together they are the window's own scaffold - top
    // chrome, the nav surfaces inside it, bottom chrome - and a reviewer
    // auditing chrome against the approved HTML wants the three adjacent
    // rather than scattered among Input and DataDisplay.
    return {Category::Foundation, Category::TitleBar, Category::Navigation,
            Category::Footer, Category::Input, Category::DataDisplay,
            Category::Feedback};
}

bool ComponentSpec::isValid() const
{
    return !id.isEmpty() && !name.isEmpty() && build != nullptr;
}

QVector<Scenario> ComponentSpec::supportedScenarios() const
{
    // A page that declares nothing understands the one situation every page
    // understands. Returning an empty list here would give the gallery an
    // empty selector, which reads as a broken page rather than a simple one.
    if (scenarios.isEmpty()) {
        return {Scenario::Normal};
    }
    return scenarios;
}

bool ComponentSpec::supports(Scenario scenario) const
{
    return supportedScenarios().contains(scenario);
}

bool ComponentSpec::isUsedBy(const QString& applicationId) const
{
    return !applicationId.isEmpty() && usedBy.contains(applicationId);
}

bool ComponentSpec::isDesignedFor(const QString& applicationId) const
{
    if (applicationId.isEmpty()) {
        return false;
    }
    // Anything already in use is also, trivially, applicable - so the filter
    // cannot hide a component from the very application that consumes it.
    return designedFor.contains(applicationId) || usedBy.contains(applicationId);
}

bool ComponentSpec::matches(const QString& query) const
{
    const QString needle = query.trimmed();
    if (needle.isEmpty()) {
        return true;
    }

    const auto contains = [&needle](const QString& haystack) {
        return haystack.contains(needle, Qt::CaseInsensitive);
    };

    if (contains(id) || contains(name) || contains(description)
        || contains(categoryLabel(category))) {
        return true;
    }
    for (const QStringList* list : {&keywords, &variants, &states}) {
        for (const QString& entry : *list) {
            if (contains(entry)) {
                return true;
            }
        }
    }
    return false;
}

} // namespace novu::playground
