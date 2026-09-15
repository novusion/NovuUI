#include "novu/ui/Text.h"

#include <QEvent>

#include "novu/ui/ThemeManager.h"

#include <QLabel>
#include <QWidget>

namespace novu::ui {

QColor inkColor(const QWidget* context, Ink ink)
{
    const ThemeProfile& profile = themeFor(context);
    const ThemeColors& c = profile.colors;
    const SemanticPalette& s = profile.semantic;
    switch (ink) {
    case Ink::Secondary: return c.textSecondary;
    case Ink::Muted:     return c.textDisabled;
    case Ink::Accent:    return c.accent;
    // Through the semantic palette, not the raw hues: status text and a status
    // badge are the same statement, so they resolve through the same table.
    // Info was accentLight here as well - the application accent standing in
    // for a status.
    case Ink::Success:   return s.success.text;
    case Ink::Warning:   return s.warning.text;
    case Ink::Danger:    return s.danger.text;
    case Ink::Info:      return s.info.text;
    case Ink::Default:
    default:             return c.textPrimary;
    }
}

namespace {

// The roles the display face is for: a title, and nothing that is not a title.
// Kept here rather than as a flag on TypeStyle because it is a property of what
// the role *is*, and a profile that had to say "and this one is a title too"
// for every role would eventually disagree with itself.
bool isDisplayRole(TypeRole role)
{
    switch (role) {
    case TypeRole::PageTitle:
    case TypeRole::SectionTitle:
    case TypeRole::CardTitle:
    case TypeRole::WindowTitle:
    case TypeRole::Metric:
        return true;
    default:
        return false;
    }
}

// The named family first, then the profile's fallbacks. Qt walks the list and
// takes the first installed one, so a machine without the design's faces gets
// the profile's stated second choice instead of whatever Qt would have
// substituted on its own.
QFont familyStack(const QString& primary, const QStringList& fallbacks)
{
    QStringList families;
    if (!primary.isEmpty()) {
        families.append(primary);
    }
    families.append(fallbacks);
    QFont f(families.isEmpty() ? QString() : families.first());
    f.setFamilies(families);
    return f;
}

} // namespace

QFont roleFont(const QWidget* context, TypeRole role, bool bold)
{
    const ThemeProfile& profile = themeFor(context);
    const TypeStyle style = profile.text(role);
    const TypeScale& scale = profile.type;

    // A role that declares itself monospace gets the mono stack here as well.
    // Before, only roleMonoFont() honoured the flag, so TypeRole::Mono came out
    // in the body face at every call site that asked for it by role.
    const bool mono = style.monospace;
    const QString primary = mono
        ? scale.monoFamily
        : (isDisplayRole(role) && !scale.displayFamily.isEmpty() ? scale.displayFamily
                                                                 : scale.family);

    QFont f = familyStack(primary, mono ? scale.monoFallbackFamilies
                                        : scale.fallbackFamilies);
    f.setPixelSize(style.size);
    f.setWeight(bold ? QFont::Bold : static_cast<QFont::Weight>(style.weight));
    return f;
}

QFont roleMonoFont(const QWidget* context, TypeRole role, bool bold)
{
    const ThemeProfile& profile = themeFor(context);
    const TypeStyle style = profile.text(role);

    QFont f = familyStack(profile.type.monoFamily, profile.type.monoFallbackFamilies);
    f.setPixelSize(style.size);
    f.setWeight(bold ? QFont::Bold : static_cast<QFont::Weight>(style.weight));
    return f;
}

QFont iconFont(int pixelSize)
{
    QFont f(QStringLiteral("Segoe Fluent Icons"));
    f.setFamilies({QStringLiteral("Segoe Fluent Icons"),
                   QStringLiteral("Segoe MDL2 Assets")});
    f.setPixelSize(pixelSize);
    return f;
}

namespace {

// The dynamic property an Ink role is remembered under. A string rather than
// the enum so it survives a QVariant round trip and is visible in a debugger.
constexpr const char* kInkRole = "novuInkRole";

// One filter for every label in the process: it holds no state, so a second
// instance would buy nothing. Installed by setInk() when a role - rather than
// a bare colour - is what was asked for.
class InkWatcher : public QObject
{
public:
    static InkWatcher* instance()
    {
        static InkWatcher watcher;
        return &watcher;
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() != ThemeChangeEvent::type()) {
            return QObject::eventFilter(watched, event);
        }
        auto* lb = qobject_cast<QLabel*>(watched);
        if (!lb) {
            return QObject::eventFilter(watched, event);
        }
        const QVariant role = lb->property(kInkRole);
        if (role.isValid()) {
            // Through the colour overload, which is what actually writes the
            // palette and the one-property sheet.
            setInk(lb, inkColor(lb, static_cast<Ink>(role.toInt())));
        }
        return QObject::eventFilter(watched, event);
    }
};

void rememberInk(QLabel* lb, Ink ink)
{
    if (!lb) {
        return;
    }
    const bool first = !lb->property(kInkRole).isValid();
    lb->setProperty(kInkRole, static_cast<int>(ink));
    if (first) {
        lb->installEventFilter(InkWatcher::instance());
    }
}

} // namespace

QLabel* label(const QString& text, QWidget* parent, TypeRole role, Ink ink, bool bold)
{
    auto* lb = new QLabel(text, parent);
    lb->setFont(roleFont(parent, role, bold));
    rememberInk(lb, ink);
    setInk(lb, inkColor(parent, ink));
    return lb;
}

QLabel* caption(const QString& text, QWidget* parent)
{
    // The uppercasing and the tracking are the treatment, not the font, so they
    // are applied here rather than stored in the role. Without the tracking a
    // 10px uppercase mono line reads as a solid bar.
    auto* lb = label(text.toUpper(), parent, TypeRole::Eyebrow, Ink::Muted);
    QFont f = lb->font();
    f.setLetterSpacing(QFont::PercentageSpacing, 110);
    lb->setFont(f);
    return lb;
}

void applyText(QLabel* lb, TypeRole role, Ink ink, bool bold)
{
    if (!lb) {
        return;
    }
    lb->setFont(roleFont(lb, role, bold));
    // Through the role overload, so the ink is remembered and a theme change
    // reaches this label the way it reaches a component.
    setInk(lb, ink);
}

void setInk(QLabel* lb, Ink ink)
{
    if (!lb) {
        return;
    }
    rememberInk(lb, ink);
    setInk(lb, inkColor(lb, ink));
}

void setInk(QLabel* lb, const QColor& color)
{
    if (!lb) {
        return;
    }
    QPalette pal = lb->palette();
    pal.setColor(QPalette::WindowText, color);
    lb->setPalette(pal);

    // The palette alone is not enough. The scope sheet carries
    // `QLabel { color: ... }` so that a label with no ink of its own is not
    // left with the light system palette on a dark surface - and a stylesheet
    // colour outranks a palette one. Every ink set inside a themed subtree was
    // therefore being dropped: a loss printed in exactly the same colour as a
    // profit, and an inactive window title stayed at full strength.
    //
    // A widget's own sheet outranks its ancestors', so one declaration on the
    // label restores the role. It is a single property and no selector, so
    // nothing else about the label - background, border, tone rules - changes.
    const QString sheet = QStringLiteral("color: %1;")
        .arg(color.alpha() == 255 ? color.name() : rgba(color, color.alphaF()));
    // Guarded: setInk() is called again on every polish and every theme change,
    // and re-setting an identical sheet re-polishes the widget for nothing.
    if (lb->styleSheet() != sheet) {
        lb->setStyleSheet(sheet);
    }
}

} // namespace novu::ui
