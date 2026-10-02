#pragma once
// DSP del EQ sin dependencias de JUCE: filtros biquad (formulas RBJ).
#include <algorithm>
#include <cmath>
#include <complex>

namespace aqeq
{
constexpr double kPi = 3.14159265358979323846;
constexpr int kNumBands = 6; // 0 LowCut, 1 LowShelf, 2 LowMid, 3 HighMid, 4 HighShelf, 5 HighCut
constexpr double kShelfQ = 0.70710678;

struct Coeffs
{
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
};

inline Coeffs normalise (double b0, double b1, double b2, double a0, double a1, double a2)
{
    Coeffs c;
    c.b0 = b0 / a0;
    c.b1 = b1 / a0;
    c.b2 = b2 / a0;
    c.a1 = a1 / a0;
    c.a2 = a2 / a0;
    return c;
}

inline double safeFreq (double f, double fs)
{
    return std::min (std::max (f, 10.0), fs * 0.45);
}

inline Coeffs makePeak (double fs, double f0, double q, double gainDb)
{
    f0 = safeFreq (f0, fs);
    const double A = std::pow (10.0, gainDb / 40.0);
    const double w0 = 2.0 * kPi * f0 / fs;
    const double cw = std::cos (w0);
    const double alpha = std::sin (w0) / (2.0 * q);
    return normalise (1.0 + alpha * A, -2.0 * cw, 1.0 - alpha * A,
                      1.0 + alpha / A, -2.0 * cw, 1.0 - alpha / A);
}

inline Coeffs makeLowShelf (double fs, double f0, double gainDb)
{
    f0 = safeFreq (f0, fs);
    const double A = std::pow (10.0, gainDb / 40.0);
    const double w0 = 2.0 * kPi * f0 / fs;
    const double cw = std::cos (w0);
    const double alpha = std::sin (w0) / (2.0 * kShelfQ);
    const double beta = 2.0 * std::sqrt (A) * alpha;
    return normalise (A * ((A + 1.0) - (A - 1.0) * cw + beta),
                      2.0 * A * ((A - 1.0) - (A + 1.0) * cw),
                      A * ((A + 1.0) - (A - 1.0) * cw - beta),
                      (A + 1.0) + (A - 1.0) * cw + beta,
                      -2.0 * ((A - 1.0) + (A + 1.0) * cw),
                      (A + 1.0) + (A - 1.0) * cw - beta);
}

inline Coeffs makeHighShelf (double fs, double f0, double gainDb)
{
    f0 = safeFreq (f0, fs);
    const double A = std::pow (10.0, gainDb / 40.0);
    const double w0 = 2.0 * kPi * f0 / fs;
    const double cw = std::cos (w0);
    const double alpha = std::sin (w0) / (2.0 * kShelfQ);
    const double beta = 2.0 * std::sqrt (A) * alpha;
    return normalise (A * ((A + 1.0) + (A - 1.0) * cw + beta),
                      -2.0 * A * ((A - 1.0) + (A + 1.0) * cw),
                      A * ((A + 1.0) + (A - 1.0) * cw - beta),
                      (A + 1.0) - (A - 1.0) * cw + beta,
                      2.0 * ((A - 1.0) - (A + 1.0) * cw),
                      (A + 1.0) - (A - 1.0) * cw - beta);
}

inline Coeffs makeHighPass (double fs, double f0)
{
    f0 = safeFreq (f0, fs);
    const double w0 = 2.0 * kPi * f0 / fs;
    const double cw = std::cos (w0);
    const double alpha = std::sin (w0) / (2.0 * kShelfQ);
    return normalise ((1.0 + cw) / 2.0, -(1.0 + cw), (1.0 + cw) / 2.0,
                      1.0 + alpha, -2.0 * cw, 1.0 - alpha);
}

inline Coeffs makeLowPass (double fs, double f0)
{
    f0 = safeFreq (f0, fs);
    const double w0 = 2.0 * kPi * f0 / fs;
    const double cw = std::cos (w0);
    const double alpha = std::sin (w0) / (2.0 * kShelfQ);
    return normalise ((1.0 - cw) / 2.0, 1.0 - cw, (1.0 - cw) / 2.0,
                      1.0 + alpha, -2.0 * cw, 1.0 - alpha);
}

// Respuesta en magnitud (ganancia lineal) de un biquad a una frecuencia.
inline double magnitudeAt (const Coeffs& c, double fs, double freq)
{
    const double w = 2.0 * kPi * freq / fs;
    const std::complex<double> z1 = std::polar (1.0, -w);
    const std::complex<double> z2 = z1 * z1;
    const std::complex<double> num = c.b0 + c.b1 * z1 + c.b2 * z2;
    const std::complex<double> den = 1.0 + c.a1 * z1 + c.a2 * z2;
    return std::abs (num / den);
}

class Biquad
{
public:
    void setCoeffs (const Coeffs& c) { coeffs = c; }
    void setActive (bool a) { active = a; }
    bool isActive() const { return active; }

    void reset()
    {
        for (int i = 0; i < 2; ++i)
        {
            z1[i] = 0.0;
            z2[i] = 0.0;
        }
    }

    // Forma directa II transpuesta. ch: 0 o 1.
    double process (int ch, double x)
    {
        const double y = coeffs.b0 * x + z1[ch];
        z1[ch] = coeffs.b1 * x - coeffs.a1 * y + z2[ch];
        z2[ch] = coeffs.b2 * x - coeffs.a2 * y;
        return y;
    }

private:
    Coeffs coeffs;
    bool active = false;
    double z1[2] = { 0.0, 0.0 };
    double z2[2] = { 0.0, 0.0 };
};

struct Settings
{
    float lcFreq = 20.0f;
    float lsFreq = 100.0f, lsGain = 0.0f;
    float lmFreq = 400.0f, lmGain = 0.0f, lmQ = 1.0f;
    float hmFreq = 2500.0f, hmGain = 0.0f, hmQ = 1.0f;
    float hsFreq = 8000.0f, hsGain = 0.0f;
    float hcFreq = 20000.0f;
    float outGain = 0.0f;
};

struct Chain
{
    Coeffs c[kNumBands];
    bool on[kNumBands] = { false, false, false, false, false, false };
};

inline Chain buildChain (const Settings& s, double fs)
{
    Chain ch;

    ch.on[0] = s.lcFreq > 20.5f; // 20 Hz = apagado
    ch.c[0] = makeHighPass (fs, s.lcFreq);

    ch.on[1] = std::abs (s.lsGain) > 0.01f;
    ch.c[1] = makeLowShelf (fs, s.lsFreq, s.lsGain);

    ch.on[2] = std::abs (s.lmGain) > 0.01f;
    ch.c[2] = makePeak (fs, s.lmFreq, s.lmQ, s.lmGain);

    ch.on[3] = std::abs (s.hmGain) > 0.01f;
    ch.c[3] = makePeak (fs, s.hmFreq, s.hmQ, s.hmGain);

    ch.on[4] = std::abs (s.hsGain) > 0.01f;
    ch.c[4] = makeHighShelf (fs, s.hsFreq, s.hsGain);

    ch.on[5] = s.hcFreq < 19990.0f; // 20 kHz = apagado
    ch.c[5] = makeLowPass (fs, s.hcFreq);

    return ch;
}
} // namespace aqeq
