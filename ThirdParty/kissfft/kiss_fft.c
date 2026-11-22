#include "kiss_fft.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

struct kiss_fft_state {
    int nfft;
    int inverse_fft;
};

static int is_power_of_two(int value) {
    return value > 0 && (value & (value - 1)) == 0;
}

static unsigned reverse_bits(unsigned value, int bit_count) {
    unsigned reversed = 0U;
    for (int i = 0; i < bit_count; ++i) {
        reversed = (reversed << 1U) | (value & 1U);
        value >>= 1U;
    }
    return reversed;
}

static void dft_fallback(kiss_fft_cfg cfg, const kiss_fft_cpx* fin, kiss_fft_cpx* fout) {
    const float sign = cfg->inverse_fft ? 1.0f : -1.0f;
    const float scale = cfg->inverse_fft ? (1.0f / (float)cfg->nfft) : 1.0f;

    for (int k = 0; k < cfg->nfft; ++k) {
        float sum_r = 0.0f;
        float sum_i = 0.0f;
        for (int n = 0; n < cfg->nfft; ++n) {
            const float phase = (2.0f * (float)M_PI * (float)k * (float)n) / (float)cfg->nfft;
            const float c = cosf(phase);
            const float s = sinf(phase) * sign;
            sum_r += (fin[n].r * c) - (fin[n].i * s);
            sum_i += (fin[n].r * s) + (fin[n].i * c);
        }
        fout[k].r = sum_r * scale;
        fout[k].i = sum_i * scale;
    }
}

kiss_fft_cfg kiss_fft_alloc(int nfft, int inverse_fft, void* mem, size_t* lenmem) {
    const size_t needed = sizeof(struct kiss_fft_state);

    if (lenmem != NULL) {
        *lenmem = needed;
    }

    if (mem != NULL) {
        if (lenmem != NULL && *lenmem < needed) {
            return NULL;
        }
        struct kiss_fft_state* cfg = (struct kiss_fft_state*)mem;
        cfg->nfft = nfft;
        cfg->inverse_fft = inverse_fft;
        return cfg;
    }

    struct kiss_fft_state* cfg = (struct kiss_fft_state*)malloc(needed);
    if (cfg == NULL) {
        return NULL;
    }

    cfg->nfft = nfft;
    cfg->inverse_fft = inverse_fft;
    return cfg;
}

void kiss_fft(kiss_fft_cfg cfg, const kiss_fft_cpx* fin, kiss_fft_cpx* fout) {
    if (!is_power_of_two(cfg->nfft)) {
        dft_fallback(cfg, fin, fout);
        return;
    }

    int bit_count = 0;
    for (int n = cfg->nfft; n > 1; n >>= 1) {
        ++bit_count;
    }

    for (int i = 0; i < cfg->nfft; ++i) {
        const unsigned reversed = reverse_bits((unsigned)i, bit_count);
        fout[reversed] = fin[i];
    }

    for (int len = 2; len <= cfg->nfft; len <<= 1) {
        const float angle = (2.0f * (float)M_PI / (float)len) * (cfg->inverse_fft ? 1.0f : -1.0f);
        const float wlen_r = cosf(angle);
        const float wlen_i = sinf(angle);

        for (int offset = 0; offset < cfg->nfft; offset += len) {
            float wr = 1.0f;
            float wi = 0.0f;
            const int half = len / 2;

            for (int j = 0; j < half; ++j) {
                const int even = offset + j;
                const int odd = even + half;

                const float odd_r = fout[odd].r;
                const float odd_i = fout[odd].i;
                const float tr = (wr * odd_r) - (wi * odd_i);
                const float ti = (wr * odd_i) + (wi * odd_r);

                const float even_r = fout[even].r;
                const float even_i = fout[even].i;

                fout[even].r = even_r + tr;
                fout[even].i = even_i + ti;
                fout[odd].r = even_r - tr;
                fout[odd].i = even_i - ti;

                const float next_wr = (wr * wlen_r) - (wi * wlen_i);
                const float next_wi = (wr * wlen_i) + (wi * wlen_r);
                wr = next_wr;
                wi = next_wi;
            }
        }
    }

    if (cfg->inverse_fft) {
        const float scale = 1.0f / (float)cfg->nfft;
        for (int i = 0; i < cfg->nfft; ++i) {
            fout[i].r *= scale;
            fout[i].i *= scale;
        }
    }
}

void kiss_fft_free(void* cfg) {
    free(cfg);
}
