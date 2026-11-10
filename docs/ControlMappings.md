# Control mappings — McBoy v0.9

| Control IDs | Actual range | Initial value | Meaning |
|---|---|---|---|
| `fx001`–`fx100` | 0..1 raw | 0 | One numbered effect per knob; all 100 have independent DSP in C++ core |
| `degree` | 0..1 | 0.30 | Global effect ceiling; `effective=raw×degree`, smoothed approx. 28 ms |
| `mix` | 0..1 | 0.75 | Blends original input and normalized wet composite |
| `output` | -18 to +6 dB | 0 | Gain applied to processed signal |
| `guard` | Boolean | ON | Asymptotic -1 dBFS digital sample-peak guard (not true peak) |
| `bypass` | Boolean | OFF | Smooth dry-path crossfade; dry bypass not peak limited |
| `labels` | Boolean | OFF | Changes visible knob text, not audio |

For all `fxNNN` ranges, initial raw values and actual effect names, see `ParameterMap.csv`; functional measured data for all 100 live effect IDs is in `All100Measured.csv`. Every numbered control is automatable in the written JUCE APVTS source, pending macOS compilation and host testing. The original release specification calls for per-effect perceptual calibration, not yet completed; several algorithms are deliberately lo-fi approximations documented in `AlgorithmNotes.md`.

`MCBOY!`/R/Space triggers deterministic seeded randomization of unlocked controls. Option-click/right-click mini-knob toggles a lock. Lock all/unlock all, reset, 20-entry Undo/Redo, 17 preset definitions, Copy/Paste, and a stored A/B comparison slot are present in the JUCE source. Randomize changes raw effect values but does not change DEGREE. Saving/pasting state serializes seed, locks, labels, utilities, Undo/Redo and A/B comparison slot (in written code; host untested).

The current hover shows numbered ID, name, raw/effective amount, live status and lock status. Every knob tooltip now includes the original prose description as well as ID, name, raw/effective values and lock status (host GUI behavior remains unverified). GUI accessibility, automation stability, high-DPI window scaling and control editing still require macOS VST3 testing.
