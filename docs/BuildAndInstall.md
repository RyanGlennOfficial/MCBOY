# Reproducible builds and installation

## Prerequisites

- CMake 3.22+ and a C++17 toolchain.
- JUCE 8 (prefer `8.0.16-security`) checkout or CMake package (not included; check licensing before distribution).
- Xcode/macOS for an Apple Silicon or Intel `.vst3`; Visual Studio 2022+ for Windows x64. Linux builds are possible with system libraries required by JUCE but were **not** attempted for the plugin wrapper in this environment.

## DSP-only tests (no JUCE needed)

```sh
cmake -S . -B build-core -DMCBOY_BUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-core --parallel
ctest --test-dir build-core --output-on-failure
```

## macOS, universal release (recommended)

```sh
bash scripts/build_macos.sh universal
```

This clones pinned JUCE if needed, generates an Xcode project, compiles a VST3 plus Standalone application, runs core/JUCE smoke tests, verifies the universal architectures, and produces a ZIP **only on a successful macOS build**. It cannot be run on Linux. The generated Xcode project can be opened in the GUI; the bundle must be separately installed with `bash scripts/install_macos.sh`.

See [MacBuildAndHostQA.md](MacBuildAndHostQA.md) for validation caveats, CI workflow, and host tests.

## macOS, manual Xcode build

```sh
cmake -S . -B build-mac -G Xcode -DJUCE_DIR=/absolute/path/to/JUCE -DMCBOY_BUILD_PLUGIN=ON
cmake --build build-mac --config Release --parallel
```

The included build script uses `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` and fails if either expected architecture is missing; use `native` to explicitly request a single architecture. Check signatures, hardened runtime/notarization and the real host architectures before packaging.

## Windows x64

```powershell
cmake -S . -B build-win -G "Visual Studio 17 2022" -A x64 -DJUCE_DIR=C:/path/to/JUCE
cmake --build build-win --config Release --parallel
```

## Alternative JUCE acquisition

If internet access is permitted, omit `JUCE_DIR` and configure with `-DMCBOY_FETCH_JUCE=ON` to fetch the `8.0.16-security` tag. For long-term reproducibility, pin a reviewed JUCE version and mirror it in an approved dependency repository.

## Locate VST3 output

JUCE normally writes the compiled bundle beneath the plugin target's artifacts directory, typically `build*/McBoyPlugin_artefacts/<configuration>/VST3/McBoy.vst3`. Inspect actual CMake output before assuming the path; this handoff has **not produced a VST3 binary**. JUCE also builds a `Standalone/McBoy` executable under its artifacts directory.

Standard local plugin folders:

- macOS: `~/Library/Audio/Plug-Ins/VST3` or `/Library/Audio/Plug-Ins/VST3`
- Windows: `C:\Program Files\Common Files\VST3`

Copy only your **successfully built**, appropriately signed binary bundle; rescan in a VST3 host. Keep a copy of your instrument/project state when upgrading because the state schema is only v1.

## Pro Tools warning

**Pro Tools does not natively load VST3 audio-effect plugins.** McBoy VST3 would require a compatible AAX plugin that hosts VST3 (for example, a verified current bridge/host, if supported). Alternatively produce a proper AAX version using the licensed AAX SDK and conduct host certification tests. No Pro Tools, AAX host, macOS, Windows or bridge installation test has occurred for this package; do not claim Pro Tools compatibility from the CMake target alone.

## Troubleshooting

If configure fails with `JUCE not found`, supply a valid `-DJUCE_DIR=/absolute/path/to/JUCE` or configure core-only tests. If builds fail with macOS SDK/Visual Studio errors, check configured compiler/SDK and JUCE version. Build plugin sources on a desktop workstation with appropriate graphics/audio SDKs rather than assuming the Linux core tests validate JUCE UI compatibility.
