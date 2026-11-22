#include "control/NoteDetector.h"

#include "domain/NotePiConfig.h"
#include "domain/PianoFrequencies.h"

#include <cmath>
#include <unordered_map>

NoteDetector::NoteDetector(double fs, int frameSize)
    : fs_(fs), noteBins_(buildNoteTable(frameSize)) {}

std::vector<NoteDetector::NoteBin> NoteDetector::buildNoteTable(int frameSize) const {
    std::vector<NoteBin> table;
    table.reserve(piano::NOTES.size());

    // bin = round(frequency * frameSize / fs)
    // Ref: Eksempel_L10c.m: freq_bin = round(f_cutoff * M / f_sample)
    for (const auto& note : piano::NOTES) {
        NoteBin b;
        b.name           = note.name;
        b.midiPitch      = note.midiPitch;
        b.frequencyHz    = note.frequencyHz;
        b.fundamentalBin = static_cast<int>(std::round(note.frequencyHz * 1.0 * frameSize / fs_));
        b.overtone1Bin   = static_cast<int>(std::round(note.frequencyHz * 2.0 * frameSize / fs_));
        b.overtone2Bin   = static_cast<int>(std::round(note.frequencyHz * 3.0 * frameSize / fs_));
        table.push_back(b);
    }
    return table;
}

std::optional<std::string> NoteDetector::detectNote(const std::vector<float>& magnitudes) {
    namespace cfg = notepi::config;

    // 1) Silence check.
    float totalEnergy = 0.0f;
    for (float m : magnitudes) {
        totalEnergy += m;
    }
    if (totalEnergy < cfg::MIN_ENERGY) {
        // Lige kommet ind i silence efter aktiv tone — ryd vinduet så næste
        // tone ikke startes med leftover-detections fra den forrige.
        if (!silenceDetected_) {
            recentDetections_.clear();
        }
        silenceDetected_ = true;
        requireSilenceBeforeNext_ = false;
        return std::nullopt;
    }

    // Hvis sidste detection lige er committed, kræv at brugeren først går
    // gennem silence. Ellers ville en holdet tone trigge gentagne commits.
    if (requireSilenceBeforeNext_) {
        return std::nullopt;
    }

    // 2) Score each candidate note (fundamental + 2 overtones, weighted).
    //    Ref: fft_filt.m - energy at specific frequency bins.
    const int maxBin = static_cast<int>(magnitudes.size()) - 1;
    int bestIndex = -1;
    float bestScore = 0.0f;

    for (std::size_t i = 0; i < noteBins_.size(); ++i) {
        const NoteBin& b = noteBins_[i];
        float score = 0.0f;
        if (b.fundamentalBin <= maxBin) {
            score += cfg::FUNDAMENTAL_WEIGHT * magnitudes[static_cast<std::size_t>(b.fundamentalBin)];
        }
        if (b.overtone1Bin <= maxBin) {
            score += cfg::OVERTONE1_WEIGHT * magnitudes[static_cast<std::size_t>(b.overtone1Bin)];
        }
        if (b.overtone2Bin <= maxBin) {
            score += cfg::OVERTONE2_WEIGHT * magnitudes[static_cast<std::size_t>(b.overtone2Bin)];
        }
        if (score > bestScore) {
            bestScore = score;
            bestIndex = static_cast<int>(i);
        }
    }

    // 3) Dominance check.
    if (bestIndex < 0 || bestScore < cfg::DOMINANCE_THRESHOLD * totalEnergy) {
        return std::nullopt;
    }

    // 4) Push to majority window. Once full, return the most common note.
    recentDetections_.push_back(noteBins_[static_cast<std::size_t>(bestIndex)]);
    silenceDetected_ = false;

    if (recentDetections_.size() < cfg::RECENT_DETECTIONS_SIZE) {
        return std::nullopt;
    }

    std::unordered_map<int, int> counts;
    int winnerMidi = recentDetections_.front().midiPitch;
    int winnerCount = 0;
    for (const NoteBin& d : recentDetections_) {
        const int c = ++counts[d.midiPitch];
        if (c > winnerCount) {
            winnerCount = c;
            winnerMidi = d.midiPitch;
        }
    }

    std::string winnerName;
    for (const NoteBin& d : recentDetections_) {
        if (d.midiPitch == winnerMidi) {
            winnerName = d.name;
            break;
        }
    }

    recentDetections_.clear();
    requireSilenceBeforeNext_ = true;
    return winnerName;
}

void NoteDetector::reset() {
    recentDetections_.clear();
    silenceDetected_ = true;
    requireSilenceBeforeNext_ = false;
}
