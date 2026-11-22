#include "boundary/MicrophoneDriver.h"
#include "control/SoundAnalyzer.h"
#include "domain/NotePiConfig.h"
#include "domain/NoteSequence.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

int main() {
    namespace cfg = notepi::config;
    constexpr int durationSeconds = 10;

    const char *envDevice = std::getenv("RPP_MIC_DEVICE");
    std::string device = (envDevice && *envDevice) ? envDevice : cfg::MIC_DEVICE;
    std::cout << "Mic device: " << device << "\nListening for "
              << durationSeconds << " seconds...\n" << std::flush;

    MicrophoneDriver mic(device,
                         static_cast<unsigned int>(cfg::SAMPLE_RATE_HZ),
                         static_cast<std::size_t>(cfg::BLOCK_SIZE));
    if (!mic.start()) {
        std::cerr << "Failed to start microphone.\n";
        return 1;
    }

    SoundAnalyzer analyzer;
    NoteSequence  sequence;

    const auto  startedAt         = std::chrono::steady_clock::now();
    std::size_t lastReportedCount = 0;

    while (std::chrono::steady_clock::now() - startedAt
           < std::chrono::seconds(durationSeconds)) {
        std::vector<float> samples;
        if (mic.readBlock(samples) == 0) {
            continue;
        }

        analyzer.analyze(samples, sequence);

        if (sequence.getNotes().size() != lastReportedCount) {
            lastReportedCount = sequence.getNotes().size();
            std::cout << "NOTE: " << sequence.getNotes().back()
                      << "\n" << std::flush;
        }
    }

    std::cout << "Detected " << sequence.getNotes().size() << " notes.\n";

    mic.stop();
    return 0;
}
