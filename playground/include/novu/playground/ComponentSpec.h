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
// A component is filed by what it is *for*, not by how it behaves.
//
// The two that are easiest to confuse, so they are written down:
//
//   Feedback  tells the person about the application - its state, or the
//             result or progress of something they did. Toast, Spinner,
//             ProgressBar, StatusLine, EmptyState, InfoBanner.
//   Help      explains the interface itself. Contextual guidance, offered
//             rather than reported. Tooltip, HelpOverlay.
//
// A tooltip appearing on hover is not feedback about anything: nothing
// happened, and the application has not changed. See docs/VOCABULARY.md.
enum class Category {
    Foundation,
    TitleBar,
    Navigation,
    Footer,
    Input,
    DataDisplay,
    Feedback,
    Help,
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

    // Which Novusion applications use this component, by theme profile id:
    // "novuspace", "novuai", "novuvoice", "novutrade". Empty means membership
    // has not been recorded yet, which is not the same as "nobody uses it".
    //
    // Declared here rather than derived, because most applications do not link
    // NovuUI yet - there is nothing to measure until they do. The list is
    // seeded from docs/ECOSYSTEM_AUDIT.md and becomes fact as each application
    // migrates. One place per component, which is what keeps it maintainable:
    // the gallery's filter reads this and holds no membership rules of its own.
    //
    // NovuExplorer and NovuHub have no profile of their own - they wear
    // "novuspace" - so their usage is recorded under that id.
    // MEASURED. Which applications include this component's header and name
    // one of its classes today, derived from their sources.
    //
    // This field is a fact and must stay one. It was previously a single list
    // that mixed "uses it" with "will use it", and the gallery then told anyone
    // who looked that NovuAI used twenty-six components while NovuAI did not
    // link the library at all. A catalogue that overstates adoption is worse
    // than one that says nothing, because it is the thing people quote.
    //
    // Empty is the normal state during extraction and means exactly what it
    // says: built, not yet adopted.
    QStringList usedBy;

    // INTENT. Which applications this component was extracted for, and is
    // expected to serve once they migrate.
    //
    // Seeded from the approved designs and from docs/EXTRACTION_AUDIT.md. This
    // is the useful list for navigating the catalogue - "show me what NovuSpace
    // needs" - and it is deliberately a different name from the one above so
    // that neither can be read as the other.
    QStringList designedFor;

    // True when nothing consumes this component yet.
    //
    // The design system asks for a generic component to be created here *before*
    // the page that needs it, so "no application uses it" is a legitimate state
    // rather than a defect - but it is indistinguishable from a spec whose
    // author forgot to record membership, and that one is a defect. Saying it
    // out loud keeps the two apart, and keeps the test that catches the second
    // one from having to allow the first.
    bool awaitingAdoption = false;

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

    // Membership, for the gallery's application filter. A component with no
    // recorded membership belongs to no application yet, so a filtered view
    // hides it rather than guessing.
    // Measured adoption. False for almost everything during extraction.
    bool isUsedBy(const QString& applicationId) const;

    // Stated applicability. What the gallery's application filter runs on,
    // because a filter over measured adoption would hide the whole library.
    bool isDesignedFor(const QString& applicationId) const;
};

} // namespace novu::playground
