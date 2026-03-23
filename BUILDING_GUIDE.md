# VST Equalizer - Complete Build & Development Guide

## Quick Start

### 1. Install Dependencies

**macOS** (using Homebrew):
```bash
brew install cmake
brew install git
```

**Ubuntu/Debian**:
```bash
sudo apt-get update
sudo apt-get install cmake git build-essential
```

**Windows** (using Chocolatey):
```powershell
choco install cmake git
```

### 2. Clone & Setup

```bash
# Create workspace
mkdir -p /workspaces && cd /workspaces

# Clone JUCE (if not already present)
git clone https://github.com/juce-framework/JUCE.git
cd JUCE
git checkout 7.0.9  # Use stable release

# Return to VST-equalizer directory
cd /workspaces/VST-equalizer
```

### 3. Build

```bash
cd /workspaces/VST-equalizer
mkdir -p build
cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build (parallel for faster compile)
cmake --build . --config Release --parallel 4
```

### 4. Installation

**macOS**:
```bash
mkdir -p ~/Library/Audio/Plug-Ins/VST3
cp -r build/VSTEqualizer_artefacts/Release/VST3/VSTEqualizer.vst3 ~/Library/Audio/Plug-Ins/VST3/
```

**Ubuntu/Linux**:
```bash
mkdir -p ~/.vst3
cp -r build/VSTEqualizer_artefacts/Release/VST3/VSTEqualizer.vst3 ~/.vst3/
```

**Windows**:
```powershell
$vst3Path = "C:\Program Files\Common Files\VST3"
New-Item -ItemType Directory -Force -Path $vst3Path
Copy-Item -Path "build\vst3\Release\*" -Destination $vst3Path -Recurse
```

---

## Detailed Setup Instructions

### macOS

1. **Install Xcode Command Line Tools**:
```bash
xcode-select --install
```

2. **Install CMake**:
```bash
brew install cmake
```

3. **Clone JUCE** (v7.0.9 stable):
```bash
cd /workspaces
git clone https://github.com/juce-framework/JUCE.git
cd JUCE
git checkout 7.0.9
```

4. **Build VST Equalizer**:
```bash
cd /workspaces/VST-equalizer
mkdir build && cd build
cmake .. -GXcode -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

5. **Install Plugin**:
```bash
# For system-wide installation
sudo cp -r build/VSTEqualizer_artefacts/Release/VST3/VSTEqualizer.vst3 /Library/Audio/Plug-Ins/VST3/

