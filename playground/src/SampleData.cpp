#include "novu/playground/SampleData.h"

#include "novu/playground/Paths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonValue>

namespace novu::playground {

namespace {

constexpr int kManyItems = 40;
// Large enough that a per-cell measurement is visible in the millisecond
// figure the TableView page prints, and small enough that the developer who
// selects it is not waiting on a window that has stopped responding.
// TableView re-measures every cell on every resize, which is the cost this
// number is chosen to expose rather than to hide.
constexpr int kLargeDataset = 2000;

// The two strings that break layouts, kept here rather than in the JSON so a
// scenario behaves identically for a set that forgot to supply them.
const char* kFallbackLongText =
    "A description long enough that a column which assumed a short value has to "
    "decide what it does about the rest of it, which is the decision this "
    "scenario exists to make visible.";
const char* kFallbackLongIdentifier =
    "7f3c1e9ab84d2f60c5a17e93b0d46f28ac59e17b3d820f4e6c91a5d7b8e30246";

// Which field of a row a long value is written into. Not a guess: the first
// string field is the one a Text column is sized from.
QString firstStringField(const QJsonObject& row)
{
    for (auto it = row.constBegin(); it != row.constEnd(); ++it) {
        if (it.value().isString()) {
            return it.key();
        }
    }
    return {};
}

QJsonArray repeat(const QJsonArray& rows, int target)
{
    if (rows.isEmpty()) {
        return rows;
    }
    QJsonArray result;
    for (int i = 0; i < target; ++i) {
        QJsonObject row = rows.at(i % rows.size()).toObject();
        // Suffixed so a duplicated row is recognisable as one, and so a table
        // does not look like it is rendering the same value repeatedly by
        // mistake.
        const QString field = firstStringField(row);
        if (!field.isEmpty() && i >= rows.size()) {
            row.insert(field, QStringLiteral("%1 #%2")
                                  .arg(row.value(field).toString())
                                  .arg(i + 1));
        }
        result.append(row);
    }
    return result;
}

} // namespace

SampleData::SampleData(QString directory)
    : m_directory(directory.isEmpty() ? paths::sampleDataDir() : std::move(directory))
{
}

int SampleData::largeDatasetSize()
{
    return kLargeDataset;
}

bool SampleData::reload()
{
    m_sets.clear();
    m_errors.clear();
    m_loaded = false;

    QDir dir(m_directory);
    if (!dir.exists()) {
        // Reported, not thrown. A gallery with no sample data still previews
        // every component that does not need any, and says why the rest are
        // empty.
        m_errors.append(QStringLiteral("sample data directory not found: %1").arg(m_directory));
        return false;
    }

    const QStringList files = dir.entryList({QStringLiteral("*.json")}, QDir::Files, QDir::Name);
    if (files.isEmpty()) {
        m_errors.append(QStringLiteral("no sample data files in %1").arg(m_directory));
        return false;
    }

    for (const QString& fileName : files) {
        const QString path = dir.filePath(fileName);
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            m_errors.append(QStringLiteral("%1: cannot be opened").arg(fileName));
            continue;
        }

        QJsonParseError parseError{};
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            m_errors.append(QStringLiteral("%1: %2 (offset %3)")
                                .arg(fileName, parseError.errorString())
                                .arg(parseError.offset));
            continue;
        }
        if (!document.isObject()) {
            m_errors.append(QStringLiteral("%1: top level is not an object").arg(fileName));
            continue;
        }

        const QJsonObject object = document.object();
        // The set name defaults to the file name, so a fixture that forgets
        // the field is still usable.
        const QString set = object.value(QStringLiteral("set")).toString(
            QFileInfo(fileName).completeBaseName());
        if (m_sets.contains(set)) {
            m_errors.append(QStringLiteral("%1: set \"%2\" is already defined").arg(fileName, set));
            continue;
        }
        m_sets.insert(set, object);
    }

    m_loaded = m_errors.isEmpty() && !m_sets.isEmpty();
    return m_loaded;
}

QJsonObject SampleData::setObject(const QString& set) const
{
    return m_sets.value(set);
}

QStringList SampleData::setNames() const
{
    QStringList names = m_sets.keys();
    names.sort();
    return names;
}

bool SampleData::has(const QString& set) const
{
    return m_sets.contains(set);
}

QString SampleData::describe(const QString& set) const
{
    return setObject(set).value(QStringLiteral("description")).toString();
}

QJsonArray SampleData::rows(const QString& set) const
{
    return setObject(set).value(QStringLiteral("rows")).toArray();
}

QStringList SampleData::values(const QString& set) const
{
    QStringList result;
    const QJsonArray array = setObject(set).value(QStringLiteral("values")).toArray();
    for (const QJsonValue& value : array) {
        result.append(value.toString());
    }
    return result;
}

QString SampleData::text(const QString& key, const QString& fallback) const
{
    const QJsonObject strings =
        setObject(QStringLiteral("text")).value(QStringLiteral("strings")).toObject();
    const QString value = strings.value(key).toString();
    if (!value.isEmpty()) {
        return value;
    }
    if (!fallback.isEmpty()) {
        return fallback;
    }
    if (key == QLatin1String("long_text")) {
        return QString::fromUtf8(kFallbackLongText);
    }
    if (key == QLatin1String("long_identifier")) {
        return QString::fromUtf8(kFallbackLongIdentifier);
    }
    return {};
}

QStringList SampleData::optionalFields(const QString& set) const
{
    QStringList result;
    const QJsonArray array = setObject(set).value(QStringLiteral("optional")).toArray();
    for (const QJsonValue& value : array) {
        result.append(value.toString());
    }
    return result;
}

QJsonArray SampleData::rowsFor(const QString& set, Scenario scenario) const
{
    const QJsonArray base = rows(set);

    switch (scenario) {
    case Scenario::Normal:
    case Scenario::Disabled:
        return base;

    case Scenario::Empty:
    case Scenario::ErrorState:
        // Error yields no rows for the same reason Empty does; what separates
        // them is what the page says about it, which is the page's business.
        return {};

    case Scenario::SingleItem:
        return base.isEmpty() ? base : QJsonArray{base.first()};

    case Scenario::ManyItems:
        return repeat(base, kManyItems);

    case Scenario::LargeDataset:
        return repeat(base, kLargeDataset);

    case Scenario::LongText:
    case Scenario::LongIdentifier: {
        const QString replacement = scenario == Scenario::LongText
            ? text(QStringLiteral("long_text"))
            : text(QStringLiteral("long_identifier"));
        QJsonArray result;
        for (const QJsonValue& value : base) {
            QJsonObject row = value.toObject();
            const QString field = firstStringField(row);
            if (!field.isEmpty()) {
                row.insert(field, replacement);
            }
            result.append(row);
        }
        return result;
    }

    case Scenario::MissingOptional: {
        const QStringList optional = optionalFields(set);
        QJsonArray result;
        for (const QJsonValue& value : base) {
            QJsonObject row = value.toObject();
            for (const QString& field : optional) {
                row.remove(field);
            }
            result.append(row);
        }
        return result;
    }
    }
    return base;
}

} // namespace novu::playground
