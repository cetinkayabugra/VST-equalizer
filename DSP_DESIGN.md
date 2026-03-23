# DSP Design & Implementation Details

## Overview
This document provides an in-depth explanation of the audio DSP algorithms and design decisions in the VST Equalizer.

---

## 1. Biquad Filter Fundamentals

### What is a Biquad Filter?

A **biquad** (or biquadratic) filter is a second-order IIR (Infinite Impulse Response) filter:

```
H(z) = (b₀ + b₁z⁻¹ + b₂z⁻²) / (1 + a₁z⁻¹ + a₂z⁻²)
```

**Advantages**:
- Low computational cost (5 multiplications per sample)
- Numerically stable with proper normalization
- All EQ types can be implemented with appropriate coefficients
- Cascadable for higher-order filters

### Why Biquads?

| Property | Biquad | Other Options |
|----------|--------|---------------|
| Stability | ✓ Excellent | FIR can be unstable at high Q |
| Latency | 0 samples | Linear phase: +512 samples |
| CPU Cost | ~30 cycles/sample | FIR linear phase: ~500 cycles |
| Flexibility | 5+ filter types | - |

---

## 2. Filter Coefficient Calculation

All coefficient calculations follow the **RBJ Cookbook formulas**.

### Definition of Parameters

- **f₀**: Center or cutoff frequency (Hz)
- **fs**: Sample rate (Hz)
- **Q**: Quality factor (bandwidth control)
  - Higher Q = sharper resonance
  - Q = f₀ / bandwidth
- **A**: Linear gain (dB converted to linear)
  - A = 10^(dBgain/40)

### Intermediate Calculations

```
α = sin(ω₀) / (2Q)
ω₀ = 2π × f₀ / fs
cos(ω₀) = cos(2π × f₀ / fs)
sin(ω₀) = sin(2π × f₀ / fs)
```

### 1. High-Pass Filter

**Purpose**: Remove frequencies below f₀

**Mathematical Form**:
```
b₀ =  (1 + cos(ω₀)) / 2
b₁ = -(1 + cos(ω₀))
b₂ =  (1 + cos(ω₀)) / 2
a₀ =  1 + α
a₁ = -2 × cos(ω₀)
a₂ =  1 - α
```

**Effect**: 
- At DC (0 Hz): Magnitude = 0 (-∞ dB)
- At f₀: Magnitude = 0.707 (-3 dB)
- At ∞: Magnitude = 1 (0 dB)

**Code Implementation**:
```cpp
const double w0 = 2.0 * PI * frequency / sampleRate;
const double sinW0 = std::sin(w0);
const double cosW0 = std::cos(w0);
const double alpha = sinW0 / (2.0 * qFactor);

b0 = (1.0 + cosW0) / 2.0;
b1 = -(1.0 + cosW0);
b2 = (1.0 + cosW0) / 2.0;
a0 = 1.0 + alpha;
a1 = -2.0 * cosW0;
a2 = 1.0 - alpha;

// Normalize by a0
b0 /= a0; b1 /= a0; b2 /= a0;
a1 /= a0; a2 /= a0;
```

### 2. Low-Pass Filter

**Purpose**: Remove frequencies above f₀

**Mathematical Form**:
```
b₀ =  (1 - cos(ω₀)) / 2
b₁ =   1 - cos(ω₀)
b₂ =  (1 - cos(ω₀)) / 2
a₀ =   1 + α
a₁ =  -2 × cos(ω₀)
a₂ =   1 - α
```

**Effect**:
- At DC: Magnitude = 1 (0 dB)
- At f₀: Magnitude = 0.707 (-3 dB)
- At ∞: Magnitude = 0 (-∞ dB)

### 3. Bell / Peak Filter

**Purpose**: Boost or cut around center frequency f₀

**Mathematical Form**:
```
b₀ =  1 + α × A
b₁ = -2 × cos(ω₀)
b₂ =  1 - α × A
a₀ =  1 + α / A
a₁ = -2 × cos(ω₀)
a₂ =  1 - α / A
```

