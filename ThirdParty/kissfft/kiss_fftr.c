#include "kiss_fftr.h"

#include <stdlib.h>

struct kiss_fftr_state {
    int nfft;
    int inverse_fft;
    kiss_fft_cfg subcfg;
};

kiss_fftr_cfg kiss_fftr_alloc(int nfft, int inverse_fft, void* mem, size_t* lenmem) {
    const size_t needed = sizeof(struct kiss_fftr_state);

    if (lenmem != NULL) {
        *lenmem = needed;
    }

    if (nfft <= 0) {
        return NULL;
    }

    if (mem != NULL) {
        return NULL;
    }

    struct kiss_fftr_state* cfg = (struct kiss_fftr_state*)malloc(needed);
    if (cfg == NULL) {
        return NULL;
    }

    cfg->subcfg = kiss_fft_alloc(nfft, inverse_fft, NULL, NULL);
    if (cfg->subcfg == NULL) {
        free(cfg);
        return NULL;
    }

    cfg->nfft = nfft;
    cfg->inverse_fft = inverse_fft;
    return cfg;
}

void kiss_fftr(kiss_fftr_cfg cfg, const kiss_fft_scalar* timedata, kiss_fft_cpx* freqdata) {
    if (cfg == NULL || timedata == NULL || freqdata == NULL) {
        return;
    }

    const int spectrum_bins = (cfg->nfft / 2) + 1;
    kiss_fft_cpx* complex_in = (kiss_fft_cpx*)malloc((size_t)cfg->nfft * sizeof(kiss_fft_cpx));
    kiss_fft_cpx* complex_out = (kiss_fft_cpx*)malloc((size_t)cfg->nfft * sizeof(kiss_fft_cpx));
    if (complex_in == NULL || complex_out == NULL) {
        free(complex_in);
        free(complex_out);
        return;
    }

    for (int i = 0; i < cfg->nfft; ++i) {
        complex_in[i].r = timedata[i];
        complex_in[i].i = 0.0f;
    }

    kiss_fft(cfg->subcfg, complex_in, complex_out);

    for (int i = 0; i < spectrum_bins; ++i) {
        freqdata[i] = complex_out[i];
    }

    free(complex_in);
    free(complex_out);
}

void kiss_fftri(kiss_fftr_cfg cfg, const kiss_fft_cpx* freqdata, kiss_fft_scalar* timedata) {
    if (cfg == NULL || freqdata == NULL || timedata == NULL) {
        return;
    }

    const int half = cfg->nfft / 2;
    kiss_fft_cpx* complex_in = (kiss_fft_cpx*)calloc((size_t)cfg->nfft, sizeof(kiss_fft_cpx));
    kiss_fft_cpx* complex_out = (kiss_fft_cpx*)malloc((size_t)cfg->nfft * sizeof(kiss_fft_cpx));
    if (complex_in == NULL || complex_out == NULL) {
        free(complex_in);
        free(complex_out);
        return;
    }

    complex_in[0] = freqdata[0];
    for (int k = 1; k <= half; ++k) {
        complex_in[k] = freqdata[k];

        const int mirror = cfg->nfft - k;
        if (mirror != k && mirror < cfg->nfft) {
            complex_in[mirror].r = freqdata[k].r;
            complex_in[mirror].i = -freqdata[k].i;
        }
    }

    kiss_fft(cfg->subcfg, complex_in, complex_out);

    for (int i = 0; i < cfg->nfft; ++i) {
        timedata[i] = complex_out[i].r;
    }

    free(complex_in);
    free(complex_out);
}

void kiss_fftr_free(void* cfg) {
    struct kiss_fftr_state* state = (struct kiss_fftr_state*)cfg;
    if (state == NULL) {
        return;
    }

    kiss_fft_free(state->subcfg);
    free(state);
}
