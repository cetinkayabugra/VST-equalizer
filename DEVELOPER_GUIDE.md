# Developer Guide & Extension Points

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│  AudioHost (DAW: Reaper, Studio One, etc.)                  │
├─────────────────────────────────────────────────────────────┤
│                     VST3 Plugin Interface                    │
├─────────────────────────────────────────────────────────────┤
│  PluginProcessor (PluginProcessor.h/.cpp)                   │
│  ├─ prepareToPlay()      : Prepare audio engine             │
│  ├─ processBlock()       : Main audio callback @ 44.1kHz    │
│  └─ parameterChanged()   : UI updates → DSP params          │
├─────────────────────────────────────────────────────────────┤
│  PluginEditor (PluginEditor.h/.cpp)                         │
│  ├─ FrequencyResponseComponent : Visualize EQ curve         │
│  ├─ BandControlComponent : 5× per-band controls            │
│  └─ Timer : Update UI @ 100 Hz                             │
├─────────────────────────────────────────────────────────────┤
│  DSP Engine                                                  │
│  ├─ FilterBank (DSP/FilterBank.h)                          │
│  │  └─ 5× BiQuadFilter                                     │
│  │     ├─ setParameters() : Calculate coefficients          │
│  │     └─ processSample() : Process sample (noexcept)       │
│  └─ SpectrumAnalyzer (UI/SpectrumAnalyzer.h)              │
│     ├─ FFT Engine (2048 points)                            │
│     └─ Magnitude → dB conversion                            │
└─────────────────────────────────────────────────────────────┘
```

---

## File Organization

### Source/
- **PluginProcessor.h/cpp**: Audio engine, parameter management
- **PluginEditor.h/cpp**: GUI components and interaction

### Source/DSP/
- **BiQuadFilter.h**: Single filter implementation (header-only)
- **FilterBank.h**: 5-band filter management (header-only)

### Source/UI/
- **SpectrumAnalyzer.h**: Real-time FFT analysis (header-only)

---

## Adding New Features

### 1. Adding a 6th EQ Band

**Step 1**: Modify FilterBank.h
```cpp
class FilterBank {
    static constexpr int NUM_BANDS = 6;  // ← Change from 5 to 6
    std::array<BiQuadFilter, NUM_BANDS> filters;
    // Rest of class...
};
```

**Step 2**: Update PluginProcessor.cpp parameter layout
```cpp
juce::AudioProcessorValueTreeState::ParameterLayout 
VSTEqualizerAudioProcessor::createParameterLayout()
{
    // ... existing params ...
    
    // Change loop:
    for (int i = 0; i < DSP::FilterBank::NUM_BANDS; ++i)  // Now 6
    {
        // Existing band setup...
    }
}
```

**Step 3**: Update PluginEditor.h UI
```cpp
class VSTEqualizerAudioProcessorEditor {
    std::array<BandControlComponent, DSP::FilterBank::NUM_BANDS> bandControls;
    // Will automatically create 6 instances
};
```

**Step 4**: Recompile and test
```bash
cd build
cmake --build . --config Release
```

---

### 2. Implementing Mid/Side EQ

**Purpose**: Process mid (L+R) and side (L-R) channels separately

**Step 1**: Create new file `Source/DSP/MidSideProcessing.h`
```cpp
namespace DSP {
    class MidSideProcessor {
    public:
        void encodeStereo(const float* left, const float* right,
                         float* mid, float* side, int numSamples) {
            for (int i = 0; i < numSamples; ++i) {
                mid[i] = (left[i] + right[i]) * 0.5f;
                side[i] = (left[i] - right[i]) * 0.5f;
            }
        }
        
        void decodeStereo(const float* mid, const float* side,
                         float* left, float* right, int numSamples) {
            for (int i = 0; i < numSamples; ++i) {
                left[i] = mid[i] + side[i];
                right[i] = mid[i] - side[i];
            }
        }
    };
}
```

**Step 2**: Update PluginProcessor.h
```cpp
class VSTEqualizerAudioProcessor {
private:
    // New member
    DSP::MidSideProcessor msProcessor;
    
    // Configuration parameter
    juce::AudioParameterBool* midSideEnabledParam;
};
```

**Step 3**: Update PluginProcessor.cpp processBlock()
```cpp
void VSTEqualizerAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    
    if (midSideEnabledParam->get()) {
        // M/S mode
        msProcessor.encodeStereo(
            buffer.getReadPointer(0),
            buffer.getReadPointer(1),
            /* process mid and side separately */
        );
        filterBank.processBlock(buffer);  // Now processes M and S
        msProcessor.decodeStereo(/* convert back to L/R */);
    } else {
        // Normal stereo
        filterBank.processBlock(buffer);
    }
}
```

---

### 3. Adding Linear Phase Option

**Purpose**: Zero phase distortion (but with latency)

**Step 1**: Create `Source/DSP/LinearPhaseEQ.h`
```cpp
namespace DSP {
    class LinearPhaseEQ {
    public:
        static constexpr int LATENCY = 512;  // samples
        
