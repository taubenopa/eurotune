#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

namespace eurotune
{
inline int nextPow2 (int v) { int p = 1; while (p < v) p <<= 1; return p; }

//==============================================================================
// YIN Pitch-Detektor (de Cheveigne & Kawahara) mit Parabel-Interpolation
//==============================================================================
class YinDetector
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        decim = std::max (1, (int) std::floor (sampleRate / 22000.0));
        effSr = sampleRate / (double) decim;
        tauCap = (int) std::ceil (effSr / 60.0) + 2;
        win = (int) (0.023 * effSr);
        hop = std::max (32, (int) (0.006 * effSr));
        ring.assign ((size_t) nextPow2 (win + tauCap + hop + 16), 0.0f);
        mask = (int) ring.size() - 1;
        scratch.assign ((size_t) (win + tauCap + 16), 0.0f);
        cmnd.assign ((size_t) (tauCap + 16), 1.0f);
        reset();
    }

    void reset()
    {
        std::fill (ring.begin(), ring.end(), 0.0f);
        wp = 0; acc = 0.0f; accCount = 0; hopCount = 0;
    }

    double hopSeconds() const { return (double) (hop * decim) / sr; }

    // true, wenn ein neuer Analyse-Schritt faellig ist
    bool push (float x)
    {
        acc += x;
        if (++accCount < decim) return false;
        ring[(size_t) wp] = acc / (float) decim;
        acc = 0.0f; accCount = 0;
        wp = (wp + 1) & mask;
        if (++hopCount >= hop) { hopCount = 0; return true; }
        return false;
    }

    bool detect (float minHz, float maxHz, float& f0, float& rms)
    {
        const int tauMin = std::max (2, (int) std::floor (effSr / (double) maxHz));
        const int tauMax = std::min (tauCap, (int) std::ceil (effSr / (double) minHz));
        if (tauMax <= tauMin + 2) return false;

        const int total = win + tauMax + 1;
        for (int i = 0; i < total; ++i)
            scratch[(size_t) i] = ring[(size_t) ((wp - total + i) & mask)];

        double e = 0.0;
        for (int i = total - win; i < total; ++i) e += (double) scratch[(size_t) i] * scratch[(size_t) i];
        rms = (float) std::sqrt (e / (double) win);
        if (rms < 1.0e-5f) return false;

        const float* x = scratch.data();
        float running = 0.0f;
        cmnd[0] = 1.0f;
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            float d = 0.0f;
            for (int j = 0; j < win; ++j) { const float diff = x[j] - x[j + tau]; d += diff * diff; }
            running += d;
            cmnd[(size_t) tau] = running > 1.0e-12f ? d * (float) tau / running : 1.0f;
        }

        int best = -1;
        for (int tau = tauMin; tau < tauMax; ++tau)
        {
            if (cmnd[(size_t) tau] < threshold)
            {
                while (tau + 1 < tauMax && cmnd[(size_t) tau + 1] < cmnd[(size_t) tau]) ++tau;
                best = tau;
                break;
            }
        }
        if (best < 0) return false;

        float tauF = (float) best;
        if (best > 1 && best < tauMax)
        {
            const float a = cmnd[(size_t) best - 1], b = cmnd[(size_t) best], c = cmnd[(size_t) best + 1];
            const float denom = a - 2.0f * b + c;
            if (std::fabs (denom) > 1.0e-9f)
                tauF += std::max (-1.0f, std::min (1.0f, 0.5f * (a - c) / denom));
        }
        f0 = (float) (effSr / (double) tauF);
        return f0 > 0.0f;
    }

    float threshold = 0.17f;

private:
    double sr = 44100.0, effSr = 22050.0;
    int decim = 2, tauCap = 400, win = 512, hop = 128, mask = 0, wp = 0, accCount = 0, hopCount = 0;
    float acc = 0.0f;
    std::vector<float> ring, scratch, cmnd;
};

//==============================================================================
// Delay-Line mit kubischer (Hermite) Interpolation
//==============================================================================
class DelayLine
{
public:
    void prepare (int minSize)
    {
        const int s = nextPow2 (minSize);
        buf.assign ((size_t) s, 0.0f);
        mask = s - 1; wp = 0;
    }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); }
    void write (float x) { wp = (wp + 1) & mask; buf[(size_t) wp] = x; }
    float readInt (int d) const { return buf[(size_t) ((wp - d) & mask)]; }

    float readFrac (float d) const   // d >= 1
    {
        const int di = (int) d;
        const float t = d - (float) di;
        const float ym1 = buf[(size_t) ((wp - (di - 1)) & mask)];
        const float y0  = buf[(size_t) ((wp - di) & mask)];
        const float y1  = buf[(size_t) ((wp - (di + 1)) & mask)];
        const float y2  = buf[(size_t) ((wp - (di + 2)) & mask)];
        const float c1 = 0.5f * (y1 - ym1);
        const float c2 = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
        const float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);
        return ((c3 * t + c2) * t + c1) * t + y0;
    }

private:
    std::vector<float> buf;
    int mask = 0, wp = 0;
};
} // namespace eurotune
