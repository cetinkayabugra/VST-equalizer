# Implementation Notes

## Design Choices

### Framework: JUCE 7
JUCE was chosen because:
- It is the industry standard for cross-platform VST/AU/AAX development.
- It provides `juce::dsp::IIR::Filter` and `juce::dsp::IIR::Coefficients` with all required filter types out of the box, including `getMagnitudeForFrequency()` for display.
- `juce::AudioProcessorValueTreeState` (APVTS) handles parameter layout, DAW automation binding, and XML state serialization automatically.
- `juce::AudioProcessorValueTreeState::SliderAttachment / ComboBoxAttachment / ButtonAttachment` eliminate boilerplate for UI ↔ parameter synchronization.

### Build System: CMake + FetchContent
CMake with `FetchContent_Declare(JUCE ...)` was chosen over the Projucer tool so the project builds from a clean clone with no pre-generated files required.  This keeps the repository clean and makes CI straightforward.

---

## DSP Approach

### Filter Implementation
Each EQ band is implemented as a **biquad IIR filter** using JUCE's `juce::dsp::IIR::Filter<float>`.  This is the standard choice for parametric EQ because:
- Biquad filters are computationally inexpensive (5 multiplications + 4 additions per sample).
- JUCE's implementation is well-tested and numerically stable for audio frequencies.
- Coefficient updates can be applied every block with negligible overhead.

### Filter Coefficient Types

| Band | Filter Type | JUCE API |
|------|------------|---------|
| High Pass | 2nd-order Butterworth HP | `Coefficients::makeHighPass(sr, freq, Q)` |
| Low Shelf | 2nd-order shelf | `Coefficients::makeLowShelf(sr, freq, Q, gain)` |
| Bell / Peak | Peaking EQ biquad | `Coefficients::makePeakFilter(sr, freq, Q, gain)` |
| High Shelf | 2nd-order shelf | `Coefficients::makeHighShelf(sr, freq, Q, gain)` |
| Low Pass | 2nd-order Butterworth LP | `Coefficients::makeLowPass(sr, freq, Q)` |

Gain values are stored in dB and converted to linear (`Decibels::decibelsToGain`) before being passed to the coefficient constructors.

### Coefficient Update Strategy
`updateFilters()` is called at the top of every `processBlock()`.  This ensures coefficient changes (from automation or UI) are applied with one audio block of latency.  Since biquad coefficient computation is O(1) and far cheaper than audio processing itself, this does not measurably impact CPU usage.

Thread safety is maintained because coefficient updates and audio processing both happen on the audio thread (the host calls `processBlock` from a single real-time thread).

### Stereo Processing
Each band uses two separate `juce::dsp::IIR::Filter<float>` instances (one per channel, stored as `std::array<..., 2>`).  Both share the same coefficient object; only the filter *state* (delay registers) is kept per-channel.  This is correct for independent L/R processing.

Processing uses `juce::dsp::AudioBlock` + `ProcessContextReplacing` for block-based efficiency rather than sample-by-sample iteration.

---

## Parameter Architecture

All parameters are registered with `juce::AudioProcessorValueTreeState` so they are:
- Automatable by any VST3-compatible DAW.
- Serialized/deserialized automatically for session recall.
- Accessible via atomic `getRawParameterValue()` for lock-free reads.

### Parameter Ranges

| Parameter | Range | Skew |
|-----------|-------|------|
| Frequency | 20 Hz – 20 kHz | 0.25 (logarithmic feel) |
| Gain | −24 dB to +24 dB | linear |
| Q | 0.1 – 18.0 | 0.5 (sqrt – wider resolution at low Q) |
| Output Gain | −24 dB to +24 dB | linear |

---

## UI Architecture

### FrequencyResponseComponent
Draws the combined EQ magnitude response curve at 30 fps using a `juce::Timer`.
- The curve is computed by calling `processor.getFrequencyResponse(frequencies)` which uses `Coefficients::getMagnitudeForFrequency()` for each enabled band.
- 512 log-spaced frequency points are pre-computed and reused every frame.
- The curve is drawn as a stroked `juce::Path` with a semi-transparent filled region below it.

### BandControlComponent
One row per EQ band.  Contains:
- **Enable toggle** (ToggleButton)
- **Filter type selector** (ComboBox)
- **Frequency, Gain, Q knobs** (Rotary sliders)

All controls are connected to APVTS via attachments, so no manual synchronization code is needed.

---

## Assumptions Made from the Specification

1. **Filter types are per-band selectable** – The spec lists different filter types but does not explicitly say whether the type can be changed. I chose to make the type selectable via a ComboBox so the plugin is more flexible.
2. **6 bands** – The spec's example shows 5 bands; I added a 6th (Low Pass at 20 kHz) as it is listed as an optional feature and completes the classic EQ band set.
3. **Stereo only** – Mono input is also accepted by using the same filter state for both channels when only one channel is present.
4. **Output gain is in dB** – More intuitive for audio engineers than a linear multiplier.
5. **Q range 0.1–18** – Covers everything from very wide (gentle shelves) to very narrow (surgical notch cuts).

---

## Future Improvements

### Near-term
- **Preset system**: Save/load named presets to XML, with a UI preset menu.
- **Undo/Redo**: APVTS supports UndoManager; just needs wiring.
- **Double-click to reset**: Standard UX for knobs.

### Medium-term
- **Live spectrum analyser**: FFT-based input/output overlay.  Scaffold: add a `juce::dsp::FFT` in the processor, accumulate magnitude data in a lock-free ring buffer, and draw in `FrequencyResponseComponent`.
- **Mid/Side mode**: Add an M/S matrix encode/decode around the filter chain.
- **Oversampling**: Wrap the filter chain in `juce::dsp::Oversampling<float>` for higher quality at high frequencies.

### Advanced
- **Linear phase mode**: Replace IIR filters with FIR equivalents computed via frequency-domain design, with latency compensation reported to the host.
- **Dynamic EQ**: Add per-band sidechain detection (peak/RMS) driving the gain parameter.
- **Draggable EQ nodes**: Replace static knobs with interactive nodes on the response curve.
- **AI-assisted EQ suggestions**: Analyse input spectrum and suggest EQ curves.

---

## Project Structure

```
VST-equalizer/
├── CMakeLists.txt              # Build system (CMake + JUCE FetchContent)
├── README.md                   # User-facing documentation + build steps
├── IMPLEMENTATION_NOTES.md     # This file
└── Source/
    ├── ParameterIDs.h          # Filter type enum, band defaults, parameter ID helpers
    ├── PluginProcessor.h       # AudioProcessor declaration
    ├── PluginProcessor.cpp     # DSP engine, parameter layout, state persistence
    ├── PluginEditor.h          # Editor/UI declaration
    └── PluginEditor.cpp        # FrequencyResponseComponent, BandControlComponent, Editor
```