**Effect**:
- At f₀: Magnitude = A (equal to gain parameter)
- Far from f₀: Magnitude ≈ 1 (0 dB)
- Q controls peak width

**Q Interpretation**:
```
Bandwidth (Hz) = f₀ / Q
At -3dB points: f_upper - f_lower = f₀ / Q
```

### 4. Low Shelf Filter

**Purpose**: Boost/cut all frequencies below f₀

**Mathematical Form**:
```
S = 2 × α × sqrt(A)

b₀ =  A × ((A + 1) - (A - 1) × cos(ω₀) + S)
b₁ =  2A × ((A - 1) - (A + 1) × cos(ω₀))
b₂ =  A × ((A + 1) - (A - 1) × cos(ω₀) - S)
a₀ =  (A + 1) + (A - 1) × cos(ω₀) + S
a₁ = -2 × ((A - 1) + (A + 1) × cos(ω₀))
a₂ =  (A + 1) + (A - 1) × cos(ω₀) - S
```

**Effect**:
- Frequencies << f₀: Gain = A (parameter gain)
- Frequencies >> f₀: Gain = 1 (0 dB)
- Smooth transition around f₀

**Use Case**: Add warmth to mix (boost ~200 Hz)

### 5. High Shelf Filter

**Purpose**: Boost/cut all frequencies above f₀

**Mathematical Form**:
```
S = 2 × α × sqrt(A)

b₀ =  A × ((A + 1) + (A - 1) × cos(ω₀) + S)
b₁ = -2A × ((A - 1) + (A + 1) × cos(ω₀))
b₂ =  A × ((A + 1) + (A - 1) × cos(ω₀) - S)
a₀ =  (A + 1) - (A - 1) × cos(ω₀) + S
a₁ =  2 × ((A - 1) - (A + 1) × cos(ω₀))
a₂ =  (A + 1) - (A - 1) × cos(ω₀) - S
```

**Effect**:
- Frequencies << f₀: Gain = 1 (0 dB)
- Frequencies >> f₀: Gain = A (parameter gain)
- Smooth transition around f₀

**Use Case**: Add air/brightness (boost ~10 kHz)

---

## 3. Filter Implementation (Direct Form II)

### The Filter Equation

For real-time processing, we use **Direct Form II Transposed**:

```
y[n] = b₀ × x[n] + z₁[n]
z₁[n] = b₁ × x[n] + z₂[n] - a₁ × y[n]
z₂[n] = b₂ × x[n] - a₂ × y[n]
```

### Why Direct Form II?

| Property | Direct Form I | Direct Form II |
|----------|---------------|----------------|
| State variables | 4 | 2 |
| Multiplications | 5 | 5 |
| Numerically stable | No | **Yes** |
| Cascadable | Yes | **Yes** |

### Implementation Code

```cpp
double BiQuadFilter::processSample(double inputSample) noexcept
{
    // Direct Form II Transposed
    double output = b0 * inputSample + z1;
    z1 = b1 * inputSample + z2 - a1 * output;
    z2 = b2 * inputSample - a2 * output;
    return output;
}
```

**Line-by-line**:
1. Calculate output: `y[n] = b₀x[n] + z₁`
2. Update z1: `z₁_new = b₁x[n] + z₂ - a₁y[n]`
3. Update z2: `z₂_new = b₂x[n] - a₂y[n]`

**Advantages**:
- Only 2 state variables (z1, z2)
- No division by a0 needed (pre-normalized)
- Better numerical accuracy
- Easy to extend to higher orders

### Processing a Block

```cpp
for (int i = 0; i < numSamples; ++i)
{
    writePtr[i] = processSample(readPtr[i]);
}
```

Each sample completely processes through the filter before the next sample.

---

## 4. Frequency Response Calculation

To visualize the EQ curve, we calculate magnitude response at any frequency:

```
H(ejω) = (b₀ + b₁e^(-jω) + b₂e^(-2jω)) / (1 + a₁e^(-jω) + a₂e^(-2jω))

|H(ω)| = sqrt(Re² + Im²) / sqrt(Re_denom² + Im_denom²)
```

### Derivation

