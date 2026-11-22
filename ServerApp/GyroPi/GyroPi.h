// ServerApp/GyroPi/GyroPi.h — GyroPi shared primitives
// Vec2, physics constants, and GameState (the mutex-protected data shared
// between the input select thread and the physics tick thread).

#pragma once
#include <mutex>

// ── 2D vector ─────────────────────────────────────────────────────────────────
struct Vec2 { float x, y; };

// ── Physics constants ─────────────────────────────────────────────────────────
static constexpr int   COURSE_WIDTH  = 800;
static constexpr int   COURSE_HEIGHT = 480;
static constexpr float BALL_SPEED    = 3.0f;
static constexpr int   TICK_MS       = 33;    // ~30 Hz
static constexpr float GOAL_RADIUS   = 30.0f;

// ── Shared state between Thread 1 (input) and Thread 2 (physics) ─────────────
// Ball and Course are owned exclusively by the physics thread and live in
// GyroPiGame directly — only tilt input, score, and the active flag need
// cross-thread synchronisation.
struct GameState {
    std::mutex mutex;
    float      tiltX  = 0;
    float      tiltY  = 0;
    int        score  = 0;
    bool       active = false;
};
