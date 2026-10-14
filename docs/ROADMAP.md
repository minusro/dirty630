# Roadmap

Ordered by value. Tick items off here and note the commit.

## 1. Make it real (next session)
- [ ] Build on macOS, fix any AppleClang warnings-as-errors
- [ ] `auval` and pluginval clean
- [ ] Listening pass in the Standalone and in a DAW; note anything that sounds wrong in `HANDOFF.md`
- [ ] Screenshot the editor; fix layout overlaps in `resized()`

## 2. Correctness polish
- [ ] `SmoothedValue` on Input trim and Output (zipper noise under automation)
- [ ] Move oversampling-factor rebuild off the audio thread (pre-build all three, swap a pointer)
- [ ] Denormal check at low levels with the coupling cap (`ScopedNoDenormals` is on; verify on Intel)
- [ ] Mono-in / stereo-out and sidechain-mono layouts tested in Logic and Live
- [ ] Latency reporting sanity: compare 1×/2×/4× phase against a dry track

## 3. Calibrate against hardware
Follow the checklist at the bottom of `MODEL.md`. Add `tests/calibration/`
with recorded WAVs (or a pointer to where they live) and a fitting script.
Priority order: RMS τ → static curves → THD vs GR → feedthrough → gate.

## 4. Features worth having
- [ ] GR meter driven by a smoothed peak-hold value rather than block-end sample
- [ ] Input/output level LEDs (the hardware has a −∞/+ clip style pair)
- [ ] Per-channel "B follows A" switch separate from Link (3630 Link forces A's controls; some people want mirrored knobs without tied detectors)
- [ ] Preset set: the classic "drum bus smash", "NIN bass", "gate for snare" settings
- [ ] Resizable editor (scale factor from APVTS state)
- [ ] "Modded" toggle: THAT 2180 VCA + good op-amps → symmetry error, feedthrough and noise all near zero, rail at 14.5 V. Trivial to add, worth a lot to users.

## 5. Later / maybe
- [ ] CLAP format (clap-juce-extensions)
- [ ] Headless CLI that runs the model over a WAV for offline A/B against hardware captures
- [ ] Null-test harness: given input + hardware output WAVs, report residual in dB per band
- [ ] Windows build (nothing platform-specific in the code; just untested)

## Explicitly out of scope
- Neural / black-box modelling. See `HANDOFF.md` decisions.
- Emulating the specific unit-to-unit failures (dead gates, oscillating sidechains). "Mismatch" is as far as that goes.