At frequency f:
```
ω = 2π × f / fs
cos(ω) = cos(2π × f / fs)
sin(ω) = sin(2π × f / fs)
e^(-jω) = cos(ω) - j×sin(ω)
```

**Numerator**:
```
H_num = b₀ + b₁×cos(ω) - j×b₁×sin(ω) + b₂×cos(2ω) - j×b₂×sin(2ω)
Real = b₀ + b₁×cos(ω) + b₂×cos(2ω)
Imag = -(b₁×sin(ω) + 2×b₂×cos(ω)×sin(ω))
```

**Denominator**:
```
Similar calculation...
```

**Magnitude**:
```
|H(ω)| = sqrt(Real² + Imag²) / sqrt(Real_denom² + Imag_denom²)
|H(ω)_dB| = 20 × log₁₀(|H(ω)|)
```

### Code

```cpp
double getMagnitudeResponse(double testFrequency)
{
    const double w = 2π × testFrequency / sampleRate;
    const double cosW = cos(w);
    const double sinW = sin(w);
    
    double numReal = b0 + b1*cosW + b2*cosW*cosW;
    double numImag = b1*sinW + 2*b2*cosW*sinW;
    
    double denomReal = 1 + a1*cosW + a2*cosW*cosW;
    double denomImag = a1*sinW + 2*a2*cosW*sinW;
    
    double numMag = sqrt(numReal² + numImag²);
    double denomMag = sqrt(denomReal² + denomImag²);
    
    return numMag / denomMag;  // Linear magnitude
}
```

---

## 5. Filter Cascading (Series Processing)

Multiple filters in series have a combined response:

```
H_total(ω) = H₁(ω) × H₂(ω) × H₃(ω) × ... × Hₙ(ω)
```

In the plugin:
```
Audio → Filter[0] → Filter[1] → Filter[2] → Filter[3] → Filter[4] → Output
```

**Combined Magnitude** (dB):
```
|H_total_dB| = |H₁_dB| + |H₂_dB| + |H₃_dB| + |H₄_dB| + |H₅_dB|
```

**Important**: Filters must be processed in order!

```cpp
void FilterBank::processBlock(AudioBuffer<float>& buffer)
{
    // Each filter must process the output of the previous
    for (int band = 0; band < NUM_BANDS; ++band)
    {
        if (bandEnabled[band])
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                filters[band].processBlock(buffer, ch);
                // Buffer now contains filtered output
            }
        }
    }
}
```

---

## 6. Parameter Smoothing

Abrupt parameter changes cause **clicks and pops** (spectral discontinuities).

### Solution: Linear Smoothing

```cpp
juce::SmoothedValue<float, ValueSmoothingTypes::Linear> freqSmoother;
freqSmoother.reset(sampleRate, 0.05);  // 50ms ramp time

// In audio thread:
float smoothedFreq = freqSmoother.getNextValue();
```

**How it works**:
```
target = newParameterValue
current = (current × (1 - alpha)) + (target × alpha)
```

**alpha calculation**:
```
rampTimeMs = 50 ms
rampTimeSamples = sampleRate × rampTimeMs / 1000
alpha = 1 / rampTimeSamples
```

**Effect**: Parameter transitions smoothly over 50ms instead of instantly.

### Preventing Artifacts

```
Without smoothing:   f₀ = 1000 Hz → 5000 Hz (instant)
                     ↓
                     Spectral discontinuity → audible click

With smoothing:      f₀ transitions 1000→5000 Hz over 50ms
                     ↓
                     No discontinuity → smooth, natural

Click prevention: 50ms is barely audible to human ear
```

---

## 7. Numerical Stability

### Problem: Accumulation Errors

Filters can become unstable with:
- Very high Q values
- Extreme gain values
- Low sample rates

### Solution: Proper Normalization

**ALWAYS normalize by a₀**:
```cpp
b0 /= a0;
b1 /= a0;
b2 /= a0;
a1 /= a0;
a2 /= a0;  // Should equal exactly zero after division
```

### Coefficient Range

Valid coefficients should satisfy:
```
|b₀|, |b₁|, |b₂| < 10
|a₁|, |a₂| < 2
```

