// GyroPiPerformerWindow.h — GyroPi gameplay screen for the Performer role
//
// Shown on the Performer's RPi during UC3 gameplay. Provides graphical
// feedback that satisfies S1 + S2:
//   S1 — indicate the direction in which tilt is registered
//   S2 — indicate the degree (magnitude) of the registered tilt
//
// Layout:
//   * a circular dial of segments around the centre
//   * a ball in the middle of the dial that shifts in the tilt direction
//   * segments closest to the tilt direction darken with intensity
//     proportional to the tilt magnitude
//
// The Performer's GyroscopeReader produces normalised tilt values in
// [-1.0, 1.0] which the server then echoes back as `tilt_feedback`
// events. The window's setTilt() slot accepts those values.
//
// ECB role: BOUNDARY (part of the GUI subsystem).

#pragma once

#include "../base/BaseMenu.h"
#include <QPaintEvent>
#include <QPainter>
#include <QPushButton>
#include <QWidget>

// ── Inner widget that paints the tilt indicator ─────────────────────────────
class GyroPiTiltIndicator : public QWidget
{
    Q_OBJECT

public:
    explicit GyroPiTiltIndicator(QWidget *parent = nullptr);

public slots:
    // Tilt values normalised to [-1.0, 1.0]. Positive tiltX = tilt right,
    // positive tiltY = tilt down (matches the server's coordinate system).
    void setTilt(float tiltX, float tiltY);

    // Optional reset — drawn briefly when the server reports a black-hole
    // hit, just to give the Performer a visual cue too.
    void onBlackHoleHit();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    float tiltX_ = 0.0f;
    float tiltY_ = 0.0f;
    bool  flash_ = false;

    void drawDial(QPainter &painter, const QPointF &centre, qreal radius);
    void drawBall(QPainter &painter, const QPointF &centre, qreal radius);
};

// ── Performer gameplay window ───────────────────────────────────────────────
class GyroPiPerformerWindow : public BaseMenu
{
    Q_OBJECT

public:
    explicit GyroPiPerformerWindow(QWidget *parent = nullptr);

public slots:
    void setTilt(float tiltX, float tiltY);
    void onBlackHoleHit();

signals:
    void exitRequested();

private:
    GyroPiTiltIndicator *indicator_ = nullptr;
    QPushButton         *exitBtn_   = nullptr;
};
