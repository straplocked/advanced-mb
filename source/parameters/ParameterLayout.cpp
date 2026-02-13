/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ParameterLayout.h"
#include "ParameterIDs.h"
#include "../utils/Constants.h"

juce::AudioProcessorValueTreeState::ParameterLayout ParameterLayout::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Global parameters
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParameterIDs::Global::processMode, "Processing Mode",
        juce::StringArray{"Dynamic Phase", "Linear Phase", "Minimum Phase"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParameterIDs::Global::oversample, "Oversampling",
        juce::StringArray{"1x", "2x", "4x"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParameterIDs::Global::analyzerMode, "Analyzer",
        juce::StringArray{"Off", "Pre", "Post", "Pre+Post"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::Global::globalOutput, "Global Output",
        juce::NormalisableRange<float>(Constants::kMinGlobalOutput, Constants::kMaxGlobalOutput, 0.1f),
        Constants::kDefaultGlobalOutput));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::Global::mix, "Mix",
        juce::NormalisableRange<float>(Constants::kMinMix, Constants::kMaxMix, 0.1f),
        Constants::kDefaultMix));

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        ParameterIDs::Global::numBands, "Num Bands", 1, Constants::kMaxBands, Constants::kDefaultNumBands));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        ParameterIDs::Global::midiLearn, "MIDI Learn", false));

    // Per-crossover parameters (5 crossovers)
    for (int i = 0; i < Constants::kMaxCrossovers; ++i)
    {
        float defaultFreq;
        switch (i)
        {
            case 0: defaultFreq = Constants::kDefaultCrossover1; break;
            case 1: defaultFreq = Constants::kDefaultCrossover2; break;
            case 2: defaultFreq = Constants::kDefaultCrossover3; break;
            case 3: defaultFreq = Constants::kDefaultCrossover4; break;
            case 4: defaultFreq = Constants::kDefaultCrossover5; break;
            default: defaultFreq = 1000.0f; break;
        }

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            ParameterIDs::crossoverParam(i, ParameterIDs::Crossover::frequency),
            "Crossover " + juce::String(i + 1) + " Freq",
            juce::NormalisableRange<float>(Constants::kMinFrequency, Constants::kMaxFrequency, 0.1f, 0.25f),
            defaultFreq));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            ParameterIDs::crossoverParam(i, ParameterIDs::Crossover::slope),
            "Crossover " + juce::String(i + 1) + " Slope",
            juce::StringArray{"6 dB/oct", "12 dB/oct", "18 dB/oct", "24 dB/oct", "36 dB/oct", "48 dB/oct"}, 3));
    }

    // Per-band parameters (6 bands)
    for (int i = 0; i < Constants::kMaxBands; ++i)
    {
        juce::String prefix = "Band " + juce::String(i + 1) + " ";

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            ParameterIDs::bandParam(i, ParameterIDs::Band::enabled),
            prefix + "Enabled", true));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            ParameterIDs::bandParam(i, ParameterIDs::Band::mode),
            prefix + "Mode",
            juce::StringArray{"Compress", "Expand"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            ParameterIDs::bandParam(i, ParameterIDs::Band::threshold),
            prefix + "Threshold",
            juce::NormalisableRange<float>(Constants::kMinThreshold, Constants::kMaxThreshold, 0.1f),
            Constants::kDefaultThreshold));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            ParameterIDs::bandParam(i, ParameterIDs::Band::range),
            prefix + "Range",
            juce::NormalisableRange<float>(Constants::kMinRange, Constants::kMaxRange, 0.1f),
            Constants::kDefaultRange));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            ParameterIDs::bandParam(i, ParameterIDs::Band::ratio),
            prefix + "Ratio",
            juce::NormalisableRange<float>(Constants::kMinRatio, Constants::kMaxRatio, 0.01f, 0.3f),
            Constants::kDefaultRatio));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            ParameterIDs::bandParam(i, ParameterIDs::Band::attack),
            prefix + "Attack",
            juce::NormalisableRange<float>(Constants::kMinAttackPct, Constants::kMaxAttackPct, 0.1f),
            Constants::kDefaultAttackPct));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            ParameterIDs::bandParam(i, ParameterIDs::Band::release),
            prefix + "Release",
            juce::NormalisableRange<float>(Constants::kMinReleasePct, Constants::kMaxReleasePct, 0.1f),
            Constants::kDefaultReleasePct));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            ParameterIDs::bandParam(i, ParameterIDs::Band::outputGain),
            prefix + "Output",
            juce::NormalisableRange<float>(Constants::kMinOutputGain, Constants::kMaxOutputGain, 0.1f),
            Constants::kDefaultOutputGain));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            ParameterIDs::bandParam(i, ParameterIDs::Band::knee),
            prefix + "Knee",
            juce::NormalisableRange<float>(Constants::kMinKnee, Constants::kMaxKnee, 0.1f),
            Constants::kDefaultKnee));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            ParameterIDs::bandParam(i, ParameterIDs::Band::lookahead),
            prefix + "Lookahead",
            juce::NormalisableRange<float>(Constants::kMinLookahead, Constants::kMaxLookahead, 0.1f),
            Constants::kDefaultLookahead));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            ParameterIDs::bandParam(i, ParameterIDs::Band::stereoLink),
            prefix + "Stereo Link",
            juce::NormalisableRange<float>(Constants::kMinStereoLink, Constants::kMaxStereoLink, 0.1f),
            Constants::kDefaultStereoLink));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            ParameterIDs::bandParam(i, ParameterIDs::Band::stereoMode),
            prefix + "Stereo Mode",
            juce::StringArray{"Stereo", "Mid/Side"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            ParameterIDs::bandParam(i, ParameterIDs::Band::scMode),
            prefix + "SC Mode",
            juce::StringArray{"Band", "Free"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            ParameterIDs::bandParam(i, ParameterIDs::Band::scFreqLo),
            prefix + "SC Lo",
            juce::NormalisableRange<float>(Constants::kMinSCFreq, Constants::kMaxSCFreq, 0.1f, 0.25f),
            Constants::kDefaultSCFreqLo));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            ParameterIDs::bandParam(i, ParameterIDs::Band::scFreqHi),
            prefix + "SC Hi",
            juce::NormalisableRange<float>(Constants::kMinSCFreq, Constants::kMaxSCFreq, 0.1f, 0.25f),
            Constants::kDefaultSCFreqHi));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            ParameterIDs::bandParam(i, ParameterIDs::Band::scExternal),
            prefix + "SC External", false));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            ParameterIDs::bandParam(i, ParameterIDs::Band::audition),
            prefix + "Audition", false));
    }

    return { params.begin(), params.end() };
}
