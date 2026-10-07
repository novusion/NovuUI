#include "novu/ui/StyleSheet.h"

#include "novu/ui/Style.h"

#include "novu/ui/ThemeManager.h"
#include "novu/ui/ThemeScope.h"

#include <QStyle>
#include <QApplication>
#include <QAbstractScrollArea>
#include <QScrollBar>
#include <QPointer>
#include <QEvent>
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

// The owner of a web scrollbar supplies :hover, including its content children.
// QSS does not reliably invalidate ancestor pseudo-state selectors, so carry
// the owner state onto the bars and invalidate only those two small controls.
class ScrollHoverWatcher final:public QObject {
    QPointer<QAbstractScrollArea> active;
    static void mark(QAbstractScrollArea* area,bool hovered){
        if(!area)return;
        for(auto* bar:{area->verticalScrollBar(),area->horizontalScrollBar()}){
            if(bar->property("novuScrollHovered").toBool()==hovered)continue;
            bar->setProperty("novuScrollHovered",hovered);repolish(bar);
        }
    }
public:
    explicit ScrollHoverWatcher(QObject* parent):QObject(parent){}
    bool eventFilter(QObject* object,QEvent* event)override{
        if(event->type()!=QEvent::Enter&&event->type()!=QEvent::Leave)return false;
        auto* widget=qobject_cast<QWidget*>(object);if(!widget)return false;
        if(event->type()==QEvent::Leave){
            if(active&&(widget==active||widget==active->viewport())){mark(active,false);active=nullptr;}
            return false;
        }
        QAbstractScrollArea* area=nullptr;
        for(auto* parent=widget;parent;parent=parent->parentWidget())if((area=qobject_cast<QAbstractScrollArea*>(parent)))break;
        if(area&&!themeFor(area).controls.scrollThumbOnAreaHover)area=nullptr;
        if(area!=active){mark(active,false);active=area;mark(active,true);}
        return false;
    }
};

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
    const int radiusButton = qMin(p.radius(SurfaceRole::Button),
                                  p.controls.height / 2);
    const int radiusSegment = qMin(p.radius(SurfaceRole::Segment),
                                   p.controls.heightSmall / 2);
    const int radiusInput = p.radius(SurfaceRole::Input);
    // A sheet states one number and cannot ask the widget how tall it is, so a
    // pill radius has to be capped here or Qt draws a rounded rectangle whose
    // corners overlap and flatten. Half the control height is the pill.
    const int radiusSelect = qMin(p.radius(SurfaceRole::Select),
                                  p.controls.height / 2);
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

    // Hover lightens the button; it does not repaint it in another colour.
    // Primary used accentLight, which is a different hue entirely - a green
    // button turned blue under the pointer, which reads as a state change
    // rather than as "you are over this".
    qss += buttonVariant(QStringLiteral("primary"), css(c.accent),
                         css(c.textOnAccent), QString(),
                         css(c.accent.lighter(115)));
    qss += buttonVariant(QStringLiteral("secondary"), rgba(c.textPrimary, 0.05),
                         css(c.textPrimary), css(c.border),
                         rgba(c.textPrimary, 0.10));
    qss += buttonVariant(QStringLiteral("ghost"), QStringLiteral("transparent"),
                         css(c.textSecondary), css(c.border),
                         rgba(c.textPrimary, 0.08));
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
               .arg(radiusSegment);

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
    // The same five roles Badge paints, resolved through the same palette, so a
    // tone written as a property and a tone painted by a component cannot come
    // out different. "primary" is the odd one and stays: it is the application
    // accent, deliberately, and it is the only entry here that is allowed to
    // change identity between products.
    //
    // The tint was re-derived here as `the hue at 0.16` - the third copy of
    // that recipe. It comes from the palette now.
    const struct { const char* key; SemanticColor color; } tones[] = {
        {"neutral", p.semantic.neutral},
        {"primary", semanticColorFrom(c.accent)},
        {"success", p.semantic.success},
        {"warning", p.semantic.warning},
        {"danger", p.semantic.danger},
        {"info", p.semantic.info},
    };
    for (const auto& t : tones) {
        qss += QStringLiteral("QLabel[%1=\"%2\"] { color: %3; background: %4;"
                              " border-radius: %5px; border: 1px solid %6;"
                              " padding: 2px 8px; font-weight: 500; }")
                   .arg(QLatin1String(prop::tone), QLatin1String(t.key),
                        css(t.color.text), css(t.color.fill))
                   .arg(p.radii.medium)
                   .arg(css(t.color.border));
    }

    // -- inputs ---------------------------------------------------------------
    // panelBg, not panelBgAlt: the design's `.input,.select,.search input,
    // textarea.input` all share one rule, `background:var(--bg)`, which is
    // this token, not the panel-alt shade a popup or a sub-panel sits on. A
    // field is a surface a page sits its own content on, not a panel nested
    // inside one.
    qss += QStringLiteral("QLineEdit { background: %1; color: %2; border: 1px solid %3;"
                          " border-radius: %4px; padding: 0 10px; }"
                          "QLineEdit:focus { border-color: %5; }"
                          "QLineEdit:disabled { color: %6; }"
                          // The multi-line half of the same control. Left
                          // unstyled it arrives with the light system palette -
                          // white paper on a dark form - and it is not reached
                          // by the QLineEdit rule above, because a plain text
                          // edit is a scroll area with a viewport rather than a
                          // line edit that grew.
                          "QPlainTextEdit { background: %1; color: %2;"
                          " border: 1px solid %3; border-radius: %4px;"
                          " padding: 6px 8px; }"
                          "QPlainTextEdit:focus { border-color: %5; }"
                          "QPlainTextEdit:disabled { color: %6; }"
                          // Renaming a row in place is not filling in a form.
                          // The editor stands exactly where the label stood,
                          // in the same ink, on the row's own background - so
                          // what changes when an edit begins is that the text
                          // is selected, not that a box appeared around it.
                          "QLineEdit#novuTreeRename { background: transparent;"
                          " border: none; border-radius: 0; padding: 0;"
                          " color: %2; }"
                          "QLineEdit#novuTreeRename:focus { border: none; }")
               .arg(css(c.panelBg), css(c.textPrimary), css(c.border))
               .arg(radiusInput)
               .arg(css(c.accent), css(c.textDisabled));

    if(c.inputBorder.isValid()||c.inputInk.isValid())
        qss+=QStringLiteral("QLineEdit { border-color: %1; color: %2; }").arg(css(c.inputBorder.isValid()?c.inputBorder:c.border),css(c.inputInk.isValid()?c.inputInk:c.textPrimary));
    if(c.inputHoverBorder.isValid())qss+=QStringLiteral("QLineEdit:hover { border-color: %1; }").arg(css(c.inputHoverBorder));
    if(c.inputFocusBg.isValid())qss+=QStringLiteral("QLineEdit:focus { background: %1; border-color: %2; }").arg(css(c.inputFocusBg),css(c.accent));
    // A field whose value was refused - an inline rename that collides with an
    // existing name. After the focus rule so that it still reads as wrong while
    // the person is fixing it. Set with setProperty("invalid", true).
    qss += QStringLiteral("QLineEdit[invalid=\"true\"], QLineEdit[invalid=\"true\"]:focus"
                          " { border-color: %1; }").arg(css(c.danger));
    // A refused value, the approved design's `.field.bad .input`.
    //
    // After the rules above, so it wins on equal specificity, and repeated for
    // :focus - without that, clicking into the offending control clears the one
    // mark saying which control it was, exactly when the user is looking at it.
    qss += QStringLiteral("QLineEdit[novuInvalid=\"true\"],"
                          " QPlainTextEdit[novuInvalid=\"true\"],"
                          " QComboBox[novuInvalid=\"true\"],"
                          " QDateEdit[novuInvalid=\"true\"] { border-color: %1; }"
                          "QLineEdit[novuInvalid=\"true\"]:focus,"
                          " QPlainTextEdit[novuInvalid=\"true\"]:focus,"
                          " QComboBox[novuInvalid=\"true\"]:focus,"
                          " QDateEdit[novuInvalid=\"true\"]:focus { border-color: %2; }")
               .arg(css(p.semantic.danger.border), css(p.semantic.danger.text));

    // QDateEdit is a spin box, not a line edit, so none of the rules above reach
    // it. Without this a date field arrives with the light system palette beside
    // inputs that carry the profile.
    qss += QStringLiteral("QDateEdit, QDateTimeEdit { background: %1; color: %2;"
                          " border: 1px solid %3; border-radius: %4px;"
                          " padding: 0 6px 0 10px; }"
                          "QDateEdit:focus, QDateTimeEdit:focus { border-color: %5; }"
                          // Wide enough to aim at. The mark DateEdit paints
                          // sits inside this strip.
                          "QDateEdit::drop-down, QDateTimeEdit::drop-down {"
                          " border: none; width: 30px; }"
                          "QDateEdit::up-button, QDateEdit::down-button,"
                          " QDateTimeEdit::up-button, QDateTimeEdit::down-button {"
                          " width: 0; height: 0; border: none; }")
               .arg(css(c.panelBg), css(c.textPrimary), css(c.border))
               .arg(radiusInput)
               .arg(css(c.accent));

    // `.select{color:var(--text3)}` - the closed field's own value is the
    // muted ink, same step as a placeholder. Only the closed field: once the
    // list is open the row under the pointer is being read to be chosen, and
    // stays the primary ink (%2, below) same as every other popup in the
    // sheet - a select whose open list also muted itself would be the one
    // control in the product that got harder to read at the moment it asked
    // to be read.
    qss += QStringLiteral("QComboBox { background: %1; color: %8; border: 1px solid %3;"
                          " border-radius: %4px; padding: 0 10px; }"
                          "QComboBox::drop-down { border: none; width: 20px; }"
                          // `.select.open{border-color:var(--acc)}` --
                          // the list is showing, and the control says so.
                          "QComboBox[novuOpen=\"true\"] { border-color: %7; }"
                          // The arrow is painted by ComboBox: Qt cannot draw a
                          // CSS border triangle — it comes out as a rectangle
                          // that reads as a dash, and the combo then looks
                          // exactly like a line edit.
                          "QComboBox::down-arrow { image: none; width: 0; height: 0; }"
                          // Square where it meets the closed field, so the two
                          // read as one shape continuing downward rather than a
                          // second, separately-rounded box; rounded to the
                          // field's own radius everywhere else Qt's own popup
                          // was drawing hard corners with no radius at all.
                          "QComboBox QAbstractItemView { background: %1; color: %2;"
                          " border: 1px solid %3; outline: none; padding: 2px;"
                          " border-top-left-radius: 0; border-top-right-radius: 0;"
                          " border-bottom-left-radius: %4px;"
                          " border-bottom-right-radius: %4px; }"
                          "QComboBox QAbstractItemView::item { min-height: 24px;"
                          " padding: 2px 8px; border: none; }"
                          // A popup list moves its selection with the pointer,
                          // so :selected *is* the hover state. It was the accent
                          // at 22% alpha, which on this panel is a shade nobody
                          // can see - the row under the pointer has to be
                          // unmistakable or the list reads as inert.
                          "QComboBox QAbstractItemView::item:selected,"
                          " QComboBox QAbstractItemView::item:hover {"
                          " background: %5; color: %6; }"
                          // The completer's popup is a separate top-level view
                          // and is not a child of the combo, so the rules above
                          // do not reach it.
                          "QCompleter QAbstractItemView { background: %1; color: %2;"
                          " border: 1px solid %3; outline: none;"
                          " border-top-left-radius: 0; border-top-right-radius: 0;"
                          " border-bottom-left-radius: %4px;"
                          " border-bottom-right-radius: %4px; }"
                          "QCompleter QAbstractItemView::item { min-height: 24px;"
                          " padding: 2px 8px; }"
                          "QCompleter QAbstractItemView::item:selected,"
                          " QCompleter QAbstractItemView::item:hover {"
                          " background: %5; color: %6; }")
               .arg(css(c.panelBg), css(c.textPrimary), css(c.border))
               .arg(radiusSelect)
               .arg(css(c.surfaceActive.lighter(145)), css(c.textPrimary))
               .arg(css(c.accent))
               .arg(css(c.textDisabled));

    // The label only. The indicator is deliberately *not* styled here.
    //
    // A stylesheet can colour a check indicator and size it, but it cannot draw
    // a shape inside it, so the only state it can express is "flood the box
    // with the accent" - a solid square that reads as a colour swatch rather
    // than as something the user ticked. And a sheet reaches no indicator at
    // all inside a table cell, a tree item or a menu.
    //
    // NovuStyle draws all of them instead, from the same tokens. The moment a
    // ::indicator rule appears here the sheet takes the drawing back and the
    // filled square returns, so this rule stays about text and nothing else.
    qss += QStringLiteral("QCheckBox { color: %1; background: transparent; }"
                          "QCheckBox:disabled { color: %2; }")
               .arg(css(c.textSecondary), css(c.textDisabled));

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

    if(p.controls.scrollThumbOnAreaHover){
        const int inset=qMax(0,p.controls.scrollThumbInset);
        qss+=QStringLiteral(
            "QScrollBar:horizontal { height:10px; }"
            "QScrollBar::handle:vertical,QScrollBar::handle:horizontal { background:transparent; border:%1px solid transparent; background-clip:content; border-radius:5px; min-height:0; min-width:0; }"
            "QScrollBar[novuScrollHovered=\"true\"]::handle:vertical,QScrollBar[novuScrollHovered=\"true\"]::handle:horizontal { background:%2; }"
            "QScrollBar::handle:vertical:hover,QScrollBar::handle:horizontal:hover { background:%3; }")
            .arg(inset).arg(css(c.borderHover),css(c.textDisabled));
    }

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

    // Splitter handles: the colour only.
    //
    // The shape belongs to SplitterHandle, which paints its own centre line and
    // its collapse arrows — a stylesheet cannot put a control on a handle, and
    // that is the whole reason the component exists. What is left here is
    // clearing Qt's default raised bevel for any *plain* QSplitter an
    // application still uses, so one that has not been migrated is at least not
    // announcing itself as a Qt splitter.
    qss += QStringLiteral("QSplitter { background: transparent; border: none; }"
                          "QSplitter::handle { background: transparent; }"
                          "QSplitter::handle:horizontal { width: 8px; }"
                          "QSplitter::handle:vertical { height: 8px; }"
                          "QSplitter::handle:hover { background: %1; }")
               .arg(rgba(c.accent, 0.28));

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
    // Qt does not propagate a style down a subtree, so the parts only a QStyle
    // can draw - a check indicator in a table cell, a tree item, a checkable
    // group box - have to come from the application style. Installed here
    // rather than left to each application's main(), because "every Novu
    // checkbox looks the same" cannot depend on twelve applications each
    // remembering a line. Idempotent, and it wraps whatever style is already
    // there.
    installNovuStyle();
    static QPointer<ScrollHoverWatcher> scrollHover;
    if(!scrollHover&&qApp){scrollHover=new ScrollHoverWatcher(qApp);qApp->installEventFilter(scrollHover);}
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
