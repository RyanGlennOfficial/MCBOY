# Acceptance matrix — McBoy v0.9.0

| Criterion | Tested result |
|---|---|
| 100 uniquely named controls and `fx001`-`fx100` IDs | **Source inspected; 100 present** |
| 100 effects independently non-neutral | **Linux core passed: 100/100 on stereo multitone+transient** |
| Pairwise output distinct in measurement fixture | **Linux core passed: closest pair RMS difference 0.00188064** |
| DEGREE 0% bypasses all coloration | **Linux core smoke passed, guard OFF and output 0 dB** |
| Digital sample peak bound with guard on | **Linux core smoke passed high-amplitude all-on case** |
| Deterministic seeded patch, locks respected | **Linux core smoke passed** |
| Samples remain finite during randomized stress | **Linux core stress passed at 44.1/48/88.2/96/192 kHz** |
| Neutral mono silence after reset | **Linux core stress passed** |
| 17 JSON presets with 100 raw and 100 lock values each | **JSON schema shape verified** |
| Audio output fixture rendered to WAV | **Linux CLI successfully generated 5 WAV files** |
| JUCE processor compilation | **NOT TESTED; JUCE unavailable here** |
| macOS Apple Silicon/Intel VST3 binary | **NOT BUILT; requires macOS Xcode and SDK** |
| Host automation, state, 17 preset recall, A/B | **Written but NOT JUCE/DAW TESTED** |
| GUI rendering/resizing/accessibility | **NOT TESTED** |
| Real DAW realtime/offline stability and CPU/latency budgets | **NOT TESTED** |
| Spectral blur implemented with FFT | **YES — low-resolution 256-point STFT wet branch; core tests only** |
| Oversampled antialiased distortion/true-peak limiting | **NO — release-blocking improvements** |

Source: tests under `Tests/`, `docs/All100Measured.csv`, `docs/ValidationReport.md`.