# Or user-only
mkdir -p ~/Library/Audio/Plug-Ins/VST3
cp -r build/VSTEqualizer_artefacts/Release/VST3/VSTEqualizer.vst3 ~/Library/Audio/Plug-Ins/VST3/
```

### Ubuntu/Linux

1. **Install Build Tools**:
```bash
sudo apt-get update
sudo apt-get install build-essential cmake git libx11-dev libxrandr-dev libgl1-mesa-dev libglu1-mesa-dev libwebkit2gtk-4.0-dev
```

2. **Clone JUCE**:
```bash
cd /workspaces
git clone https://github.com/juce-framework/JUCE.git
cd JUCE && git checkout 7.0.9
```

3. **Build**:
```bash
cd /workspaces/VST-equalizer
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel 4
```

4. **Install**:
```bash
mkdir -p ~/.vst3
cp -r build/VSTEqualizer_artefacts/Release/VST3/VSTEqualizer.vst3 ~/.vst3/
```

### Windows (Visual Studio)

1. **Install Visual Studio 2019+**:
   - Download from https://visualstudio.microsoft.com/
   - Install C++ Desktop Development workload

2. **Install CMake**:
   - Download from https://cmake.org/download/
   - Or use: `choco install cmake`

3. **Clone JUCE**:
```powershell
cd C:\Dev
git clone https://github.com/juce-framework/JUCE.git
cd JUCE
git checkout 7.0.9
```

4. **Build VST Equalizer**:
```powershell
cd C:\Dev\VST-equalizer
mkdir build
cd build
cmake .. -GVisualStudio16
cmake --build . --config Release
```

5. **Install**:
```powershell
$vst3Dir = "C:\Program Files\Common Files\VST3"
Copy-Item -Path "build\VSTEqualizer_artefacts\Release\VST3\*" -Destination $vst3Dir -Recurse -Force
```

---

## IDE Setup Alternatives

### CLion (Recommended for Development)

1. **Open Project**:
   - File → Open → Select `/workspaces/VST-equalizer`

2. **Configure CMake**:
   - Preferences → Build, Execution, Deployment → CMake
   - Set CMake executable
   - Set Build Type: Release

3. **Build**:
   - Build → Build Project
   - Or Ctrl/Cmd+F9

4. **Run/Debug**:
   - CLion automatically detects the plugin target
   - You can set breakpoints and debug directly

### Visual Studio Code

1. **Install Extensions**:
   - C/C++ Extension Pack
   - CMake Tools

2. **Open Folder**:
   - File → Open Folder → Choose `/workspaces/VST-equalizer`

3. **Configure Kit**:
   - Ctrl+Shift+P → CMake: Select a Kit
   - Choose your compiler (GCC, Clang, MSVC)

4. **Build**:
   - Ctrl+Shift+P → CMake: Build
   - Or use the build button in the taskbar

### Xcode (macOS)

1. **Open Project**:
   ```bash
   cd /workspaces/VST-equalizer/build
   open VSTEqualizer.xcodeproj
   ```

2. **Configure Scheme**:
   - Select "VSTEqualizer" scheme
   - Set build configuration to Release

3. **Build**:
   - Cmd+B to build
   - The plugin will appear in your VST3 directory

---

## Troubleshooting

### CMake Configuration Errors

**Issue**: `JUCE not found`
```
Solution: Ensure JUCE is in /workspaces/JUCE
cd /workspaces && git clone https://github.com/juce-framework/JUCE.git
```

**Issue**: `C++ 17 not supported`
```
Solution: Update your compiler
macOS: xcode-select --install
Linux: sudo apt-get install g++-9 (or newer)
Windows: Use Visual Studio 2019 or later
```

### Build Errors

**Issue**: `Missing X11 headers` (Linux)
```
Solution: sudo apt-get install libx11-dev libxrandr-dev libgl1-mesa-dev
```

**Issue**: `WebKit2 not found` (Linux)
```
Solution: sudo apt-get install libwebkit2gtk-4.0-dev
```

**Issue**: `Plugin not appearing in DAW`
```
Solution:
1. Rescan plugin directory in DAW
2. Check file permissions: chmod 755 ~/.vst3/VSTEqualizer.vst3/*
3. On Windows: Register DLL with regsvr32 (if needed)
4. Check DAW's plugin scan log for errors
```

---

## Testing the Plugin

### Test Cases to Verify

1. **Plugin Loading**:
   - Open your DAW
   - Rescan plugins
   - Plugin should appear in effect list

2. **Audio Processing**:
   - Insert the plugin on an audio track
   - Send audio signal to the track
   - Adjust frequency and gain knobs
   - Audio should respond to EQ changes

3. **Parameter Control**:
   - Each rotary knob should adjust smoothly
   - Type selector should change filter behavior
   - Enable/disable button should bypass band

4. **GUI Responsiveness**:
   - Frequency response graph should update
   - All controls should be responsive
   - No stuttering or lag

5. **Stability**:
   - Run 1+ hours of continuous audio
   - No CPU spikes or audio dropouts
   - No memory leaks

### Automated Testing

Create a test script `test_plugin.cpp`:
```cpp
// Run sine sweep from 20Hz to 20kHz
// Measure output RMS at each frequency
// Verify response matches expected curve
// Report CPU usage
```

---

## Development Workflow

### Making Changes

1. **Edit Source Files**:
   ```bash
   nano Source/DSP/BiQuadFilter.h
   # or use your IDE
   ```

2. **Rebuild**:
   ```bash
   cd /workspaces/VST-equalizer/build
   cmake --build . --config Release
   ```

3. **Reinstall**:
   ```bash
   # Copy new binary to VST3 directory
   cp -r build/VSTEqualizer_artefacts/Release/VST3/*.vst3 ~/.vst3/
   ```

4. **Test in DAW**:
   - Rescan plugins or restart DAW
   - Load updated plugin
   - Verify changes

### Adding New Features

**Example: Add Mid/Side Processing**

1. Create new header:
   ```bash
   touch Source/DSP/MidSideProcessor.h
   ```

2. Implement class in header

3. Add to CMakeLists.txt:
   ```cmake
   target_sources(VSTEqualizer PRIVATE
       Source/DSP/MidSideProcessor.h
   )
   ```

4. Integrate in PluginProcessor:
   ```cpp
   #include "DSP/MidSideProcessor.h"
   MidSideProcessor msProcessor;
   ```

5. Rebuild and test

---

## Performance Profiling

### Using Xcode Instruments (macOS)

1. Open plugin in DAW
2. Product → Profile (Cmd+I)
3. Select "System Trace" or "CPU Sampler"
4. Run audio through plugin
5. Analyze results

### Using Linux perf

```bash
perf record -F 99 -p $(pgrep -f "daw_binary")
perf report
```

### CPU Usage Monitoring

- **macOS**: Activity Monitor (% CPU column)
- **Linux**: `top` or `htop`
- **Windows**: Task Manager (Performance tab)

Target: <2% CPU per instance on 4-core system

---

## Documentation

### Code Comments

Each function should have a doc comment:
```cpp
/**
 * Filter the input signal with all bands in series
 * @param buffer Audio buffer to process (modified in-place)
 * @param sample_count Number of samples to process
 * @note Real-time safe, no allocs
 */
void process(float* buffer, int sample_count);
```

### Parameter Documentation

ALL parameters must be documented:
```cpp
params.push_back(std::make_unique<juce::AudioParameterFloat>(
    "paramID",         // Unique string identifier
    "Display Name",    // Human-readable name
    range,             // Allowed range
    defaultValue,      // Initial value
    "dB"               // Unit suffix
));
```

---

## Distribution

### Creating a Plugin Archive

For macOS:
```bash
cd /workspaces/VST-equalizer/build/VSTEqualizer_artefacts/Release/VST3
tar -czf VST-Equalizer-macOS.tar.gz VSTEqualizer.vst3
```

For Windows:
```powershell
Compress-Archive -Path "build\VSTEqualizer_artefacts\Release\VST3\*" -DestinationPath "VST-Equalizer-Windows.zip"
```

For Linux:
```bash
tar -czf VST-Equalizer-linux.tar.gz build/VSTEqualizer_artefacts/Release/VST3/*
```

---

## Additional Resources

- **JUCE Forum**: https://forum.juce.com/
- **JUCE Documentation**: https://docs.juce.com/master/index.html
- **VST3 SDK**: https://github.com/steinbergmedia/vst3sdk
- **Audio DSP**: https://www.dsprelated.com/
- **Filter Design**: https://www.earlevel.com/

---

## Summary

You now have:
1. ✓ Complete source code (7 files)
2. ✓ CMake build system
3. ✓ Full documentation
4. ✓ Build instructions for all platforms
5. ✓ Plugin ready to compile and distribute

Next steps:
1. Install dependencies (CMake, JUCE v7.0.9)
2. Run build commands
3. Install to DAW plugin directory
4. Test in your favorite DAW
5. Enjoy your professional EQ! 🎵