        void processBlock(AudioBuffer<float>& buffer,
                         const FilterBank& filterBank) {
            // 1. Create FIR approximation of filter response
            // 2. Process with FFT convolution
            // 3. Add latency compensation
        }
    };
}
```

**Step 2**: Add parameter to PluginProcessor
```cpp
params.push_back(std::make_unique<juce::AudioParameterBool>(
    "linearPhaseMode", "Linear Phase Mode", false));
```

**Step 3**: Update latency reporting
```cpp
int VSTEqualizerAudioProcessor::getLatencySamples() const override {
    auto param = apvts.getRawParameterValue("linearPhaseMode");
    return param->load() ? DSP::LinearPhaseEQ::LATENCY : 0;
}
```

---

### 4. Adding Preset System

**Option A**: Use JUCE's ValueTreeState (built-in)
```cpp
// Automatically saves/loads state:
void getStateInformation(MemoryBlock& destData) override {
    auto state = apvts.copyValueTreeToXml(true);
    copyXmlToBinary(*state, destData);
}

void setStateInformation(const void* data, int sizeInBytes) override {
    auto xmlState = getXmlFromBinary(data, sizeInBytes);
    apvts.replaceValueTreeFromXml(*xmlState);
}
```

**Option B**: Custom preset browser UI
```cpp
class PresetBrowserComponent : public Component {
private:
    // List of presets
    juce::StringArray presetNames{"Bright", "Dark", "Warm"};
    
    // Save/Load methods
    void savePreset(const juce::String& name) {
        auto xml = apvts.copyValueTreeToXml(true);
        xml->writeTo(File(name + ".xml"));
    }
    
    void loadPreset(const juce::String& name) {
        auto xml = std::make_unique<XmlElement>(*
            XmlDocument::parse(File(name + ".xml")));
        apvts.replaceValueTreeFromXml(*xml);
    }
};
```

---

### 5. Adding GPU-Accelerated FFT Analyzer

**Current**: CPU-based FFT with JUCE

**Future**: GPU acceleration using OpenGL

**Step 1**: Create GPU FFT shader
```glsl
// fragment_shader.glsl
#version 330 core
uniform sampler2D audioBuffer;
out vec4 fragColor;

void main() {
    // GPU FFT computation
    vec4 fft_result = texture(audioBuffer, gl_FragCoord.xy);
    fragColor = fft_result;
}
```

**Step 2**: Update SpectrumAnalyzer
```cpp
class SpectrumAnalyzer {
private:
    #ifdef USE_GPU_FFT
        OpenGLContext glContext;
        GLShader fftShader;
    #else
        juce::dsp::FFT fftEngine;
    #endif
};
```

---

## Code Quality Guidelines

### Naming Conventions

```cpp
// Classes/Structs: PascalCase
class BiQuadFilter { };
struct AudioConfig { };

// Methods: camelCase
void setParameters();
float getMagnitudeResponse();

// Member variables: camelCase with trailing underscore
float frequency_;
int bandIndex_;

// Constants: UPPER_CASE
constexpr int NUM_BANDS = 5;
constexpr float PI = 3.14159f;

// Private members: prefix with underscore (optional)
bool _enabled;
float _gain;
```

### Documentation Requirements

Every public method needs a doc comment:
```cpp
/**
 * Process a block of audio samples
 * 
 * @param buffer Audio buffer to modify in-place
 * @param channel Which channel (0=left, 1=right)
 * @note This is real-time safe (no allocations)
 * @warning Channel must be < buffer.getNumChannels()
 */
void processBlock(juce::AudioBuffer<float>& buffer, int channel);
```

### Real-Time Safety

Audio thread rules:
```cpp
// ✓ ALLOWED in audio thread
ptr[i] = value;          // Direct memory access
x = a * b + c;           // Math operations
lock.tryLock();          // Non-blocking locks ONLY

