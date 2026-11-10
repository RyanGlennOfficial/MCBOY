# Actual validation report — McBoy v0.9.0

**Environment:** Linux x86_64, GCC 14.2, CMake 3.31, C++17. **No macOS Xcode/macOS SDK; no JUCE checkout.** Mac VST3 could not be compiled or tested here. JUCE is pinned to the published official `8.0.16-security` tag for macOS downloads; no source compatibility test of this tag was run locally.

## Executed and passed

1. `cmake -S . -B build-core -DMCBOY_BUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Release` — configured successfully.
2. `cmake --build build-core --parallel 3` — compiled standalone native C++ core smoke, randomized audio stress, per-knob uniqueness test, and demo WAV renderer.
3. `ctest --test-dir build-core --output-on-failure` — **3/3 passed**, including a full 100/100 non-neutral behavior check. CoreSmoke: ~0.6 s. AudioStress: ~2.2 s. All100Uniqueness: ~0.8 s on this Linux host (timings are not DAW realtime CPU percentages).
4. `build-core/Tests/mcboy_uniqueness docs/All100Measured.csv` — all hundred independently non-neutral on a 48 kHz 0.5-second original multitone/transient stereo fixture. Pairwise closest RMS difference **0.00188064** (non-identical). This is a fixture-specific result, not proof of perceptual uniqueness or correct named behavior.
5. `build-core/Tests/mcboy_render_demo Examples/McBoy` — generated five 3-second 48 kHz stereo PCM16 original synthesized audio fixtures: dry plus four named presets. Measured processed digital sample peaks all below 0.892FS.
6. JSON verification — exactly **17** JSON presets, each with **100** explicit raw values and **100** explicit lock values.
7. AddressSanitizer + UndefinedBehaviorSanitizer **CoreSmoke passed**, no reported findings under the provided sample fixtures. The full 200-pass sanitizer stress run did **not** complete within available time, but a reduced **12-pass sanitizer stress test** compiled and passed at 44.1/48/88.2/96/192 kHz. The regular non-sanitized 200-pass suite passed.

## What remains unverified and what is incomplete

- **macOS VST3 and native Standalone GUI:** no binary produced and no macOS compilation executed. macOS UI/host automation and JUCE wrapper tests were **NOT run**. The written CMake Xcode project and build script are only a reproducible build attempt.
- **Acoustic fidelity:** all 100 effects measurably transform audio, but some are intentionally inexpensive approximations. fx060 now has a real low-resolution 256-point STFT magnitude-blur wet branch; numerous other effects retain approximations. See `docs/AlgorithmNotes.md` for explicit per-bank gaps.
- **Release safety/quality:** true-peak limiting, oversampled nonlinear alias suppression, exact sample-position transport sync, CPU profiling, DAW project-state roundtrips and listening-based click audits remain incomplete. Guard is a digital sample-peak bound only and does not constrain pure bypass.
- **Mac architectures:** neither arm64 nor x86_64 macOS VST3 runtime has been tested. An installed VST3 must first be built with the provided script on a Mac.
- **Pro Tools:** VST3 cannot be loaded directly without a compatible AAX VST3 host or an independent AAX version; no such host was tested.

Do not advertise this as a signed/notarized/DAW-validated commercial release. It is a complete 100-control functional *DSP implementation* and an unvalidated JUCE/VST3 frontend source package.
