/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "../utils/Constants.h"

/**
 * Represents a single active compressor band mapped to a parameter slot.
 * Bands are ordered by frequency; the slot index is stable for DAW automation.
 */
struct Band
{
    int slotIndex = -1;          // Which APVTS parameter slot (0-5) this band uses
    float leftFreqHz  = Constants::kMinFrequency;
    float rightFreqHz = Constants::kMaxFrequency;

    bool isValid() const { return slotIndex >= 0 && slotIndex < Constants::kMaxBands; }
};

/**
 * A contiguous frequency range in the region-based processing model.
 * slotIndex >= 0 → user band (processed by BandCompressorCore[slotIndex])
 * slotIndex == -1 → gap (audio passes through unprocessed)
 */
struct Region
{
    float leftFreqHz  = Constants::kMinFrequency;
    float rightFreqHz = Constants::kMaxFrequency;
    int   slotIndex   = -1;  // -1 = gap (passthrough), 0-5 = user band slot

    bool isGap() const { return slotIndex < 0; }
};
