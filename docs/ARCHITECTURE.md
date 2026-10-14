# Architecture

## File map

```
CMakeLists.txt            JUCE via FetchContent (or -DDIRTY630_JUCE_DIR), plugin + tests
src/
  dsp/                    PURE C++17, NO JUCE — the model
    Circuit.h             OpAmpStage, CouplingCap, RmsDetector, Vca, Noise
    Sidechain.h           GainComputer, TimingNetwork, Gate
    Compressor3630.h      Channel (one strip), Compressor3630 (stereo + link + calibration)
  PluginProcessor.{h,cpp} AudioProcessor, APVTS layout, oversampling, bus layout
  PluginEditor.{h,cpp}    LookAndFeel, GrMeter, editor layout
tests/dsp_tests.cpp       Self-contained checks against the model
docs/                     These files
```

## Signal flow (one channel)

```
in (FS) ──×inputScale──▶ OpAmpStage ──┬──▶ Vca(gainDb) ──+noise──▶ CouplingCap ──×output──▶ OpAmpStage ──÷inputScale──▶ out
                                      │
            ext SC ──────────────────▶├─▶ [peak hold] ─▶ RmsDetector ─▶ levelDbu ─┬─▶ GainComputer ─▶ TimingNetwork ─┐
                                                                                 └─▶ Gate ──────────────────────────┴─▶ gainDb
```

`inputScale` converts full scale to volts: `0 dBFS peak = calibDbu` (default
18 dBu, so 1.0 FS = 8.7 V peak and the rails at 13.5 V sit ~3.8 dB above full
scale). Everything between the two `inputScale` steps is in volts.

## Stereo link

`Compressor3630::processSample` runs both detectors, then if `stereoLink`:

1. mean-square states of both `RmsDetector`s are averaged (dbx tied-detector behaviour),
2. the shared level is written back into both channels,
3. channel **A's** `GainComputer` (threshold/ratio/knee) drives both timing
   networks. Each channel keeps its own `TimingNetwork` and `Gate` so the
   "Mismatch" spread still applies.

## Threading / realtime

- `processBlock` → `updateParameters()` reads APVTS atomics and rebuilds the
  `ChannelParams` / `GlobalParams` structs every block. Cheap (a few dozen
  `exp`/`log` calls) and avoids a listener/lock. If parameter count grows a
  lot, switch to change flags.
- Oversampling is `juce::dsp::Oversampling` (polyphase IIR half-band,
  max-quality off). Factor changes rebuild it on the audio thread —
  that allocates. Acceptable because it only happens when the user flips the
  combo box; if a host automates it, move the rebuild to a message-thread
  callback with a pre-built set.
- `stereoBuffer` / `scBuffer` are sized in `prepareToPlay`. The resize branch
  in `processBlock` only triggers if the host lies about block size.
- Meters: two `std::atomic<float>` per channel written at the end of each
  block, read by a 30 Hz editor timer.

## Parameters

IDs are `ids::` constants in `PluginProcessor.h`. Per-channel IDs are
suffixed `A`/`B` by `ids::ch()`. All ranges:

| ID | Range | Default | Notes |
|---|---|---|---|
| threshold{A,B} | −40…+20 dB | 0 | dBu at the detector |
| ratio{A,B} | 1…100, skew 0.3 | 4 | ≥ 99 displays and behaves as ∞:1 |
| attack{A,B} | 0.1…200 ms, skew 0.3 | 10 | time constant of the charge diode path |
| release{A,B} | 50…3000 ms, skew 0.4 | 300 | |
| output{A,B} | ±20 dB | 0 | after coupling cap, before output op-amp |
| peak{A,B} | bool | off | peak-hold cap in front of the RMS sensor |
| soft{A,B} | bool | off | 10 dB knee vs 0.5 dB |
| gateThr{A,B} | −70…+10 dB | −70 (= Off) | |
| gateRate{A,B} | 20…2000 ms, skew 0.4 | 100 | close time; open is fixed 0.5 ms |
| bypass{A,B} | bool | off | skips VCA/coupling/output stage, keeps input buffer |
| link | bool | **on** | |
| inputTrim | −12…+24 dB | 0 | |
| calib | 10…24 dBu | 18 | what 0 dBFS peak means at the jack |
| thump | 0…1 | 0.5 | CV feedthrough, 0…3 mV/dB |
| noise | 0…1 | 0.5 | −90 dBu white at 1.0 |
| mismatch | 0…1 | 0.5 | B-channel spread of symmetry/feedthrough/RMS τ |
| extSc | bool | off | needs sidechain bus enabled by host |
| oversamp | 1×/2×/4× | 2× | |

## Buses

Main in/out: mono or stereo (must match). Bus 2 "Sidechain": mono or stereo,
disabled by default. Mono main input is duplicated to both model channels and
the output is taken from channel A.

## Editor

980 × 430, fixed. `Dirty630LookAndFeel` draws rotary knobs (ring + cap +
pointer), toggle buttons as rounded "switches", and `GrMeter` the 12-segment
LED ladder. All attachments are the stock APVTS ones. Nothing here is
resizable-aware yet.
