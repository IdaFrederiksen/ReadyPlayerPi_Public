#pragma once

// All adjustable NotePi parameters.
// Adjust here, recompile, no other file edits.

namespace notepi::config {

// Audio / framing
inline constexpr int BLOCK_SIZE = 256;                  // ALSA period: samples per readBlock()
inline constexpr int HOP_SIZE = 1024;                   // SampleProcessor advance per frame
inline constexpr int FRAME_SIZE = 2048;                 // FFT frame size (overlap = FRAME_SIZE - HOP_SIZE)
inline constexpr double SAMPLE_RATE_HZ = 48000.0;

inline constexpr const char* MIC_DEVICE = "default";
// inline constexpr const char* MIC_DEVICE = "plughw:2,0";  // explicit ALSA device if "default" doesn't pick up the right mic

// Game flow
inline constexpr int SEQUENCE_LENGTH = 5;               // Notes per attempt

// Note detection
// MIN_ENERGY: total spectrum energy below this is treated as silence.
//   Raise if background noise triggers false detections.
//   Lower if quiet notes get missed.
//   200 = sensible default for laptop mic with capture ~50% (use `make mic-fix`).
inline constexpr float MIN_ENERGY = 1000.0f;

// DOMINANCE_THRESHOLD: winning note must hold at least this fraction of total energy.
//   Raise if noise triggers wrong notes. Lower if real notes get rejected.
inline constexpr float DOMINANCE_THRESHOLD = 0.30f;

// Score weights for fundamental + first two overtones.
// score = W0 * |X[f]|^2 + W1 * |X[2f]|^2 + W2 * |X[3f]|^2
inline constexpr float FUNDAMENTAL_WEIGHT = 1.0f;
inline constexpr float OVERTONE1_WEIGHT = 0.3f;
inline constexpr float OVERTONE2_WEIGHT = 0.1f;

// Majority window: a candidate note is committed only after this many
// consecutive detections, taking the most frequent as the winner.
// 12 frames at HOP_SIZE=1024 / 48kHz ≈ 256 ms held tone before commit.
inline constexpr std::size_t RECENT_DETECTIONS_SIZE = 18;

}  // namespace notepi::config
