#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <vector>

namespace gdsp
{
inline double dbToLin (double db) { return std::pow (10.0, db / 20.0); }
inline double linToDb (double l)  { return 20.0 * std::log10 (std::max (l, 1.0e-9)); }
constexpr double kPi = 3.14159265358979323846;

//==============================================================================
// Filtre biquad (formules RBJ), état en double précision
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    inline double process (double x)
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void reset() { z1 = z2 = 0; }

    void set (double B0, double B1, double B2, double A0, double A1, double A2)
    {
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
    static double clampF (double fs, double f) { return std::min (f, fs * 0.45); }

    void highPass (double fs, double f, double q)
    {
        f = clampF (fs, f);
        const double w = 2 * kPi * f / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void peak (double fs, double f, double q, double dB)
    {
        f = clampF (fs, f);
        const double A = std::pow (10.0, dB / 40.0);
        const double w = 2 * kPi * f / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void highShelf (double fs, double f, double dB)
    {
        f = clampF (fs, f);
        const double A = std::pow (10.0, dB / 40.0);
        const double w = 2 * kPi * f / fs, c = std::cos (w), s = std::sin (w);
        const double al = s / 2.0 * std::sqrt (2.0);
        const double sA = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) + (A - 1) * c + sA),
             -2 * A * ((A - 1) + (A + 1) * c),
             A * ((A + 1) + (A - 1) * c - sA),
             (A + 1) - (A - 1) * c + sA,
             2 * ((A - 1) - (A + 1) * c),
             (A + 1) - (A - 1) * c - sA);
    }
    void lowShelf (double fs, double f, double dB)
    {
        f = clampF (fs, f);
        const double A = std::pow (10.0, dB / 40.0);
        const double w = 2 * kPi * f / fs, c = std::cos (w), s = std::sin (w);
        const double al = s / 2.0 * std::sqrt (2.0);
        const double sA = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) - (A - 1) * c + sA),
             2 * A * ((A - 1) - (A + 1) * c),
             A * ((A + 1) - (A - 1) * c - sA),
             (A + 1) + (A - 1) * c + sA,
             -2 * ((A - 1) + (A + 1) * c),
             (A + 1) + (A - 1) * c - sA);
    }
};

//==============================================================================
// Égaliseur : passe-haut 24 dB/oct + corps + boîte + présence + air
struct ChannelEq
{
    Biquad hp1, hp2, body, boxy, pres, air;
    inline double process (double x)
    {
        x = hp2.process (hp1.process (x));
        x = body.process (x);
        x = boxy.process (x);
        x = pres.process (x);
        return air.process (x);
    }
};

//==============================================================================
// De-esser : réduit uniquement la bande des sibilantes
struct DeEsser
{
    double fs = 44100, grDb = 0, atk = 0, rel = 0;
    Biquad hp[2];
    void prepare (double sr)
    {
        fs = sr; grDb = 0;
        atk = std::exp (-1.0 / (0.0005 * fs));
        rel = std::exp (-1.0 / (0.050 * fs));
        hp[0].reset(); hp[1].reset();
    }
    void setFreq (double f) { hp[0].highPass (fs, f, 0.7071); hp[1].highPass (fs, f, 0.7071); }

    inline void process (double& l, double& r, double thresholdDb)
    {
        const double sl = hp[0].process (l), sr = hp[1].process (r);
        if (thresholdDb >= -0.5) { grDb = 0; return; }
        const double lvl = linToDb (std::max (std::abs (sl), std::abs (sr)));
        const double over = lvl - thresholdDb;
        double gr = over > 0 ? -over * 0.75 : 0.0;   // ratio 4:1 sur la bande
        gr = std::max (gr, -12.0);
        if (gr < grDb) grDb = atk * grDb + (1 - atk) * gr;
        else           grDb = rel * grDb + (1 - rel) * gr;
        const double g = dbToLin (grDb);
        l = l - sl + sl * g;
        r = r - sr + sr * g;
    }
};

