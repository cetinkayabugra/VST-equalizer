# VST Equalizer - Command Reference & Cheat Sheet

## 🏗️ Build Commands

### Quick Build (All Platforms)

```bash
# Clone dependencies
cd /workspaces
git clone https://github.com/juce-framework/JUCE.git
cd JUCE && git checkout 7.0.9

# Build plugin
cd /workspaces/VST-equalizer
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release --parallel 4
```

### Platform-Specific

#### macOS (Xcode)
```bash
cd build
cmake .. -GXcode -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
# Or: xcode-select --install (if needed)
```

#### Linux (Make)
```bash
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel 4
# Install deps: sudo apt-get install libx11-dev libgl1-mesa-dev libwebkit2gtk-4.0-dev
```

#### Windows (Visual Studio)
```powershell
cd build
cmake .. -GVisualStudio16 -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
# Requires VS2019+ with C++ workload
```

---

## 📦 Installation Commands

### macOS
```bash
# User installation
mkdir -p ~/Library/Audio/Plug-Ins/VST3
cp -r build/VSTEqualizer_artefacts/Release/VST3/*.vst3 ~/Library/Audio/Plug-Ins/VST3/

# System-wide (requires admin)
sudo cp -r build/VSTEqualizer_artefacts/Release/VST3/*.vst3 /Library/Audio/Plug-Ins/VST3/
```

### Linux
```bash
mkdir -p ~/.vst3
cp -r build/VSTEqualizer_artefacts/Release/VST3/*.vst3 ~/.vst3/
chmod -R 755 ~/.vst3/VSTEqualizer.vst3
```

### Windows
```powershell
# Create directory
$vst3Dir = "C:\Program Files\Common Files\VST3"
New-Item -ItemType Directory -Force -Path $vst3Dir

# Copy files
Copy-Item -Path "build\VSTEqualizer_artefacts\Release\VST3\*" `
          -Destination $vst3Dir -Recurse -Force
```

---

## 🧹 Clean & Reset

```bash
# Clean build directory
cd /workspaces/VST-equalizer
rm -rf build/
mkdir build && cd build

# Full rebuild
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --clean-first --config Release --parallel 4

# Remove plugins from DAW
rm -rf ~/.vst3/VSTEqualizer.vst3 # Linux/macOS
```

---

## 🔍 Verification Commands

### Check if plugin installed
```bash
# Linux/macOS
ls ~/.vst3/VSTEqualizer.vst3/
ls ~/Library/Audio/Plug-Ins/VST3/VSTEqualizer.vst3/

# Windows
dir "C:\Program Files\Common Files\VST3\VSTEqualizer.vst3"
```

### Find build issues
```bash
# Verbose build output
cmake --build . --config Release --verbose

# Check CMake version
cmake --version

# Verify JUCE path
ls /workspaces/JUCE/modules/juce_audio_processors

# Test compilation with details
g++ -std=c++17 -v -c Source/PluginProcessor.cpp -o test.o
```

### Monitor plugin loading
```bash
# Linux: Check system logs
journalctl -e | grep VST
dmesg | grep plugin

# macOS: Console.app
open /Applications/Utilities/Console.app

# Windows: Event Viewer
eventvwr.msc
```

---

## 📊 Performance Testing

### CPU Profiling

**macOS (Instruments)**:
```bash
# In Xcode:
Product → Profile → System Trace
# Run with audio, analyze CPU impact
```

**Linux (perf)**:
```bash
# Find plugin process
PID=$(pgrep -f "reaper|studio" | head -1)

# Profile
perf record -F 99 -p $PID
perf report

# View call graph
perf record -g -p $PID
perf report -g
```

**Windows (Performance Monitor)**:
```powershell
# Open Performance Monitor
Start-Process perfmon.exe

# Or use command line
typeperf -cf items.txt -sc 10
```

### Memory Checking

**Linux (Valgrind)**:
```bash
valgrind --leak-check=full /path/to/daw 2>&1 | grep VSTEqualizer
```

**macOS (Instruments)**:
```
Xcode → Product → Profile → Allocations
```

### Audio Testing

```bash
# Generate test signals
# Pink noise (44.1 kHz, 10 sec)
sox -n -r 44100 -b 16 pink_noise.wav synth 10 pink

# Sine sweep (20 Hz → 20 kHz)
sox -n -r 44100 -b 16 sweep.wav synth 10 sine 20:20000

