#pragma once

// FIR filter (Direct Form I).
// DSB Lesson 9 - FIR filter design.
// Ref: Eksempel_L10c.m (frequency sampling method).
//
//   y[n] = b[0]*x[n] + b[1]*x[n-1] + ... + b[M]*x[n-M]
//
// Equivalent to Matlab: y = filter(b, 1, x)

#include <vector>

class FirFilter {
public:
    FirFilter();

    // Convolution: filteredFrame[n] = sum_k coefficients_[k] * delayLine_[k]
    // delayLine_ shifts forward each sample with rawFrame[n] inserted at index 0.
    void applyFilter(const std::vector<float>& rawFrame, std::vector<float>& filteredFrame);

    void reset();

private:
    std::vector<double> coefficients_;
    std::vector<double> delayLine_;
};
