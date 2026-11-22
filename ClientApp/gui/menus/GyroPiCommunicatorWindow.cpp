// GyroPiCommunicatorWindow.cpp — Implementation

#include "GyroPiCommunicatorWindow.h"

#include <algorithm>
#include <QFont>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QRadialGradient>

// ════════════════════════════════════════════════════════════════════════════
//  GyroPiCourseView
// ════════════════════════════════════════════════════════════════════════════

// Server course dimensions
// Kept locally so this class does not depend on server headers.
static constexpr float SERVER_COURSE_WIDTH  = 800.0f;
static constexpr float SERVER_COURSE_HEIGHT = 480.0f;
static constexpr float SERVER_GOAL_RADIUS   = 30.0f;

GyroPiCourseView::GyroPiCourseView(QWidget *parent)
    : QWidget(parent)
{
    // The course view is sized by its parent layout; coordinates received
    // from the server (range 0..800 × 0..480) are scaled to the view's
    // actual size in scaleX() / scaleY() below so the layout adapts to the
    // available area.
    flashTimer_ = new QTimer(this);
    flashTimer_->setSingleShot(true);
    connect(flashTimer_, &QTimer::timeout, this, [this]() {
        blackHoleFlash_ = false;
        update();
    });
}

// ── Coordinate scaling helpers ──────────────────────────────────────────────
// Server sends ball/hole positions in the 800×480 course space. The view
// itself may be smaller, so every drawn coordinate must be scaled.
namespace {
    inline qreal scaleX(qreal serverX, int viewWidth) {
        return serverX * viewWidth / SERVER_COURSE_WIDTH;
    }
    inline qreal scaleY(qreal serverY, int viewHeight) {
        return serverY * viewHeight / SERVER_COURSE_HEIGHT;
    }
    // Holes use a single radius; scale by the smaller axis ratio so circles
    // remain circular even if the view's aspect ratio drifts slightly.
    inline qreal scaleRadius(qreal serverRadius, int viewWidth, int viewHeight) {
        const qreal sx = viewWidth  / SERVER_COURSE_WIDTH;
        const qreal sy = viewHeight / SERVER_COURSE_HEIGHT;
        return serverRadius * std::min(sx, sy);
    }
}

// ── Slots ───────────────────────────────────────────────────────────────────

void GyroPiCourseView::onCourseLoaded(GyroCourseInfo info)
{
    courseInfo_   = info;
    ballPosition_ = QPointF(info.startX, info.startY);
    courseLoaded_ = true;
    update();
}

void GyroPiCourseView::onBallUpdate(float x, float y)
{
    ballPosition_ = QPointF(x, y);
    update();
}

void GyroPiCourseView::onBlackHoleHit()
{
    blackHoleFlash_ = true;
    if (courseLoaded_) {
        ballPosition_ = QPointF(courseInfo_.startX, courseInfo_.startY);
    }
    flashTimer_->start(400);
    update();
}

void GyroPiCourseView::onScoreUpdate(int score)
{
    currentScore_ = score;
    update();
}

// ── Paint ───────────────────────────────────────────────────────────────────

void GyroPiCourseView::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    drawCourse(painter);

    if (!courseLoaded_) {
        drawWaitingMessage(painter);
        return;
    }

    drawHoles(painter);
    drawGoal(painter);
    drawBall(painter);
    drawHud(painter);

    if (blackHoleFlash_) {
        drawBlackHoleMessage(painter);
    }
}

// ── Drawing helpers ─────────────────────────────────────────────────────────

void GyroPiCourseView::drawCourse(QPainter &painter)
{
    // White play-field with the dark colored border that ties back to BaseMenu.
    // A subtle red colour overlays the field briefly when a hole is hit.
    QColor fieldColour = blackHoleFlash_ ? QColor(255, 235, 235)
                                         : QColor(255, 255, 255);

    painter.fillRect(rect(), fieldColour);

    QPen borderPen(QColor("#1a237e"), 3);
    painter.setPen(borderPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect().adjusted(1, 1, -2, -2));
}

void GyroPiCourseView::drawHoles(QPainter &painter)
{
    // Black filled circles — matches the GUI sketch.
    painter.setPen(QPen(Qt::black, 2));
    painter.setBrush(QColor(20, 20, 20));
    for (const GyroHoleInfo &hole : courseInfo_.holes) {
        const QPointF centre(scaleX(hole.x, width()),
                             scaleY(hole.y, height()));
        const qreal   r = scaleRadius(hole.radius, width(), height());
        painter.drawEllipse(centre, r, r);
    }
}

