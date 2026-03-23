#pragma once

#include <JuceHeader.h>
#include "DSP/FilterBank.h"
#include "UI/SpectrumAnalyzer.h"

/**
 * VSTEqualizerAudioProcessor - Main audio plugin processor
 * Handles audio I/O, parameter management, and DSP orchestration
 */
class VSTEqualizerAudioProcessor : public juce::AudioProcessor,
                                  public juce::AudioProcessorValueTreeState::Listener
{
public:
    VSTEqualizerAudioProcessor();
    ~VSTEqualizerAudioProcessor() override;

    // Audio processing
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // Editor
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // Plugin info
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // Programs
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    // State management
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Parameter callbacks
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // Audio bus layout
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    // Getters
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    DSP::SpectrumAnalyzer& getAnalyzer() { return analyzer; }

private:
    // Parameter IDs
    static constexpr auto PARAM_OUTPUT_GAIN = "outputGain";
    static constexpr auto PARAM_INPUT_GAIN = "inputGain";

    static constexpr auto PARAM_FREQ_PREFIX = "freq";
    static constexpr auto PARAM_GAIN_PREFIX = "gain";
    static constexpr auto PARAM_Q_PREFIX = "q";
    static constexpr auto PARAM_ENABLED_PREFIX = "enabled";
    static constexpr auto PARAM_TYPE_PREFIX = "type";

    // DSP components
    DSP::FilterBank filterBank;
    DSP::SpectrumAnalyzer analyzer;

    // Parameter state
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Smoothers for parameter transitions
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, DSP::FilterBank::NUM_BANDS> frequencySmoothers;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, DSP::FilterBank::NUM_BANDS> gainSmoothers;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, DSP::FilterBank::NUM_BANDS> qSmoothers;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inputGainSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGainSmoother;

    // State
    double sampleRate = 44100.0;
    int samplesPerBlock = 512;
    bool needsUpdate = true;

    void updateFilterParameters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VSTEqualizerAudioProcessor)
};