// ✗ FORBIDDEN in audio thread
new float[100];          // Memory allocation
delete ptr;              // Deallocation
audioFile.read();        // File I/O
mutex.lock();            // Blocking lock
```

---

## Testing New Features

### Unit Testing Template

```cpp
// test_newfeature.cpp
void testNewFeature() {
    // Setup
    YourNewClass feature;
    feature.initialize(44100);  // Sample rate
    
    // Test
    float testInput = 1.0f;
    float result = feature.process(testInput);
    
    // Verify
    assert(result > 0.0f);
    assert(!isnan(result));
    assert(abs(result - expectedValue) < 0.001f);
    
    std::cout << "✓ Feature test passed\n";
}
```

### Integration Testing

```cpp
void testWithAudio() {
    AudioBuffer<float> buffer(2, 44100);
    
    // Fill with test signal (1kHz sine)
    for (int i = 0; i < 44100; ++i) {
        float phase = (2 * PI * 1000 * i) / 44100;
        float sample = sin(phase);
        buffer.setSample(0, i, sample);
        buffer.setSample(1, i, sample);
    }
    
    // Process
    FilterBank filterBank;
    filterBank.prepare(44100);
    filterBank.processBlock(buffer);
    
    // Verify (should be ~similar level, slightly filtered)
    float inputRMS = calculateRMS(buffer);
    assert(inputRMS > 0.0f);
}
```

---

## Performance Profiling

### Measuring CPU Usage

**Method 1: JUCE Profiler**
```cpp
#include "JUCE/modules/juce_core/measurements/juce_StatisticsAccumulator.h"

juce::StatisticsAccumulator<double> timing;

// In processBlock():
auto start = juce::Time::getHighResolutionTicks();
filterBank.processBlock(buffer);
auto elapsed = juce::Time::getHighResolutionTicksToMilliseconds() 
    * (juce::Time::getHighResolutionTicksPerSecond() / 1000.0);
timing.addValue(elapsed);
```

**Method 2: System Profiler**
- macOS: Instruments (Xcode)
- Linux: perf, valgrind
- Windows: Visual Studio Profiler

### Optimization Checklist

- [ ] Profile code with real audio (pink noise)
- [ ] Identify bottleneck (use profiler)
- [ ] Check for unnecessary allocations
- [ ] Verify coefficient caching working
- [ ] Use #pragma SIMD hints where needed
- [ ] Test with extreme parameters
- [ ] Benchmark on target hardware

---

## Common Pitfalls

### Memory Leaks

```cpp
// ✗ WRONG - Leaks memory
void processBlock(...) {
    auto buffer = new float[1024];  // Never deleted!
    // ...
}

// ✓ CORRECT - Automatic deletion
void processBlock(...) {
    std::array<float, 1024> buffer;  // Stack allocation
    // ...
}  // Automatically freed
```

### Numerical Instability

```cpp
// ✗ WRONG - Can overflow
float magnitude = sqrt(real*real + imag*imag);
if (magnitude == 0.0f) { /* ... */ }  // May never be true!

// ✓ CORRECT - Numerically stable
float magnitude = sqrt(real*real + imag*imag);
if (magnitude < 1e-6f) { /* ... */ }  // Safe comparison
```

### Multiple Allocations

```cpp
// ✗ WRONG - Allocates every parameter change
void setFrequency(float f) {
    delete[] coefficients;
    coefficients = new float[5];  // Leak + slow
    calculateCoefficients(f);
}

// ✓ CORRECT - Pre-allocate once
void prepare() {
    coefficients.resize(5);  // Once during prepare
}

void setFrequency(float f) {
    calculateCoefficients(f);  // Just recalculate, no alloc
}
```

---

## Debugging Tips

### Enable Debug Logging

```cpp
#ifdef DEBUG
    #define DBG_LOG(msg) std::cerr << msg << std::endl;
#else
    #define DBG_LOG(msg)
#endif

// In code:
DBG_LOG("Filter frequency: " << frequency);
```

### Breakpoint in Audio Thread

```cpp
// Reachable from processBlock()
void processBlock(...) {
    // Set IDE breakpoint here
    filterBank.processBlock(buffer);  // ← breakpoint
}
```

### Add Assertions

```cpp
void processSample(float sample) {
    jassert(sample > -2.0f && sample < 2.0f);  // ← Catches bugs
    return filterBank.processSample(sample);
}
```

---

## Future Enhancement Ideas

### High Priority
- [ ] Mid/Side processing mode
- [ ] Preset browser UI
- [ ] Undo/Redo system
- [ ] Parameter smoothing curves (log vs linear)

### Medium Priority
- [ ] Linear phase EQ option
- [ ] Analyzer waterfall display
- [ ] A/B comparison mode
- [ ] Metering (in/out levels)

### Nice To Have
- [ ] GPU acceleration
- [ ] Oversampling for ultra-high frequencies
- [ ] Dynamic EQ (frequency-dependent compression)
- [ ] Spectral shaping with AI suggestions
- [ ] Theme customization

---

## Resources for Developers

- **JUCE API**: https://docs.juce.com
- **Audio DSP**: https://www.dsprelated.com
- **Filter Design**: https://www.earlevel.com
- **CppReference**: https://en.cppreference.com
- **CMake Guide**: https://cmake.org/documentation

---

## Contributing

To add new features:

1. Create a feature branch
2. Implement in feature branch
3. Add unit tests
4. Run full build test
5. Document changes
6. Submit PR with screenshots

---

**Happy developing!** The codebase is designed to be extended. Start with small changes (adjust parameters) and work up to new DSP modules.
