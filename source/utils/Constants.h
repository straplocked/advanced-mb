/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

namespace Constants
{
    // Band configuration
    constexpr int kMaxBands = 6;
    constexpr int kMaxCrossovers = 5;
    constexpr int kDefaultNumBands = 0;

    // Region configuration (N user bands → up to 2N+1 regions with gaps)
    constexpr int kMaxRegions = 13;           // 6 user + 7 gaps max
    constexpr int kMaxRegionCrossovers = 12;  // kMaxRegions - 1

    // Band width limits (in octaves)
    constexpr float kMinBandWidthOctaves = 0.25f;
    constexpr float kDefaultBandWidthOctaves = 2.0f;

    // Frequency range
    constexpr float kMinFrequency = 20.0f;
    constexpr float kMaxFrequency = 20000.0f;

    // Default crossover frequencies (Hz)
    constexpr float kDefaultCrossover1 = 100.0f;
    constexpr float kDefaultCrossover2 = 500.0f;
    constexpr float kDefaultCrossover3 = 2000.0f;
    constexpr float kDefaultCrossover4 = 6000.0f;
    constexpr float kDefaultCrossover5 = 14000.0f;

    // Threshold range (dB)
    constexpr float kMinThreshold = -90.0f;
    constexpr float kMaxThreshold = 0.0f;
    constexpr float kDefaultThreshold = -20.0f;

    // Range (dB) - controls max gain reduction/expansion
    constexpr float kMinRange = -30.0f;
    constexpr float kMaxRange = 30.0f;
    constexpr float kDefaultRange = 30.0f;

    // Ratio
    constexpr float kMinRatio = 1.0f;
    constexpr float kMaxRatio = 100.0f;
    constexpr float kDefaultRatio = 4.0f;

    // Attack (ms) — logarithmic range
    constexpr float kMinAttackMs = 0.01f;
    constexpr float kMaxAttackMs = 500.0f;
    constexpr float kDefaultAttackMs = 5.0f;

    // Release (ms) — logarithmic range
    constexpr float kMinReleaseMs = 1.0f;
    constexpr float kMaxReleaseMs = 5000.0f;
    constexpr float kDefaultReleaseMs = 100.0f;

    // Knee (dB)
    constexpr float kMinKnee = 0.0f;
    constexpr float kMaxKnee = 36.0f;
    constexpr float kDefaultKnee = 6.0f;

    // Lookahead (ms)
    constexpr float kMinLookahead = 0.0f;
    constexpr float kMaxLookahead = 20.0f;
    constexpr float kDefaultLookahead = 0.0f;

    // Stereo link (%)
    constexpr float kMinStereoLink = 0.0f;
    constexpr float kMaxStereoLink = 100.0f;
    constexpr float kDefaultStereoLink = 100.0f;

    // Output gain (dB)
    constexpr float kMinOutputGain = -30.0f;
    constexpr float kMaxOutputGain = 30.0f;
    constexpr float kDefaultOutputGain = 0.0f;

    // Global output gain (dB)
    constexpr float kMinGlobalOutput = -30.0f;
    constexpr float kMaxGlobalOutput = 30.0f;
    constexpr float kDefaultGlobalOutput = 0.0f;

    // Mix (0-200%)
    constexpr float kMinMix = 0.0f;
    constexpr float kMaxMix = 200.0f;
    constexpr float kDefaultMix = 100.0f;

    // Sidechain filter frequencies
    constexpr float kMinSCFreq = 20.0f;
    constexpr float kMaxSCFreq = 20000.0f;
    constexpr float kDefaultSCFreqLo = 20.0f;
    constexpr float kDefaultSCFreqHi = 20000.0f;

    // Editor dimensions
    constexpr int kDefaultEditorWidth = 1100;
    constexpr int kDefaultEditorHeight = 660;
    constexpr int kMinEditorWidth = 825;
    constexpr int kMinEditorHeight = 560;
    constexpr int kMaxEditorWidth = 2112;
    constexpr int kMaxEditorHeight = 1188;

    // UI layout
    constexpr int kHeaderHeight = 36;
    constexpr int kBottomStripHeight = 28;

    // Metering section
    constexpr int kSingleMeterWidth = 20;                                       // one stereo meter
    constexpr int kMeterLabelHeight = 14;                                       // label below each meter
    constexpr int kMeterGap = 3;                                                // gap between meters
    constexpr int kMeterSectionWidth = kSingleMeterWidth * 3 + kMeterGap * 2;  // 66px total
    constexpr int kMeterWidth = kMeterSectionWidth;                             // backward compat alias

    // Floating popup panel
    constexpr int kPopupPanelWidth           = 660;
    constexpr int kPopupPanelHeight          = 148;
    constexpr int kPopupPanelBottomMargin    = 24;
    constexpr int kPopupCornerRadius         = 8;
    constexpr int kPopupAnimationMs          = 200;
    constexpr int kPopupHeaderHeight         = 28;
    constexpr float kPopupMinWidthRatio      = 0.55f;
    constexpr float kPopupMaxWidthRatio      = 0.70f;

    // FFT configuration
    constexpr int kFFTOrder = 12;
    constexpr int kFFTSize = 1 << kFFTOrder;  // 4096

    // UI timer
    constexpr int kUITimerHz = 30;

    // Crossover slopes (dB/oct)
    enum CrossoverSlope
    {
        Slope6 = 0,
        Slope12,
        Slope18,
        Slope24,
        Slope36,
        Slope48,
        NumSlopes
    };

    // Processing modes
    enum ProcessingMode
    {
        DynamicPhase = 0,
        LinearPhase,
        MinimumPhase,
        NumModes
    };

    // Band compression modes
    enum BandMode
    {
        Compress = 0,
        Expand,
        NumBandModes
    };

    // Stereo modes
    enum StereoMode
    {
        Stereo = 0,
        MidSide,
        NumStereoModes
    };

    // Sidechain modes
    enum SidechainMode
    {
        Band = 0,
        Free,
        NumSCModes
    };

    // Analyzer modes
    enum AnalyzerMode
    {
        AnalyzerOff = 0,
        AnalyzerPre,
        AnalyzerPost,
        AnalyzerPrePost,
        NumAnalyzerModes
    };

    // Oversampling factors
    enum OversamplingFactor
    {
        Oversample1x = 0,
        Oversample2x,
        Oversample4x,
        NumOversamplingFactors
    };
}
