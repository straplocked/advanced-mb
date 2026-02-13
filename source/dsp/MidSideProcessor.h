/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

/**
 * @file MidSideProcessor.h
 * @brief In-place Mid/Side encoding and decoding for stereo audio buffers.
 *
 * Provides encode and decode functions that convert between standard
 * Left/Right stereo and Mid/Side representation. Used by the multiband
 * processing chain to apply compression independently to the mid (center)
 * and side (width) components of the stereo image.
 *
 * Encoding equations:
 *     Mid  = (L + R) / 2
 *     Side = (L - R) / 2
 *
 * Decoding equations (perfect reconstruction):
 *     L = Mid + Side
 *     R = Mid - Side
 *
 * The 0.5 scaling factor in encoding ensures that a mono signal (L == R)
 * maps to the same amplitude in the Mid channel, and the decode step
 * reconstructs the original levels exactly.
 */

// In-place Mid/Side encoding and decoding for stereo audio buffers.
// Encode: L -> Mid, R -> Side
// Decode: Mid -> L, Side -> R

/**
 * @brief Mid/Side stereo matrix encoding and decoding utilities.
 *
 * All operations are performed in-place on raw float arrays for
 * maximum efficiency (no allocation, no virtual dispatch).
 */
namespace MidSideProcessor
{
    /**
     * @brief Encode a stereo buffer from Left/Right to Mid/Side representation in-place.
     *
     * After this call, L[i] contains the mid signal (L+R)/2 and R[i]
     * contains the side signal (L-R)/2.
     *
     * @param L           Pointer to left-channel sample array (overwritten with mid).
     * @param R           Pointer to right-channel sample array (overwritten with side).
     * @param numSamples  Number of samples to process.
     */
    inline void encode (float* L, float* R, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float mid  = 0.5f * (L[i] + R[i]);
            float side = 0.5f * (L[i] - R[i]);
            L[i] = mid;
            R[i] = side;
        }
    }

    /**
     * @brief Decode a stereo buffer from Mid/Side back to Left/Right representation in-place.
     *
     * Assumes L[i] contains the mid signal and R[i] contains the side signal
     * (as produced by encode()). After this call, L[i] = mid + side and
     * R[i] = mid - side, perfectly reconstructing the original stereo signal
     * (provided no processing was applied between encode and decode, the
     * reconstruction is bit-exact).
     *
     * @param L           Pointer to mid-channel sample array (overwritten with left).
     * @param R           Pointer to side-channel sample array (overwritten with right).
     * @param numSamples  Number of samples to process.
     */
    inline void decode (float* L, float* R, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float l = L[i] + R[i];  // mid + side
            float r = L[i] - R[i];  // mid - side
            L[i] = l;
            R[i] = r;
        }
    }
}
