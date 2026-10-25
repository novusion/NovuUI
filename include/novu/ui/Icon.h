#pragma once

#include "novu/ui/ThemeProfile.h"

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QString>
#include <QVector>

class QPainter;
class QPainterPath;
class QRectF;
class QWidget;

namespace novu::ui {

// The Novu icon set.
//
// One drawing per idea, and the drawing is the approved one: every glyph below
// is the outline the NovuSpace design carries, in the same 24x24 box, at the
// same 1.7 stroke weight, with round caps and joins. Nothing here is redrawn
// "in the spirit of" the design — the outlines are the design's own, so a Qt
// window and the HTML it was approved from show the same shape.
//
// Why not an icon font: iconFont() in Text.h reaches Segoe Fluent Icons, which
// is a different drawing at a different weight and is absent on anything that
// is not Windows 11. Why not QtSvg: a stroke outline is a QPainterPath, and
// asking for an extra Qt module plus a renderer per glyph to draw twelve line
// segments is the kind of weight this library exists to avoid.
//
// A glyph is painted, never instantiated. There is no icon *widget* here and
// there must not be one — an icon is decoration, and decoration that becomes a
// QWidget is how a card ends up with thirty children.

enum class Glyph {
    None,

    // module and navigation marks
    Overview, Wallet, Bots, Trades, Positions, Orders, History, Markets,
    Ai, Plug, Settings, Diagnostics, Sidebar, Chart, Activity, Cpu,

    // actions
    Search, Refresh, Plus, Minus, Play, Pause, Stop, Restart, Trash, Edit,
    Download, Upload, Close, Check, Copy, Save, Filter, External, Link,
    Sliders,

    // objects and state
    Key, Keys, Lock, Unlock, Warning, Info, Clock, Eye, Calendar, File,
    Folder, Archive, Shield, Zap, Flame, Scale, Bell, Globe, Mail, Vault,
    Inbox, Star, Book, Users,

    // help
    HelpTip, HelpCenter, WhatsNew,

    // NovuAI's set, at the same scale. Its mockup states these in a 16x16
    // box; they are the same outlines at 24, converted rather than redrawn.
    // Anything NovuUI already drew - Users, Stop, Refresh - is not repeated
    // here: one drawing per idea.
    Sun,
    Moon,
    Chat,
    Image,
    Video,
    Audio,
    Code,
    Branch,
    Commit,
    Push,
    Pull,
    Layers,
    Terminal,
    Send,
    Sparkle,
    Task,
    Grid,
    Graph,
    Brain,
    Hub,
    Design,
    Flow,
    At,
    Slash,
    Keyboard,
    PanelLeft,
    PanelRight,
    Agent,
    Workspace,
    Group,
    FanIn,
    FanOut,
    User,
    Doc,
    Cloud,

    // direction
    ChevronDown, Back, Forward, More,

    // A floating window's own controls. The design draws these as paths in
    // the same viewBox as every other icon - `M5 12h14` and a rounded square -
    // rather than leaving them to a platform title bar, because a `.floatwin`
    // is drawn by the application. Declared last so the existing enumerators
    // keep their values.
    //
    // Minus already draws the dash; these two exist so a bar can name what it
    // means rather than what it looks like, and so Restore has somewhere to
    // be.
    WindowMinimise, WindowMaximise, WindowRestore,

    // -- real-time communication ------------------------------------------
    //
    // Copied outline-for-outline from the approved NovuVoice design:
    //
    //     g:\Design\design\NovuVoice\mockups\NovuVoice-brand-mockup.html
    //
    // The library could not draw a call at all before these: there was no
    // phone, no microphone, no screen share and no read receipt. They are
    // family vocabulary rather than one product's - every Novusion window that
    // shows somebody calling needs the same phone - so they live here.
    //
    // Appended at the end on purpose: an enumerator inserted anywhere above
    // would renumber every value after it.
    Phone, PhoneIncoming, PhoneOutgoing, PhoneMissed, PhoneOff,
    Mic, MicOff, VideoOff, Monitor, Radio, Waveform,
    Hash, Paperclip, Smile, CheckDouble, WifiOff, Alert, Pin, Headset,

    // direction, the two the set lacked: a "previous / next" pair drawn as
    // the same chevron as ChevronDown (Back is an arrow, not its mirror).
    // Appended for the reason above.
    ChevronLeft, ChevronRight,

