/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "ParameterIDs.h"
#include "../dsp/MultibandEngine.h"
#include "../utils/Constants.h"
#include <array>
#include <atomic>

class MultibandParameterAttachment
{
public:
    MultibandParameterAttachment() = default;

    void init(juce::AudioProcessorValueTreeState& apvts)
    {
        // Global
        processMode = apvts.getRawParameterValue(ParameterIDs::Global::processMode);
        oversample = apvts.getRawParameterValue(ParameterIDs::Global::oversample);
        analyzerMode = apvts.getRawParameterValue(ParameterIDs::Global::analyzerMode);
        globalOutput = apvts.getRawParameterValue(ParameterIDs::Global::globalOutput);
        mix = apvts.getRawParameterValue(ParameterIDs::Global::mix);
        numBands = apvts.getRawParameterValue(ParameterIDs::Global::numBands);
        midiLearn = apvts.getRawParameterValue(ParameterIDs::Global::midiLearn);

        // Crossovers
        for (int i = 0; i < Constants::kMaxCrossovers; ++i)
        {
            crossoverFreq[i] = apvts.getRawParameterValue(
                ParameterIDs::crossoverParam(i, ParameterIDs::Crossover::frequency));
            crossoverSlope[i] = apvts.getRawParameterValue(
                ParameterIDs::crossoverParam(i, ParameterIDs::Crossover::slope));
        }

        // Bands
        for (int i = 0; i < Constants::kMaxBands; ++i)
        {
            bandEnabled[i] = apvts.getRawParameterValue(ParameterIDs::bandParam(i, ParameterIDs::Band::enabled));
            bandMode[i] = apvts.getRawParameterValue(ParameterIDs::bandParam(i, ParameterIDs::Band::mode));
            bandThreshold[i] = apvts.getRawParameterValue(ParameterIDs::bandParam(i, ParameterIDs::Band::threshold));
            bandRange[i] = apvts.getRawParameterValue(ParameterIDs::bandParam(i, ParameterIDs::Band::range));
            bandRatio[i] = apvts.getRawParameterValue(ParameterIDs::bandParam(i, ParameterIDs::Band::ratio));
            bandAttack[i] = apvts.getRawParameterValue(ParameterIDs::bandParam(i, ParameterIDs::Band::attack));
            bandRelease[i] = apvts.getRawParameterValue(ParameterIDs::bandParam(i, ParameterIDs::Band::release));
            bandOutputGain[i] = apvts.getRawParameterValue(ParameterIDs::bandParam(i, ParameterIDs::Band::outputGain));
            bandKnee[i] = apvts.getRawParameterValue(ParameterIDs::bandParam(i, ParameterIDs::Band::knee));
            bandLookahead[i] = apvts.getRawParameterValue(ParameterIDs::bandParam(i, ParameterIDs::Band::lookahead));
            bandStereoLink[i] = apvts.getRawParameterValue(ParameterIDs::bandParam(i, ParameterIDs::Band::stereoLink));
            bandStereoMode[i] = apvts.getRawParameterValue(ParameterIDs::bandParam(i, ParameterIDs::Band::stereoMode));
            bandSCMode[i] = apvts.getRawParameterValue(ParameterIDs::bandParam(i, ParameterIDs::Band::scMode));
            bandSCFreqLo[i] = apvts.getRawParameterValue(ParameterIDs::bandParam(i, ParameterIDs::Band::scFreqLo));
            bandSCFreqHi[i] = apvts.getRawParameterValue(ParameterIDs::bandParam(i, ParameterIDs::Band::scFreqHi));
            bandSCExternal[i] = apvts.getRawParameterValue(ParameterIDs::bandParam(i, ParameterIDs::Band::scExternal));
            bandAudition[i] = apvts.getRawParameterValue(ParameterIDs::bandParam(i, ParameterIDs::Band::audition));
        }
    }

    MultibandParameters getCurrentParameters() const
    {
        MultibandParameters p;

        p.numBands = (int)numBands->load();
        p.processMode = (int)processMode->load();
        p.oversampleIndex = (int)oversample->load();
        p.analyzerMode = (int)analyzerMode->load();
        p.globalOutput = globalOutput->load();
        p.mix = mix->load();

        for (int i = 0; i < Constants::kMaxCrossovers; ++i)
        {
            p.crossovers[i].frequency = crossoverFreq[i]->load();
            p.crossovers[i].slope = (int)crossoverSlope[i]->load();
        }

        for (int i = 0; i < Constants::kMaxBands; ++i)
        {
            p.bands[i].enabled = bandEnabled[i]->load() > 0.5f;
            p.bands[i].expandMode = (int)bandMode[i]->load() == 1;
            p.bands[i].threshold = bandThreshold[i]->load();
            p.bands[i].range = bandRange[i]->load();
            p.bands[i].ratio = bandRatio[i]->load();
            p.bands[i].attackPct = bandAttack[i]->load();
            p.bands[i].releasePct = bandRelease[i]->load();
            p.bands[i].outputGain = bandOutputGain[i]->load();
            p.bands[i].knee = bandKnee[i]->load();
            p.bands[i].lookaheadMs = bandLookahead[i]->load();
            p.bands[i].stereoLink = bandStereoLink[i]->load();
        }

        return p;
    }

    bool isMidiLearnActive() const { return midiLearn->load() > 0.5f; }
    int getAnalyzerMode() const { return (int)analyzerMode->load(); }

private:
    // Global
    std::atomic<float>* processMode = nullptr;
    std::atomic<float>* oversample = nullptr;
    std::atomic<float>* analyzerMode = nullptr;
    std::atomic<float>* globalOutput = nullptr;
    std::atomic<float>* mix = nullptr;
    std::atomic<float>* numBands = nullptr;
    std::atomic<float>* midiLearn = nullptr;

    // Crossovers
    std::array<std::atomic<float>*, Constants::kMaxCrossovers> crossoverFreq = {};
    std::array<std::atomic<float>*, Constants::kMaxCrossovers> crossoverSlope = {};

    // Bands
    std::array<std::atomic<float>*, Constants::kMaxBands> bandEnabled = {};
    std::array<std::atomic<float>*, Constants::kMaxBands> bandMode = {};
    std::array<std::atomic<float>*, Constants::kMaxBands> bandThreshold = {};
    std::array<std::atomic<float>*, Constants::kMaxBands> bandRange = {};
    std::array<std::atomic<float>*, Constants::kMaxBands> bandRatio = {};
    std::array<std::atomic<float>*, Constants::kMaxBands> bandAttack = {};
    std::array<std::atomic<float>*, Constants::kMaxBands> bandRelease = {};
    std::array<std::atomic<float>*, Constants::kMaxBands> bandOutputGain = {};
    std::array<std::atomic<float>*, Constants::kMaxBands> bandKnee = {};
    std::array<std::atomic<float>*, Constants::kMaxBands> bandLookahead = {};
    std::array<std::atomic<float>*, Constants::kMaxBands> bandStereoLink = {};
    std::array<std::atomic<float>*, Constants::kMaxBands> bandStereoMode = {};
    std::array<std::atomic<float>*, Constants::kMaxBands> bandSCMode = {};
    std::array<std::atomic<float>*, Constants::kMaxBands> bandSCFreqLo = {};
    std::array<std::atomic<float>*, Constants::kMaxBands> bandSCFreqHi = {};
    std::array<std::atomic<float>*, Constants::kMaxBands> bandSCExternal = {};
    std::array<std::atomic<float>*, Constants::kMaxBands> bandAudition = {};
};
