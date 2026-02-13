/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

/**
 * @file SVFFilter.h
 * @brief Cytomic TPT (Topology-Preserving Transform) State Variable Filter.
 *
 * Implementation based on Andy Simper's technical papers available at
 * https://cytomic.com/technical-papers/. The TPT structure preserves the
 * analog prototype's topology in the digital domain, yielding coefficient
 * changes that are free of transient artifacts (no zipper noise) and
 * unconditionally stable under fast modulation.
 *
 * The filter operates in either second-order mode (two integrators,
 * producing 12 dB/oct slopes) or first-order mode (single integrator,
 * 6 dB/oct slopes). Filter shape is selected via a set of mix coefficients
 * (m0, m1, m2) that blend the input, bandpass, and lowpass outputs:
 *
 *     output = m0 * input + m1 * bandpass + m2 * lowpass
 *
 * This approach avoids separate code paths for each filter type, since only
 * the mix coefficients change between Bell, Shelf, HP, LP, Notch, etc.
 *
 * @see CascadedFilter, EqBand
 */

#include "FilterTypes.h"
#include <complex>

// Cytomic TPT (Topology-Preserving Transform) State Variable Filter
// Based on Andy Simper's technical papers: https://cytomic.com/technical-papers/
// Handles fast parameter modulation without zipper noise or instability.

/**
 * @brief Second-order (or first-order) state variable filter using the Cytomic TPT structure.
 *
 * Internally maintains two integrator states (ic1eq, ic2eq) and precomputed
 * coefficients derived from the bilinear-transform-matched analog prototype.
 * The key coefficient `g = tan(pi * freq / sampleRate)` maps the analog
 * frequency to the digital domain without frequency warping artifacts at low
 * frequencies.
 *
 * Supports all FilterType shapes via mix coefficients. When setFirstOrder(true)
 * is called, the second integrator is bypassed, producing a single-pole
 * 6 dB/oct response -- used for odd-order Butterworth trailing sections in
 * CascadedFilter.
 *
 * @see CascadedFilter, EqBand
 */
class SVFFilter
{
public:
    SVFFilter() = default;

    /**
     * @brief Initialize the filter for a given sample rate.
     *
     * Must be called before any processing. Resets internal state and
     * recomputes coefficients for the current parameters at the new rate.
     *
     * @param sampleRate  Host sample rate in Hz.
     */
    void prepare (double sampleRate);

    /**
     * @brief Clear integrator states (ic1eq, ic2eq) to zero.
     *
     * Call when the audio stream is discontinuous (e.g., transport restart)
     * to avoid clicks from stale state.
     */
    void reset();

    /**
     * @brief Set all filter parameters and recompute coefficients.
     *
     * Internally calls updateCoefficients() to derive g, k, a1-a3, and
     * the mix coefficients m0, m1, m2 for the selected filter type.
     *
     * @param type    Filter shape (Bell, LowShelf, HighPass, etc.).
     * @param freq    Center/cutoff frequency in Hz (typically 20-20000).
     * @param Q       Quality factor / resonance (higher = narrower bandwidth).
     * @param gainDB  Boost or cut in decibels (used by Bell and Shelf types; ignored by HP/LP/Notch/BP).
     */
    void setParameters (FilterType type, double freq, double Q, double gainDB);

    /**
     * @brief Process a single audio sample through the filter.
     *
     * Uses the TPT SVF tick equations:
     *   v3 = input - ic2eq
     *   v1 = a1*ic1eq + a2*v3
     *   v2 = ic2eq + a2*ic1eq + a3*v3
     *   ic1eq = 2*v1 - ic1eq
     *   ic2eq = 2*v2 - ic2eq
     *   output = m0*input + m1*v1 + m2*v2
     *
     * @param input  Input sample value.
     * @return       Filtered output sample.
     */
    float processSample (float input);

    /**
     * @brief Process a block of audio samples in-place.
     *
     * Equivalent to calling processSample() for each sample in the buffer.
     *
     * @param buffer      Pointer to the audio sample array (modified in-place).
     * @param numSamples  Number of samples to process.
     */
    void process (float* buffer, int numSamples);

    /**
     * @brief Evaluate the complex frequency response H(e^{j*omega}) at a given frequency.
     *
     * Used by the UI for real-time curve rendering. Evaluates the z-domain
     * transfer function of the filter at the specified frequency, returning
     * both magnitude and phase information as a complex number.
     *
     * For second-order mode this evaluates the standard biquad transfer
     * function H(z) = (b0 + b1*z^-1 + b2*z^-2) / (1 + a1*z^-1 + a2*z^-2).
     * For first-order mode the z^-2 terms are zero.
     *
     * @param freq  Frequency in Hz at which to evaluate the response.
     * @return      Complex frequency response (magnitude = |H|, phase = arg(H)).
     *
     * @note This is called from the UI thread and must not modify filter state.
     */
    std::complex<double> getResponse (double freq) const;

    /**
     * @brief Get the magnitude response in decibels at a given frequency.
     *
     * Convenience wrapper: returns 20 * log10(abs(getResponse(freq))).
     *
     * @param freq  Frequency in Hz.
     * @return      Magnitude in dB.
     */
    double getMagnitudeAtFrequency (double freq) const;

    // First-order mode (for 6 dB/oct slopes and odd-order Butterworth)

    /**
     * @brief Enable or disable first-order (single-pole) operation.
     *
     * When enabled, the filter uses only one integrator, producing a 6 dB/oct
     * slope. This is used by CascadedFilter for the trailing section of
     * odd-order Butterworth designs (e.g., 18, 36, 72 dB/oct).
     *
     * @param firstOrder  True to enable first-order mode, false for second-order (default).
     */
    void setFirstOrder (bool firstOrder);

    /**
     * @brief Query whether the filter is in first-order mode.
     * @return True if the filter is operating as a first-order (6 dB/oct) section.
     */
    bool getIsFirstOrder() const { return isFirstOrder; }

private:
    void updateCoefficients();

    // SVF state variables (integrator states)
    double ic1eq = 0.0;
    double ic2eq = 0.0;

    // Target parameters
    FilterType type = FilterType::Bell;
    double frequency = 1000.0;
    double q = 1.0;
    double gain = 0.0; // dB
    bool isFirstOrder = false;

    // Computed coefficients
    double g = 0.0;   // tan(pi * freq / sampleRate)
    double k = 0.0;   // 1/Q (or damping)
    double a1 = 0.0;
    double a2 = 0.0;
    double a3 = 0.0;
    double m0 = 0.0, m1 = 0.0, m2 = 0.0; // Mix coefficients

    double currentSampleRate = 44100.0;
};
