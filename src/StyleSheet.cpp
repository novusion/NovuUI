#include "novu/ui/StyleSheet.h"

#include "novu/ui/ThemeManager.h"
#include "novu/ui/ThemeScope.h"

#include <QStyle>
#include <QWidget>

namespace novu::ui {

namespace {

// rgba() comes from Theme.h - this file deliberately does not carry its own.

// A colour as QSS understands it, alpha included.
//
// QColor::name() drops the alpha channel. A token defined as a translucent
// overlay - NovuTrade's borders are slate-400 at 18%, deliberately, so an
// outline reads as a hairline over whatever surface is behind it - therefore
// reached the sheet fully opaque and painted as a near-white line around
// every input and combo box. The profile was right and the sheet was
// throwing half of it away.
//
// Everything that may carry alpha goes through here. For an opaque colour
// this is byte-for-byte what name() produced, so profiles with solid borders
// are unaffected.
QString css(const QColor& color)
{
    return color.alpha() == 255 ? color.name() : rgba(color, color.alphaF());
}

// Kept in step with Button.cpp, which reserves the same padding in its
// sizeHint(). The two cannot share a constant without the sheet depending on
// the component, so they are named here and there and tested together.
constexpr int kButtonPaddingH = 14;
constexpr int kButtonPaddingHSmall = 9;

// One button variant: base look plus hover, expressed against a property.
QString buttonVariant(const QString& kind, const QString& background,
                      const QString& foreground, const QString& borderColor,
                      const QString& hoverBackground)
{
    QString rule = QStringLiteral("QPushButton[%1=\"%2\"] { background: %3; color: %4; ")
                       .arg(QLatin1String(prop::kind), kind, background, foreground);
    rule += borderColor.isEmpty() ? QStringLiteral("border: none; ")
                                  : QStringLiteral("border: 1px solid %1; ").arg(borderColor);
    rule += QStringLiteral("}");
    rule += QStringLiteral("QPushButton[%1=\"%2\"]:hover { background: %3; }")
                .arg(QLatin1String(prop::kind), kind, hoverBackground);
    return rule;
}

} // namespace

QString styleSheetFor(const ThemeProfile& p)
{
    const ThemeColors& c = p.colors;
    const int radiusButton = p.radius(SurfaceRole::Button);
    const int radiusInput = p.radius(SurfaceRole::Input);
    const int padding = p.space(SpacingRole::ToolbarGap);

    QString qss;
    qss.reserve(8192);

    // -- text ----------------------------------------------------------------
    // QLabel derives from QFrame, so a bare QFrame rule would give every label
    // a background. Labels are neutralised explicitly; their colour is set per
    // label through a role, never here.
    qss += QStringLiteral("QLabel { background: transparent; border: none; color: %1; }")
               .arg(css(c.textPrimary));

    // -- buttons -------------------------------------------------------------
    // 14px, the same horizontal padding Button::sizeHint() reserves. The base
    // rule governs the plain QPushButtons a TabStrip builds, and a tab that
    // used the toolbar gap instead came out visibly narrower than a real
    // Button carrying the same caption.
    qss += QStringLiteral("QPushButton { border-radius: %1px; padding: 0 %2px; }")
               .arg(radiusButton)
               .arg(kButtonPaddingH);

    qss += buttonVariant(QStringLiteral("primary"), css(c.accent),
                         css(c.textOnAccent), QString(),
                         css(c.accentLight));
    qss += buttonVariant(QStringLiteral("secondary"), rgba(c.textPrimary, 0.05),
                         css(c.textPrimary), css(c.border),
                         rgba(c.accent, 0.16));
    qss += buttonVariant(QStringLiteral("ghost"), QStringLiteral("transparent"),
                         css(c.textSecondary), css(c.border),
                         rgba(c.accent, 0.16));
    qss += buttonVariant(QStringLiteral("danger"), rgba(c.danger, 0.14),
                         css(c.danger), rgba(c.danger, 0.40),
                         rgba(c.danger, 0.22));
    qss += buttonVariant(QStringLiteral("success"), rgba(c.success, 0.14),
                         css(c.success), rgba(c.success, 0.40),
                         rgba(c.success, 0.22));

    qss += QStringLiteral("QPushButton:disabled { color: %1; background: transparent;"
                          " border-color: %2; }")
               .arg(css(c.textDisabled), css(c.borderSubtle));

    // Geometry variant, so a small danger button needs no rule of its own.
    // A row action is tighter than a toolbar button in both padding and
    // corner: the mockup draws it at the input radius, not the button one.
    qss += QStringLiteral("QPushButton[%1=\"small\"] { border-radius: %2px; padding: 0 %3px; }")
               .arg(QLatin1String(prop::size))
               .arg(p.radii.xs)
               .arg(kButtonPaddingHSmall);

    // -- tabs ----------------------------------------------------------------
    qss += QStringLiteral("QPushButton[%1=\"tab\"] { background: transparent; color: %2;"
                          " border: 1px solid %3; border-radius: %4px; }")
               .arg(QLatin1String(prop::kind), css(c.textSecondary), css(c.border))
               .arg(radiusButton);

    // Active tab, per tone. Two tones exist because a preset switcher sitting
    // above a row of accent-coloured chips must not read as one of the chips.
    const struct { const char* key; QColor color; } tabTones[] = {
        {"primary", c.accent},
        {"accent", c.warning},
    };
    for (const auto& t : tabTones) {
        qss += QStringLiteral("QPushButton[%1=\"tab\"][%2=\"%3\"][%4=\"true\"] {"
                              " background: %5; color: %6; border-color: %7;"
                              " font-weight: 700; }")
                   .arg(QLatin1String(prop::kind), QLatin1String(prop::tone),
                        QLatin1String(t.key), QLatin1String(prop::active),
                        rgba(t.color, 0.20), css(t.color), rgba(t.color, 0.45));
        // Hover answers in the strip's own tone. A shared rule that only
        // brightened the text left the outline dead, so a four-item strip
        // gave no feedback until something was clicked.
        qss += QStringLiteral("QPushButton[%1=\"tab\"][%2=\"%3\"]:hover {"
                              " color: %4; border-color: %5; }")
                   .arg(QLatin1String(prop::kind), QLatin1String(prop::tone),
                        QLatin1String(t.key), css(c.textPrimary),
                        rgba(t.color, 0.40));
    }

    // -- chips (toggle pills, e.g. a column chooser) --------------------------
    qss += QStringLiteral("QPushButton[%1=\"pill\"] { border-radius: %2px; padding: 0 8px;"
                          " font-weight: 600; }"
                          "QPushButton[%1=\"pill\"]:checked { color: %3; background: %4;"
                          " border: 1px solid %5; }"
                          "QPushButton[%1=\"pill\"]:!checked { color: %6; background: %7;"
                          " border: 1px solid transparent; }"
                          "QPushButton[%1=\"pill\"]:!checked:hover { color: %8; background: %9; }")
               .arg(QLatin1String(prop::kind))
               .arg(p.radii.xs)
               .arg(css(c.accent), rgba(c.accent, 0.10), rgba(c.accent, 0.20),
                    css(c.textDisabled), rgba(c.textDisabled, 0.10),
                    css(c.textSecondary), rgba(c.textDisabled, 0.16));

    // -- badges ---------------------------------------------------------------
    const struct { const char* key; QColor color; } tones[] = {
        {"neutral", c.textSecondary}, {"primary", c.accent},
        {"success", c.success},       {"warning", c.warning},
        {"danger", c.danger},         {"info", c.accentLight},
    };
    for (const auto& t : tones) {
        qss += QStringLiteral("QLabel[%1=\"%2\"] { color: %3; background: %4;"
                              " border-radius: %5px; border: none; }")
                   .arg(QLatin1String(prop::tone), QLatin1String(t.key),
                        css(t.color), rgba(t.color, 0.16))
                   .arg(p.radii.medium);
    }

    // -- inputs ---------------------------------------------------------------
    qss += QStringLiteral("QLineEdit { background: %1; color: %2; border: 1px solid %3;"
                          " border-radius: %4px; padding: 0 10px; }"
                          "QLineEdit:focus { border-color: %5; }"
                          "QLineEdit:disabled { color: %6; }")
               .arg(css(c.panelBgAlt), css(c.textPrimary), css(c.border))
               .arg(radiusInput)
               .arg(css(c.accent), css(c.textDisabled));

    qss += QStringLiteral("QComboBox { background: %1; color: %2; border: 1px solid %3;"
                          " border-radius: %4px; padding: 0 10px; }"
                          "QComboBox::drop-down { border: none; width: 20px; }"
                          // The arrow is painted by ComboBox: Qt cannot draw a
                          // CSS border triangle — it comes out as a rectangle
                          // that reads as a dash, and the combo then looks
                          // exactly like a line edit.
                          "QComboBox::down-arrow { image: none; width: 0; height: 0; }"
                          "QComboBox QAbstractItemView { background: %1; color: %2;"
                          " border: 1px solid %3; selection-background-color: %5; }")
               .arg(css(c.panelBgAlt), css(c.textPrimary), css(c.border))
               .arg(radiusInput)
               .arg(rgba(c.accent, 0.22));

    // Light indicator, never black: an unstyled QCheckBox inherits the light
    // system palette and turns into a black box on a dark surface.
    //
    // The resting outline is a neutral grey derived from the muted ink, not
    // borderHover. borderHover is the *hover* colour and in NovuTrade it is
    // green, so every unchecked box was drawing itself as though the pointer
    // were already on it. Hover and checked are the accent; resting is not.
    qss += QStringLiteral("QCheckBox { color: %1; background: transparent; spacing: 8px; }"
                          "QCheckBox::indicator { width: 15px; height: 15px;"
                          " border-radius: %2px; border: 1px solid %3; background: %4; }"
                          "QCheckBox::indicator:hover { border-color: %5; }"
                          "QCheckBox::indicator:checked { background: %5; border-color: %5; }")
               .arg(css(c.textSecondary))
               .arg(p.radii.xs)
               .arg(rgba(c.textDisabled, 0.60), css(c.panelBgAlt), css(c.accent));

    // -- table ----------------------------------------------------------------
    qss += QStringLiteral("QTableWidget#%1, QTableView#%1 { background: transparent;"
                          " color: %2; border: none; outline: none;"
                          " gridline-color: transparent; }"
                          "QTableWidget#%1::item, QTableView#%1::item { border: none;"
                          " border-bottom: 1px solid %3; padding: 0 %4px; }"
                          "QTableWidget#%1::item:selected, QTableView#%1::item:selected {"
                          " background: %5; color: %2; }"
                          "QHeaderView { background: transparent; border: none; }"
                          "QHeaderView::section { background: %6; color: %7; border: none;"
                          " padding: 0 %4px; font-size: %8px; font-weight: 700; }")
               .arg(QLatin1String(name::table), css(c.textPrimary),
                    css(p.table.gridLine))
               .arg(p.space(SpacingRole::TableCellPadding))
               .arg(rgba(c.accent, 0.14), css(p.table.headerBg),
                    css(c.textDisabled))
               .arg(p.type.xs);

    // -- scrollbars ------------------------------------------------------------
    // Transparent groove, thumb in the border colour, as the approved mockup
    // draws it. The thumb is translucent so it composites against whichever
    // surface it happens to sit over and stays the same weight on a card and
    // on a page — a solid thumb has to pick one of the two and is wrong on
    // the other.
    //
    // The vertical bar is 10px with a 5px (fully round) thumb and the
    // horizontal one 9px with 4px, which is what the two mockup files
    // specify; they are not the same control and the page-level bar is the
    // one a user drags.
    qss += QStringLiteral("QScrollBar:vertical { background: transparent; width: 10px;"
                          " margin: 0; border: none; }"
                          "QScrollBar::handle:vertical { background: %1;"
                          " border-radius: 5px; min-height: 40px; }"
                          "QScrollBar::handle:vertical:hover { background: %2; }"
                          "QScrollBar:horizontal { background: transparent; height: 9px;"
                          " margin: 0; border: none; }"
                          "QScrollBar::handle:horizontal { background: %1;"
                          " border-radius: 4px; min-width: 60px; }"
                          "QScrollBar::handle:horizontal:hover { background: %2; }"
                          "QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }"
                          "QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }")
               .arg(css(c.border), rgba(c.border, 0.32));

    // Bare QScrollArea, not only the named one.
    //
    // scrollPage() now names the area it creates, but an application is free
    // to build its own, and an unnamed QScrollArea matched nothing at all: it
    // kept Qt's default palette and painted a grey block inside a navy card.
    // A bare QScrollArea rule is safe in a way a bare QFrame rule is not —
    // QScrollArea has no subclasses in this library and the descendant rule
    // addresses exactly the viewport's content widget.
    qss += QStringLiteral("QScrollArea { background: transparent; border: none; }"
                          "QScrollArea > QWidget > QWidget { background: transparent; }"
                          "QScrollArea#%1 { background: transparent; border: none; }")
               .arg(QLatin1String(name::scrollArea));

    // -- containers addressed by object name -----------------------------------
    qss += QStringLiteral("QFrame#%1 { background: %2; border: none; }")
               .arg(QLatin1String(name::divider), css(c.border));

    return qss;
}

int resolveRadius(int radius, int height)
{
    // A radius wider than half the box is meaningless; clamping is what makes
    // the pill marker resolve to an actual pill.
    return qMin(radius, qMax(0, height) / 2);
}

void applyStyleSheet(QWidget* root, const QString& profileId)
{
    if (!root) {
        return;
    }
    // Marks the subtree so themeFor() inside it resolves to the same profile
    // the sheet was composed from. Doing only one of the two is how a subtree
    // ends up styled by one profile and measured by another.
    ThemeScope::apply(root, profileId);
    root->setStyleSheet(styleSheetFor(ThemeManager::instance().profileRef(profileId)));
}

void repolish(QWidget* widget)
{
    if (!widget || !widget->style()) {
        return;
    }
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

} // namespace novu::ui
