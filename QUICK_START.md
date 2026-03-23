# VST Equalizer Quick Start Guide

## 30-Second Setup

```bash
# 1. Get JUCE
cd /workspaces && git clone https://github.com/juce-framework/JUCE.git
cd JUCE && git checkout 7.0.9

# 2. Build plugin
cd /workspaces/VST-equalizer
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel 4

# 3. Install to DAW
# Linux/macOS:
mkdir -p ~/.vst3
cp -r build/VSTEqualizer_artefacts/Release/VST3/*.vst3 ~/.vst3/

# Windows:
Copy-Item build\VST3\Release\* -Destination "C:\Program Files\Common Files\VST3" -Recurse
```

---

## First Time Loading in DAW

1. **Open your DAW** (Reaper, Studio One, Ableton, etc.)
2. **Rescan plugins**:
   - Reaper: Options → Show REAPER menu → Tools → Rescan Plugin Paths
   - Studio One: Studio One > Options > Locations > Plug-in Manager > Rescan
   - Ableton: Preferences > Plug-in / MIDI
3. **Create audio track**
4. **Insert "VST Equalizer" as effect**
5. **Play audio** and adjust knobs!

---

## GUI Tour (3 minutes)

### Top Section: Frequency Response Graph
- **X-axis**: Frequency (20 Hz to 20 kHz, logarithmic)
- **Y-axis**: Gain (-24 dB to +24 dB, linear)
- **Grid lines**: Help read exact values
- **Blue curve**: Your EQ curve (updates in real time)
- **Flat analyzer**: Shows output spectrum (gray)

### Left Column: Global Gain
- **Input Gain**: Pre-EQ level (-24 to +24 dB)
- **Output Gain**: Post-EQ level (-24 to +24 dB)

### 5 Band Sections (left to right)

Each band has the same layout:

```
┌─────────────┐
│    Band 1   │
├─────────────┤
│ [Type ▼]    │  ← Select filter type (High-Pass, Low-Pass, etc.)
│ [ON/OFF]    │  ← Enable/disable this band
├─────────────┤
│   ◯   Freq  │  ← Frequency knob (logarithmic)
│   ◯   Gain  │  ← Gain knob (-24 to +24 dB)
│   ◯   Q     │  ← Q/bandwidth knob (0.1 to 10)
└─────────────┘
```

---

## Common Mixing Tasks

### Vocal Clarity (Pro Tip ⭐)

```
Band 1 (High-Pass):
  Type: High-Pass
  Freq: 80 Hz
  Q: 0.7
  → Effect: Removes mud and rumble below 80 Hz

Band 3 (Bell):
  Type: Bell
  Freq: 3 kHz
  Gain: +4 dB
  Q: 1.5
  → Effect: Adds presence/clarity to voice

Band 5 (High-Shelf):
  Type: High-Shelf
  Freq: 10 kHz
  Gain: +2 dB
  Q: 0.8
  → Effect: Adds air and brightness
```

### Bass Punch

```
Band 2 (Low-Shelf):
  Type: Low-Shelf
  Freq: 100 Hz
  Gain: +3 dB
  Q: 0.9
  → Effect: Boost low frequencies for punch

Band 3 (Bell):
  Type: Bell
  Freq: 60 Hz
  Gain: +2 dB
  Q: 0.7
  → Effect: Add sub-bass weight
```

### De-Essing Vocals

```
Band 4 (Bell):
  Type: Bell
  Freq: 6 kHz
  Gain: -3 dB
  Q: 2.0
  → Effect: Reduces harshness/sibilance
```

### Master Bus Gloss

```
Band 2 (Low-Shelf):
  Type: Low-Shelf
  Freq: 200 Hz
  Gain: +1 dB
  Q: 0.9
  → Effect: Slight warmth

Band 5 (High-Shelf):
  Type: High-Shelf
  Freq: 8 kHz
  Gain: +1 dB
  Q: 0.8
  → Effect: Slight sparkle
```

---

## Parameter Reference

### Frequency
- **Range**: 20 Hz - 20,000 Hz
- **Type**: Logarithmic (musical intervals)
- **Typical values**: 50, 100, 250, 500, 1k, 2k, 5k, 10k, 15k Hz

### Gain
- **Range**: -24 dB to +24 dB
- **Type**: Linear  
- **0 dB**: No change
- **+6 dB**: Double amplitude (~2x louder)
- **-6 dB**: Half amplitude (~2x quieter)

