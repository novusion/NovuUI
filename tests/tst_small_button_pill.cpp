// A small button is rounded wherever it is placed.
//
// From NovuAI's Share invitation card, where Accept - the same component and
// preset as the chat composer's Send (primaryButton at ControlSize::Small) -
// was reported square with a strip on its right edge. That was not
// reproduced here under the offscreen platform; what this pins is that the
// preset, in the card's own row and in the composer's, draws its corners
// round at 100, 125 and 150 % scale (run with QT_SCALE_FACTOR), measured
// through the parent so the corners show what is really behind them.

#include "novu/ui/Button.h"
#include "novu/ui/Card.h"
#include "novu/ui/StyleSheet.h"
#include "novu/ui/Text.h"
#include "novu/ui/ThemeManager.h"
#include "novu/ui/ThemeProfile.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QImage>
#include <QStyle>
#include <QTest>
#include <QVBoxLayout>
#include <QWidget>

using namespace novu::ui;

class SmallButtonPillTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // The base style NovuAI runs on (main.cpp: app.setStyle("Fusion")).
        QApplication::setStyle(QStringLiteral("Fusion"));
        ThemeManager::instance().registerProfile(novuAiProfile());
        ThemeManager::instance().setDefaultProfileId(QStringLiteral("novuai"));
    }
    void smallButtonsAreRoundedWherePlaced();
};

static void expectPill(const char* what, Button* b)
{
    // The PARENT's pixels where the button is. b->grab() paints the button's
    // own background role first, which the style sheet has set to its fill,
    // so its corners would read as filled even on a perfect pill.
    QWidget* parent = b->parentWidget();
    const QImage img = parent->grab(b->geometry()).toImage();
    const QColor corner = img.pixelColor(1, 1);
    const QColor mid = img.pixelColor(img.width() / 2, img.height() / 2);
    const QColor right = img.pixelColor(img.width() - 1, img.height() / 2);
    qInfo().noquote() << QStringLiteral("%1: geom=%2x%3 hint=%4x%5 corner=%6 mid=%7 rightEdge=%8 radius=%9")
        .arg(QLatin1String(what)).arg(b->width()).arg(b->height())
        .arg(b->sizeHint().width()).arg(b->sizeHint().height())
        .arg(corner.name(QColor::HexArgb), mid.name(QColor::HexArgb), right.name(QColor::HexArgb))
        .arg(b->resolvedStyle().radius);
    qInfo().noquote() << "  sheet:" << b->styleSheet().left(260) << " style:" << b->style()->name();
    img.save(QStringLiteral("J:/tmp/pill_%1.png").arg(QLatin1String(what)));
    // The button's own fill, just inside its left end at mid height.
    const QColor fill = img.pixelColor(img.height() / 2, img.height() / 2);
    QVERIFY2(corner != fill,
             qPrintable(QStringLiteral("%1 is not rounded: its corner is its own fill %2")
                            .arg(QLatin1String(what), fill.name(QColor::HexArgb))));
}

void SmallButtonPillTest::smallButtonsAreRoundedWherePlaced()
{
    // The composer's Send, as ModChat builds it: in a footer row.
    QWidget composer;
    auto* foot = new QHBoxLayout(&composer);
    foot->addStretch(1);
    auto* send = primaryButton(QStringLiteral("Send"), &composer);
    send->setControlSize(ControlSize::Small);
    foot->addWidget(send);
    composer.resize(400, 60);
    // As the application does: the root carries the profile's sheet and the
    // application its style.
    applyStyleSheet(&composer, QStringLiteral("novuai"));
    composer.show();

    // The invitation card's row, as ToolWindows builds it.
    QWidget panel;
    panel.setFixedWidth(360);
    auto* column = new QVBoxLayout(&panel);
    auto* card = new Card(QStringLiteral("Test_Modulu"), &panel);
    auto* fromRow = new QWidget(card);
    auto* from = new QHBoxLayout(fromRow);
    from->setContentsMargins(0, 0, 0, 0);
    from->setSpacing(6);
    from->addWidget(elidedLabel(QStringLiteral("Hub account"), fromRow, TypeRole::Body, Ink::Default), 1);
    auto* decline = new Button(QStringLiteral("Decline"), Button::Variant::Secondary, fromRow);
    decline->setControlSize(ControlSize::Small);
    auto* accept = primaryButton(QStringLiteral("Accept"), fromRow);
    accept->setControlSize(ControlSize::Small);
    from->addWidget(decline, 0, Qt::AlignVCenter);
    from->addWidget(accept, 0, Qt::AlignVCenter);
    card->body()->addWidget(fromRow);
    column->addWidget(card);
    applyStyleSheet(&panel, QStringLiteral("novuai"));
    panel.show();
    QTest::qWait(100);

    expectPill("send", send);
    expectPill("accept", accept);
    expectPill("decline", decline);
}

QTEST_MAIN(SmallButtonPillTest)
#include "tst_small_button_pill.moc"
