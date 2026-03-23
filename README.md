# VST Equalizer

A parametric VST3 equalizer plugin built with [JUCE](https://juce.com), featuring six EQ bands, a real-time frequency response curve, and full DAW automation support.

---

## Build Instructions

### Prerequisites

| Tool | Version |
|------|---------|
| CMake | ≥ 3.22 |
| C++ compiler | C++17 capable (GCC 9+, Clang 10+, MSVC 2019+) |
| Git | Any recent version |
| Internet access | Required for the first build (JUCE is fetched via FetchContent) |

### Steps

```bash
# 1. Clone the repository
git clone https://github.com/cetinkayabugra/VST-equalizer.git
cd VST-equalizer

# 2. Configure (downloads JUCE automatically)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. Build
cmake --build build --config Release

# 4. The built plugin will be at:
#    build/VSTEqualizer_artefacts/Release/VST3/VST Equalizer.vst3   (Linux / Windows)
#    build/VSTEqualizer_artefacts/Release/VST3/VST Equalizer.vst3   (macOS bundle)
```

### Running the Standalone version

The build also produces a **Standalone** executable for quick testing without a DAW:

```bash
./build/VSTEqualizer_artefacts/Release/Standalone/VST\ Equalizer
```

---

## Plugin Features

| Feature | Details |
|---------|---------|
| **EQ bands** | 6 bands: High Pass, Low Shelf, Bell ×2, High Shelf, Low Pass |
| **Per-band controls** | Frequency, Gain, Q, Enable/Disable toggle, Filter type selector |
| **Filter types** | High Pass, Low Shelf, Bell/Peak, High Shelf, Low Pass |
| **Frequency response display** | Real-time EQ curve drawn at 30 fps |
| **Global bypass** | Hard bypass that passes audio unprocessed |
| **Output gain** | ±24 dB output trim |
| **DAW automation** | All parameters are automatable via VST3 |
| **State persistence** | Plugin state saved/loaded by the DAW |
| **Stereo processing** | Full stereo support (mono input also accepted) |
| **Sample rate support** | Any sample rate supported by the DAW |

### Default Band Configuration

| Band | Type | Frequency | Gain | Q |
|------|------|-----------|------|---|
| 1 | High Pass | 40 Hz | — | 0.71 |
| 2 | Low Shelf | 120 Hz | +2.5 dB | 0.90 |
| 3 | Bell | 500 Hz | −3.0 dB | 1.40 |
| 4 | Bell | 2.5 kHz | +1.8 dB | 1.10 |
| 5 | High Shelf | 10 kHz | +3.5 dB | 0.80 |
| 6 | Low Pass | 20 kHz | — | 0.71 |

---

## Known Limitations

- **No spectrum analyser**: The frequency response curve shows the EQ curve only. A live input/output spectrum overlay is marked as TODO in the source.
- **No mid/side mode**: Only L/R stereo processing is implemented.
- **No presets**: Preset save/load is not yet implemented; state is persisted per DAW session via the normal VST3 state mechanism.
- **No linear phase mode**: All filters are minimum-phase IIR biquads.
- **No dynamic EQ**: Static EQ only.
- **No oversampling**: Processing runs at the host's native sample rate.

---

## Overview
A **VST Equalizer** is an audio plugin that allows users to shape the frequency balance of an audio signal. It is commonly used in music production, mixing, mastering, podcast editing, and sound design.

This plugin is designed to provide precise control over different frequency ranges while maintaining low latency and clean sound processing.

---

## Core Purpose
The main goal of a VST Equalizer is to:

- Boost or reduce specific frequency bands
- Clean up muddy mixes
- Add clarity and brightness
- Control harshness or resonances
- Shape the tonal balance of instruments, vocals, and full mixes

---

## Key Features

### 1. Multiple EQ Bands
The plugin can include several adjustable bands, such as:

- **Low Cut / High Pass Filter**
- **Low Shelf**
- **Bell / Peak Band**
- **High Shelf**
- **High Cut / Low Pass Filter**

Each band should support:

- **Frequency**
- **Gain**
- **Q / Bandwidth**
- **Enable / Disable toggle**

---

### 2. Real-Time Audio Processing
The equalizer should process audio in real time with minimal CPU usage and low latency.

---

### 3. Spectrum Analyzer
A built-in visual analyzer can help users see the frequency content of the incoming and processed audio.

Possible display layers:

- Input spectrum
- Output spectrum
- EQ curve overlay

---

### 4. User Interface
The UI should be clean and intuitive.

Suggested UI components:

- Draggable EQ nodes
- Frequency graph
- Gain sliders or knobs
- Q controls
- Band on/off buttons
- Analyzer display
- Output level meter

---

## Typical Frequency Ranges

| Range | Approx. Frequency | Common Perception |
|------|-------------------|-------------------|
| Sub Bass | 20 Hz - 60 Hz | Deep rumble, power |
| Bass | 60 Hz - 250 Hz | Warmth, fullness |
| Low Mids | 250 Hz - 500 Hz | Body, muddiness |
| Midrange | 500 Hz - 2 kHz | Core tone, presence |
| Upper Mids | 2 kHz - 6 kHz | Attack, clarity |
| Highs | 6 kHz - 20 kHz | Air, sparkle |

---

## Functional Requirements

### Audio Processing
- Accept mono and stereo input
- Apply EQ filters in sequence
- Maintain stable output without clipping
- Support common sample rates
- Support automation from DAW hosts

### Controls
- Adjustable gain per band
- Adjustable center/cutoff frequency
- Adjustable Q factor
- Bypass switch
- Global output gain

### Compatibility
- VST3 support
- Works in DAWs such as:
  - Ableton Live
  - FL Studio
  - Cubase
  - Reaper
  - Studio One
  - Logic Pro (through compatible wrapper or AU version if extended)

---

## Non-Functional Requirements
- Low CPU consumption
- Stable performance during long sessions
- Responsive GUI
- Accurate filter calculations
- Safe parameter handling
- Cross-platform support where possible

---

## Suggested Technical Architecture

### Main Components
1. **Audio Processor**
   - Handles incoming audio buffers
   - Applies filter calculations
   - Manages plugin parameters

2. **Filter Engine**
   - Implements EQ filters
   - Updates coefficients when parameters change
   - Supports different filter types

3. **Parameter Manager**
   - Stores band settings
   - Syncs with DAW automation
   - Handles presets

4. **GUI / Editor**
   - Renders spectrum and EQ graph
   - Allows user interaction
   - Displays current parameter values

---

## Common Filter Types

### High-Pass Filter
Removes low frequencies below a cutoff point.

### Low-Pass Filter
Removes high frequencies above a cutoff point.

### Bell Filter
Boosts or cuts around a center frequency.

### Low Shelf
Boosts or cuts all frequencies below a selected point.

### High Shelf
Boosts or cuts all frequencies above a selected point.

---

## Example Parameter Set

```text
Band 1: High Pass   | 40 Hz   | Q 0.71
Band 2: Low Shelf   | 120 Hz  | +2.5 dB | Q 0.90
Band 3: Bell        | 500 Hz  | -3.0 dB | Q 1.40
Band 4: Bell        | 2.5 kHz | +1.8 dB | Q 1.10
Band 5: High Shelf  | 10 kHz  | +3.5 dB | Q 0.80
```

---

## Development Suggestions

### Recommended Frameworks
- **JUCE** for cross-platform audio plugin development
- **iPlug2** as an alternative lightweight framework

### Programming Language
- **C++** is the standard choice for VST plugin development

### Extra Features to Consider
- Preset saving/loading
- Mid/Side EQ mode
- Left/Right independent processing
- Linear phase mode
- Dynamic EQ mode
- Oversampling
- Undo/redo support

---

## Basic Processing Flow

```text
Audio Input
   â
Input Gain
   â
Filter Chain
   â
Optional Analyzer
   â
Output Gain
   â
Audio Output
```

---

## Example Use Cases

### Vocal Mixing
- Cut low rumble below 80 Hz
- Reduce muddy frequencies around 250 Hz
- Add presence around 3 kHz
- Add air above 10 kHz

### Drum Processing
- Boost punch around 60-100 Hz
- Cut boxiness around 400 Hz
- Add snap around 5 kHz

### Master Bus
- Gentle low shelf for warmth
- Slight upper-mid reduction for smoothness
- Small high shelf boost for air

---

## Future Improvements
- AI-assisted EQ suggestions
- Match EQ mode
- Real-time resonance detection
- Intelligent auto-gain compensation
- Custom skin/themes

---

## Conclusion
A VST Equalizer is one of the most essential tools in digital audio production. A well-designed EQ plugin combines accurate signal processing, efficient performance, and a simple user experience. With strong filter design and a responsive visual interface, it can become a practical and professional tool for both beginner and advanced producers.
