#include "novu/playground/Scenario.h"

namespace novu::playground {

QString scenarioKey(Scenario scenario)
{
    switch (scenario) {
    case Scenario::Normal:          return QStringLiteral("normal");
    case Scenario::Empty:           return QStringLiteral("empty");
    case Scenario::SingleItem:      return QStringLiteral("single");
    case Scenario::ManyItems:       return QStringLiteral("many");
    case Scenario::LongText:        return QStringLiteral("long-text");
    case Scenario::LongIdentifier:  return QStringLiteral("long-id");
    case Scenario::MissingOptional: return QStringLiteral("missing-optional");
    case Scenario::ErrorState:      return QStringLiteral("error");
    case Scenario::Disabled:        return QStringLiteral("disabled");
    case Scenario::LargeDataset:    return QStringLiteral("large");
    }
    return QStringLiteral("normal");
}

QString scenarioLabel(Scenario scenario)
{
    switch (scenario) {
    case Scenario::Normal:          return QStringLiteral("Normal");
    case Scenario::Empty:           return QStringLiteral("Empty");
    case Scenario::SingleItem:      return QStringLiteral("Single item");
    case Scenario::ManyItems:       return QStringLiteral("Many items");
    case Scenario::LongText:        return QStringLiteral("Long text");
    case Scenario::LongIdentifier:  return QStringLiteral("Long identifier");
    case Scenario::MissingOptional: return QStringLiteral("Missing optional data");
    case Scenario::ErrorState:      return QStringLiteral("Error");
    case Scenario::Disabled:        return QStringLiteral("Disabled");
    case Scenario::LargeDataset:    return QStringLiteral("Large dataset");
    }
    return QStringLiteral("Normal");
}

QString scenarioDescription(Scenario scenario)
{
    switch (scenario) {
    case Scenario::Normal:
        return QStringLiteral("The data a page sees on a good day.");
    case Scenario::Empty:
        return QStringLiteral("Nothing to show. The component must say so, not collapse.");
    case Scenario::SingleItem:
        return QStringLiteral("One row. Exposes layouts that only work in a list.");
    case Scenario::ManyItems:
        return QStringLiteral("Enough rows to scroll and to wrap.");
    case Scenario::LongText:
        return QStringLiteral("A field far longer than the design assumed.");
    case Scenario::LongIdentifier:
        return QStringLiteral("An unbreakable token — a hash, a path, an order id.");
    case Scenario::MissingOptional:
        return QStringLiteral("Optional fields absent, as an API actually returns them.");
    case Scenario::ErrorState:
        return QStringLiteral("The load failed. The component shows why, not blankness.");
    case Scenario::Disabled:
        return QStringLiteral("Every control disabled.");
    case Scenario::LargeDataset:
        return QStringLiteral("Enough rows that a per-row measurement becomes visible.");
    }
    return {};
}

QVector<Scenario> allScenarios()
{
    return {Scenario::Normal,          Scenario::Empty,
            Scenario::SingleItem,      Scenario::ManyItems,
            Scenario::LongText,        Scenario::LongIdentifier,
            Scenario::MissingOptional, Scenario::ErrorState,
            Scenario::Disabled,        Scenario::LargeDataset};
}

Scenario scenarioFromKey(const QString& key, bool* ok)
{
    for (const Scenario scenario : allScenarios()) {
        if (scenarioKey(scenario) == key) {
            if (ok) {
                *ok = true;
            }
            return scenario;
        }
    }
    // Deliberately not an error: the key arrives on a command line written by
    // a previous build of this executable, and an unknown one must not stop
    // the gallery from opening.
    if (ok) {
        *ok = false;
    }
    return Scenario::Normal;
}

} // namespace novu::playground
