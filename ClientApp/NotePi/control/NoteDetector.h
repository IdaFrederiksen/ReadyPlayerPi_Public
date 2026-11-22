#pragma once

// Note detection from FFT power spectrum.
// DSB Lesson 10 - Real-time analysis.
// Ref: fft_filt.m - energy at specific frequency bins.
//
// For each piano note: score = W0*|X[f]|^2 + W1*|X[2f]|^2 + W2*|X[3f]|^2.
// Note with the highest score wins, gated by silence + dominance + majority.

#include <optional>
#include <string>
#include <vector>

class NoteDetector {
public:
    NoteDetector(double fs, int frameSize);

    // Returns the name of the detected note ("A4", "C#5", ...) once
    // a stable majority is reached, or nullopt if undecided / silence.
    std::optional<std::string> detectNote(const std::vector<float>& magnitudes);

    void reset();

private:
    struct NoteBin {
        std::string name;
        int         midiPitch;
        double      frequencyHz;
        int         fundamentalBin;
        int         overtone1Bin;
        int         overtone2Bin;
    };

    std::vector<NoteBin> buildNoteTable(int frameSize) const;

    double fs_;
    std::vector<NoteBin> noteBins_;
    std::vector<NoteBin> recentDetections_;
    bool silenceDetected_ = true;

    // Sat efter en note er committed; forhindrer at samme (eller en ny)
    // tone committes igen før brugeren har sluppet og passeret silence.
    bool requireSilenceBeforeNext_ = false;
};