void GyroPiCourseView::drawGoal(QPainter &painter)
{
    // Hollow ring with a faint indigo fill "Goal Area".
    const QPointF goalCentre(scaleX(courseInfo_.goalX, width()),
                             scaleY(courseInfo_.goalY, height()));
    const qreal   goalR = scaleRadius(SERVER_GOAL_RADIUS, width(), height());

    painter.setBrush(QColor(26, 35, 126, 35));  
    painter.setPen(QPen(QColor("#1a237e"), 3));
    painter.drawEllipse(goalCentre, goalR, goalR);

    // Inner bullseye dot to make the target unambiguous
    painter.setBrush(QColor("#1a237e"));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(goalCentre, 4.0, 4.0);
}

void GyroPiCourseView::drawBall(QPainter &painter)
{
    // Red ball with a soft highlight.
    constexpr qreal BALL_RADIUS = 9.0;

    const QPointF centre(scaleX(ballPosition_.x(), width()),
                         scaleY(ballPosition_.y(), height()));

    QRadialGradient gradient(centre, BALL_RADIUS);
    gradient.setColorAt(0.0, QColor(255, 120, 120));
    gradient.setColorAt(1.0, QColor(200, 40, 40));

    painter.setBrush(gradient);
    painter.setPen(QPen(QColor(120, 20, 20), 1));
    painter.drawEllipse(centre, BALL_RADIUS, BALL_RADIUS);
}

void GyroPiCourseView::drawHud(QPainter &painter)
{
    // Small score badge in the top-left corner of the course area.
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(26, 35, 126, 220));
    painter.drawRoundedRect(QRectF(12, 12, 130, 32), 6, 6);

    painter.setPen(Qt::white);
    painter.setFont(QFont("Sans", 14, QFont::Bold));
    painter.drawText(QRectF(12, 12, 130, 32),
                     Qt::AlignCenter,
                     QString("Time: %1s").arg(currentScore_));
}

void GyroPiCourseView::drawBlackHoleMessage(QPainter &painter)
{
    // Banner across the middle of the course — clears after 400 ms
    //triggered when black hole is hit.
    QRectF banner(0, height() / 2.0 - 30, width(), 60);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(180, 40, 40, 220));
    painter.drawRect(banner);

    painter.setPen(Qt::white);
    painter.setFont(QFont("Sans", 22, QFont::Bold));
    painter.drawText(banner, Qt::AlignCenter,
                     "BLACK HOLE — BALL RESET");
}

void GyroPiCourseView::drawWaitingMessage(QPainter &painter)
{
    painter.setPen(QColor("#1a237e"));
    painter.setFont(QFont("Sans", 22, QFont::Bold));
    painter.drawText(rect(), Qt::AlignCenter, "Waiting for course...");
}

// ════════════════════════════════════════════════════════════════════════════
//  GyroPiCommunicatorWindow
// ════════════════════════════════════════════════════════════════════════════

GyroPiCommunicatorWindow::GyroPiCommunicatorWindow(QWidget *parent)
    : BaseMenu(parent)
{
    QWidget *central = centralWidget();

    // ── Course view fills most of the screen ────────────────────────────────
    // Reserves a 60 px top strip for the BaseMenu close button and a
    // 60 px bottom strip for the EXIT button. Width fits inside the 800 px
    // canvas. The course view is exactly
    // 720×360 — close to the 800×480 server coordinate space.
    courseView_ = new GyroPiCourseView(central);
    courseView_->setGeometry(40, 60, 720, 360);

    // ── EXIT button ─────────────────────────────────────────────────────────
    exitBtn_ = new QPushButton("EXIT", central);
    exitBtn_->setGeometry(800 - 110, 480 - 50, 80, 35);
    exitBtn_->setStyleSheet(BaseMenu::btnStyle());
    connect(exitBtn_, &QPushButton::clicked,
            this, &GyroPiCommunicatorWindow::exitRequested);
}

// ── Slot forwarders to the inner view ───────────────────────────────────────

void GyroPiCommunicatorWindow::onCourseLoaded(GyroCourseInfo info)
{
    courseView_->onCourseLoaded(info);
}

void GyroPiCommunicatorWindow::onBallUpdate(float x, float y)
{
    courseView_->onBallUpdate(x, y);
}

void GyroPiCommunicatorWindow::onBlackHoleHit()
{
    courseView_->onBlackHoleHit();
}

void GyroPiCommunicatorWindow::onScoreUpdate(int score)
{
    courseView_->onScoreUpdate(score);
}