    // an open book, two pages (NovuAI's `book` - Calendar in the Flow view
    // menu); Book is the closed one. Appended for the reason above.
    BookOpen,
};

// The name the design uses for a glyph ("chevron-down"), and the reverse.
// Present so a spec, a JSON theme overlay or a gallery listing can name an
// icon without the caller reaching for the enumerator.
struct GlyphSource { Glyph glyph; QString name; QString svg; };
// Product-approved outlines share the same renderer. None assigns a named
// extension glyph; existing semantic glyphs can carry profile-specific art.
void registerGlyphSet(const QString& id,const QVector<GlyphSource>& sources,qreal viewBox=24.0,qreal strokeWidth=1.7);
QString glyphName(Glyph glyph);
Glyph glyphFromName(const QString& name,const QWidget* context=nullptr);

// Every glyph, in declaration order. The gallery draws this; a test walks it
// to prove no outline fails to parse.
QVector<Glyph> allGlyphs(const QWidget* context=nullptr);

// The outline in its own 24x24 coordinate space, parsed once and cached.
// Exposed for a component that wants to fold a glyph into a larger drawing
// rather than blit a pixmap into it.
const QPainterPath& glyphPath(Glyph glyph,const QWidget* context=nullptr);

// Strokes `glyph` to fit `box`, in `color`.
//
// The stroke weight scales with the box, exactly as the design's `stroke-width`
// does inside a scaled viewBox, so a 13px row glyph and a 20px header glyph are
// the same drawing rather than the same line weight at two sizes.
// `rotationDeg` turns the drawing about the centre of `box`, clockwise. It is
// here rather than left to the caller's painter transform because a spinner is
// the same outline at an angle, and a second drawing path for it would be a
// second thing to keep in step.
// `strokeWidth` is in the glyph's own 24x24 viewBox units, as the design
// writes it. 0 takes the set's own 1.7, which is what `svg[viewBox]` states
// for every icon; the disclosure chevrons are the exception the sheet makes
// explicitly - `.navbtn .tw svg{stroke-width:2}` and
// `.sb-bhead .cv svg{stroke-width:2.2}` - because a chevron at 11px in the
// body weight reads as a hairline rather than as a control.
void paintGlyph(QPainter* painter, Glyph glyph, const QRectF& box, const QColor& color,
                double rotationDeg = 0.0, double strokeWidth = 0.0,const QWidget* context=nullptr);

// -----------------------------------------------------------------------------
// The same outline, filled
// -----------------------------------------------------------------------------
// No second set of shapes: the glyph table is copied from the approved design
// and says so ("change the design first"), and a filled variant invented here
// would be the divergence it warns about. This paints THE SAME path with a
// brush instead of a pen.
//
// Only for a glyph whose outline is a single closed shape. A folder and a chat
// bubble are; a workspace cube is an outer hexagon plus the interior edges that
// make it read as a cube, and filling it would bury them.
// glyphFillsCleanly() answers which is which, so a caller does not have to know
// the shapes.
bool glyphFillsCleanly(Glyph glyph);

// A glyph whose solid form is its OUTLINE, with the interior detail dropped.
//
// A workspace cube is an outer hexagon plus the three edges that make it read
// as a cube. Those edges are drawn on an empty shape; once the shape is
// filled they have nothing to sit on and only break the silhouette up, which
// at sidebar size is the opposite of what filling it is for. Filled, the
// hexagon IS the icon.
//
// Separate from glyphFillsCleanly() because the two mean different things: a
// folder fills WITH its stroke and keeps every line it has, and this one
// keeps only the first.
bool glyphFillsAsSilhouette(Glyph glyph);
void paintGlyphFilled(QPainter* painter, Glyph glyph, const QRectF& box,
                      const QColor& color, const QWidget* context = nullptr);

// A square pixmap, cached per (glyph, size, colour, device ratio). Components
// that draw a glyph every paint event should call paintGlyph() instead; this is
// for the places Qt insists on a QPixmap or a QIcon.
// Rotation is quantised into a fixed number of steps before it reaches the
// cache key. A spinner asking for a continuously varying angle would otherwise
// add an entry per frame and never hit.
QPixmap glyphPixmap(Glyph glyph, int size, const QColor& color, qreal devicePixelRatio = 0.0,
                    double rotationDeg = 0.0,const QWidget* context=nullptr);

// A QIcon carrying the normal and disabled renderings, for QAbstractButton and
// anything else that takes one.
QIcon glyphIcon(Glyph glyph, int size, const QColor& color,const QWidget* context=nullptr);

// The profile's icon size for a control size, so no component invents a pixel
// size for a glyph. Small is a row action, Medium a toolbar, Large a header.
int glyphSizeFor(const QWidget* context, ControlSize size);

} // namespace novu::ui
