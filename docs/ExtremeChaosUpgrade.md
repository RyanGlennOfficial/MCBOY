# McBoy 1.0.0: Extreme Chaos DSP release notes

**Plugin:** McBoy | **VST3 vendor:** RG Audio | **Bundle ID:** audio.rgaudio.mcboy

## Why the sound was weak
The original randomizer selected small effect parameter values and limited each bank to four slots, while the audio engine summed many nearly-dry parallel signals. Because several effects had similar timbres to the source, mixing large numbers of them averaged away the most dramatic transformations. Prior to v0.9.1, the wet bus also directly mixed in a proportion of dry audio.

## What changed in 1.0.0
- The 100 JUCE parameter IDs (fx001–fx100), six utility IDs, plugin code MCBY, and **legacy internal** manufacturer code HWMG remain unchanged to preserve host session identity. The *displayed manufacturer* remains **RG Audio**.
- The randomizer uses a deterministic shuffled selection within each of ten banks. This avoids the earlier preference for low-numbered knobs. It ensures category-diverse, high-intensity candidate sounds at high DEGREE, and protects locked controls.
- High DEGREE permits up to the original full safe effect ranges. Its random raw values bias toward the top end instead of toward silence; a given stored raw patch is still shaped by effective=raw*DEGREE.
- The new IntensityStage is a realtime-safe **wet-only serial** layer. It uses existing per-knob effective settings to modulate wet-bus variable delays/warble, hard rhythmic gates, nonlinear saturation, filtering and space/pitch dominance. There is **no additional direct-input branch** in this processor.
- The parallel micro-effect mixer adds dynamic, seed-dependent category focus so complex combinations remain distinctive. Hall and Freeze settings increase bounded comb feedback, and the host-visible tail estimate is 15 seconds.
- New instances default to **DEGREE 72%**, **MIX 100%**, **LABELS OFF**, and **digital peak guard ON**. Existing saved parameter values retain their original values on load. A subset of factory presets now prioritises wet intensity.
- MIX=0 outputs dry input at unity output even with the digital peak guard enabled. MIX=100 outputs solely processed effects whenever any effect is engaged; a wet activation ramp rises from *silence*, not dry audio. With all 100 effects neutral, output is intentionally transparent.
- The guard limits **digital sample peaks** (asymptotic ceiling -1 dBFS). It is not a certified true-peak limiter, and there is no claim of guaranteed speaker/headphone protection, full reverb hardware fidelity, or professional DAW compatibility without listening tests.

## Objective local tests

On Linux x86_64 the independent C++17 core successfully compiled in Release mode; all four tests passed: CoreSmoke, AudioStress, All100Uniqueness and ChaosIntensity.

- 0% MIX: exact waveform equality for the stereo fixture with all effects set high and the guard ON.
- 100% MIX: one-sample impulse plus only the Slap delay—zero output through the first 2,500 frames and an audible delayed response.
- Seven deterministic seeds: at DEGREE 0.12 the active micro-effect count ranged 6–11; at DEGREE 0.95 it ranged 53–58.
- Mean high-degree RMS difference from the original test signal was approximately 1.75x the mean low-degree difference.
- Minimum inter-patch RMS distance among the seven high-degree fixture outputs was 0.0333.
- A saved raw patch tested at DEGREE 0.07 and 1.0 had a substantial audio difference without mutating stored raw knob settings.
- Every individual control continued to produce a measurable, different sound with the synthetic fixture.
- Mono/silence/stress/sample-rate tests completed over 44.1–192 kHz with finite output in the tested cases.

These are synthetic audio metrics, **not subjective musical sound-quality ratings** or a claim of zero clicks in every host. The GitHub macOS job additionally builds JUCE, checks Xcode arm64/x86_64 slices, compiles and executes the new intensity test, and verifies the compiled vendor metadata. Always inspect the linked Actions run to establish whether the macOS build actually passed.

## Installation, validation
Install the released McBoy.vst3 into ~/Library/Audio/Plug-Ins/VST3/. Close/reopen the DAW and rescan. Some DAWs or nested VST3 hosts can route a parallel dry track independent of McBoy: check host mix/parallel routing if source audio remains loud at 100% wet.

In a host, test in stereo and mono with vocals, guitar, drums and bus material, rapid randomization, saved-project reload, offline bounce, automation, CPU usage at 96kHz, and recovery from extreme feedback presets.
