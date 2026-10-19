# Handoff — state of the project

_Last updated: 2026-10-14 (second session, on the Mac). Update this file at
the end of every session._

## Where things stand

Version 0.4. Built, validated and rendered on macOS; CI green on both
platforms since 0.3. The repo lives at `~/Documents/JUCE/dirty630` on
Daniel's Mac. The listening pass happened: "sounds about right". The Windows
installer/plugin has not been tried on a real Windows machine yet.

| Item | State |
|---|---|
| DSP model (`src/dsp/`) | 30 automated checks pass at 96 kHz, on AppleClang too |
| JUCE processor | One set of controls for both channels; detector = mono sum of input (or of ext SC); stereo + optional sidechain bus, 1×/2×/4× oversampling, APVTS state |
| Editor | One strip + DIRT row, 980×300; hardware-style black fascia, brand header, LED ladder; build stamp under the header |
| macOS build | Universal VST3/AU/Standalone build clean (Xcode 26.3); one `-Wshadow-field` fixed |
| `auval` | **Passed** (`aufx D630 Minu`) |
| pluginval | **Passed** at strictness 5 (VST3) |
| Listening test | **Not done** |
| Windows build | Not possible locally (macOS); plan is a GitHub Actions workflow |
| Calibration against a real 3630 | **Not done** — all "educated guess" constants are listed in `MODEL.md` |

## Releasing

`v0.6.0` is the first tagged release:
<https://github.com/minusro/dirty630/releases/tag/v0.6.0>. Pushing a `v*`
tag builds both platforms and attaches the two installers to a Release;
pushes to main still only produce run artifacts. `gh` is installed and
authenticated here, so `gh run watch <id>` and `gh release view <tag>` work
directly. The committed `Plugins/` folder of 0.4.0 zips was removed in
favour of Releases — do not add binaries back to the repo.

## Changed in 0.6

- **Wordmark redrawn** to match the real logo (`docs/design/`): rounded, not
  chamfered, leaning forward ~11°, initial at full cap height and the rest at
  80%. Built as centre lines stroked with round caps/joins rather than filled
  outlines. Watch the stroke weight: at the first attempt (3.0 on 72%-height
  letters) the S counters filled in and it read "MINU8".
- **CLEAN ↔ BYPASS swapped.** CLEAN (was MODDED) joins PEAK/SOFT in the
  switch column; BYPASS moves to the global row under EXT SC. The parameter
  id stays `"modded"` — ids are saved state — so `ids::clean` deliberately
  holds the old string, and the DSP still calls the flag `modded` because it
  names the hardware change rather than its effect.

## Changed in 0.5.2

- **Product name is `DIRTY630`**, so the bundles are `DIRTY630.vst3` /
  `.component` / `.app`. The CMake target, repo, installer filenames and
  bundle ID stay lowercase `dirty630` (bundle IDs are conventionally
  lower-case, and it is not part of plugin identity). Installer scripts, CI
  paths and docs follow the new bundle names; delete any old lowercase
  bundle from `~/Library/Audio/Plug-Ins/` or you will have both.
- **Install steps documented** in the root `README.md` for the unsigned builds
  (Gatekeeper "Open Anyway", SmartScreen "Run anyway", and the `xattr`
  quarantine strip for hand-placed bundles). Signing is deliberately
  deferred — see `docs/BUILD.md`.
- README's Controls section was still describing per-channel A/B strips and
  the Link switch, both gone since 0.2; corrected, and Modded/Mix added.

## Changed in 0.5 (this session)

- **Company is "minus"** (was minusro): `COMPANY_NAME`, bundle/package IDs
  (`com.minus.dirty630`), Inno publisher, header byline, and the
  manufacturer code `Mnro` → **`Minu`** (FourCC, so "Minus" truncates). That
  code feeds the AU registration *and* JUCE's VST3 UID, so 0.5.1 is a
  different plugin to any host that saw an earlier build — sessions made
  with one would show it missing. Fine while nothing has shipped; do not
  change it again after release. The GitHub account and clone URL are still
  `minusro`.
