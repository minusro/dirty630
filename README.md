# dirty630

A white-box software model of a budget late-80s/90s stereo VCA compressor/gate
(the 3630), built as a JUCE plugin. VST3 and AU on macOS, VST3 on Linux.

The DSP is written from the circuit, not fitted to a response: a dbx-style
log-domain RMS sensor, a 2150-class VCA with its symmetry and feedthrough
errors, diode-steered attack/release RC, the resistor-blend soft knee, a
chattery one-dB-hysteresis gate, and LF347 stages that saturate at the rails.
The deliberately imperfect bits are on their own knobs under **DIRT** so you
can turn them off.

## Build on macOS

Requires Xcode command line tools and CMake ≥ 3.22. JUCE 8.0.4 is fetched
automatically on first configure.

```sh
git clone https://github.com/minusro/dirty630.git
cd dirty630
cmake -B build -G Xcode
cmake --build build --config Release
```

Builds a universal (arm64 + x86_64) VST3, AU and Standalone. With
`COPY_PLUGIN_AFTER_BUILD` on, the VST3 lands in `~/Library/Audio/Plug-Ins/VST3`
and the AU in `~/Library/Audio/Plug-Ins/Components`. Run `auval -v aufx D630 Minu`
to validate the AU after a build, and restart Logic/Live if it doesn't show up.

Ninja works too: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`.

If you keep a JUCE checkout around, pass `-DDIRTY630_JUCE_DIR=/path/to/JUCE`
to skip the download.

## DSP tests

The model has no JUCE dependency, so the tests build with any C++17 compiler:

```sh
g++ -std=c++17 -O2 -I src tests/dsp_tests.cpp -o dsp_tests && ./dsp_tests
```

or `ctest --test-dir build`. They check the static curve, knee shape, inf:1
limiting, attack/release timing, peak vs RMS, gate, unity-path level and THD,
distortion vs gain reduction, rail clipping, feedthrough thump and the stereo
link.

## Controls

Per channel (A / B): Threshold −40…+20 dB, Ratio 1:1…∞:1, Attack 0.1…200 ms,
Release 50 ms…3 s, Output ±20 dB, Peak/RMS, Hard/Soft knee, Gate threshold
(off…+10 dB), Gate rate 20 ms…2 s, Bypass.

Global: Stereo link (channel A drives both, detectors are tied), Ext sidechain
(bus 2), Input trim, `0 dBFS =` calibration (what full scale means in dBu at
the input jack; 18 dBu by default, so rail clipping starts around +22 dBu),
Oversampling 1×/2×/4×.

DIRT: **Thump** (control-voltage feedthrough, 0…3 mV/dB), **Noise** (−90 dBu
floor), **Mismatch** (channel-to-channel spread of symmetry trim, feedthrough
and timing cap).

## Model notes and what to calibrate

Each block lives in `src/dsp/`:

| File | Block | Hardware origin |
|---|---|---|
| `Circuit.h` | `OpAmpStage` | LF347 output swing, ±13.5 V, saturation from 12 V |
| | `RmsDetector` | dbx 2252-family log-domain RMS sensor; one-pole on mean-square, τ = 20 ms; peak-hold cap τ = 10 ms in PEAK mode |
| | `Vca` | 2150-class: exact dB gain law, 2nd-harmonic symmetry error, 3rd-harmonic rising with gain reduction, CV feedthrough |
| | `CouplingCap` | 10 µF / 100 k output coupling (turns feedthrough DC steps into thumps) |
| `Sidechain.h` | `GainComputer` | threshold/ratio op-amp, hard knee ≈ 0.5 dB diode rounding, soft knee 10 dB resistor blend |
| | `TimingNetwork` | diode-steered attack/release RC with a 0.15 dB dead band |
| | `Gate` | 0.5 ms open, Rate-controlled close, 1 dB hysteresis, −80 dB depth |
| `Compressor3630.h` | `Channel`, `Compressor3630` | signal flow, link, calibration, channel mismatch |

The constants that are educated guesses rather than datasheet values, in the
order they are worth measuring on a real unit:

1. `Compressor3630::kRmsTauSec` and `kPeakHoldSec` (timing caps). Measure with
   a tone burst at Attack = 0.1 ms and watch the gain-reduction envelope.
2. `Vca` distortion coefficients. Sweep level at 0, 10, 20 dB of reduction and
   fit the 2nd/3rd harmonic.
3. `Vca::feedthroughVPerDb` at Thump = 1. Hit it with a step and read the DC
   offset before the coupling cap.
4. Knee widths and `TimingNetwork::kDeadBandDb`.
5. Gate hysteresis and depth.

Everything is in volts at the circuit node, so numbers read from a scope drop
straight in.
