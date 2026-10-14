# The model

What each block in `src/dsp/` represents on the 3630 board, the constants it
uses, where they came from, and how to measure the real value.

Confidence key: **D** = datasheet/spec sheet, **S** = schematic/mod-guide
reference, **G** = educated guess, refit from measurement.

## What is known about the hardware

- VCA: dbx 2150 per channel (U3/U7). THAT 2180/2181 is the drop-in upgrade people fit. **S**
- Detector: "dbx-designed RMS level sensor" — 2252 family behaviour, log-domain averaging. **S**
- Op-amps: LF347N quads (U2/U6 input stage and elsewhere), ±15 V rails. **S**
- Peak mode: C42/C56 add a hold cap; removing them kills PEAK. **S**
- Knee: resistor blend R83/R106 (5.6 k) and R17/R59 (6.8 k) for SOFT. **S**
- Gate: drives the same VCA, widely described as chattery; disabled by cutting two jumpers. **S**
- Spec sheet ranges: threshold −40…+20 dB, ratio 1:1…∞:1, attack 0.1…200 ms,
  release 50 ms…3 s, gate rate 20 ms…2 s. **D**

Source for the board references: diyrecordingequipment.com 3630 mod guide.
A full schematic is in the Alesis service manual (not in this repo).

## Blocks

### OpAmpStage (`Circuit.h`)
LF347 output into a few kΩ: linear to ~12 V, saturates to ~13.5 V.
- `rail = 13.5 V` **D**, `softStart = 12.0 V` **G**.
- Shape: linear then `tanh` approach. Real saturation is harder-kneed; if
  clipping sounds too soft, shrink `rail − softStart`.
- Measure: sine at +20…+26 dBu, scope the output stage; note the level where
  the top flattens and the shape of the corner.

### CouplingCap
10 µF into 100 k → 0.16 Hz. **S** (C16/C43 and the 100 k load). Only matters
because it turns feedthrough DC steps into decaying thumps.

### RmsDetector
Log-domain averaging: in mean-square terms a one-pole `dy/dt = (x² − y)/τ`.
Output is `10·log10(y / 0.775²)` dBu.
- `kRmsTauSec = 20 ms` **G**. dbx application notes put typical timing caps in
  the 10–50 ms range. **This is the single most important constant to measure.**
- `kPeakHoldSec = 10 ms` **G**. Leak of the C42 hold cap.
- Measure: Attack 0.1 ms, Release 50 ms, ∞:1, threshold −20. Feed a −40 → 0 dBu
  1 kHz burst and record the gain-reduction envelope (or the output envelope).
  Fit `τ` to the 63 % point in RMS mode; repeat in PEAK mode for the hold τ.
- Noise floor state resets to −90 dBu so the gate behaves on silence.

### Vca
Gain law exact in dB (2150: 6 mV/dB). Errors:
- `symmetryErr = 0.003` (2nd harmonic) **G**. 2150 THD at unity is quoted
  ~0.02–0.05 % without trim; the 3630 is factory-trimmed once.
- `oddBase = 0.010`, `oddPerDb = 0.0045` (3rd harmonic vs GR) **G**, chosen to
  give 0.03 % at 0 dB GR → ~0.12 % at 20 dB GR at +4 dBu, roughly the slope
  of the 2150 datasheet THD-vs-gain plot.
- `feedthroughVPerDb`: set from Thump, 0…3 mV/dB **G**. 2150 spec feedthrough
  is tens of mV across the full range; budget trimming makes it worse.
- Coefficients are relative to `kRef = 10 V`, so harmonic ratio ≈ coeff·(v/10)².
- Measure: 1 kHz at +4 dBu, read H2/H3 at 0, 10, 20 dB GR (use Output to keep
  the level constant at the ADC). For feedthrough: fastest attack, ∞:1, feed
  a 20 kHz tone that steps 20 dB, scope the output DC before the coupling cap
  (or just after it and back out the 0.16 Hz decay).

### GainComputer (`Sidechain.h`)
- Slope = 1 − 1/ratio.
- `kSoftWidthDb = 10` **G** (resistor-blend knee of this type is usually
  8–12 dB), `kHardWidthDb = 0.5` **G** (a diode's worth of rounding).
- Measure: static sine sweep −40…+20 dBu in 2 dB steps, both knee settings,
  ratio 4:1. Fit width.

### TimingNetwork
Diode-steered RC: `cv += diff·attackCoef` when rising past the dead band,
`releaseCoef` when falling.
- Pot ranges **D**; taper **G** (plugin uses skewed ranges, not a measured law).
- `kDeadBandDb = 0.15` **G**. Measure: slow 0.5 dB triangle modulation of a
  tone around a fixed GR; the CV steps rather than tracks.

### Gate
- Open 0.5 ms **G**, close = Rate pot **D**, hysteresis 1 dB **G**, depth 80 dB
  **G** (VCA bottoms out well below −80 dB anyway).
- Measure: pink noise just above gate threshold; count chatter rate vs level
  offset to fit hysteresis.

### Stereo link
Tied detector outputs → average of mean-squares. **S/G** — dbx detectors
summed into a shared cap behave this way; confirm the 3630 ties the cap node
rather than the log output (the latter would give a max-like law).

### Mismatch (channel B spread at Mismatch = 1)
+60 % feedthrough, +80 % symmetry error, +10 % RMS τ. **G**, chosen to be
audible on a stereo bus without being broken. Unit-to-unit spread of real
3630s is large enough that any one unit is a sample, not the truth.

## Calibration session checklist

Hardware needed: one 3630, an interface with known dBu calibration, a loopback.
Record everything at 96 kHz. For each test save `input.wav`, `output.wav`,
and the knob settings; a `tests/calibration/` folder with a small fitting
script (Python/numpy) is the intended home.

1. Level calibration: unity (1:1, +20 thr, gate off), −20…+20 dBu → confirms
   `calib` mapping and finds the rail-clip point.
2. RMS τ and peak-hold τ (burst test above).
3. Static curves, hard and soft knee, ratios 2/4/10/∞.
4. Attack/release at pot min, 10 %, 50 %, 90 %, max → fit the pot law.
5. THD vs GR at +4 dBu.
6. Feedthrough step.
7. Gate chatter vs level.
8. Link law: loud L / quiet R at several level differences.
