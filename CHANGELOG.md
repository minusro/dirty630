# Changelog

Notable changes per version, newest first. Versions come from
`project(dirty630 VERSION …)` in `CMakeLists.txt` and are stamped into the
plugin header at build time; the policy for bumping them is in `CLAUDE.md`.

## Unreleased

## 0.6.0 — 2026-10-19

First tagged release. Installers are attached to it; earlier versions were
only ever built in CI.

- **CLEAN and BYPASS swap places.** CLEAN joins PEAK and SOFT in the switch
  column; BYPASS moves to the global row beside EXT SC.
- **MODDED is renamed CLEAN.** The saved parameter id is unchanged, so
  existing sessions keep their setting.
- Wordmark redrawn with rounded terminals, a forward lean and an oversized
  initial, after the hardware's own logo.

## 0.5.2 — 2026-10-19

- **The plugin is now named `DIRTY630`** and the bundles are
  `DIRTY630.vst3` / `.component` / `.app`. On macOS a lower-case
  `dirty630.vst3` from an earlier build survives alongside the new one
  rather than being replaced — delete it, or the host lists the plugin
  twice.
- Install instructions written for the unsigned builds.

## 0.5.1 — 2026-10-19

- **Manufacturer code changed from `Mnro` to `Minu`.** This is part of both
  the AU registration and the VST3 identifier, so hosts treat this build as
  a different plugin from any earlier one: a session saved against 0.5.0 or
  before will report DIRTY630 as missing. Nothing had been released, so this
  is a one-time break; the code must not move again.
- Brand wordmark drawn as vector letterforms rather than a placeholder.

## 0.5.0 — 2026-10-19

- Editor redrawn after the hardware front panel: black brushed fascia, white
  legends, knurled knob caps, latching switches, and a green/amber/red LED
  gain-reduction ladder.
- **PEAK now defaults on**, in the parameter layout and in the Default
  preset.
- Company name is `minus`; bundle and package identifiers follow
  (`com.minus.dirty630`).

## 0.4.0 — 2026-10-14

- **MODDED switch** (CLEAN from 0.6): models the THAT 2180 and op-amp swap,
  cutting distortion, control feedthrough and noise to a few per cent of
  stock and raising the rails.
- **Four presets**: Default, Drum Bus Smash, NIN Bass, Snare Gate.
- Gain-reduction meter now shows each block's peak with proper ballistics,
  so fast transients register instead of flickering.
- **Oversampling defaults to 4×**, and changing the factor no longer
  allocates on the audio thread.
- Input trim and Output are smoothed, so automating them no longer zippers.

## 0.3.0 — 2026-10-14

- **Mix control** for parallel compression, with the dry path delayed to
  stay aligned with the oversampled wet path.
- macOS `.pkg` and Windows Inno Setup installers, built in CI.

## 0.2.0 — 2026-10-14

- **One set of controls drives both channels**, replacing the two
  independent strips and the LINK switch. Detection runs on the mono sum of
  the programme, or of the sidechain when EXT SC is on.
- First macOS build: universal VST3, AU and Standalone, passing `auval` and
  pluginval at strictness 5.
- Plugin header shows the version and build time.

## 0.1.0 — 2026-10-14

- Initial white-box model of the hardware: dbx-style log-domain RMS sensor,
  2150-class VCA with symmetry and feedthrough errors, diode-steered
  attack/release network, resistor-blend soft knee, hysteretic gate, and
  op-amp stages that saturate at the rails. Imperfections exposed as the
  DIRT controls.
- Built and tested on Linux only; never run in a host.