//==============================================================================
// "Vocal rider" : ramène lentement le niveau de la voix vers une cible
struct Rider
{
    double fs = 44100, ms = 0, gDb = 0, avgCoef = 0, gCoef = 0;
    void prepare (double sr)
    {
        fs = sr; ms = 0; gDb = 0;
        avgCoef = std::exp (-1.0 / (0.300 * fs));
        gCoef   = std::exp (-1.0 / (0.800 * fs));
    }
    inline void process (double& l, double& r, double amount, double targetDb)
    {
        const double e = 0.5 * (l * l + r * r);
        ms = avgCoef * ms + (1 - avgCoef) * e;
        if (amount <= 0.0) gDb = 0.0;
        else
        {
            const double lvl = 10.0 * std::log10 (ms + 1.0e-12);
            if (lvl > -42.0)   // en dessous : silence / souffle, on garde le gain (pas de remontée du bruit)
            {
                const double want = juce::jlimit (-8.0, 8.0, targetDb - lvl) * amount;
                gDb = gCoef * gDb + (1 - gCoef) * want;
            }
        }
        const double g = dbToLin (gDb);
        l *= g; r *= g;
    }
};

//==============================================================================
// Compresseur stéréo lié, genou doux
struct Compressor
{
    double fs = 44100, grDb = 0, atk = 0, rel = 0, thr = -18, ratio = 2, knee = 6, makeup = 0;
    void prepare (double sr) { fs = sr; grDb = 0; }
    void setParams (double t, double r, double a, double rl, double k, double m)
    {
        thr = t; ratio = std::max (1.0, r); knee = std::max (0.01, k); makeup = m;
        atk = std::exp (-1.0 / (0.001 * std::max (0.05, a) * fs));
        rel = std::exp (-1.0 / (0.001 * std::max (5.0, rl) * fs));
    }
    inline void process (double& l, double& r)
    {
        const double lvl = linToDb (std::max (std::abs (l), std::abs (r)));
        const double over = lvl - thr;
        const double slope = 1.0 / ratio - 1.0;
        double gr;
        if (2 * over < -knee)              gr = 0.0;
        else if (2 * std::abs (over) <= knee) gr = slope * (over + knee / 2) * (over + knee / 2) / (2 * knee);
        else                               gr = slope * over;
        if (gr < grDb) grDb = atk * grDb + (1 - atk) * gr;
        else           grDb = rel * grDb + (1 - rel) * gr;
        const double g = dbToLin (grDb + makeup);
        l *= g; r *= g;
    }
};

//==============================================================================
// Limiteur "lookahead" (~2 ms) : le plafond n'est jamais dépassé
struct Limiter
{
    int L = 96;
    std::vector<double> dl, dr, req, mins;
    int pos = 0;
    double sum = 0, g = 1, lastMin = 1;

    int latency() const { return L - 1; }
    void prepare (double fs)
    {
        L = std::max (8, (int) std::round (0.002 * fs));
        dl.assign ((size_t) L, 0.0); dr.assign ((size_t) L, 0.0);
        req.assign ((size_t) L, 1.0); mins.assign ((size_t) L, 1.0);
        pos = 0; sum = (double) L; g = 1; lastMin = 1;
    }
    inline void process (double& l, double& r, double ceilLin, double relCoef)
    {
        const double pk = std::max (std::abs (l), std::abs (r));
        const double rq = pk > ceilLin ? ceilLin / pk : 1.0;
        dl[(size_t) pos] = l; dr[(size_t) pos] = r; req[(size_t) pos] = rq;
        double mn = 1.0;
        for (int i = 0; i < L; ++i) mn = std::min (mn, req[(size_t) i]);
        sum += mn - mins[(size_t) pos];
        mins[(size_t) pos] = mn;
        const double avg = juce::jlimit (0.0, 1.0, sum / (double) L);
        pos = (pos + 1) % L;
        if (avg < g) g = avg;
        else         g = relCoef * g + (1 - relCoef) * avg;
        lastMin = std::min (lastMin, g);
        l = juce::jlimit (-ceilLin, ceilLin, dl[(size_t) pos] * g);
        r = juce::jlimit (-ceilLin, ceilLin, dr[(size_t) pos] * g);
    }
};