### Q (Bandwidth)
- **Range**: 0.1 to 10.0
- **Lower Q** (0.1-1.0): Wide, gentle curve (shelving filters)
- **Middle Q** (1.0-3.0): Moderate curve (general use)
- **Higher Q** (3.0-10.0): Narrow, surgical cuts/boosts

### Filter Types

1. **High-Pass**
   - Removes frequencies BELOW cutoff
   - Use: Remove sub-bass rumble

2. **Low-Pass**
   - Removes frequencies ABOVE cutoff
   - Use: Smooth out harsh highs

3. **Bell (Peak)**
   - Boost or cut AT center frequency
   - Narrowness controlled by Q
   - Use: General tone shaping

4. **Low-Shelf**
   - Boost or cut ALL frequencies BELOW cutoff
   - Smooth, non-resonant
   - Use: Add warmth/bass

5. **High-Shelf**
   - Boost or cut ALL frequencies ABOVE cutoff
   - Smooth, non-resonant
   - Use: Add air/brightness

---

## Keyboard Shortcuts

| Action | Shortcut |
|--------|----------|
| Reset plugin | (None - manually reset sliders) |
| Undo parameter | Ctrl/Cmd+Z (DAW undo) |
| Full reset | Click band ON button to re-enable |
| Type change | Click dropdown, select type |

---

## Troubleshooting

### "Plugin not appearing in DAW"

1. Check installation directory:
   - macOS: `ls ~/Library/Audio/Plug-Ins/VST3/`
   - Linux: `ls ~/.vst3/`
   - Windows: `dir C:\Program Files\Common Files\VST3\`

2. Verify file permissions (Linux/macOS):
   ```bash
   chmod -R 755 ~/.vst3/VSTEqualizer.vst3
   ```

3. Rescan plugins in your DAW

### "Audio doesn't change when adjusting"

1. Check Input Gain isn't muted (-∞ dB)
2. Verify band is enabled (ON button lit)
3. Check Output Gain isn't at -∞ dB
4. Try extreme settings (Freq=20Hz, Gain=+24dB) to verify

### "Hearing digital artifacts/crackling"

1. Lower gain (reduce amplitude)
2. Reduce Q factor (wider bandwidth)
3. Check DAW buffer size (lower = more CPU)

### "High CPU usage"

1. Reduce number of active bands (disable unused ones)
2. Increase DAW buffer size
3. On Linux: Check JACK configuration

---

## Advanced Tips

### Layering Multiple Instances

Use multiple EQ instances for complex shaping:
```
Track 1: Insert EQ #1 (gentle low-pass at 12 kHz)
Track 1: Insert EQ #2 (sharp bell cut at 500 Hz)
```

### Parallel EQ

In your DAW:
1. Duplicate track (or use send)
2. Insert EQ on duplicate
3. Mix wet (EQ'd) with dry (unprocessed)
4. Adjust blend for tonal sweetness

### Automating Parameters

Most DAWs support automation:
1. Select plugin parameter
2. Enter automation record mode
3. Adjust knobs during playback
4. DAW records the curve

TIP: Automate Gain +2 dB during chorus for energy!

---

## File Locations

### Plugin Binary
- **macOS**: `~/.vst3/VSTEqualizer.vst3/Contents/MacOS/VSTEqualizer`
- **Linux**: `~/.vst3/VSTEqualizer.vst3/VSTEqualizer.so`
- **Windows**: `C:\Program Files\Common Files\VST3\VSTEqualizer.vst3`

### Source Code
- **All controls**: `Source/PluginEditor.cpp`
- **DSP engine**: `Source/DSP/BiQuadFilter.h`
- **Parameters**: `Source/PluginProcessor.cpp` line ~40

---

## Support & Resources

### Recommended Learning
- **EQ Basics**: YouTube "Parametric EQ Tutorial"
- **Mixing**: "Music Production with Reaper" course
- **DSP**: Stanford CCRMA (ccrma.stanford.edu)

### Troubleshooting Links
- **JUCE Forum**: forum.juce.com
- **VST3 Spec**: https://steinbergmedia.github.io/vst3_dev_portal/
- **This Project**: Check BUILDING_GUIDE.md for detailed setup

---

## Next Steps

1. ✓ Install plugin
2. ✓ Load in your DAW
3. ✓ Try the mixing examples above
4. ✓ A/B test before/after
5. ✓ Enjoy professional-grade EQ!

---

**Happy mixing!** 🎵 If you want to customize or extend the plugin, check `PluginEditor.h` for UI changes or `DSP/BiQuadFilter.h` for algorithm modifications.
