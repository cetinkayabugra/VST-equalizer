#pragma once

#include <JuceHeader.h>
#include "ParameterIDs.h"

//==============================================================================
/**
 * VSTEqualizerAudioProcessor
 *
 * Main audio processor responsible for:
 *   - Parameter layout and state management (via AudioProcessorValueTreeState)
 *   - DSP filter chain preparation and processing
 *   - Coefficient update logic for all EQ bands
 *   - Frequency response computation for the editor's display curve
 */
class VSTEqualizerAudioProcessor : public juce::AudioProcessor,
                                   private juce::AudioProcessorValueTreeState::Listener
{
public:
    VSTEqualizerAudioProcessor();
    ~VSTEqualizerAudioProcessor() override;

    //==========================================================================
    // AudioProcessor interface
    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==========================================================================
    // Editor
    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    // Plugin identity
    //==========================================================================
    const juce::String getName() const override               { return JucePlugin_Name; }
    bool acceptsMidi() const override                         { return false; }
    bool producesMidi() const override                        { return false; }
    bool isMidiEffect() const override                        { return false; }
    double getTailLengthSeconds() const override              { return 0.0; }

    //==========================================================================
    // Programs (not used)
    //==========================================================================
    int  getNumPrograms() override                                         { return 1; }
    int  getCurrentProgram() override                                      { return 0; }
    void setCurrentProgram (int) override                                  {}
    const juce::String getProgramName (int) override                       { return {}; }
    void changeProgramName (int, const juce::String&) override             {}

    //==========================================================================
    // State persistence
    //==========================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    // Public API for the editor
    //==========================================================================

    /**
     * Computes the combined EQ magnitude response (in dB) at each frequency
     * in the supplied vector.  Thread-safe for editor repaints – reads only
     * current parameter values, does not touch filter state.
     */
    std::vector<double> getFrequencyResponse (const std::vector<double>& frequencies) const;

    /** APVTS exposed to the editor for attachments and parameter reads. */
    juce::AudioProcessorValueTreeState apvts;

private:
    //==========================================================================
    // Parameter layout factory
    //==========================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==========================================================================
    // DSP helpers
    //==========================================================================

    /** Re-compute and install coefficients for all bands from current parameters. */
    void updateFilters();

    /**
     * Build a coefficient object for the given band from the current APVTS
     * parameter values.  Returns nullptr if the band is disabled (caller
     * should skip processing for that band).
     */
    juce::dsp::IIR::Coefficients<float>::Ptr makeCoefficientsForBand (int bandIndex) const;

    /** Called by APVTS on the message thread when any parameter changes. */
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    //==========================================================================
    // State
    //==========================================================================

    /// One stereo pair of filters per band (index 0 = L, 1 = R).
    using StereoFilter = std::array<juce::dsp::IIR::Filter<float>, 2>;
    std::array<StereoFilter, NUM_BANDS> filters;

    /// Set to true whenever a parameter changes; cleared after coefficients are updated.
    std::atomic<bool> filtersDirty { true };

    double currentSampleRate { 44100.0 };
    int    currentBlockSize  { 512 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VSTEqualizerAudioProcessor)
};