# Load into your DAW and test EQ
```

---

## 🐛 Debugging

### Enable Debug Logging

Create `Source/DebugConfig.h`:
```cpp
#ifdef DEBUG
    #define DBG_LOG(msg) std::cerr << "[DEBUG] " << msg << std::endl
#else
    #define DBG_LOG(msg) 
#endif
```

Add to PluginProcessor.cpp:
```cpp
#include "DebugConfig.h"

void processBlock(...) {
    DBG_LOG("Processing block: " << buffer.getNumSamples() << " samples");
}
```

### GDB Debugging (Linux/macOS)

```bash
# Start DAW under GDB
gdb --args reaper

# In GDB:
(gdb) break PluginProcessor.cpp:100
(gdb) run
(gdb) next
(gdb) print variable_name
(gdb) bt  # backtrace
```

### VS Debugger (Windows)

```
Visual Studio → Debug → Attach to Process
Select: reaper.exe or your_daw.exe
Set breakpoints in code
Run and debug
```

---

## 🎯 Parameter Reference

### Frequency Control
```
Minimum: 20 Hz
Maximum: 20000 Hz
Scaling: Logarithmic (musical)
Default: Varies per band
Grain: 0.1 Hz
```

### Gain Control
```
Minimum: -24 dB
Maximum: +24 dB
Scaling: Linear
Default: 0 dB (no change)
Grain: 0.1 dB
```

### Q Control
```
Minimum: 0.1 (very wide)
Maximum: 10.0 (very narrow)
Scaling: Logarithmic
Default: 1.0 (moderate)
Grain: 0.01
```

### Filter Types
```
0: High-Pass   (remove lows)
1: Low-Pass    (remove highs)
2: Low-Shelf   (boost/cut lows)
3: High-Shelf  (boost/cut highs)
4: Bell/Peak   (surgical cut/boost)
```

---

## 📐 DSP Constants

### Sample Rates
```cpp
// Common rates
44100 Hz  (CD quality)
48000 Hz  (Professional)
96000 Hz  (High-res)
192000 Hz (Ultra hi-res)
```

### FFT Parameters
```cpp
FFT_SIZE = 2048
NUM_BINS = 1024
FREQUENCY_RESOLUTION = sampleRate / FFT_SIZE
  @ 44.1 kHz: 21.5 Hz/bin
  @ 48 kHz:   23.4 Hz/bin
```

### Filter Coefficients
```cpp
// All normalized by a0
a0 = 1.0 (always after normalization)
a1 = [-2, 2]      (typical range)
a2 = [-1, 1]      (typical range)
b0, b1, b2:       varies by type
```

---

## 🎼 Mixing Recipes

### Vocal Clarity
```
Band 1: High-Pass @ 80 Hz
Band 3: Bell @ 3 kHz, Gain +4 dB, Q 1.5
Band 5: High-Shelf @ 10 kHz, Gain +2 dB

Result: Clear, present vocal with air
```

### Bass Punch
```
Band 2: Low-Shelf @ 100 Hz, Gain +3 dB
Band 3: Bell @ 60 Hz, Gain +2 dB
Band 5: Low-Pass @ 15 kHz

Result: Punchy bass without harshness
```

### De-Essing
```
Band 4: Bell @ 6-8 kHz, Gain -3 dB, Q 2.0

Result: Smooth sibilants
```

### Master Bus Polish
```
Band 2: Low-Shelf @ 200 Hz, Gain +0.5 dB
Band 3: Bell @ 500 Hz, Gain -1 dB
Band 4: Bell @ 2 kHz, Gain +0.5 dB
Band 5: High-Shelf @ 8 kHz, Gain +1 dB

Result: Professional sheen
```

---

## 🔧 Code Snippets

### Access APVTS Parameter
```cpp
auto freqParam = dynamic_cast<juce::AudioParameterFloat*>(
    apvts.getParameter("freq0"));
float frequency = freqParam->get();
```

### Create New Parameter
```cpp
params.push_back(std::make_unique<juce::AudioParameterFloat>(
    "paramID", "Display Name",
    juce::NormalisableRange<float>(minVal, maxVal, 0.01f),
    defaultValue, "Unit"));
