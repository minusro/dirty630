# Roadmap

Ordered by value. Tick items off here and note the commit.

## 1. Make it real
- [x] Build on macOS, fix any AppleClang warnings-as-errors (0.2, 2026-10-14)
- [x] `auval` and pluginval (strictness 5) clean (0.2, 2026-10-14)
- [ ] Listening pass in the Standalone and in a DAW; note anything that sounds wrong in `HANDOFF.md`
- [x] Screenshot the editor; fix layout overlaps (0.2 — bottom row was clipped; editor now single-strip 980×300)
- [x] Windows VST3 + installers via GitHub Actions (CI green since 0.3; artifacts not yet tried on a Windows machine)

## 2. Correctness polish
- [x] Smoothing on Input trim and Output (0.4 — `Smoothed` in the model, 10 ms)
- [x] Oversampling-factor switch allocation-free (0.4 — all three pre-built, pointer swap)
- [ ] Denormal check at low levels with the coupling cap (`ScopedNoDenormals` is on; verify on Intel)
- [ ] Mono-in / stereo-out and sidechain-mono layouts tested in Logic and Live
- [ ] Latency reporting sanity: compare 1×/2×/4× phase against a dry track

## 3. Calibrate against hardware
Follow the checklist at the bottom of `MODEL.md`. Add `tests/calibration/`
with recorded WAVs (or a pointer to where they live) and a fitting script.
Priority order: RMS τ → static curves → THD vs GR → feedthrough → gate.

## 4. Features worth having
- [x] GR meter driven by block max + ballistics (0.4)
- [ ] Input/output level LEDs (the hardware has a −∞/+ clip style pair)
- [x] Preset set: Default / Drum Bus Smash / NIN Bass / Snare Gate (0.4)
- [ ] Resizable editor (scale factor from APVTS state)
- [x] "Modded" toggle (0.4): THAT 2180 + op-amp swap, core errors ×0.03, rail 14.5 V

## 5. Later / maybe
- [ ] CLAP format (clap-juce-extensions)
- [ ] Headless CLI that runs the model over a WAV for offline A/B against hardware captures
- [ ] Null-test harness: given input + hardware output WAVs, report residual in dB per band

## Explicitly out of scope
- Neural / black-box modelling. See `HANDOFF.md` decisions.
- Emulating the specific unit-to-unit failures (dead gates, oscillating sidechains). "Mismatch" is as far as that goes.
