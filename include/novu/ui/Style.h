#pragma once

#include <QProxyStyle>

class QApplication;

namespace novu::ui {

// The Novu style: the parts of a control that only a QStyle can reach.
//
// Almost everything in this library is either self-painted or driven by the
// scope stylesheet, and that covers a widget the library builds. It does not
// cover a check indicator inside a table cell, a tree item, a checkable group
// box or a menu, because those are not widgets - they are primitives the style
// draws, and no stylesheet selector reaches them.
//
// It also does not cover a plain QCheckBox an application created itself. The
// scope sheet can colour that one's box but cannot put a shape inside it, so
// the only state a sheet can express is "flood the box with the accent" - a
// solid square that reads as a colour swatch rather than as something ticked.
//
// A style is the one mechanism that reaches all of them at once. It has to be
// the *application's* style: Qt does not propagate a style set on a subtree
// root to the widgets inside it (verified, not assumed - QWidget::style() falls
// back to QApplication::style() and never consults the parent), so scoping this
// the way ThemeScope scopes tokens is not possible.
//
// That is not a scoping regression. The style resolves its colours through
// themeFor(widget) like every other component, so a host process running
// several products draws each product's checkboxes in that product's palette
// from this one style object.
class NovuStyle : public QProxyStyle
{
    Q_OBJECT

public:
    explicit NovuStyle(QStyle* base = nullptr);

    void drawPrimitive(PrimitiveElement element, const QStyleOption* option,
                       QPainter* painter, const QWidget* widget = nullptr) const override;

    int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr,
                    const QWidget* widget = nullptr) const override;
};

// Installs NovuStyle as the application style, wrapping whatever is there.
//
// Idempotent, and safe to call from anywhere: applyStyleSheet() calls it, so an
// application that themes a subtree gets correct indicators without knowing
// this exists. Call it explicitly from main() in an application that draws
// checkable table or tree items before it themes anything.
//
// Returns the installed style, or nullptr when there is no QApplication yet.
NovuStyle* installNovuStyle();

// Chooses the mark drawn inside a checked indicator. Set on the widget as a
// dynamic property, so it works for a plain QCheckBox as well as for
// novu::ui::CheckBox, and so the style can read it without knowing the class.
namespace prop {
// "check" (default) | "cross"
inline constexpr const char* mark = "novuMark";
} // namespace prop

} // namespace novu::ui
