#include "novu/ui/Theme.h"

#include <QPainter>
#include <QPen>
#include <QWidget>
#include <QVariant>

namespace novu::ui {

namespace {
ThemeColors g_theme;
}

const ThemeColors& theme()
{
    return g_theme;
}

void setTheme(const ThemeColors& colors)
{
    g_theme = colors;
}

QString rgba(const QColor& color, double alpha)
{
    return QStringLiteral("rgba(%1, %2, %3, %4)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(alpha, 0, 'f', 3);
}

QColor withAlpha(const QColor& color, double alpha)
{
    QColor out = color;
    out.setAlphaF(static_cast<float>(alpha));
    return out;
}

const SemanticHues& sharedSemanticHues()
{
    static const SemanticHues hues;
    return hues;
}

const QList<FamilyHue>& familyHues()
{
    // NovuExplorer-brand-mockup-v2.html states the set with its names, as the
    // choices in its folder-colour picker, and draws the same hues as a ring in
    // its colour wheel. Listed rather than derived: a named palette is a
    // decision, and deriving one from the accent would make Cyan a different
    // cyan in every product - which is the one thing these promise not to be.
    static const QList<FamilyHue> hues = {
        {QStringLiteral("Rose"),   QColor(QStringLiteral("#C47B85"))},
        {QStringLiteral("Amber"),  QColor(QStringLiteral("#FFB14D"))},
        {QStringLiteral("Lime"),   QColor(QStringLiteral("#A8CC45"))},
        {QStringLiteral("Green"),  QColor(QStringLiteral("#3CCB7F"))},
        {QStringLiteral("Teal"),   QColor(QStringLiteral("#58B09E"))},
        {QStringLiteral("Cyan"),   QColor(QStringLiteral("#2FC6D8"))},
        {QStringLiteral("Blue"),   QColor(QStringLiteral("#4DA3FF"))},
        {QStringLiteral("Indigo"), QColor(QStringLiteral("#7B87E8"))},
        {QStringLiteral("Purple"), QColor(QStringLiteral("#7345CC"))},
        {QStringLiteral("Slate"),  QColor(QStringLiteral("#8793A3"))},
    };
    return hues;
}

SemanticColor semanticColorFrom(const QColor& ink, const QColor& plate,
                                double fillAlpha, double borderAlpha)
{
    // Hover and pressed are the design's own step: `.badge` does not have them,
    // but every tinted control that answers the pointer moves by roughly half
    // the fill again and then again. Derived rather than stated because no
    // mockup states them, and a component that invented its own was the
    // problem this function exists to solve.
    SemanticColor c;
    c.text = ink;
    c.fill = withAlpha(plate, fillAlpha);
    c.border = withAlpha(plate, borderAlpha);
    c.hover = withAlpha(plate, fillAlpha * 2.0);
    c.pressed = withAlpha(plate, fillAlpha * 2.8);
    return c;
}

// One colour standing in for both, for the two places that genuinely have one:
// the application accent, and Neutral - which is the profile's own secondary
// ink and has no plate of its own to be mixed from.
//
// The alphas are `.badge.acc` / `.tag.acc`, which agree across the mockups at a
// .42 border. The fill was 0.16 here and is .10-.13 in all four.
SemanticColor semanticColorFrom(const QColor& hue)
{
    return semanticColorFrom(hue, hue, 0.12, 0.42);
}

bool focusRingSuppressed(const QWidget* widget)
{
    for (const QWidget* w = widget; w; w = w->parentWidget()) {
        if (w->property(focusRingSuppressedProperty).toBool()) {
            return true;
        }
    }
    return false;
}

bool FocusRing::visibleOn(const QWidget* widget) const
{
    return widget && widget->hasFocus() && m_visible && !focusRingSuppressed(widget);
}

bool selectionIsActive(const QWidget* widget)
{
    if (!widget) {
        return true;
    }
    if (widget->hasFocus()) {
        return true;
    }
    // Walk up looking for a panel that has declared itself. The first one found
    // decides: a panel inside a panel is the inner one's business.
    for (const QWidget* parent = widget->parentWidget(); parent;
         parent = parent->parentWidget()) {
        const QVariant declared = parent->property(activePanelProperty);
        if (declared.isValid()) {
            return declared.toBool();
        }
    }
    // No panel above it - a dialog, a popover, a preview. Those have no sibling
    // to be confused with, so a dimmed selection in one would be dimmed for no
    // reason a user could see.
    return true;
}


void paintHairline(QPainter* painter, const QPoint& from, const QPoint& to,
                   const QColor& colour, int weight)
{
    if (!painter || !colour.isValid()) {
        return;
    }
    const int thickness = qMax(1, weight);
    if (from.y() == to.y()) {
        const int left = qMin(from.x(), to.x());
        const int right = qMax(from.x(), to.x());
        painter->fillRect(QRect(left, from.y(), right - left + 1, thickness), colour);
        return;
    }
    if (from.x() == to.x()) {
        const int top = qMin(from.y(), to.y());
        const int bottom = qMax(from.y(), to.y());
        painter->fillRect(QRect(from.x(), top, thickness, bottom - top + 1), colour);
        return;
    }
    // A diagonal is not a hairline; it is a drawing, and it belongs to
    // whatever is drawing it.
    painter->save();
    painter->setPen(QPen(colour, thickness));
    painter->drawLine(from, to);
    painter->restore();
}

} // namespace novu::ui
