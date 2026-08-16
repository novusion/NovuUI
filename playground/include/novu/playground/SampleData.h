#pragma once

#include "novu/playground/Scenario.h"

#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace novu::playground {

// Preview data, kept out of the pages.
//
// A page asks for "users" and gets rows; it never contains them. Two reasons,
// and the second is the one that matters:
//
//   1. A page with forty rows of literal JSON in it is a page nobody edits.
//   2. Data that lives in a file can be edited while the gallery is running.
//      A JSON change is a reload, not a rebuild — which is the only part of
//      the live-preview workflow C++ can honestly offer.
//
// This is development data. It is loaded from the playground's source tree,
// it is never installed, and no production target can reach it: the class
// itself lives in NovuUIPlaygroundLib, which nothing but the gallery and its
// tests link.
//
// Failure is a first-class result. A malformed file records an error and
// leaves the rest of the sets loaded — a broken fixture must degrade the one
// page that uses it, not take down the tool.
class SampleData
{
public:
    // An empty directory means paths::sampleDataDir().
    explicit SampleData(QString directory = QString());

    QString directory() const { return m_directory; }

    // True when the directory existed and every file in it parsed.
    bool reload();
    bool isLoaded() const { return m_loaded; }
    QStringList errors() const { return m_errors; }

    QStringList setNames() const;
    bool has(const QString& set) const;
    QString describe(const QString& set) const;

    // Rows exactly as they are on disk.
    QJsonArray rows(const QString& set) const;

    // Rows shaped for a scenario: emptied, truncated, multiplied, or with a
    // field replaced by something deliberately hostile to a layout. The
    // shaping lives here rather than in the pages so every data-driven
    // component is stressed the same way.
    QJsonArray rowsFor(const QString& set, Scenario scenario) const;

    // Flat string lists ("tags", "columns").
    QStringList values(const QString& set) const;

    // Named strings from the "text" set: long_text, long_identifier, ...
    QString text(const QString& key, const QString& fallback = QString()) const;

    // Fields a set marks as optional. MissingOptional removes these.
    QStringList optionalFields(const QString& set) const;

    // Number of rows LargeDataset expands to. Public because the TableView
    // page states it in its own caption rather than guessing.
    static int largeDatasetSize();

private:
    QJsonObject setObject(const QString& set) const;

    QString m_directory;
    QHash<QString, QJsonObject> m_sets;
    QStringList m_errors;
    bool m_loaded = false;
};

} // namespace novu::playground
