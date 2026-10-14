# Build, test, validate

## macOS (target platform)

Requirements: Xcode command line tools, CMake ≥ 3.22. JUCE 8.0.4 is fetched on
first configure unless you point at a checkout.

```sh
cmake -B build -G Xcode                      # or -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Produces universal (arm64 + x86_64, deployment target 11.0):

- `build/dirty630_artefacts/Release/VST3/dirty630.vst3` → copied to `~/Library/Audio/Plug-Ins/VST3`
- `.../AU/dirty630.component` → `~/Library/Audio/Plug-Ins/Components`
- `.../Standalone/dirty630.app`

Validate:

```sh
auval -v aufx D630 Mnro            # AU
pluginval --strictness-level 5 --validate build/dirty630_artefacts/Release/VST3/dirty630.vst3
```

If Logic doesn't list the AU after a rebuild: `killall -9 AudioComponentRegistrar`
and reopen, or bump `VERSION` in `CMakeLists.txt`.

Local JUCE: `-DDIRTY630_JUCE_DIR=/path/to/JUCE`.
Plugin off, tests only: `-DDIRTY630_BUILD_PLUGIN=OFF`.

## Installers

CI (`.github/workflows/build.yml`) builds both plugins and both installers on
every push to main and uploads them as artifacts:

- macOS: `installer/macos/make_pkg.sh` wraps the VST3 in a `.pkg`
  (`pkgbuild`) installing to `/Library/Audio/Plug-Ins/VST3`. Works locally:
  `sh installer/macos/make_pkg.sh build/dirty630_artefacts/Release/VST3/dirty630.vst3 <ver> dist/dirty630-<ver>-macos.pkg`
- Windows: `installer/windows/dirty630.iss` (Inno Setup, preinstalled on the
  GitHub runner) installs the bundle to `C:\Program Files\Common Files\VST3`.

Neither is code-signed yet; macOS users need right-click → Open on the pkg.

## DSP tests (any platform, no JUCE)

```sh
g++ -std=c++17 -O2 -Wall -Wextra -I src tests/dsp_tests.cpp -o /tmp/dsp_tests && /tmp/dsp_tests
# or after a CMake build:
./build/dirty630_tests        # (build/Release/dirty630_tests with the Xcode generator)
ctest --test-dir build
```

Prints a table per test and ends with `ALL PASSED` or a failure count. Runs in
a few seconds (it simulates ~15 s of audio at 96 kHz).

## Linux (sanity build only)

The Linux VST3 compiles but hasn't been run in a host. The cloud workspace it
was built in has no apt access, so the CMake file disables the optional X11
extensions and audio backends for Linux:

```
JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0 JUCE_USE_XSHM=0 JUCE_ALSA=0 JUCE_JACK=0
```

`juceaide` (JUCE's build helper) is configured in a sub-process that ignores
`CMAKE_CXX_FLAGS`, so on a machine without `Xrandr.h` you also need:

```sh
CXXFLAGS="-DJUCE_USE_XRANDR=0 -DJUCE_USE_XINERAMA=0 -DJUCE_USE_XCURSOR=0 -DJUCE_USE_XSHM=0" \
  cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
```

On a normal Linux box with `libasound2-dev libxrandr-dev libxinerama-dev
libxcursor-dev libfreetype-dev libfontconfig1-dev` installed none of this is
needed; feel free to drop the defines once nobody builds in that sandbox.

## Toolchain notes

- GCC 13 with `juce_recommended_warning_flags`: clean apart from JUCE's own
  unused-parameter warnings in `juce_XWindowSystem_linux.cpp`.
- AppleClang may flag things GCC didn't (e.g. `-Wshadow`, implicit float
  conversions in `PluginEditor.cpp`). Fix the source rather than relaxing flags.
- The `≈ 10 min` first configure on macOS is JUCE being cloned; subsequent
  configures are seconds.