- **Brand wordmark** drawn as vector letterforms (`minusWordmark()` in
  `PluginEditor.cpp`): squared geometric caps, 3-unit stroke on a 14-unit
  cap height, 45° chamfers at top-left and bottom-right, in the panel
  logo's cyan.
- **PEAK defaults on**, including in the Default preset.
- **Front-panel look** (`docs/design/panel-reference.png`): black brushed
  fascia with horizontal grain, white silkscreen, black knurled knob caps
  with a white pointer, square latching switches that light red, and round
  green/amber/red LEDs under a "GAIN REDUCTION dB" caption. The header is a
  40 px band with rack screws, a minus brand tile, "DIRTY630 COMPRESSOR" in
  medium and "RMS/PEAK STEREO COMPRESSOR LIMITER WITH GATE" in light, both
  letter-spaced; the build stamp moved underneath it.

## Changed in 0.4 (previous session)

- **Modded toggle** (under EXT SC): THAT 2180 + op-amp swap — symmetry,
  odd harmonics, feedthrough and noise ×0.03, rails 14.5 V (`MODEL.md`,
  "Modded"). New `modded` param; test checks unity THD drops ~7x and the
  gain law is untouched.
- **Input/Output smoothing**: 10 ms one-pole `Smoothed` inside the model, no
  more zipper under automation.
- **Oversampling**: all three factors pre-built in `prepareToPlay`; factor
  switch is now allocation-free. Default is **4x**.
- **GR meter**: block-max value + instant-attack/~40 dB/s-release ballistics.
- **Presets**: Default / Drum Bus Smash / NIN Bass / Snare Gate as host
  programs; they only touch the strip controls and mix.

## Changed in 0.3 (previous session)

- **Dry/wet.** `mix` parameter (0–100 %, default 100) blended by
  `juce::dsp::DryWetMixer` outside the circuit model, dry path delayed by the
  oversampling latency. Big MIX knob bottom-right, spanning both editor rows.
- **Installers.** CI now also builds a macOS `.pkg` (`pkgbuild`, installs to
  `/Library/Audio/Plug-Ins/VST3`) and a Windows Inno Setup exe (installs to
  `C:\Program Files\Common Files\VST3`), uploaded as artifacts alongside the
  raw VST3s (`docs/BUILD.md`, "Installers"). Unsigned.
- Small-knob labels raised 4 px so their base sits at the same distance from
  the 12-o'clock tick as on the big knobs. auval + pluginval re-passed.

## Changed in 0.2 (this session)

- **Single control set.** The two per-channel strips and the Link switch are
  gone; one row drives both VCAs. Parameter IDs keep the `A` suffix; the `B`
  set and `link` were deleted (nothing had shipped, so no state to preserve).
- **Mono-sum detector.** Both RMS sensors get the mono sum of the stereo
  programme, or of the external sidechain when EXT SC is on (`MODEL.md`,
  "Shared detection"). Mismatch still spreads the two channels.
- **Version/build stamp** in the header; policy in `CLAUDE.md`
  (`cmake/BuildInfo.cmake` regenerates `BuildInfo.h` every build).
- Editor bottom row was clipped by the old 430 px window; the single-strip
  layout is 300 px and everything fits.

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
- Link: loud L / quiet R → identical 10.4 dB GR on both (mono-sum detector).

## Immediate next steps (in order)

1. Open the Standalone, feed a drum loop, and sanity-listen: does Thump do
   anything audible at 0.1 ms attack? Does the gate chatter? Is the GR ladder
   sensible?
2. Windows build via GitHub Actions (no local Windows toolchain).
3. Then go to `docs/ROADMAP.md`.

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
- **Always linked, one control set** (0.2, Daniel's call). Detection runs on
  the mono sum; there is no dual-mono mode. The hardware's per-channel strips
  are not coming back without a new decision.

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
- `dirty630_tests` prints to stdout rather than using a test framework. Fine
  for now; don't pull in Catch2 unless it earns it.
- The Standalone starts with "audio input muted to avoid feedback"; remember
  to enable input (or play a file through it in a DAW) for the listening pass.
