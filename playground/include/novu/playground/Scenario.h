#pragma once

#include <QString>
#include <QVector>

namespace novu::playground {

// A situation a component has to survive, named once for the whole gallery.
//
// These are the shapes that break layouts in real applications: a table with
// no rows, a name that is forty characters of hexadecimal, an optional field
// the server did not send. A page declares which of them it understands; the
// gallery offers exactly those and nothing else, so a scenario selector never
// promises a state the component does not have.
enum class Scenario {
    Normal,
    Empty,
    SingleItem,
    ManyItems,
    LongText,
    LongIdentifier,
    MissingOptional,
    ErrorState,
    Disabled,
    LargeDataset,
};

// Stable across builds: the key is what the gallery puts on its command line
// when it restarts itself after a rebuild.
QString scenarioKey(Scenario scenario);
QString scenarioLabel(Scenario scenario);
QString scenarioDescription(Scenario scenario);

QVector<Scenario> allScenarios();

// Returns Normal for anything unrecognised — a stale command line from an
// older build must not stop the gallery from starting.
Scenario scenarioFromKey(const QString& key, bool* ok = nullptr);

} // namespace novu::playground
