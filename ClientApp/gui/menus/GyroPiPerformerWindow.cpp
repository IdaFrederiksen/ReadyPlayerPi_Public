// GyroPiPerformerWindow.cpp — Implementation

#include "GyroPiPerformerWindow.h"

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QPen>
#include <QPointF>
#include <QRadialGradient>
#include <QtMath>
#include <algorithm>
#include <cmath>

// ════════════════════════════════════════════════════════════════════════════
//  GyroPiTiltIndicator
// ════════════════════════════════════════════════════════════════════════════

namespace {
    // Number of segments in the radial dial. 24 gives a smooth feel while
    // staying readable at the 7" RPi display resolution.
    constexpr int   NUM_SEGMENTS         = 24;
    // How wide each segment is (in degrees). A bit less than the spacing
    // so neighbouring segments have a visible gap.
    constexpr qreal SEGMENT_ARC_DEGREES  = 11.0;
    // Visual fraction of the dial radius covered by the segment "tick".
    constexpr qreal SEGMENT_INNER_FRAC   = 0.78;
    constexpr qreal SEGMENT_OUTER_FRAC   = 1.00;
    // How much the ball drifts off centre in proportion to tilt magnitude.
    // Keeps the ball inside the inner ring even at full tilt.
    constexpr qreal BALL_DRIFT_FRAC      = 0.35;
    // Tilt magnitude (after clamping) at which a segment is fully dark.
    constexpr qreal FULL_DARK_MAGNITUDE  = 1.0;
}

GyroPiTiltIndicator::GyroPiTiltIndicator(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent, false);
}

void GyroPiTiltIndicator::setTilt(float tiltX, float tiltY)
{
    // Server applies position -= tilt, so negate here to match game direction.
    tiltX_ = std::clamp(-tiltX, -1.0f, 1.0f);
    tiltY_ = std::clamp(-tiltY, -1.0f, 1.0f);
    update();
}

void GyroPiTiltIndicator::onBlackHoleHit()
{
    flash_ = true;
    update();
    // Brief visual flag — clears on the next tilt update or next paint.
    // No timer is strictly needed because tilt updates arrive at 30 Hz.
}

void GyroPiTiltIndicator::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // White play-field with a subtle indigo border.
    QColor fieldColour = flash_ ? QColor(255, 235, 235) : QColor(255, 255, 255);
    painter.fillRect(rect(), fieldColour);
    flash_ = false;

    QPen borderPen(QColor("#1a237e"), 3);
    painter.setPen(borderPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect().adjusted(1, 1, -2, -2));

    // The dial sits in the centre.
    const QPointF centre(width() / 2.0, height() / 2.0);
    const qreal   radius = std::min(width(), height()) * 0.40;

    drawDial(painter, centre, radius);
    drawBall(painter, centre, radius);
}

