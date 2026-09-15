#include "novu/ui/ModuleBar.h"

#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"

#include <QEvent>
#include <QFontMetrics>
#include <QPainter>
#include <QPen>

#include <utility>

namespace novu::ui {

namespace {

// `.modbtn { padding:5px 11px; gap:7px; border-radius:7px }`, `.modbar { gap:2px }`.
constexpr int kPaddingH = 11;
constexpr int kPaddingV = 5;
constexpr int kGap = 7;
constexpr int kBarGap = 2;
constexpr int kRadius = 7;

// `.modbtn svg { 15px }`.
constexpr int kGlyphSide = 15;

// `.modbtn .cnt { padding:0 5px }`.
constexpr int kCountPaddingH = 5;

// `[disabled]` is not in the design's module bar, but a module that is not
// licensed or not yet loaded has to be shown somehow. The rail's own disabled
// opacity, so the two agree.
constexpr double kDisabledOpacity = 0.42;

QFont moduleFont(const QWidget* context, bool current)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleFont(context, TypeRole::Body);
    font.setPixelSize(profile.type.sm);
    // `.modbtn.on { font-weight:600 }` - the active module is set heavier as
    // well as tinted, so the bar still reads where you are in a light theme
    // where the tint is faint.
    font.setWeight(current ? QFont::DemiBold : QFont::Medium);
    return font;
}

QFont countFont(const QWidget* context)
{
    const ThemeProfile& profile = themeFor(context);
    QFont font = roleMonoFont(context, TypeRole::Mono);
    font.setPixelSize(qMax(8, profile.type.xxs - 1));
    return font;
}

} // namespace