```

### Get Frequency Response
```cpp
double response = filterBank.getFrequencyResponse(1000.0);  // @ 1 kHz
double responseDb = 20.0 * std::log10(response);
```

### Process Audio Buffer
```cpp
juce::AudioBuffer<float> buffer(numChannels, numSamples);
filterBank.processBlock(buffer);
// Buffer now contains filtered audio
```

### Update Filter Parameters
```cpp
filterBank.setBandParameters(
    0,  // band index
    BiQuadFilter::FilterType::HighPass,
    80.0,  // frequency Hz
    0.0,   // gain dB
    0.7    // Q
);
```

---

## 📝 File Modification Checklist

### To Add 6th Band:
```
[ ] FilterBank.h: NUM_BANDS = 6
[ ] PluginProcessor.cpp: Loop from 0 to 6
[ ] PluginEditor.h: Add 6th BandControlComponent
[ ] PluginEditor.cpp: Update resized() layout
[ ] Rebuild & test
```

### To Change UI Colors:
```
[ ] PluginEditor.cpp: Modify juce::Colour() values
[ ] Look for: Colour(30, 30, 35), Colour(100, 200, 255)
[ ] Change RGB values to desired colors
[ ] Recompile
```

### To Add Parameter:
```
[ ] PluginProcessor.cpp: createParameterLayout()
[ ] Add std::make_unique<AudioParameterXXX>(...)
[ ] Bind in parameterChanged()
[ ] Update PluginEditor UI
[ ] Recompile
```

---

## 🚨 Error Messages & Solutions

### "JUCE not found"
```
Solution:
export JUCE_PATH=/path/to/JUCE
cmake .. -DCMAKE_PREFIX_PATH=$JUCE_PATH
```

### "C++17 not supported"
```
Solution (macOS):
xcode-select --install
gcc --version  # Check version >= 8.0

Solution (Linux):
sudo apt-get install g++-9
cmake .. -DCMAKE_CXX_COMPILER=g++-9

Solution (Windows):
Use Visual Studio 2019+
```

### "Plugin crashes on load"
```
Steps:
1. Check DAW error log
2. Verify VST3 folder structure correct
3. Run: chmod 755 ~/.vst3/*.vst3/*
4. Check symbol stripping: strip is_off
5. Rebuild in Debug mode for details
```

### "No audio output"
```
Check:
[ ] Input Gain not at -∞ dB
[ ] Output Gain not at -∞ dB
[ ] At least one band enabled (ON button)
[ ] Sample rate matches DAW
[ ] Mono vs Stereo (plugin handles both)
```

---

## 📱 Development Environment

### Recommended IDE SetUp

**CLion**:
```
Preferences → Tools → CMake
Set CMake: /usr/bin/cmake
Set Build Type: Release
```

**VS Code**:
```json
{
  "cmake.configureOnOpen": true,
  "cmake.buildDirectory": "${workspaceFolder}/build",
  "cmake.sourceDirectory": "${workspaceFolder}"
}
```

**Xcode**:
```bash
cd build
cmake .. -GXcode
open VSTEqualizer.xcodeproj
```

---

## 🎓 Learning Resources

### Quick References
- **JUCE Docs**: https://docs.juce.com/master/index.html
- **Biquad Math**: https://www.earlevel.com/main/2013/06/01/biquad-calculator/
- **VST3 Spec**: https://steinbergmedia.github.io/vst3_dev_portal/
- **Audio DSP**: https://www.dsprelated.com

### In-Code Documentation
- **BiQuadFilter.h**: Read math comments for coefficient derivation
- **PluginProcessor.cpp**: See parameter setup examples
- **PluginEditor.h**: Study GUI component layout

---

## ⚡ Performance Tips

### Optimization Ideas
```cpp
// Cache frequently accessed values
const float outputGain = apvts.getParameter("outputGain")->load();
buffer.applyGain(outputGain);  // Faster than  repeated lookups

// Use SIMD hints
[[gnu::simd]]
void processSamples(float* data, int count) {
    for (int i = 0; i < count; ++i)
        data[i] *= gainFactor;
}

// Inline hot paths
inline double processSample(double sample) noexcept {
    // Direct Form II
    double out = b0*sample + z1;
    z1 = b1*sample + z2 - a1*out;
    z2 = b2*sample - a2*out;
    return out;
}
```

---

## 📞 Quick Reference Table

| Task | Command | File |
|------|---------|------|
| Build | `cmake --build .` | CMakeLists.txt |
| Install | `cp -r build...` | BUILDING_GUIDE.md |
| Debug | Open in IDE | .vscode/launch.json |
| Profile | Use IDE tools | BUILDING_GUIDE.md |
| Document | Read INDEX.md | INDEX.md |
| Extend | Read DEVELOPER_GUIDE.md | DEVELOPER_GUIDE.md |

---

**Print this sheet for quick reference while developing!** 📋