void GyroPiTiltIndicator::drawDial(QPainter &painter,
                                    const QPointF &centre,
                                    qreal radius)
{
    // Tilt direction (in degrees on screen, measuring clockwise from "up").
    // Screen Y grows downwards, so  atan2(y, x) — which
    // measures counter-clockwise from "right" — needs to be transformed.
    //   * up = -Y in screen space → tiltY = -1 means "up"
    //   * angle 0° on the dial = up 
    const qreal magnitude = std::min(qreal(1.0), qreal(std::hypot(tiltX_, tiltY_)));
    const bool  hasTilt   = magnitude > 0.02;          // dead-zone

    // Direction in screen-space radians, 0 = up, growing clockwise.
    qreal directionRad = 0.0;
    if (hasTilt) {
        directionRad = std::atan2(tiltX_, -tiltY_);    // see above
    }

    const qreal innerR = radius * SEGMENT_INNER_FRAC;
    const qreal outerR = radius * SEGMENT_OUTER_FRAC;

    for (int i = 0; i < NUM_SEGMENTS; ++i) {
        // Segment angle on screen, 0° at top, growing clockwise.
        const qreal segAngleRad =
            (qreal(i) / NUM_SEGMENTS) * 2.0 * M_PI;

        // Angular distance between this segment and the tilt direction.
        qreal delta = segAngleRad - directionRad;
        // Wrap to [-π, π]
        while (delta >  M_PI) delta -= 2.0 * M_PI;
        while (delta < -M_PI) delta += 2.0 * M_PI;
        const qreal absDelta = std::abs(delta);

        // Segments within ~25° of the tilt vector darken with magnitude.
        // 0 at full distance (π) → 1 at delta = 0.
        // Uses a smooth falloff.
        constexpr qreal HALF_BEAM = M_PI / 7.0;        // ≈ 25.7°
        qreal proximity = 0.0;
        if (hasTilt && absDelta < HALF_BEAM) {
            proximity = 1.0 - (absDelta / HALF_BEAM);
        }
        const qreal intensity = std::clamp(
            proximity * (magnitude / FULL_DARK_MAGNITUDE), 0.0, 1.0);

        // Base segment colour: light grey. Highlighted segments go toward
        // the dark indigo accent so the tilt direction reads instantly.
        const int base = 210;                          // light grey
        const QColor baseColour(base, base, base);
        const QColor highlightColour("#1a237e");
        const QColor segColour(
            int(baseColour.red()   * (1.0 - intensity) + highlightColour.red()   * intensity),
            int(baseColour.green() * (1.0 - intensity) + highlightColour.green() * intensity),
            int(baseColour.blue()  * (1.0 - intensity) + highlightColour.blue()  * intensity)
        );

        // Draw a trapezoid from inner radius to outer radius along the
        // segment angle, with a small gap between neighbours.
        const qreal halfArc = qDegreesToRadians(SEGMENT_ARC_DEGREES) / 2.0;

        // Convert dial angle (0 = up, clockwise) into Cartesian unit vectors.
        auto unit = [](qreal angRad) {
            return QPointF(std::sin(angRad), -std::cos(angRad));
        };

        const QPointF dirA = unit(segAngleRad - halfArc);
        const QPointF dirB = unit(segAngleRad + halfArc);

        QPolygonF poly;
        poly << centre + dirA * innerR
             << centre + dirA * outerR
             << centre + dirB * outerR
             << centre + dirB * innerR;

        painter.setPen(Qt::NoPen);
        painter.setBrush(segColour);
        painter.drawPolygon(poly);
    }
}

void GyroPiTiltIndicator::drawBall(QPainter &painter,
                                    const QPointF &centre,
                                    qreal radius)
{
    // Ball drifts off-centre proportional to the tilt vector. Magnitude is
    // capped so the ball stays inside the dial regardless of tilt input.
    const qreal driftLimit = radius * BALL_DRIFT_FRAC;
    const qreal magnitude  = std::min(qreal(1.0), qreal(std::hypot(tiltX_, tiltY_)));

    QPointF offset(0, 0);
    if (magnitude > 0.0001) {
        offset = QPointF(tiltX_, tiltY_) * driftLimit;
    }

    const QPointF ballCentre = centre + offset;
    const qreal   ballRadius = radius * 0.30;

    // Blue 3D-looking ball.
    QRadialGradient gradient(
        ballCentre + QPointF(-ballRadius * 0.30, -ballRadius * 0.30),
        ballRadius * 1.4);
    gradient.setColorAt(0.00, QColor(180, 220, 255));
    gradient.setColorAt(0.55, QColor( 60, 130, 220));
    gradient.setColorAt(1.00, QColor( 20,  50, 130));

    painter.setBrush(gradient);
    painter.setPen(QPen(QColor("#0d1554"), 1));
    painter.drawEllipse(ballCentre, ballRadius, ballRadius);
}

// ════════════════════════════════════════════════════════════════════════════
//  GyroPiPerformerWindow
// ════════════════════════════════════════════════════════════════════════════

GyroPiPerformerWindow::GyroPiPerformerWindow(QWidget *parent)
    : BaseMenu(parent)
{
    QWidget *central = centralWidget();

    // Indicator fills the central play-field, matching the sketch.
    indicator_ = new GyroPiTiltIndicator(central);
    indicator_->setGeometry(40, 60, 720, 360);

    // EXIT button bottom-right (req. C1).
    exitBtn_ = new QPushButton("EXIT", central);
    exitBtn_->setGeometry(800 - 110, 480 - 50, 80, 35);
    exitBtn_->setStyleSheet(BaseMenu::btnStyle());
    connect(exitBtn_, &QPushButton::clicked,
            this, &GyroPiPerformerWindow::exitRequested);
}

void GyroPiPerformerWindow::setTilt(float tiltX, float tiltY)
{
    indicator_->setTilt(tiltX, tiltY);
}

void GyroPiPerformerWindow::onBlackHoleHit()
{
    indicator_->onBlackHoleHit();
}
