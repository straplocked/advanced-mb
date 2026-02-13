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
