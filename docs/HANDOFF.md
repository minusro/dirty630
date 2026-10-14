# Handoff — state of the project

_Last updated: 2026-10-14. Update this file at the end of every session._

## Where things stand

The whole plugin exists and compiles. It has **not yet been built or heard on
macOS**; everything so far was done in a Linux cloud workspace.

| Item | State |
|---|---|
| DSP model (`src/dsp/`) | Written from the circuit; 30 automated checks pass at 96 kHz |
| JUCE processor | Done: stereo + optional sidechain bus, 1×/2×/4× oversampling, APVTS state |
| Editor | Done: two channel strips, 12-segment GR ladders, DIRT section |
| Linux VST3 build | Compiles clean (JUCE 8.0.4, GCC 13) with X11 extensions disabled |
| macOS build | **Untested.** CMake is set up for universal VST3/AU/Standalone |
| `auval` / pluginval | **Not run** |
| Listening test | **Not done** |
| Calibration against a real 3630 | **Not done** — all "educated guess" constants are listed in `MODEL.md` |

## What was verified, numerically

From `tests/dsp_tests.cpp` (run `./dirty630_tests` to see the table):

- Static curve 4:1 at −20 dBu threshold: within 0.15 dB of ideal from −40 to +10 dBu.
- Soft knee: compression begins ~5 dB below threshold, 0.79 dB at threshold.
- ∞:1: 15 dB GR for +15 dB over.
- Attack pot 10 ms measures 16.7 ms to 63 % (pot + 20 ms RMS sensor); release 300 ms → 346 ms.
- Peak mode leads RMS mode by ~1.7 dB after 5 ms on a 100 Hz burst.
- Gate: 0 dB open, 80 dB closed.
- Unity path (1:1, +20 threshold): 0.00 dB level error, 0.017 % THD at 0 dBu.
- THD at +4 dBu: 0.028 % at 0 dB GR → 0.123 % at 20 dB GR.
- +24 dBu in: output pinned at 22.8 dBu, 11 % THD (rail clipping).
- Thump: fast 20 dB gain step produces a −44 dBFS DC transient at Thump = 1.
- Link: loud L / quiet R → identical 12.6 dB GR on both.

## Immediate next steps (in order)

1. **Build on a Mac** (`docs/BUILD.md`). Expect possible warnings-as-errors
   from `juce_recommended_warning_flags` under AppleClang that GCC didn't flag;
   fix in source, don't disable the flags.
2. Run `auval -v aufx D630 Mnro` and pluginval (strictness 5+) on the VST3.
3. Open the Standalone, feed a drum loop, and sanity-listen: does Thump do
   anything audible at 0.1 ms attack? Does the gate chatter? Is the GR ladder
   sensible?
4. Then go to `docs/ROADMAP.md`.

## Decisions already taken (don't re-litigate without reason)

- **White-box over black-box.** Release times run to seconds and the knobs
  interact, which neural/LSTM models handle badly; the circuit is simple
  enough to model directly.
- **Grey-box calibration later.** Constants are placed from datasheets; a
  measurement session on a real unit refines them (`MODEL.md` has the list).
- **DSP has no JUCE dependency.** So tests run anywhere and the model could go
  into a non-JUCE host later.
- **Audio in volts, levels in dBu.** Scope readings drop straight into the code.
- **Linux build disables XRandR/Xinerama/Xcursor/XShm/ALSA/JACK** purely so
  the cloud workspace (no apt access) can compile. Harmless on macOS.
- **Imperfections are user-controllable** (Thump / Noise / Mismatch) rather
  than baked in. Default 0.5 each.
- **Licence GPLv3**, which the repo already had and which works with JUCE's
  GPL licence.

## Known gaps / things that look wrong

- `GrMeter` ladder steps hard-coded to 1-2-3-4-5-6-8-10-12-15-20-30 dB from
  memory of the front panel; verify against a photo.
- Attack/release/gate pot ranges are from the published spec sheet, not from
  measuring the pot law. The 3630's pots are probably log-taper; the plugin
  uses a 0.3–0.4 skew which is only approximately that.
- No parameter smoothing on Output/Input trim; zipper noise possible when
  automated. Cheap to add with `juce::SmoothedValue`.
- Sidechain is used at host rate inside the oversampled loop (zero-order
  hold). Fine for the 20 ms detector, but worth a comment if anyone profiles.
- The editor was never rendered. Layout numbers in `resized()` are untested.
- `dirty630_tests` prints to stdout rather than using a test framework. Fine
  for now; don't pull in Catch2 unless it earns it.
