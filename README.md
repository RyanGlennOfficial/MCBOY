# McBoy — 100-knob experimental audio multi-effect

**Version 0.9.1 — corrected master wet/dry routing.**
**macOS binaries are built by GitHub Actions.** See the most recent successful Actions artifact and build logs; DAW sound-quality/host compatibility still need hands-on confirmation.

McBoy is an original 100-knob experimental effects console for music production. Ten banks of ten rotary controls surround a giant **MCBOY!** shuffle button and an independent **DEGREE** intensity ceiling. Labels are OFF by default; Option-/right-click locks a knob. A/B storage and swapping, Undo/Redo, factory preset selection, copy/paste state, true parameter automation, and randomization controls are part of the written JUCE wrapper.

## 0.9.1 mix bug fix

In prior versions the internal wet return was calculated as `(input + effects) / (1 + effectWeights)`, which injected dry audio even at 100% MIX and attenuated the effect returns. The corrected build normalizes the effect-only returns first, then applies the master MIX. A short wet-bus activation ramp preserves click-free neutral behavior at DEGREE=0. The regression test with a solo slap-delay impulse now verifies that the original transient cannot appear at 100% MIX while the delayed transient remains audible.

## What's implemented

- **100 separately mapped DSP behaviors**, all enabled in the effect map, with stable `fx001`–`fx100` parameter IDs. Twenty-nine implementations existed in the 0.2 core; the remaining 71 original behaviors were added in the 0.9 core. Effects are functional audio-transforming approximations, **not guaranteed high-fidelity models of all named studio effects**. See `docs/AlgorithmNotes.md` for specific limitations.
- Fully implemented C++17 allocation-free audio callback core (`Source/Core/Engine.h` and `Source/Core/ExtraEffects.h`) with 10 effect banks on a normalized parallel wet bus, 28 ms parameter ramps, deterministic seed-based procedural effects, and a capped sample-peak guard. Pure bypass intentionally does not cap the original signal.
- Written JUCE VST3 + standalone scaffolding, APVTS with 100 effect knobs + 6 utilities (`degree`, `mix`, `output`, `guard`, `bypass`, `labels`), scalable original editor showing all 100 controls, seeded shuffle, locks, 20-level Undo/Redo, stored A/B comparison slot, factory presets and state serialization.
- 17 fully specified original factory presets (one neutral + 16 musical choices). JSON mirrors in `Presets/` and corresponding embedded C++ definitions in `Source/Presets.h`.
- CMake/Xcode universal macOS build script with CMakePresets.json, VST3 installation script, and GitHub Actions macOS CI workflow for optional use in an authorized GitHub repository.
- Built and executed three independent DSP tests on Linux: neutral/smoke/guard, randomized stress 44.1–192 kHz, and 100/100 distinct output measurements. Five original offline WAV demo fixtures are included under `Examples/`.

## Build: on your Mac

Prerequisites: **macOS 11+**, a compatible version of full **Xcode**, **CMake 3.22+**, **Git**, and an internet connection to obtain **JUCE 8.0.16-security** (or set `JUCE_DIR` to a compatible local copy).

Unzip, open Terminal in the `McBoy_Full_Source` folder, and run:

```bash
bash scripts/build_macos.sh universal
```

This configures a CMake-generated **Xcode** project and attempts to build **both arm64 and x86_64**. It compiles VST3 and standalone targets, runs 4 core/JUCE test executables, checks universal binary slices, ad-hoc signs the VST3, and packages it into `dist/`. If the build fails, no successful Mac build should be claimed; resolve the reported error and rerun.

After a **successful** macOS compile and test pass:

```bash
bash scripts/install_macos.sh
```

The plug-in will install under `~/Library/Audio/Plug-Ins/VST3/McBoy.vst3`. The installer refuses to overwrite an existing McBoy plugin. For a native-only build, use `bash scripts/build_macos.sh native`. The Xcode project is generated at `build-mac/McBoy.xcodeproj`.

**Pro Tools does not directly host VST3.** For Pro Tools, use a tested AAX-compatible VST3 hosting solution or build a separately licensed AAX version. Nothing here establishes DAW compatibility until tested in a real host.

## Build and run the standalone DSP tests (JUCE not required)

```bash
cmake -S . -B build-core -DMCBOY_BUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-core --parallel
ctest --test-dir build-core --output-on-failure
build-core/Tests/mcboy_uniqueness docs/All100Measured.csv
build-core/Tests/mcboy_render_demo Examples/McBoy
```

These native compiled commands were run on Linux; the JUCE wrapper, VST3 and Mac builds were not. See `docs/ValidationReport.md` and `docs/AcceptanceMatrix.md` for evidence and outstanding verification.

## Controls

- **MCBOY!** (or `R`/Space while editor focused): randomizes all unlocked raw effect values from a deterministic seed, without adjusting DEGREE. Low DEGREE means fewer, subtler randomized controls; higher DEGREE allows richer chaos. Maximum 4 selected controls per bank per shuffle.
- **DEGREE**: effective amount `raw × degree`, with 0% completely neutral to the effects. Knob raw patch remains intact while dialing it down.
- **LABELS**: OFF by default; names show via tooltip/accessible description even when hidden.
- **Locks**: right-click or Option-click tiny dials to lock/unlock. Lock all/unlock all utilities included.
- **STORE A/B**: saves the current sound into the comparison slot. **SWAP A/B** trades it with the currently active patch. This slot is serialized in project state.
- **Mix / output / guard / bypass**: independent master controls. Guard bounds digital sample peaks in the processed path to ~-1 dBFS, not oversampled true peak. Guard OFF and bypass can exceed this level.
- **Undo / Redo**, 17 factory presets, **COPY / PASTE** plugin state are implemented in JUCE source but still require Mac compilation/host verification.

## Remaining release blockers

An unverified JUCE wrapper or DAW session cannot be passed as a real built plugin. Pending tasks: compile/test under Xcode on macOS and on Intel+Apple Silicon (if both available), host automation and state/preset recall, pluginval and offline bounce, waveform listening, CPU/heap/peak profiling, GUI accessibility/scaling, oversampling and antialiasing, high-fidelity DSP refinement for approximate pitch/reverb/filter modules, digital intersample true peak certification, and code-signing/notarization for redistribution. Manufacturer code `HWMG` and plugin code `MCBY` are provisional and must be checked before sale.

See `LICENSES.md` for third-party licensing details and the root `scripts/` for build steps.
