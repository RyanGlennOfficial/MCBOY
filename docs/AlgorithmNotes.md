# McBoy 0.9 DSP: functional algorithms and honest fidelity limits

The 100 miniature controls each have a unique stable parameter ID, a neutral state, a nonzero observable audio transformation at DEGREE 100%, and independent automation. Their core algorithms were written in C++17 rather than copied from any commercial plugin. `docs/All100Measured.csv` reports each output's RMS change from dry and nearest-neighbor RMS difference under one 48 kHz, 0.5-second multitone/transient stereo fixture. **A positive RMS difference is evidence that the algorithm changes samples, not evidence of correct perceptual identity, sonic quality, or uniqueness in every conceivable input.**

### Fidelity limitations of the initial 100

- **fx003, fx004, fx005, fx007–010**: bounded modulated/tapped/sliced delay variants; Rewind uses a short reverse-reading buffer, not full continuous 2-direction scratch interpolation. Ghost Pre-Echo is an illusion based on *past* material, not impossible prediction of future transients. Temporal discontinuities near slice boundaries remain an artifact to calibrate.
- **fx011–020**: primitive tapped/comb reverb networks with gated and pitch additions; Endless Hall, Halo, Hold the Clouds and Backward Bloom are musical approximations, not production-grade convolution/FDN/shimmer algorithms. Snapshots of the full long-tail engine are not serialized.
- **fx023, fx029, fx030, fx039, fx054, fx055, fx057**: all-pass and one-pole filter banks; some filters emphasize moving spectra rather than emulate calibrated resonant analog transfer functions. Tight Q tracking remains future work.
- **fx031–038 and fx040**: two-read-head tape/grain-style pitch effects. They produce transposed/warped voices with audible granular modulation and are NOT transient-preserving modern pitch shifting. They have wet-path delay; overall plugin reports zero latency because the dry path is not delayed.
- **fx041–050, fx059**: original bounded nonlinear clipping/folding/quantization algorithms. Currently no oversampling and no proven anti-alias strategy for nonlinear modules.
- **fx060 Blur Vision**: genuine 256-point FFT/STFT spectral blur with a 128-sample hop, temporal and neighboring-bin magnitude averaging, and overlap-add output. Its wet voice incurs approximately 256 samples of algorithmic displacement; it is not a production-grade high-resolution STFT with phase-vocoder transient handling. The plugin dry branch stays undelayed and host-reported latency is zero.
- **fx062–070**: coarse grid-driven repeats, swings, choppers, transient triggers, and granular scratch variants. Tempo uses host BPM when available; musical beat *position* synchronization is not implemented, so these free-run from plugin processing start.
- **fx071–080**: procedurally generated analog-inspired texture/noise; independent of copyrighted samples; exact statistical analog behavior is not claimed.
- **fx081–090**: M/S, short delay, phase and panning effects; Fake 3D is only a filtered crossfeed approximation, NOT a measured HRTF. Be cautious of phase cancellation in mono.
- **fx091–100**: amplitude envelope and local bounded feedback variants; compressors are not calibrated for professional mixing loudness/ratio metering. Cosmic Implosion combines a contracted delay, low-pass and waveshaping.

### Architecture and safety

Every effect raw slider ranges `[0,1]`. DEGREE also `[0,1]`, and the DSP wet weight uses the smoothed value of `raw × degree` *once*. No effect's individual raw adjustment is silently routed to another control's parameter. Input-to-wet processing uses bounded per-sample reads in prepared history rings and an isolated nonlinear feedback memory for fx096. `Engine::process` performs no vector resizing, file I/O or explicit locks. Most stateful delays use fixed-size buffers prepared for the selected sample rate; sudden automation over time-based parameters is smoothed by the effective control ramp, not by seamless replacement of individual delay memories.

Global output sample guard gives identity below ~0.72FS and approaches 0.89125FS for larger absolute amplitudes. It is NOT true-peak or lookahead limiting. Pure bypass and guard OFF are not peak bounded. DC is filtered in the composite wet bus only, leaving a neutral dry signal unchanged when guard OFF.

### Production readiness

The code intentionally passes the first "100 individually active" milestone, not the complete original release acceptance contract. A production version still requires critical listening, calibrated psychoacoustics, CPU profiling and macOS VST3 host tests. Some names describe musical intent rather than an exact physical-model implementation. Do not market the algorithms as faithful emulations without new tests.
