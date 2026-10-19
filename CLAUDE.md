# dirty630 — Claude Code guide

White-box model of the Alesis 3630 stereo VCA compressor/gate as a JUCE plugin
(VST3 + AU + Standalone on macOS). Start with `docs/HANDOFF.md`, then the
file that matches your task:

| Doc | Read when |
|---|---|
| `docs/HANDOFF.md` | Starting any session: current state, what is verified, what is not |
| `docs/ARCHITECTURE.md` | Touching `src/` — signal flow, file map, threading, parameters |
| `docs/MODEL.md` | Changing DSP — every constant, where it came from, how to measure it |
| `docs/BUILD.md` | Build/test/validate on macOS or Linux, known toolchain quirks |
| `docs/ROADMAP.md` | Picking the next task |

## Ground rules

- The DSP in `src/dsp/` is plain C++17 with **no JUCE dependency**. Keep it that
  way: it is what `tests/dsp_tests.cpp` compiles against and what makes the
  model reusable.
- Every DSP quantity is in volts at the circuit node, levels in dBu, gain
  reduction in positive dB. Do not introduce normalised or "0..1" units inside
  the model.
- A DSP change is not done until `dirty630_tests` passes. Add a check when you
  add behaviour; loosen a tolerance only with a comment saying why.
- No allocation, locking or logging in `processBlock`. Buffers are sized in
  `prepareToPlay`; see the "hosts occasionally hand over more" branch for the
  one permitted exception.
- Parameter IDs (`ids::` in `PluginProcessor.h`) are part of saved state. Add
  new ones, never rename.
- Commit messages: imperative subject, body explains the *model* reasoning
  (which component, which datasheet figure) not just the code change.
- Licence is GPLv3 (matches JUCE's GPL option). Don't add non-GPL-compatible
  dependencies.
- Versioning: the version comes from `project(... VERSION)` in `CMakeLists.txt`
  and is shown in the editor header next to the build time (stamped by
  `cmake/BuildInfo.cmake` on every build; a .0 patch digit is hidden). Started
  at 0.1. Small fixes and tweaks bump the patch (m.n → m.n.1 → m.n.2); a batch
  of real changes bumps the minor (m.n → m.(n+1).0); a major change jumps to
  the next whole `(n+1).0.0`. Record the change in `CHANGELOG.md`, written for
  someone using the plugin rather than reading the diff — and say so plainly
  when an upgrade breaks their saved sessions.

## Quick commands

```sh
# DSP tests, no JUCE needed
g++ -std=c++17 -O2 -I src tests/dsp_tests.cpp -o /tmp/dsp_tests && /tmp/dsp_tests

# Full build (macOS)
cmake -B build -G Xcode && cmake --build build --config Release
auval -v aufx D630 Minu
```
