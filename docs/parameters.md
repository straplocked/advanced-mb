# Parameter Reference

Advanced MB exposes approximately 116 automatable parameters organized into three groups: global parameters, per-crossover parameters (5 crossovers), and per-band parameters (6 bands).

## Global Parameters (7)

| Parameter ID      | Display Name      | Type    | Range / Choices                                | Default       | Notes                        |
|-------------------|-------------------|---------|------------------------------------------------|---------------|------------------------------|
| `process_mode`    | Processing Mode   | Choice  | Dynamic Phase, Linear Phase, Minimum Phase     | Dynamic Phase | Crossover phase behavior     |
| `oversample`      | Oversampling      | Choice  | 1x, 2x, 4x                                    | 1x            | Polyphase IIR oversampling   |
| `analyzer_mode`   | Analyzer          | Choice  | Off, Pre, Post, Pre+Post                       | Off           | Spectrum analyzer display    |
| `global_output`   | Global Output     | Float   | -30.0 .. +30.0 dB (step 0.1)                  | 0.0 dB        | Master output gain           |
| `mix`             | Mix               | Float   | 0.0 .. 200.0 % (step 0.1)                     | 100.0 %       | Dry/wet blend (>100% = over-process) |
| `num_bands`       | Num Bands         | Int     | 1 .. 6                                         | 1             | Number of active bands       |
| `midi_learn`      | MIDI Learn        | Bool    | On / Off                                       | Off           | MIDI learn assignment mode   |

## Per-Crossover Parameters (5 crossovers, 2 params each = 10)

Each crossover is identified by index 0-4. Parameter IDs follow the pattern `xover{i}_{suffix}`.

| Suffix  | Display Name              | Type    | Range / Choices                                         | Default     | Notes                              |
|---------|---------------------------|---------|--------------------------------------------------------|-------------|------------------------------------|
| `freq`  | Crossover {i+1} Freq      | Float   | 20.0 .. 20000.0 Hz (step 0.1, skew 0.25)              | See below   | Log-scaled frequency control       |
| `slope` | Crossover {i+1} Slope     | Choice  | 6 dB/oct, 12 dB/oct, 18 dB/oct, 24 dB/oct, 36 dB/oct, 48 dB/oct | 24 dB/oct   | Linkwitz-Riley crossover steepness |

### Default Crossover Frequencies

| Crossover | Default Frequency |
|-----------|-------------------|
| 1         | 100 Hz            |
| 2         | 500 Hz            |
| 3         | 2000 Hz           |
| 4         | 6000 Hz           |
| 5         | 14000 Hz          |

## Per-Band Parameters (6 bands, 17 params each = 102)

Each band is identified by index 0-5. Parameter IDs follow the pattern `band{i}_{suffix}`.

| Suffix         | Display Name         | Type    | Range / Choices                          | Default     | Notes                                     |
|----------------|----------------------|---------|------------------------------------------|-------------|--------------------------------------------|
| `enabled`      | Band {i+1} Enabled   | Bool    | On / Off                                 | On          | Bypass this band's compressor              |
| `mode`         | Band {i+1} Mode      | Choice  | Compress, Expand                         | Compress    | Downward compression or downward expansion |
| `threshold`    | Band {i+1} Threshold | Float   | -90.0 .. 0.0 dB (step 0.1)              | -20.0 dB    | Level above/below which processing begins  |
| `range`        | Band {i+1} Range     | Float   | -30.0 .. +30.0 dB (step 0.1)            | 30.0 dB     | Maximum gain reduction/expansion           |
| `ratio`        | Band {i+1} Ratio     | Float   | 1.0 .. 100.0 :1 (step 0.01, skew 0.3)  | 4.0 :1      | Compression/expansion ratio. Skewed for fine control at low ratios. |
| `attack`       | Band {i+1} Attack    | Float   | 0.0 .. 100.0 % (step 0.1)               | 50.0 %      | Percentage-based attack speed (see DSP docs) |
| `release`      | Band {i+1} Release   | Float   | 0.0 .. 100.0 % (step 0.1)               | 50.0 %      | Percentage-based release speed (see DSP docs) |
| `output_gain`  | Band {i+1} Output    | Float   | -30.0 .. +30.0 dB (step 0.1)            | 0.0 dB      | Per-band makeup gain                       |
| `knee`         | Band {i+1} Knee      | Float   | 0.0 .. 36.0 dB (step 0.1)               | 6.0 dB      | Soft-knee width around threshold           |
| `lookahead`    | Band {i+1} Lookahead | Float   | 0.0 .. 20.0 ms (step 0.1)               | 0.0 ms      | Delay input to let detector see ahead      |
| `stereo_link`  | Band {i+1} Stereo Link | Float | 0.0 .. 100.0 % (step 0.1)               | 100.0 %     | 0% = independent channels, 100% = linked   |
| `stereo_mode`  | Band {i+1} Stereo Mode | Choice | Stereo, Mid/Side                        | Stereo      | Processing domain for this band            |
| `sc_mode`      | Band {i+1} SC Mode   | Choice  | Band, Free                               | Band        | Sidechain filter mode                      |
| `sc_freq_lo`   | Band {i+1} SC Lo     | Float   | 20.0 .. 20000.0 Hz (step 0.1, skew 0.25) | 20.0 Hz    | Sidechain highpass frequency               |
| `sc_freq_hi`   | Band {i+1} SC Hi     | Float   | 20.0 .. 20000.0 Hz (step 0.1, skew 0.25) | 20000.0 Hz | Sidechain lowpass frequency                |
| `sc_external`  | Band {i+1} SC External | Bool  | On / Off                                 | Off         | Use external sidechain input               |
| `audition`     | Band {i+1} Audition  | Bool    | On / Off                                 | Off         | Solo this band's audio for monitoring      |

## Parameter Count Summary

| Group                     | Count |
|---------------------------|-------|
| Global parameters         | 7     |
| Per-crossover (5 x 2)    | 10    |
| Per-band (6 x 17)        | 102   |
| **Total**                 | **119** |

## Parameter ID Naming Convention

All parameter IDs are defined in `source/parameters/ParameterIDs.h`. Two helper functions generate per-band and per-crossover IDs:

```cpp
// Per-band: "band0_threshold", "band1_attack", etc.
ParameterIDs::bandParam(bandIndex, ParameterIDs::Band::threshold)

// Per-crossover: "xover0_freq", "xover2_slope", etc.
ParameterIDs::crossoverParam(crossoverIndex, ParameterIDs::Crossover::frequency)
```

Global parameter IDs are simple string constants under the `ParameterIDs::Global` namespace (e.g., `"process_mode"`, `"mix"`, `"global_output"`).

## Skew Factors

Some parameters use a JUCE `NormalisableRange` skew factor to provide non-linear control curves:

| Parameter          | Skew Factor | Effect                                       |
|--------------------|-------------|----------------------------------------------|
| Crossover frequency | 0.25       | Logarithmic feel for frequency selection      |
| Ratio              | 0.3         | Fine resolution at low ratios (1:1 to ~8:1)  |
| SC frequency (Lo/Hi)| 0.25       | Logarithmic feel for frequency selection      |

A skew factor less than 1.0 compresses the lower portion of the range, giving finer control at the bottom and coarser control at the top -- appropriate for frequency and ratio controls.