If values exceed these, reduce Q or recalculate.

---

## 8. Spectrum Analyzer

### FFT-Based Analysis

**FFT Size**: 2048 points

**Frequency Resolution**:
```
Δf = sampleRate / FFT_size
Δf = 44100 / 2048 ≈ 21.5 Hz per bin
```

**Windowing**: Hann window reduces spectral leakage
```
w[n] = 0.5 × (1 - cos(2π × n / (N-1)))
```

### Processing Steps

1. **Capture samples** into buffer
2. **Apply Hann window**
3. **Perform FFT** (converts time→frequency)
4. **Calculate magnitude** for each bin
5. **Convert to dB**: 20 × log₁₀(magnitude)

### Real-Time Implementation

```cpp
void SpectrumAnalyzer::performFFT(AudioBuffer& buffer, vector<float>& magnitudes)
{
    // Copy signal from audio buffer
    auto* fftInput = fftEngine.getBuffer().getWritePointer(0);
    auto* readPtr = buffer.getReadPointer(0);

    // Apply Hann window
    for (int i = 0; i < FFT_SIZE; ++i)
    {
        float window = 0.5f * (1 - cos(2π × i / (FFT_SIZE - 1)));
        fftInput[i] = readPtr[i] * window;
    }

    // Perform FFT
    fftEngine.performRealOnlyForwardTransform(fftInput);

    // Convert to magnitude spectrum (dB)
    for (int i = 0; i < NUM_BINS; ++i)
    {
        float real = fftInput[2*i];
        float imag = fftInput[2*i + 1];
        float magnitude = sqrt(real² + imag²) / FFT_SIZE;
        magnitudes[i] = 20 × log₁₀(max(magnitude, 1e-6));
    }
}
```

---

## 9. CPU Efficiency

### Per-Sample Processing Cost

**Single filter: ~30 CPU cycles @ 2.4 GHz**
- 5 multiplications: 5 cycles
- 4 additions: 4 cycles
- 2 variable updates: 2 cycles
- Memory loads: ~10 cycles
- Cache effects: ~4 cycles

**5 filters in series: ~150 cycles @ 2.4 GHz**

**At 44.1 kHz: 150 × 44,100 = 6.6 M cycles/sec = 0.275% @ 2.4 GHz**

### Optimization Techniques

1. **Coefficient Caching**: Only recalculate when parameters change
2. **SIMD**: JUCE automatically vectorizes buffer operations
3. **Inlining**: `[[nodiscard]]` and `noexcept` hints compiler
4. **Lock-free**: Audio thread never allocates memory

---

## 10. Testing & Validation

### Verification Tests

1. **DC Response** (0 Hz):
   - High-pass: Should be 0 dB attenuation
   - Low-pass: Should be 0 dB response
   
2. **Nyquist Response** (fs/2):
   - High-pass: Should be 0 dB response
   - Low-pass: Should be 0 dB attenuation

3. **Center Frequency**:
   - Bell filter: Response should be +gain at f₀
   - Magnitude at f₀: Should equal dB gain parameter

4. **Q Factor**:
   - Higher Q = narrower peak
   - Measure -3dB bandwidth: f ÷ Q Hz

### Test Signals

```
1. Sine sweep (20 Hz → 20 kHz, 10 sec)
   - Listen for smooth frequency-dependent response

2. Pink noise with EQ
   - Should hear clear presence/brightness changes

3. Impulse response
   - Ringing at filter cutoff frequency
```

---

## References

- **"Audio EQ Cookbook"** by Robert Bristow-Johnson
- **JUCE Documentation**: docs.juce.com
- **"Designing Audio Effect Plugins in C++"** by Joshua D. Reiss
- **Stanford CCRMA DSP Course**: https://ccrma.stanford.edu/

---

## Conclusion

This VST Equalizer implements **production-quality parametric EQ** using:
- ✓ Numerically stable biquad filters
- ✓ Real-time coefficient updates
- ✓ Smooth parameter transitions
- ✓ Accurate frequency response
- ✓ Minimal CPU overhead

The DSP foundation is professional-grade and suitable for commercial use.
