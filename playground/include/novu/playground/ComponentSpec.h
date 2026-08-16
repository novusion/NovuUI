#pragma once

#include "novu/playground/Scenario.h"

#include <QString>
#include <QStringList>
#include <QVector>

#include <functional>

class QWidget;

namespace novu::playground {

class SampleData;

// Where a component sits in the gallery's navigation. Categories are the
// design system's own vocabulary, not a folder listing: `Text` and `Layout`
// are Foundation even though they live beside `Button` in src/.
enum class Category {
    Foundation,
    Input,
    Navigation,
    DataDisplay,
    Feedback,
};

QString categoryLabel(Category category);
QString categoryKey(Category category);
QVector<Category> allCategories();

// One public property of a component, described rather than bound.
//
// Nothing reads this yet. It exists because a property inspector that has to
// be retrofitted onto a registry with no notion of properties means rewriting
// every page, and that is exactly the architecture this task was told to
// avoid. Declaring the surface now costs one struct; discovering later that
// there is nowhere to put it costs the sixteen pages.
struct PropertySpec
{
    QString name;          // "tone"
    QString type;          // "QString", "bool", "int", "enum"
    QStringList values;    // enumerators, when type == "enum"
    QString defaultValue;
    QString description;
};

// What a page is handed when it builds itself.
//
// A page receives its data and its situation; it does not reach for a
// singleton. That is what lets a test build every page against a fixture
// directory, and what will later let an inspector build one page twice with
// two different property sets.
struct PreviewContext
{
    QWidget* parent = nullptr;
    Scenario scenario = Scenario::Normal;
    const SampleData* data = nullptr;
    QString profileId;
};

// Builds the preview. Returns a widget parented to ctx.parent, or nullptr,
// which the gallery reports rather than crashing on.
using PreviewBuilder = std::function<QWidget*(const PreviewContext&)>;

// Everything the gallery knows about one previewable component.
//
// Deliberately a plain struct with a function member: the definition of a
// component, the example that demonstrates it, the data it demonstrates with
// and the theme it is drawn in stay four separate things. A page that reached
// into the navigation, or a registry entry that owned a widget, would fuse
// two of them and close off the inspector and the designer that come after.
struct ComponentSpec
{
    QString id;               // "table-view" — stable, used on the command line
    QString name;             // "TableView"
    Category category = Category::Foundation;
    QString description;      // one line, shown under the title

    // Repository-relative, so "Open Source" works from any checkout. The
    // tests assert both files exist: a spec pointing at a moved file is a
    // broken link nobody notices until they click it.
    QString headerPath;
    QString sourcePath;

    QStringList variants;     // "Primary", "Secondary", ...
    QStringList states;       // "Normal", "Hover", "Disabled", ...
    QStringList keywords;     // extra search terms not in the name
    QVector<PropertySpec> properties;

    // Scenarios this page actually implements. Empty means Normal only.
    QVector<Scenario> scenarios;

    PreviewBuilder build;

    bool isValid() const;
    bool supports(Scenario scenario) const;
    QVector<Scenario> supportedScenarios() const;

    // Name, id, category, keywords, variants and states, matched
    // case-insensitively on substrings.
    bool matches(const QString& query) const;
};

} // namespace novu::playground
