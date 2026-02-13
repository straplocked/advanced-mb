/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "SVFFilter.h"
#include <cmath>
#include <algorithm>

static constexpr double kPi = 3.14159265358979323846;

void SVFFilter::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    reset();
    updateCoefficients();
}

void SVFFilter::reset()
{
    ic1eq = 0.0;
    ic2eq = 0.0;
}

void SVFFilter::setParameters (FilterType newType, double freq, double Q, double gainDB)
{
    type = newType;
    frequency = std::clamp (freq, 10.0, currentSampleRate * 0.49);
    q = std::max (Q, 0.1);
    gain = gainDB;
    updateCoefficients();
}

void SVFFilter::setFirstOrder (bool firstOrder)
{
    if (isFirstOrder != firstOrder)
    {
        isFirstOrder = firstOrder;
        ic1eq = 0.0;
        ic2eq = 0.0;
        updateCoefficients();
    }
}

void SVFFilter::updateCoefficients()
{
    double A = std::pow (10.0, gain / 40.0); // sqrt of linear gain

    // --- First-order (one-pole TPT) mode ---
    if (isFirstOrder)
    {
        g = std::tan (kPi * frequency / currentSampleRate);
        k = 0.0;
        a1 = 0.0;
        a2 = 0.0;
        a3 = 0.0;

        // First-order mix: output = m0 * v0 + m2 * lp
        // (m1 unused in first-order mode)
        switch (type)
        {
            case FilterType::LowPass:
                m0 = 0.0; m1 = 0.0; m2 = 1.0;
                break;

            case FilterType::HighPass:
                m0 = 1.0; m1 = 0.0; m2 = -1.0;
                break;

            case FilterType::LowShelf:
            {
                double sqrtA = std::sqrt (A);
                g = std::tan (kPi * frequency / currentSampleRate) * sqrtA;
                m0 = 1.0; m1 = 0.0; m2 = (A * A - 1.0);
                break;
            }

            case FilterType::HighShelf:
            {
                double sqrtA = std::sqrt (A);
                g = std::tan (kPi * frequency / currentSampleRate) / sqrtA;
                m0 = A * A; m1 = 0.0; m2 = (1.0 - A * A);
                break;
            }

            default:
                // Types that don't support first-order: pass through
                m0 = 1.0; m1 = 0.0; m2 = 0.0;
                break;
        }
        return;
    }

    // --- Second-order (standard SVF) mode ---
    g = std::tan (kPi * frequency / currentSampleRate);
    k = 1.0 / q;

    // Precompute common terms
    a1 = 1.0 / (1.0 + g * (g + k));
    a2 = g * a1;
    a3 = g * a2;

    // Set mix coefficients for each filter type
    // Output = m0*v0 + m1*v1 + m2*v2
    // where v0=input, v1=bandpass, v2=lowpass
    switch (type)
    {
        case FilterType::Bell:
            k = 1.0 / (q * A);
            a1 = 1.0 / (1.0 + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
            m0 = 1.0;
            m1 = k * (A * A - 1.0);
            m2 = 0.0;
            break;

        case FilterType::LowShelf:
            k = 1.0 / q;
            a1 = 1.0 / (1.0 + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
            m0 = 1.0;
            m1 = k * (A - 1.0);
            m2 = (A * A - 1.0);
            break;

        case FilterType::HighShelf:
            k = 1.0 / q;
            a1 = 1.0 / (1.0 + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
            m0 = A * A;
            m1 = k * (1.0 - A) * A;
            m2 = (1.0 - A * A);
            break;

        case FilterType::HighPass:
            m0 = 1.0;
            m1 = -k;
            m2 = -1.0;
            break;

        case FilterType::LowPass:
            m0 = 0.0;
            m1 = 0.0;
            m2 = 1.0;
            break;

        case FilterType::Notch:
            m0 = 1.0;
            m1 = -k;
            m2 = 0.0;
            break;

        case FilterType::BandPass:
            m0 = 0.0;
            m1 = 1.0;
            m2 = 0.0;
            break;

        case FilterType::TiltShelf:
        {
            // Second-order tilt shelf: steep transition at crossover, flat at extremes
            double sqrtA = std::sqrt (A);
            k = 1.0 / q;
            g = std::tan (kPi * frequency / currentSampleRate) / sqrtA;
            a1 = 1.0 / (1.0 + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
            m0 = 1.0;
            m1 = k * (A - 1.0);
            m2 = (A * A - 1.0);
            break;
        }

        case FilterType::FlatTilt:
        {
            // Flat tilt: gradual first-order tilt across entire spectrum
            // Uses first-order mode for a gentle constant-slope response
            // crossing 0 dB at the specified frequency
            isFirstOrder = true;
            g = std::tan (kPi * frequency / currentSampleRate);
            k = 0.0;
            a1 = 0.0;
            a2 = 0.0;
            a3 = 0.0;
            m0 = 1.0;
            m1 = 0.0;
            m2 = (A * A - 1.0);
            return;
        }
    }
}

float SVFFilter::processSample (float input)
{
    double v0 = (double) input;

    if (isFirstOrder)
    {
        // One-pole TPT tick
        double v1 = (v0 - ic1eq) * g / (1.0 + g);
        double lp = v1 + ic1eq;
        ic1eq = lp + v1;

        return (float) (m0 * v0 + m2 * lp);
    }

    // Second-order Cytomic SVF tick
    double v3 = v0 - ic2eq;
    double v1 = a1 * ic1eq + a2 * v3;
    double v2 = ic2eq + a2 * ic1eq + a3 * v3;

    ic1eq = 2.0 * v1 - ic1eq;
    ic2eq = 2.0 * v2 - ic2eq;

    return (float) (m0 * v0 + m1 * v1 + m2 * v2);
}

void SVFFilter::process (float* buffer, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
        buffer[i] = processSample (buffer[i]);
}

std::complex<double> SVFFilter::getResponse (double freq) const
{
    double w = 2.0 * kPi * freq / currentSampleRate;
    std::complex<double> ejw_inv = std::exp (std::complex<double> (0.0, -w));

    if (isFirstOrder)
    {
        // First-order TPT transfer function
        // H_lp(z) = G * (1 + z^-1) / (1 + c * z^-1)
        // where G = g/(1+g), c = (g-1)/(g+1)
        double G = g / (1.0 + g);
        double c = (g - 1.0) / (g + 1.0);

        std::complex<double> den = 1.0 + c * ejw_inv;
        std::complex<double> lp = G * (1.0 + ejw_inv) / den;

        return m0 * 1.0 + m2 * lp;
    }

    // Second-order SVF transfer function
    std::complex<double> ejw_inv2 = ejw_inv * ejw_inv;

    double denom = 1.0 + g * (g + k);
    double a1_bq = 2.0 * (g * g - 1.0) / denom;
    double a2_bq = (1.0 - g * k + g * g) / denom;

    std::complex<double> commonDen = 1.0 + a1_bq * ejw_inv + a2_bq * ejw_inv2;

    // Lowpass: H_lp = g^2/D * (1 + 2z^-1 + z^-2) / commonDen
    double g2d = g * g / denom;
    std::complex<double> lpNum = g2d * (1.0 + 2.0 * ejw_inv + ejw_inv2);

    // Bandpass: H_bp = g/D * (1 - z^-2) / commonDen
    double gd = g / denom;
    std::complex<double> bpNum = gd * (1.0 - ejw_inv2);

    // Highpass: H_hp = 1/D * (1 - 2z^-1 + z^-2) / commonDen
    double invd = 1.0 / denom;
    std::complex<double> hpNum = invd * (1.0 + -2.0 * ejw_inv + ejw_inv2);

    std::complex<double> hp = hpNum / commonDen;
    std::complex<double> bp = bpNum / commonDen;
    std::complex<double> lp = lpNum / commonDen;

    return m0 * (hp + k * bp + lp) + m1 * bp + m2 * lp;
}

double SVFFilter::getMagnitudeAtFrequency (double freq) const
{
    auto response = getResponse (freq);
    return 20.0 * std::log10 (std::max (std::abs (response), 1e-10));
}
