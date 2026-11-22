#pragma once

// FFT power-spectrum computation.
// DSB Lesson 10 - Filtering with DFT / spectral multiplication.
// Ref: fft_filt.m - "X = fft(x, N_fft)" and "|X|" computation.
//
// Hann window -> kissfft -> |X[k]|^2 = re^2 + im^2.

#include <vector>
#include "kiss_fftr.h"

class FftAnalyzer {
public:
    FftAnalyzer(int frameSize, double fs);
    ~FftAnalyzer();

    FftAnalyzer(const FftAnalyzer&) = delete;
    FftAnalyzer& operator=(const FftAnalyzer&) = delete;

    // Multiplies filteredFrame by Hann window, computes FFT,
    // writes |X[k]|^2 (power spectrum) to magnitudes (size frameSize/2 + 1).
    void computeSpectrum(const std::vector<float>& filteredFrame, std::vector<float>& magnitudes);

private:
    int frameSize_;
    double fs_;
    std::vector<float> hannWindow_;
    kiss_fftr_state* cfg_ = nullptr;
};