//==============================================================================
// Mesure de sonie ITU-R BS.1770 / EBU R128 (momentané, court terme, intégré)
class LoudnessMeter
{
public:
    void prepare (double sr)
    {
        fs = sr;
        blockLen = std::max (1, (int) std::round (0.1 * fs));
        // Étage 1 : shelf aigu (+4 dB), étage 2 : passe-haut 38 Hz (pré-filtre K)
        {
            const double f0 = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196;
            const double K = std::tan (kPi * f0 / fs), Vh = std::pow (10.0, G / 20.0), Vb = std::pow (Vh, 0.4996667741545416);
            const double a0 = 1.0 + K / Q + K * K;
            for (auto& f : s1)
                f.set ((Vh + Vb * K / Q + K * K), 2.0 * (K * K - Vh), (Vh - Vb * K / Q + K * K),
                       a0, 2.0 * (K * K - 1.0), (1.0 - K / Q + K * K));
        }
        {
            const double f0 = 38.13547087602444, Q = 0.5003270373238773;
            const double K = std::tan (kPi * f0 / fs);
            const double a0 = 1.0 + K / Q + K * K;
            for (auto& f : s2)
                f.set (1.0, -2.0, 1.0, a0, 2.0 * (K * K - 1.0), (1.0 - K / Q + K * K));
        }
        gating.clear();
        gating.reserve (200000);
        resetState();
    }

    void requestReset() { resetFlag.store (true); }

    void process (const float* l, const float* r, int n)
    {
        if (resetFlag.exchange (false)) resetState();
        for (int i = 0; i < n; ++i)
        {
            const double a = s2[0].process (s1[0].process ((double) l[i]));
            const double b = s2[1].process (s1[1].process ((double) r[i]));
            blockSum += a * a + b * b;
            if (++blockCount >= blockLen) { finishBlock (blockSum / (double) blockLen); blockSum = 0; blockCount = 0; }
        }
    }

    float momentary() const  { return mom.load(); }
    float shortTerm() const  { return st.load(); }
    float integrated() const { return integ.load(); }

private:
    static double toLufs (double ms) { return ms > 1.0e-12 ? -0.691 + 10.0 * std::log10 (ms) : -100.0; }

    void resetState()
    {
        for (auto& f : s1) f.reset();
        for (auto& f : s2) f.reset();
        blockSum = 0; blockCount = 0; ring.fill (0.0); ringPos = 0; ringFilled = 0;
        gating.clear();
        mom.store (-100.f); st.store (-100.f); integ.store (-100.f);
    }

    void finishBlock (double ms)
    {
        ring[(size_t) ringPos] = ms;
        ringPos = (ringPos + 1) % (int) ring.size();
        ringFilled = std::min (ringFilled + 1, (int) ring.size());

        auto meanLast = [&] (int count)
        {
            double s = 0;
            for (int i = 0; i < count; ++i)
                s += ring[(size_t) ((ringPos - 1 - i + 2 * (int) ring.size()) % (int) ring.size())];
            return s / (double) count;
        };

        const int m = std::min (ringFilled, 4);
        const double ms400 = meanLast (m);
        mom.store ((float) toLufs (ms400));
        st.store ((float) toLufs (meanLast (ringFilled)));

        if (ringFilled >= 4 && gating.size() < gating.capacity())
        {
            gating.push_back (ms400);
            // Intégré : porte absolue -70 LUFS puis porte relative -10 LU
            double sum = 0; int cnt = 0;
            for (double v : gating) if (toLufs (v) > -70.0) { sum += v; ++cnt; }
            if (cnt > 0)
            {
                const double relThr = toLufs (sum / cnt) - 10.0;
                double s2sum = 0; int c2 = 0;
                for (double v : gating) { const double lu = toLufs (v); if (lu > -70.0 && lu > relThr) { s2sum += v; ++c2; } }
                if (c2 > 0) integ.store ((float) toLufs (s2sum / c2));
            }
        }
    }

    double fs = 44100, blockSum = 0;
    int blockLen = 4410, blockCount = 0, ringPos = 0, ringFilled = 0;
    Biquad s1[2], s2[2];
    std::array<double, 30> ring {};
    std::vector<double> gating;
    std::atomic<float> mom { -100.f }, st { -100.f }, integ { -100.f };
    std::atomic<bool> resetFlag { false };
};
} // namespace gdsp
