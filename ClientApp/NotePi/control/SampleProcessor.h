#pragma once

#include <optional>
#include <vector>

class SampleProcessor {
public:
    SampleProcessor(int frameSize, int hopSize);

    // Append samples to internal buffer. When >= frameSize is buffered,
    // copy out a frame, drop hopSize from the front, and return the frame.
    // Otherwise return nullopt.
    std::optional<std::vector<float>> prepareFrame(const std::vector<float>& samples);

    void reset();

private:
    int frameSize_;
    int hopSize_;
    std::vector<float> buffer_;
};
