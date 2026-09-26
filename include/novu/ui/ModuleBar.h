#pragma once

#include "novu/ui/Icon.h"
#include "novu/ui/ItemStrip.h"
#include "novu/ui/StyleSpec.h"

#include <QString>
#include <QVector>

namespace novu::ui {

// The application's top-level navigation, in the title bar.
//
// The design puts it there deliberately and explains why: *The four modules ARE
// the application's navigation, so they sit in the titlebar itself rather than
// in a bar of their own. Purple marks the active one and nothing else in the
// chrome uses it.*
//
// That second sentence is the constraint worth keeping. The accent appears in
// exactly one place in the window chrome — here, on the module you are in — so
// a glance at the frame answers "where am I" before anything inside it has been
// read. A second accented control in the title bar takes that away.
//
// ── how it differs from Segmented ────────────────────────────────────────
//
// They are the same machinery and not the same control, which is why they share
// ItemStrip rather than one being a variant of the other:
//
//   Segmented  a *picker*. Values of one setting, in a sunken tray, with a
//              raised plate marking the choice. Nothing happens when you
//              choose; something is now set.
//   ModuleBar  *navigation*. Places in the application, on the bare title bar,
//              with a tinted plate marking where you are. Choosing takes you
//              somewhere.
//
// A tray here would draw a box round the middle of the title bar; an accent
// tint in a settings row would read as a status. Merging them into one class
// with a `flat` flag would be one class with two designs in it.
class ModuleBar : public ItemStrip
{
    Q_OBJECT

public:
    // The tone of a module's count. The design: *A count that needs a person is
    // amber. This is the one place a status colour appears in the chrome, and
    // it is the NovuAdmin rule kept verbatim.*
    enum class CountTone { Normal, Alert };

    explicit ModuleBar(QWidget* parent = nullptr);

    // `key` is the caller's identifier and what `moduleChosen` carries. Never
    // shown, so it survives translation.
    int addModule(const QString& key, const QString& label, Glyph glyph = Glyph::None);
    void clear();

    int count() const { return m_modules.size(); }
    int indexOfKey(const QString& key) const;
    QString keyAt(int index) const;
    QString currentKey() const;
    void setCurrentKey(const QString& key);

    void setLabel(const QString& key, const QString& label);
    // -1 removes the pill. 0 draws a zero, which says something different.
    void setCount(const QString& key, int count, CountTone tone = CountTone::Normal);
    void setModuleEnabled(const QString& key, bool enabled);

    // Glyphs only — what the bar becomes in a narrow window. The design hides
    // `.modbtn span.ml` and keeps the icon, so a module is still reachable at
    // any width.
    bool isCompact() const { return m_compact; }
    void setCompact(bool compact);

    StyleSpec styleSpec() const;
    StyleSpec moduleStyleSpec(int index) const;
    ResolvedStyle resolvedStyle() const;

    // What paintEvent actually draws for this module's label - elided if the
    // module's own box turned out not to have room for it. Exposed so a
    // caller (a test, in particular) can ask "does this module still say its
    // whole name" without comparing pixels.
    QString displayedLabel(int index) const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void moduleChosen(const QString& key);

protected:
    void paintEvent(QPaintEvent* event) override;
    bool event(QEvent* event) override;
    // The ring is the keyboard's, not the mouse's - see FocusRing in Theme.h.
    void focusInEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

    int stripCount() const override { return m_modules.size(); }
    int stripItemWidth(int index) const override;
    void stripItemChosen(int index) override;
    bool stripItemEnabled(int index) const override;
    QString stripItemTooltip(int index) const override;
    int stripGap() const override;
    int stripTop() const override;
    int stripItemHeight() const override;

private:
    // The box displayedLabel() elides against and paintEvent() draws the
    // result into - one computation, asked twice, rather than two
    // computations that are supposed to agree.
    QRect labelTextBox(int index) const;

    FocusRing m_ring;

    struct Module
    {
        QString key;
        QString label;
        Glyph glyph = Glyph::None;
        int count = -1;
        CountTone tone = CountTone::Normal;
        bool enabled = true;
    };

    const Module* find(const QString& key) const;
    Module* find(const QString& key);

    QVector<Module> m_modules;
    bool m_compact = false;
};

} // namespace novu::ui
