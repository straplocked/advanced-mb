# DSP Documentation

This document describes the digital signal processing algorithms used in Advanced MB.

## Crossover Filter Design

### Algorithm: Linkwitz-Riley via Cascaded Cytomic TPT SVF

The crossover filters are Linkwitz-Riley types, constructed by cascading Cytomic TPT (Topology-Preserving Transform) State Variable Filter sections. This design was chosen for several reasons:

1. **Flat magnitude sum.** Linkwitz-Riley crossovers sum to unity at all frequencies, meaning the combined lowpass and highpass outputs reconstruct the original signal without colouration (when no per-band processing is applied).
2. **Zipper-free modulation.** The TPT SVF structure allows frequency and Q to be changed at sample rate without transient artifacts or instability.
3. **Unconditional stability.** The topology-preserving bilinear transform guarantees stability for any parameter value.

The core SVF filter is implemented in `SVFFilter` and is based on Andy Simper's technical papers (https://cytomic.com/technical-papers/). Each SVF section maintains two integrator states (`ic1eq`, `ic2eq`) and uses the key coefficient:

```
g = tan(pi * freq / sampleRate)
```

The per-sample tick equations are:

```
v3 = input - ic2eq
v1 = a1 * ic1eq + a2 * v3
v2 = ic2eq + a2 * ic1eq + a3 * v3
ic1eq = 2 * v1 - ic1eq
ic2eq = 2 * v2 - ic2eq
output = m0 * input + m1 * v1 + m2 * v2
```

The mix coefficients `m0`, `m1`, `m2` select the filter type (lowpass, highpass, bandpass, etc.) without changing the core algorithm.

### Crossover Slopes

Each crossover point supports six slope options, implemented by cascading different combinations of first-order and second-order SVF sections:

| Slope Index | Slope      | Filter Configuration                                    | Designation |
|-------------|------------|--------------------------------------------------------|-------------|
| 0           | 6 dB/oct   | 1 first-order LP + 1 first-order HP                    | LR1         |
| 1           | 12 dB/oct  | 1 second-order (Q=0.5) LP + HP                         | LR2         |
| 2           | 18 dB/oct  | 1 first-order + 1 second-order (Q=0.7071) LP + HP     | LR3         |
| 3           | 24 dB/oct  | 2 cascaded second-order Butterworth (Q=0.7071) LP + HP | LR4         |
| 4           | 36 dB/oct  | 3 cascaded second-order Butterworth LP + HP            | LR6         |
| 5           | 48 dB/oct  | 4 cascaded second-order Butterworth LP + HP            | LR8         |

For **LR2** (12 dB/oct), a single second-order section with Q=0.5 produces the characteristic Butterworth-squared Linkwitz-Riley response.

For **LR3** (18 dB/oct, odd order), the cascade uses one first-order section followed by one second-order Butterworth section (Q = 1/sqrt(2) = 0.7071).

For **LR4** and higher even orders, the cascade uses multiple second-order Butterworth sections (Q=0.7071 each), producing the standard Linkwitz-Riley all-pass-at-crossover behaviour.

### Band Splitting

The `CrossoverFilterBank` class splits an input signal into up to 6 bands using up to 5 `LinkwitzRileyFilter` instances (one per crossover point). Each `LinkwitzRileyFilter` produces both a lowpass and a highpass output simultaneously. The bank processes crossovers in sequence, feeding the highpass remainder into the next crossover stage.

## Gain Computer

The `GainComputer` class implements the static transfer function that maps input level (dB) to gain change (dB). It supports two modes: **compression** and **expansion**.

### Downward Compression (Soft-Knee)

The soft-knee formula uses a quadratic transition region around the threshold:

```
Given: inputDB, threshold, ratio, kneeWidth, range

halfKnee = kneeWidth / 2

if inputDB <= threshold - halfKnee:
    gainReduction = 0                           (below knee: no compression)

else if inputDB >= threshold + halfKnee:
    excess = inputDB - threshold
    gainReduction = excess * (1 - 1/ratio)      (above knee: full compression)

else:
    x = inputDB - threshold + halfKnee
    gainReduction = (1 - 1/ratio) * x^2 / (2 * kneeWidth)   (in knee: quadratic)

gainReduction = min(gainReduction, range)       (clamp to range)
output = -gainReduction                         (negative = gain reduction)
```

When `kneeWidth = 0`, this reduces to a hard-knee compressor. The `range` parameter limits the maximum gain reduction, allowing the compressor to act as a gentle leveller rather than a brick-wall limiter.

### Downward Expansion

In expand mode, the gain computer reduces gain when the signal falls **below** the threshold, acting as a downward expander or noise gate:

```
if inputDB >= threshold + halfKnee:
    expansion = 0                               (above threshold: no expansion)

else if inputDB <= threshold - halfKnee:
    deficit = threshold - inputDB
    expansion = deficit * (ratio - 1)           (full expansion below knee)

else:
    x = threshold + halfKnee - inputDB
    expansion = (ratio - 1) * x^2 / (2 * kneeWidth)   (in knee: quadratic)

expansion = min(expansion, abs(range))          (clamp to range)
output = -expansion                             (negative = gain reduction)
```

## Envelope Follower

The `EnvelopeFollower` smooths the instantaneous gain reduction output from the `GainComputer` using a one-pole filter with separate attack and release coefficients.

### Percentage-Based Controls

Rather than exposing raw millisecond values, attack and release are controlled as percentages (0-100%). These are mapped to time values via an exponential curve:

```
Attack:  ms = 0.1 * (200 / 0.1) ^ (pct / 100)     =>  0% -> 0.1 ms,  100% -> 200 ms
Release: ms = 5.0 * (2000 / 5.0) ^ (pct / 100)     =>  0% -> 5 ms,    100% -> 2000 ms
```

This exponential mapping provides fine control at fast settings and broad sweeps at slow settings.

### Frequency-Dependent Scaling

A key feature of the envelope follower is automatic frequency-dependent timing. Each band's center frequency scales the attack and release times:

```
freqScale = sqrt(1000 / bandFrequency)
actualAttackMs  = baseAttackMs  * freqScale
actualReleaseMs = baseReleaseMs * freqScale
```

This means:
- **Low-frequency bands** (e.g., 80 Hz): `freqScale = sqrt(1000/80) = 3.54x` -- attack and release are approximately 3.5 times slower, preventing the compressor from distorting low-frequency waveforms.
- **Mid-frequency bands** (1000 Hz): `freqScale = 1.0x` -- baseline timing.
- **High-frequency bands** (e.g., 8000 Hz): `freqScale = sqrt(1000/8000) = 0.35x` -- timing is roughly 3 times faster, allowing the compressor to respond to transient detail.

This scaling is applied automatically based on the band's crossover-derived center frequency. The user controls a single percentage knob per band, and the envelope follower adapts its absolute timing to the frequency content of that band.

### One-Pole Smoothing

The time-to-coefficient conversion uses the standard one-pole formula:

```
coefficient = exp(-1 / (time_seconds * sampleRate))
```

The per-sample processing selects the coefficient based on the direction of change:

```
if targetGR < currentGR:
    coeff = attackCoeff      (moving towards more gain reduction)
else:
    coeff = releaseCoeff     (moving towards less gain reduction)

currentGR = coeff * currentGR + (1 - coeff) * targetGR
```

## Level Detection

The `LevelDetector` class converts raw audio samples to a level measurement in dB. It supports two modes:

### Peak Detection

Returns the absolute value of the input sample, converted to dB:

```
level_dB = 20 * log10(|sample|)
```

Peak detection responds instantly to transients and is the default mode.

### RMS Detection

Uses a one-pole lowpass filter to compute a running mean-square estimate:

```
rmsSquared = rmsCoeff * rmsSquared + (1 - rmsCoeff) * sample^2
level_dB = 10 * log10(rmsSquared)
```

The RMS window coefficient is derived from a configurable window length in milliseconds. RMS detection provides a smoother, averaged level measurement that better represents perceived loudness.

## Stereo Linking

The `BandCompressorCore` supports three stereo linking behaviours controlled by the `stereo_link` parameter (0-100%):

| stereo_link | Behaviour                                                                 |
|-------------|---------------------------------------------------------------------------|
| 0%          | **Independent.** Each channel's level is detected and compressed separately. This can cause image shifts but allows maximum channel-specific dynamics control. |
| 100%        | **Fully linked.** The maximum of L and R levels is used for both channels. Gain reduction is identical on both channels, preserving the stereo image. |
| 1-99%       | **Blended.** Each channel's detected level is interpolated towards the max: `linked = independent + linkFactor * (max - independent)`. Partial linking allows some independent dynamics while mostly preserving stereo width. |

## Stereo Modes

Each band can operate in one of two stereo modes:

- **Stereo (L/R):** Compression is applied to the left and right channels directly.
- **Mid/Side:** The signal is encoded to Mid/Side before compression and decoded back to L/R afterwards. This allows independent dynamics control over the center (mid) and sides of the stereo image. Handled by `MidSideProcessor`.

## Sidechain

Each band supports two sidechain modes:

- **Band mode:** The sidechain signal is the band's own audio (after crossover splitting). The compressor reacts to the content of the band itself.
- **Free mode:** A configurable bandpass filter (defined by `sc_freq_lo` and `sc_freq_hi`) is applied to the sidechain signal before detection. This allows the compressor to react to a specific frequency range within the band.

When `sc_external` is enabled, the detector uses audio from the plugin's sidechain input bus instead of the band's own signal.

## Oversampling

The `Oversampler` wraps JUCE's `juce::dsp::Oversampling<float>` using polyphase IIR half-band filters. Three factors are available:

| Factor | Internal Sample Rate | Latency    | Use Case                                    |
|--------|---------------------|------------|----------------------------------------------|
| 1x     | Base rate           | 0 samples  | Default, lowest CPU                          |
| 2x     | 2 * base rate       | ~8 samples | Reduced aliasing for aggressive compression  |
| 4x     | 4 * base rate       | ~16 samples| Maximum quality, highest CPU cost            |

Oversampling is applied before the crossover bank and removed after summing, so all DSP (crossover filtering + compression) operates at the elevated sample rate. This reduces aliasing artifacts from nonlinear gain changes, particularly at high compression ratios.

## Processing Modes

The plugin offers three crossover phase modes selected by the `process_mode` parameter:

- **Dynamic Phase:** Standard Linkwitz-Riley IIR crossovers. Zero latency (from the crossover), natural phase response. Default mode.
- **Linear Phase:** FFT-based crossover via `LinearPhaseCrossover`. All bands are perfectly phase-aligned, but introduces latency proportional to the FFT size.
- **Minimum Phase:** IIR crossovers with minimum-phase characteristics. Low latency with a different phase profile than the dynamic mode.
