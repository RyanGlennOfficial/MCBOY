# McBoy: reproducible macOS build and host QA

## Build target

- Format: native JUCE 8/C++17 64-bit VST3 audio effect, plus optional JUCE Standalone host.
- macOS native targets: `arm64`, `x86_64`; the default script requests one **universal binary** containing both slices.
- Generator: Xcode, managed by CMake. Xcode project is generated locally, not stored as a brittle checked-in `.xcodeproj`.
- Minimum deployment target requested: macOS 11.0. Not independently verified on a real Mac or DAW.
- Source-level JUCE version: pinned `8.0.16-security` in the build script / FetchContent setting. The checkout is obtained locally and is **not included** in this archive; an existing JUCE checkout may be supplied via `JUCE_DIR`.
- Development identity: `HWMG` / `MCBY` (**provisional; verify uniqueness before publication**).

## One-command build on a Mac

Install the full Xcode app and CMake. On a Mac with Homebrew, `brew install cmake` supplies the CLI. Make sure Xcode is selected and its license accepted (`xcode-select -p`, `xcodebuild -version`). Extract the archive, open Terminal, enter the `McBoy` folder, then:

```bash
bash scripts/build_macos.sh universal
```

This does the following:

1. Checks for Xcode / macOS SDK / CMake / Git.
2. Downloads pinned JUCE if `JUCE_DIR` was not supplied (internet required).
3. Generates `build-mac/McBoy.xcodeproj` with `-G Xcode` and `CMAKE_OSX_ARCHITECTURES=arm64;x86_64`.
4. Compiles actual `McBoyPlugin_VST3`, `McBoyPlugin_Standalone`, and JUCE/core test targets.
5. Runs CTest (core audio tests + JUCE parameter/state smoke test).
6. Ad-hoc signs the plugin for local development; verifies two architectures with `lipo`; creates `dist/McBoy-macOS-universal.vst3.zip` with SHA-256 checksum.

**A successful build and passing tests have to be observed on your Mac before the resulting plugin can be called macOS-compiled.** Ad-hoc signing is not Developer ID distribution signing or notarization. macOS may restrict distributing this unsigned/notarization-free bundle to other computers.

If the universal build fails for SDK/toolchain reasons, troubleshoot rather than assuming it succeeded. To explicitly build only for your Mac's current architecture:

```bash
bash scripts/build_macos.sh native
```

For an already-installed JUCE copy:

```bash
JUCE_DIR=/Users/you/dev/JUCE bash scripts/build_macos.sh universal
```

The generated Xcode project can be opened with:

```bash
open build-mac/McBoy.xcodeproj
```

## Install manually (after successful build)

```bash
bash scripts/install_macos.sh
```

This refuses to overwrite an existing McBoy plug-in. It copies the built bundle to your account's `~/Library/Audio/Plug-Ins/VST3/`. Restart your VST3-capable DAW and rescan. **Pro Tools itself only supports AAX natively**, so this VST3 cannot be inserted directly in Pro Tools; an independently validated VST3-to-AAX host is required, or an AAX port.

## What can and cannot be verified by CTest

`CoreSmoke` and `AudioStress` exercise audio processing without JUCE. `JuceProcessorSmoke` is the extra macOS-targeted executable, verifying JUCE parameter exposure (100 fx + 6 utility), simulated automation gestures, DSP process calls, seeded randomization, locked behavior, Undo/Redo, host-state serialization/reload, factory preset selection, and reset. These are real programmatic checks **if compiled and executed successfully**; they do *not* prove that any particular DAW accepts the plug-in.

For host validation, run the following after installing a **successfully built** VST3:

- Scan/load in a VST3 host, insert on mono and stereo tracks, play audio and bypass/re-enable repeatedly.
- Confirm all 100 parameter IDs and utility parameters appear; verify automation of `fx001`, `degree`, `mix`, `output`, `guard`, `bypass`, `labels`, and `fx100`. The last has an implemented audio effect in the standalone DSP core.
- Save a host project containing multiple McBoy instances, locks, seed, and a preset; close the host and reopen; compare values.
- Randomize rapidly while playing 44.1/48/96/192-kHz audio in 1/32/512/2048-sample buffers; watch for jumps, excessive peaks, CPU load, and crashes.
- Render/bounce both real-time and offline. Compare recovered parameter state and rendered waveforms; run native arm64 and Intel x86_64 hosts when actually available.
- Test actual accessibility, UI resizing (75%-150% equivalent), host keyboard shortcuts, pluginval if installed, and real musical input with monitoring at a safe level.

## Current blockers and disclosure

The reference environment used to produce this source is **Linux x86_64 with no Xcode, macOS SDK, or JUCE checkout**. The VST3 bundle, JUCE wrapper compile, Xcode GUI, host scanning, plug-in validation, audio listening and cross-architecture builds are **not verified** here. All 100 numbered controls now have distinct non-neutral DSP implementations in the core, but acoustic fidelity and the VST3 wrapper are not validated on macOS. `JuceProcessorSmoke` exists as source but must not be counted passed before a Mac executes it.
