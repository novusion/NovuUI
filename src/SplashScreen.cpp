#include "novu/ui/SplashScreen.h"

#include "novu/ui/AppMark.h"
#include "novu/ui/Feedback.h"
#include "novu/ui/Text.h"
#include "novu/ui/Theme.h"
#include "novu/ui/ThemeManager.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QVBoxLayout>

namespace novu::ui {

namespace {

// `.splash{width:440px;height:220px;border-radius:16px}` and
// `.content{padding:34px 36px 13px}`.
constexpr int kWidth = 440;
constexpr int kHeight = 220;
constexpr int kPaddingTop = 34;
constexpr int kPaddingH = 36;
constexpr int kPaddingBottom = 13;

// `.brand{gap:6px;margin-bottom:12px}`.
constexpr int kBrandGap = 6;
constexpr int kBrandMarginBottom = 12;
constexpr int kMarkSide = 24;

// `.status-row{margin-bottom:8px}` and `.track{height:3px}`.
constexpr int kStatusRowGap = 8;
constexpr int kTrackHeight = 3;

// `.splash::before` - a soft accent wash from the top, the same shape a
// splash keeps whichever application it belongs to.
constexpr double kWashAlpha = 0.10;

} // namespace

SplashScreen::SplashScreen(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);

    auto* content = new QVBoxLayout(this);
    content->setContentsMargins(kPaddingH, kPaddingTop, kPaddingH, kPaddingBottom);
    content->setSpacing(0);

    auto* brandRow = new QHBoxLayout();
    brandRow->setContentsMargins(0, 0, 0, 0);
    brandRow->setSpacing(kBrandGap);

    auto* mark = new AppMark(Glyph::None, this);
    mark->setShape(AppMark::Shape::Gem);
    mark->setSideOverride(kMarkSide);
    brandRow->addWidget(mark);

    // `h1{font-size:26px;font-weight:600}` - a step above PageTitle's own
    // scale, so the role is kept for its family and weight while the size
    // states the mockup's own number, the way titleFont() already does in
    // FloatingPanel.
    m_title = label(QString(), this, TypeRole::PageTitle, Ink::Default, false, 600);
    QFont titleFont = roleFont(this, TypeRole::PageTitle, false, 600);
    titleFont.setPixelSize(26);
    m_title->setFont(titleFont);
    brandRow->addWidget(m_title);
    brandRow->addStretch(1);
    content->addLayout(brandRow);
    content->addSpacing(kBrandMarginBottom);

    m_subtitle = label(QString(), this, TypeRole::Caption, Ink::Muted);
    m_subtitle->setWordWrap(false);
    content->addWidget(m_subtitle);

    content->addStretch(1);

    auto* metaRow = new QHBoxLayout();
    metaRow->setContentsMargins(0, 0, 0, 0);
    metaRow->addStretch(1);
    auto* metaColumn = new QVBoxLayout();
    metaColumn->setContentsMargins(0, 0, 0, 0);
    metaColumn->setSpacing(0);
    m_versionLabel = label(QString(), this, TypeRole::Mono, Ink::Muted);
    m_versionLabel->setAlignment(Qt::AlignRight);
    m_buildLabel = label(QString(), this, TypeRole::Mono, Ink::Muted);
    m_buildLabel->setAlignment(Qt::AlignRight);
    metaColumn->addWidget(m_versionLabel);
    metaColumn->addWidget(m_buildLabel);
    metaRow->addLayout(metaColumn);
    content->addLayout(metaRow);
    content->addSpacing(10);

    auto* statusRow = new QHBoxLayout();
    statusRow->setContentsMargins(0, 0, 0, 0);
    m_statusLabel = label(QString(), this, TypeRole::Mono, Ink::Muted);
    m_percentLabel = label(QString(), this, TypeRole::Mono, Ink::Accent);
    statusRow->addWidget(m_statusLabel);
    statusRow->addStretch(1);
    statusRow->addWidget(m_percentLabel);
    content->addLayout(statusRow);
    content->addSpacing(kStatusRowGap);

    m_progress = new ProgressBar(0, ProgressBar::Tone::Accent, this);
    m_progress->setTrackMetrics(kTrackHeight, kTrackHeight / 2);
    m_progress->setTrackBackground(token(ColorRole::BorderSubtle));
    content->addWidget(m_progress);

    setStatusText(QString());
    setPercent(0);
}

QString SplashScreen::title() const
{
    return m_title->text();
}

void SplashScreen::setTitle(const QString& title)
{
    m_title->setText(title);
}

QString SplashScreen::subtitle() const
{
    return m_subtitle->text();
}

void SplashScreen::setSubtitle(const QString& subtitle)
{
    m_subtitle->setText(subtitle);
}

QString SplashScreen::versionLine() const
{
    return m_versionLabel->text();
}

void SplashScreen::setVersionLine(const QString& text)
{
    m_versionLabel->setText(text);
}

QString SplashScreen::buildLine() const
{
    return m_buildLabel->text();
}

void SplashScreen::setBuildLine(const QString& text)
{
    m_buildLabel->setText(text);
}

QString SplashScreen::statusText() const
{
    return m_statusLabel->text();
}

void SplashScreen::setStatusText(const QString& text)
{
    m_statusLabel->setText(text);
}

void SplashScreen::setPercent(int percent)
{
    m_percent = qBound(0, percent, 100);
    m_percentLabel->setText(QStringLiteral("%1%").arg(m_percent));
    m_progress->setValue(m_percent);
}

QSize SplashScreen::sizeHint() const
{
    return QSize(kWidth, kHeight);
}

void SplashScreen::paintEvent(QPaintEvent*)
{
    const ThemeProfile& profile = themeFor(this);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const double radius = profile.radii.xl;
    const double border = profile.border(BorderRole::Card);
    const QRectF frame = QRectF(rect()).adjusted(border / 2.0, border / 2.0,
                                                 -border / 2.0, -border / 2.0);

    QPainterPath clip;
    clip.addRoundedRect(frame, radius, radius);
    painter.setClipPath(clip);

    painter.fillRect(rect(), profile.colors.windowBg);

    // `.splash::before` - a soft accent wash down from the top, centred.
    QRadialGradient wash(QPointF(width() / 2.0, height() * 0.38), width() * 0.58);
    QColor inner = profile.colors.accent;
    inner.setAlphaF(kWashAlpha);
    QColor outer = profile.colors.accent;
    outer.setAlphaF(0.0);
    wash.setColorAt(0.0, inner);
    wash.setColorAt(1.0, outer);
    painter.fillRect(rect(), wash);

    painter.setClipping(false);
    painter.setPen(QPen(profile.colors.border, border));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(frame, radius, radius);
}

} // namespace novu::ui
