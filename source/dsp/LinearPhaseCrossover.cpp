/*
 * Advanced MB - Open-source multiband compressor plugin
 * Copyright (C) 2024-2026 Advanced MB contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "LinearPhaseCrossover.h"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace
{
    constexpr double kPi = 3.14159265358979323846;

    // Generate a Hann window of given size
    void makeHannWindow(float* window, int size)
    {
        for (int i = 0; i < size; ++i)
            window[i] = 0.5f * (1.0f - static_cast<float>(std::cos(2.0 * kPi * i / (size - 1))));
    }

    // Generate a windowed-sinc low-pass FIR kernel
    // fc = normalised cutoff frequency (0 to 0.5, where 0.5 = Nyquist)
    void makeLowPassKernel(float* kernel, int size, double fc)
    {
        int M = size - 1;
        double halfM = M * 0.5;

        for (int i = 0; i < size; ++i)
        {
            double n = i - halfM;

            // Sinc function
            float sinc;
            if (std::abs(n) < 1.0e-10)
                sinc = 1.0f;
            else
                sinc = static_cast<float>(std::sin(2.0 * kPi * fc * n) / (kPi * n));

            // Apply Hann window
            float window = 0.5f * (1.0f - static_cast<float>(std::cos(2.0 * kPi * i / M)));
            kernel[i] = sinc * window;
        }

        // Normalise the kernel so DC gain = 1.0
        float sum = 0.0f;
        for (int i = 0; i < size; ++i)
            sum += kernel[i];

        if (std::abs(sum) > 1.0e-10f)
        {
            float invSum = 1.0f / sum;
            for (int i = 0; i < size; ++i)
                kernel[i] *= invSum;
        }
    }
}

void LinearPhaseCrossover::prepare(double newSampleRate, int maxBlockSize, int numChannels)
{
    sampleRate = newSampleRate;
    channels = std::min(numChannels, 2);

    crossoverFreqs.fill(1000.0f);

    rebuildKernels();
}

void LinearPhaseCrossover::reset()
{
    for (auto& bandFilters : firFilters)
        for (auto& filter : bandFilters)
            filter.reset();
}

void LinearPhaseCrossover::setNumBands(int n)
{
    int newBands = std::clamp(n, 1, Constants::kMaxBands);
    if (newBands != numBands)
    {
        numBands = newBands;
        rebuildKernels();
    }
}

void LinearPhaseCrossover::setCrossoverFrequency(int index, float hz)
{
    if (index < 0 || index >= Constants::kMaxCrossovers)
        return;

    hz = std::clamp(hz, Constants::kMinFrequency, Constants::kMaxFrequency);

    if (crossoverFreqs[index] != hz)
    {
        crossoverFreqs[index] = hz;
        rebuildKernels();
    }
}

void LinearPhaseCrossover::rebuildKernels()
{
    // Clear all kernels
    for (auto& k : kernels)
        k.clear();

    if (numBands <= 1)
    {
        // Single band: identity kernel (delta function at center)
        kernels[0].resize(static_cast<size_t>(kernelSize), 0.0f);
        kernels[0][static_cast<size_t>(kernelSize / 2)] = 1.0f;

        // Set up FIR filters for band 0
        auto coeffs = juce::dsp::FIR::Coefficients<float>(kernels[0].data(), static_cast<size_t>(kernelSize));
        for (int ch = 0; ch < channels; ++ch)
            firFilters[0][ch].coefficients = new juce::dsp::FIR::Coefficients<float>(kernels[0].data(), static_cast<size_t>(kernelSize));

        return;
    }

    int numCrossovers = numBands - 1;

    // Build LP kernels for each crossover frequency
    std::vector<std::vector<float>> lpKernels(static_cast<size_t>(numCrossovers));
    for (int i = 0; i < numCrossovers; ++i)
    {
        lpKernels[i].resize(static_cast<size_t>(kernelSize));
        double fc = static_cast<double>(crossoverFreqs[i]) / sampleRate; // normalised frequency
        makeLowPassKernel(lpKernels[i].data(), kernelSize, fc);
    }

    // Build band kernels:
    //   Band 0 (lowest):  LP[0]
    //   Band i (middle):  LP[i] - LP[i-1]  (bandpass = difference of two low-pass)
    //   Band N-1 (highest): delta - LP[N-2]  (high-pass complement)

    // Band 0: low-pass of first crossover
    kernels[0] = lpKernels[0];

    // Middle bands: difference of adjacent low-pass kernels
    for (int b = 1; b < numCrossovers; ++b)
    {
        kernels[b].resize(static_cast<size_t>(kernelSize));
        for (int i = 0; i < kernelSize; ++i)
            kernels[b][i] = lpKernels[b][i] - lpKernels[b - 1][i];
    }

    // Highest band: complement of last low-pass (HP = delta - LP)
    {
        int lastBand = numBands - 1;
        kernels[lastBand].resize(static_cast<size_t>(kernelSize), 0.0f);
        int center = kernelSize / 2;
        for (int i = 0; i < kernelSize; ++i)
        {
            float delta = (i == center) ? 1.0f : 0.0f;
            kernels[lastBand][i] = delta - lpKernels[numCrossovers - 1][i];
        }
    }

    // Load kernels into FIR filter objects
    for (int b = 0; b < numBands; ++b)
    {
        if (kernels[b].empty())
            continue;

        for (int ch = 0; ch < channels; ++ch)
        {
            firFilters[b][ch].coefficients =
                new juce::dsp::FIR::Coefficients<float>(kernels[b].data(),
                                                        static_cast<size_t>(kernelSize));
        }
    }
}

void LinearPhaseCrossover::process(const juce::AudioBuffer<float>& input,
                                   std::array<juce::AudioBuffer<float>, Constants::kMaxBands>& bandOutputs)
{
    const int numSamples = input.getNumSamples();
    const int numChannels = std::min(input.getNumChannels(), channels);

    // Ensure output buffers are properly sized
    for (int b = 0; b < Constants::kMaxBands; ++b)
    {
        if (bandOutputs[b].getNumChannels() < numChannels ||
            bandOutputs[b].getNumSamples() < numSamples)
        {
            bandOutputs[b].setSize(numChannels, numSamples, false, false, true);
        }
        bandOutputs[b].clear();
    }

    // If single band, pass-through with latency compensation (delay by kernelSize/2)
    // For simplicity in this implementation, we still convolve with the delta kernel
    // which naturally introduces the correct latency.

    // Convolve each band's kernel with the input for each channel
    for (int b = 0; b < numBands; ++b)
    {
        if (kernels[b].empty())
            continue;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            // Process sample-by-sample through the FIR filter
            const float* inputData = input.getReadPointer(ch);
            float* outputData = bandOutputs[b].getWritePointer(ch);

            for (int i = 0; i < numSamples; ++i)
                outputData[i] = firFilters[b][ch].processSample(inputData[i]);
        }
    }
}
