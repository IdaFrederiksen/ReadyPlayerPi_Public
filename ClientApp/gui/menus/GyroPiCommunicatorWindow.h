// GyroPiCommunicatorWindow.h — GyroPi gameplay screen for the Communicator role
//
// Shown on the Communicator's RPi during UC3 gameplay. Renders the course
// (start, goal, black holes) and the live ball position received from the
// server. Reacts to black-hole hits with a brief flash and an inline
// "BALL RESET" message (M13).
//
//
// Coordinate system: server course is 800×480 pixels (COURSE_WIDTH ×
// COURSE_HEIGHT in ServerApp/GyroPi/GyroPi.h). The course view inside the
// window renders at the same scale so server coordinates map 1:1 to
// widget pixels.
//
// ECB role: BOUNDARY (part of the GUI subsystem).

#pragma once

#include "../base/BaseMenu.h"
#include <QLabel>
#include <QMetaType>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QPushButton>
#include <QTimer>
#include <QVector>
#include <QWidget>

// ── Course data ───────────────────────────────────────────────────────────────
struct GyroHoleInfo {
    float x;
    float y;
    float radius;
};

struct GyroCourseInfo {
    float startX;
    float startY;
    float goalX;
    float goalY;
    QVector<GyroHoleInfo> holes;
};

// ── Inner widget that paints the course ─────────────────────────────────────
// Kept as a separate class so the painting logic is isolated from the
// surrounding BaseMenu chrome (close button, background, exit button).
class GyroPiCourseView : public QWidget
{
    Q_OBJECT

public:
    explicit GyroPiCourseView(QWidget *parent = nullptr);

public slots:
    void onCourseLoaded(GyroCourseInfo info);
    void onBallUpdate(float x, float y);
    void onBlackHoleHit();
    void onScoreUpdate(int score);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    GyroCourseInfo courseInfo_;
    QPointF        ballPosition_;
    bool           courseLoaded_   = false;
    bool           blackHoleFlash_ = false;
    int            currentScore_   = 0;

    QTimer *flashTimer_ = nullptr;  // clears the flash after 400 ms

    void drawCourse(QPainter &painter);
    void drawHoles(QPainter &painter);
    void drawGoal(QPainter &painter);
    void drawBall(QPainter &painter);
    void drawHud(QPainter &painter);
    void drawBlackHoleMessage(QPainter &painter);
    void drawWaitingMessage(QPainter &painter);
};

// ── Communicator gameplay window ────────────────────────────────────────────
class GyroPiCommunicatorWindow : public BaseMenu
{
    Q_OBJECT

public:
    explicit GyroPiCommunicatorWindow(QWidget *parent = nullptr);

    // Forwarded to the inner course view. Safe to call from a non-GUI thread
    // via QMetaObject::invokeMethod with Qt::QueuedConnection.
public slots:
    void onCourseLoaded(GyroCourseInfo info);
    void onBallUpdate(float x, float y);
    void onBlackHoleHit();
    void onScoreUpdate(int score);

signals:
    // Emitted when the player taps the EXIT button (UC3 Ext. 2 / req. C1)
    void exitRequested();

private:
    GyroPiCourseView *courseView_ = nullptr;
    QPushButton      *exitBtn_    = nullptr;
};

// Skal registreres for at kunne sendes via Qt::QueuedConnection (cross-thread).
// qRegisterMetaType<GyroCourseInfo>() kaldes ved klient-startup (Client.cpp).
Q_DECLARE_METATYPE(GyroCourseInfo)