ModuleBar::ModuleBar(QWidget* parent)
    : ItemStrip(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

int ModuleBar::addModule(const QString& key, const QString& label, Glyph glyph)
{
    Module module;
    module.key = key;
    module.label = label;
    module.glyph = glyph;
    m_modules.append(module);
    if (currentIndex() < 0) {
        setCurrentIndex(0);
    }
    updateGeometry();
    update();
    return m_modules.size() - 1;
}

void ModuleBar::clear()
{
    m_modules.clear();
    resetStripState();
    updateGeometry();
    update();
}

int ModuleBar::indexOfKey(const QString& key) const
{
    for (int i = 0; i < m_modules.size(); ++i) {
        if (m_modules.at(i).key == key) {
            return i;
        }
    }
    return -1;
}

QString ModuleBar::keyAt(int index) const
{
    return index >= 0 && index < m_modules.size() ? m_modules.at(index).key : QString();
}

QString ModuleBar::currentKey() const
{
    return keyAt(currentIndex());
}

void ModuleBar::setCurrentKey(const QString& key)
{
    setCurrentIndex(indexOfKey(key));
}

const ModuleBar::Module* ModuleBar::find(const QString& key) const
{
    const int index = indexOfKey(key);
    return index < 0 ? nullptr : &m_modules.at(index);
}

ModuleBar::Module* ModuleBar::find(const QString& key)
{
    return const_cast<Module*>(std::as_const(*this).find(key));
}

void ModuleBar::setLabel(const QString& key, const QString& label)
{
    Module* module = find(key);
    if (!module || module->label == label) {
        return;
    }
    module->label = label;
    updateGeometry();
    update();
}

void ModuleBar::setCount(const QString& key, int count, CountTone tone)
{
    Module* module = find(key);
    if (!module || (module->count == count && module->tone == tone)) {
        return;
    }
    module->count = count;
    module->tone = tone;
    updateGeometry();
    update();
}

void ModuleBar::setModuleEnabled(const QString& key, bool enabled)
{
    Module* module = find(key);
    if (!module || module->enabled == enabled) {
        return;
    }
    module->enabled = enabled;
    update();
}

void ModuleBar::setCompact(bool compact)
{
    if (m_compact == compact) {
        return;
    }
    m_compact = compact;
    updateGeometry();
    update();
}

void ModuleBar::stripItemChosen(int index)
{
    emit moduleChosen(keyAt(index));
}

bool ModuleBar::stripItemEnabled(int index) const
{
    return index >= 0 && index < m_modules.size() && m_modules.at(index).enabled;
}

QString ModuleBar::stripItemTooltip(int index) const
{
    // Only when the label is not on screen. A tip repeating a word the pointer
    // is already over is noise.
    if (!m_compact || index < 0 || index >= m_modules.size()) {
        return QString();
    }
    return m_modules.at(index).label;
}

int ModuleBar::stripGap() const
{
    return kBarGap;
}

int ModuleBar::stripTop() const
{
    return 0;
}

int ModuleBar::stripItemHeight() const
{
    return height();
}

int ModuleBar::stripItemWidth(int index) const
{
    if (index < 0 || index >= m_modules.size()) {
        return 0;
    }
    const Module& module = m_modules.at(index);

    int width = 2 * kPaddingH;
    if (module.glyph != Glyph::None) {
        width += kGlyphSide;
    }
    if (!m_compact) {
        if (module.glyph != Glyph::None) {
            width += kGap;
        }
        width += QFontMetrics(moduleFont(this, index == currentIndex()))
                     .horizontalAdvance(module.label);
        if (module.count >= 0) {
            width += kGap + 2 * kCountPaddingH
                + QFontMetrics(countFont(this))
                      .horizontalAdvance(QString::number(module.count));
        }
    }
    return width;
}

StyleSpec ModuleBar::styleSpec() const
{
    StyleSpec spec;
    // The bar itself has no surface: it sits directly on the title bar, and a
    // plate here would be the box the design says not to draw.
    spec.background = token(ColorRole::Transparent);
    spec.border = token(ColorRole::Transparent);
    spec.foreground = token(ColorRole::TextDisabled);
    spec.radius = SurfaceRole::Toolbar;
    spec.borderWidth = BorderRole::Divider;
    spec.type = TypeRole::Body;
    return spec;
}

StyleSpec ModuleBar::moduleStyleSpec(int index) const
{
    StyleSpec spec = styleSpec();
    spec.radius = SurfaceRole::Button;
    spec.radiusStepOverride = static_cast<int>(RadiusStep::Small);

    spec.hoverBackground = token(ColorRole::SurfaceHover, 0.6);
    spec.hoverForeground = token(ColorRole::TextSecondary);
    // `.modbtn.on { background:rgba(--rl,.17); color:--acc-l }` — the one
    // accented thing in the chrome.
    spec.checkedBackground = token(ColorRole::Accent, 0.17);
    spec.checkedForeground = token(ColorRole::AccentLight);
    spec.disabledForeground = token(ColorRole::TextDisabled, kDisabledOpacity);

    Q_UNUSED(index);
    return spec;
}

ResolvedStyle ModuleBar::resolvedStyle() const
{
    return resolve(styleSpec(), this, height());
}

QSize ModuleBar::sizeHint() const
{
    const QFontMetrics fm(moduleFont(this, false));
    const int height = qMax(fm.height(), kGlyphSide) + 2 * kPaddingV;
    return QSize(stripWidth(), height);
}

QSize ModuleBar::minimumSizeHint() const
{
    return sizeHint();
}

bool ModuleBar::event(QEvent* event)
{
    if (event->type() == ThemeChangeEvent::type() || event->type() == QEvent::ParentChange) {
        updateGeometry();
        update();
    }
    return ItemStrip::event(event);
}

void ModuleBar::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QFont counts = countFont(this);
    const QFontMetrics countMetrics(counts);

    for (int i = 0; i < m_modules.size(); ++i) {
        const Module& module = m_modules.at(i);
        const ResolvedStyle style = resolve(moduleStyleSpec(i), this, height());
        const QRect box = itemRect(i);
        const bool current = i == currentIndex();
        const bool hot = !current && i == hoveredIndex() && module.enabled && isEnabled();

        if (current) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(style.checkedBackground);
            painter.drawRoundedRect(QRectF(box), kRadius, kRadius);
        } else if (hot || i == pressedIndex()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(style.hoverBackground);
            painter.drawRoundedRect(QRectF(box), kRadius, kRadius);
        }

        QColor ink = current ? style.checkedForeground
                             : (hot ? style.hoverForeground : style.foreground);
        if (!module.enabled || !isEnabled()) {
            ink = withAlpha(ink, kDisabledOpacity);
        }

        int left = box.left() + kPaddingH;
        if (module.glyph != Glyph::None) {
            const QRectF glyphBox(left, box.center().y() - kGlyphSide / 2.0,
                                  kGlyphSide, kGlyphSide);
            paintGlyph(&painter, module.glyph, glyphBox, ink);
            left = static_cast<int>(glyphBox.right()) + kGap;
        }

        if (m_compact) {
            continue;
        }

        int rightEdge = box.right() - kPaddingH;
        if (module.count >= 0) {
            const QString text = QString::number(module.count);
            const int pillWidth = countMetrics.horizontalAdvance(text) + 2 * kCountPaddingH;
            const int pillHeight = countMetrics.height() + 1;
            const QRectF pill(rightEdge - pillWidth, box.center().y() - pillHeight / 2.0,
                              pillWidth, pillHeight);

            QColor pillInk = profile.colors.textDisabled;
            QColor pillBorder = profile.colors.border;
            QColor pillFill = profile.colors.windowBg;
            if (module.tone == CountTone::Alert) {
                pillInk = profile.semantic.warning.text;
                pillBorder = profile.semantic.warning.border;
                pillFill = profile.semantic.warning.fill;
            } else if (current) {
                pillInk = profile.colors.accentLight;
                pillBorder = withAlpha(profile.colors.accent, 0.45);
                pillFill = Qt::transparent;
            }

            painter.setPen(QPen(pillBorder, 1));
            painter.setBrush(pillFill);
            painter.drawRoundedRect(pill, pillHeight / 2.0, pillHeight / 2.0);
            painter.setFont(counts);
            painter.setPen(pillInk);
            painter.drawText(pill, Qt::AlignCenter, text);
            rightEdge = static_cast<int>(pill.left()) - kGap;
        }

        const QFont font = moduleFont(this, current);
        const QFontMetrics fm(font);
        painter.setFont(font);
        painter.setPen(ink);
        const QRect textBox(left, box.top(), qMax(0, rightEdge - left), box.height());
        painter.drawText(textBox, Qt::AlignVCenter | Qt::AlignLeft,
                         fm.elidedText(module.label, Qt::ElideRight, textBox.width()));
    }

    if (hasFocus() && currentIndex() >= 0) {
        painter.setPen(QPen(profile.colors.accent, profile.border(BorderRole::Focus)));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(QRectF(itemRect(currentIndex())).adjusted(1, 1, -1, -1),
                                kRadius, kRadius);
    }
}

} // namespace novu::ui
