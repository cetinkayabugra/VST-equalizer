#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
 * FrequencyResponseComponent
 *
 * Custom component that draws the combined EQ magnitude response curve
 * over a standard frequency-vs-dB grid. Repaints at ~30 fps via a Timer.
 */
class FrequencyResponseComponent : public juce::Component,
                                   private juce::Timer
{
public:
    explicit FrequencyResponseComponent (VSTEqualizerAudioProcessor& p);
    ~FrequencyResponseComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    /// Map a frequency (Hz) to an x-pixel position using a log scale.
    float freqToX (float freq) const noexcept;

    /// Map a dB value to a y-pixel position.
    float dbToY (float db) const noexcept;

    VSTEqualizerAudioProcessor& processor;

    static constexpr int   NUM_POINTS   = 512;  ///< resolution of the curve
    static constexpr float MIN_FREQ_HZ  = 20.0f;
    static constexpr float MAX_FREQ_HZ  = 20000.0f;
    static constexpr float MIN_DB       = -30.0f;
    static constexpr float MAX_DB       = 30.0f;

    std::vector<double> freqPoints;  ///< pre-computed frequency axis

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyResponseComponent)
};

//==============================================================================
/**
 * BandControlComponent
 *
 * A single row of controls for one EQ band:
 *   [Enable] [Type] [Freq] [Gain] [Q]
 */
class BandControlComponent : public juce::Component
{
public:
    BandControlComponent (VSTEqualizerAudioProcessor& p, int bandIndex);
    ~BandControlComponent() override;

    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    VSTEqualizerAudioProcessor& processor;
    const int band;

    // Controls
    juce::ToggleButton  enableButton;
    juce::ComboBox      typeCombo;
    juce::Slider        freqSlider;
    juce::Slider        gainSlider;
    juce::Slider        qSlider;

    // Labels
    juce::Label freqLabel, gainLabel, qLabel, bandLabel;

    // APVTS attachments
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    std::unique_ptr<ButtonAttachment> enableAttach;
    std::unique_ptr<ComboAttachment>  typeAttach;
    std::unique_ptr<SliderAttachment> freqAttach;
    std::unique_ptr<SliderAttachment> gainAttach;
    std::unique_ptr<SliderAttachment> qAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BandControlComponent)
};

//==============================================================================
/**
 * VSTEqualizerAudioProcessorEditor
 *
 * Top-level editor window containing:
 *   - FrequencyResponseComponent  (top section)
 *   - BandControlComponent × NUM_BANDS  (band rows)
 *   - Bypass toggle + Output Gain slider  (bottom bar)
 */
class VSTEqualizerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VSTEqualizerAudioProcessorEditor (VSTEqualizerAudioProcessor&);
    ~VSTEqualizerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VSTEqualizerAudioProcessor& audioProcessor;

    // Frequency response display
    FrequencyResponseComponent responseDisplay;

    // Per-band controls
    std::array<std::unique_ptr<BandControlComponent>, NUM_BANDS> bandControls;

    // Global controls
    juce::ToggleButton bypassButton;
    juce::Slider       outputGainSlider;
    juce::Label        outputGainLabel;

    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttach;
    std::unique_ptr<SliderAttachment> outputGainAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VSTEqualizerAudioProcessorEditor)
};
