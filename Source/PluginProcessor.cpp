#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VSTEqualizerAudioProcessor::VSTEqualizerAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    // Register as listener so filtersDirty is set whenever any parameter changes.
    // This allows processBlock to skip coefficient recalculation on unchanged blocks.
    apvts.addParameterListener (ParamIDs::bypass,     this);
    apvts.addParameterListener (ParamIDs::outputGain, this);
    for (int i = 0; i < NUM_BANDS; ++i)
    {
        apvts.addParameterListener (ParamIDs::bandEnabled (i), this);
        apvts.addParameterListener (ParamIDs::bandType    (i), this);
        apvts.addParameterListener (ParamIDs::bandFreq    (i), this);
        apvts.addParameterListener (ParamIDs::bandGain    (i), this);
        apvts.addParameterListener (ParamIDs::bandQ       (i), this);
    }
}

VSTEqualizerAudioProcessor::~VSTEqualizerAudioProcessor()
{
    // Remove all listeners before destruction
    apvts.removeParameterListener (ParamIDs::bypass,     this);
    apvts.removeParameterListener (ParamIDs::outputGain, this);
    for (int i = 0; i < NUM_BANDS; ++i)
    {
        apvts.removeParameterListener (ParamIDs::bandEnabled (i), this);
        apvts.removeParameterListener (ParamIDs::bandType    (i), this);
        apvts.removeParameterListener (ParamIDs::bandFreq    (i), this);
        apvts.removeParameterListener (ParamIDs::bandGain    (i), this);
        apvts.removeParameterListener (ParamIDs::bandQ       (i), this);
    }
}

//==============================================================================
// Parameter layout
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
VSTEqualizerAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ----- Global parameters -----------------------------------------------
    layout.add (std::make_unique<juce::AudioParameterBool> (
        ParamIDs::bypass, "Bypass", false));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        ParamIDs::outputGain,
        "Output Gain",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    // ----- Per-band parameters ---------------------------------------------
    for (int i = 0; i < NUM_BANDS; ++i)
    {
        const auto& def = BAND_DEFAULTS[i];

        // Enabled toggle
        layout.add (std::make_unique<juce::AudioParameterBool> (
            ParamIDs::bandEnabled (i),
            "Band " + juce::String (i + 1) + " Enabled",
            def.enabled));

        // Filter type choice
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            ParamIDs::bandType (i),
            "Band " + juce::String (i + 1) + " Type",
            FILTER_TYPE_NAMES,
            static_cast<int> (def.type)));

        // Frequency – logarithmic feel via skewFactor
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            ParamIDs::bandFreq (i),
            "Band " + juce::String (i + 1) + " Frequency",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.0f, 0.25f),
            def.freqHz,
            juce::AudioParameterFloatAttributes().withLabel ("Hz")));

        // Gain (used by shelf and bell filters; ignored for HP/LP)
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            ParamIDs::bandGain (i),
            "Band " + juce::String (i + 1) + " Gain",
            juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f),
            def.gainDb,
            juce::AudioParameterFloatAttributes().withLabel ("dB")));

        // Q / bandwidth
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            ParamIDs::bandQ (i),
            "Band " + juce::String (i + 1) + " Q",
            juce::NormalisableRange<float> (0.1f, 18.0f, 0.0f, 0.5f),
            def.q));
    }

    return layout;
}

//==============================================================================
// Lifecycle
//==============================================================================
void VSTEqualizerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize  = samplesPerBlock;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels      = 1; // each filter is mono; we duplicate per channel

    for (auto& stereoFilter : filters)
        for (auto& f : stereoFilter)
            f.prepare (spec);

    updateFilters();
}

void VSTEqualizerAudioProcessor::releaseResources()
{
    for (auto& stereoFilter : filters)
        for (auto& f : stereoFilter)
            f.reset();
}

bool VSTEqualizerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Accept mono or stereo – reject anything else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Input and output layouts must match
    return (layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet());
}

//==============================================================================
// DSP
//==============================================================================
juce::dsp::IIR::Coefficients<float>::Ptr
VSTEqualizerAudioProcessor::makeCoefficientsForBand (int bandIndex) const
{
    const auto type  = static_cast<FilterType> (
        static_cast<int> (*apvts.getRawParameterValue (ParamIDs::bandType (bandIndex))));
    const float freq  = *apvts.getRawParameterValue (ParamIDs::bandFreq (bandIndex));
    const float gain  = *apvts.getRawParameterValue (ParamIDs::bandGain (bandIndex));
    const float q     = *apvts.getRawParameterValue (ParamIDs::bandQ    (bandIndex));
    const float linearGain = juce::Decibels::decibelsToGain (gain);
    const double sr   = currentSampleRate;

    switch (type)
    {
        case FilterType::HighPass:
            return juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, freq, q);

        case FilterType::LowShelf:
            return juce::dsp::IIR::Coefficients<float>::makeLowShelf (sr, freq, q, linearGain);

        case FilterType::Bell:
            return juce::dsp::IIR::Coefficients<float>::makePeakFilter (sr, freq, q, linearGain);

        case FilterType::HighShelf:
            return juce::dsp::IIR::Coefficients<float>::makeHighShelf (sr, freq, q, linearGain);

        case FilterType::LowPass:
            return juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, freq, q);

        default:
            return juce::dsp::IIR::Coefficients<float>::makePeakFilter (sr, freq, q, linearGain);
    }
}

void VSTEqualizerAudioProcessor::updateFilters()
{
    for (int i = 0; i < NUM_BANDS; ++i)
    {
        auto newCoeffs = makeCoefficientsForBand (i);
        for (auto& f : filters[i])
            *f.coefficients = *newCoeffs;
    }
    filtersDirty.store (false);
}

void VSTEqualizerAudioProcessor::parameterChanged (const juce::String&, float)
{
    // Any parameter change marks coefficients as needing recalculation.
    // processBlock will call updateFilters() on the next audio callback.
    filtersDirty.store (true);
}

void VSTEqualizerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Global bypass: pass audio through unprocessed
    const bool bypassed = *apvts.getRawParameterValue (ParamIDs::bypass) > 0.5f;
    if (bypassed)
        return;

    // Refresh filter coefficients only when parameters have changed
    if (filtersDirty.load())
        updateFilters();

    const int numChannels = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples  = buffer.getNumSamples();

    // Apply each enabled EQ band
    for (int bandIdx = 0; bandIdx < NUM_BANDS; ++bandIdx)
    {
        const bool enabled =
            *apvts.getRawParameterValue (ParamIDs::bandEnabled (bandIdx)) > 0.5f;
        if (!enabled)
            continue;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);

            // Use block processing via AudioBlock for efficiency
            juce::dsp::AudioBlock<float> block (&data, 1, static_cast<size_t> (numSamples));
            juce::dsp::ProcessContextReplacing<float> ctx (block);
            filters[bandIdx][static_cast<size_t> (ch)].process (ctx);
        }
    }

    // Output gain
    const float outGainDb    = *apvts.getRawParameterValue (ParamIDs::outputGain);
    const float outGainLinear = juce::Decibels::decibelsToGain (outGainDb);
    buffer.applyGain (outGainLinear);
}

//==============================================================================
// Frequency response for the editor's display
//==============================================================================
std::vector<double>
VSTEqualizerAudioProcessor::getFrequencyResponse (const std::vector<double>& frequencies) const
{
    const size_t N = frequencies.size();
    std::vector<double> magnitudesDb (N, 0.0);

    for (int bandIdx = 0; bandIdx < NUM_BANDS; ++bandIdx)
    {
        const bool enabled =
            *apvts.getRawParameterValue (ParamIDs::bandEnabled (bandIdx)) > 0.5f;
        if (!enabled)
            continue;

        auto coeffs = makeCoefficientsForBand (bandIdx);
        for (size_t k = 0; k < N; ++k)
        {
            // getMagnitudeForFrequency returns linear magnitude
            const double mag = coeffs->getMagnitudeForFrequency (frequencies[k], currentSampleRate);
            magnitudesDb[k] += juce::Decibels::gainToDecibels (static_cast<float> (mag));
        }
    }

    // Add output gain
    const float outGainDb = *apvts.getRawParameterValue (ParamIDs::outputGain);
    for (auto& db : magnitudesDb)
        db += static_cast<double> (outGainDb);

    return magnitudesDb;
}

//==============================================================================
// State persistence
//==============================================================================
void VSTEqualizerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void VSTEqualizerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// Plugin entry point
//==============================================================================
juce::AudioProcessorEditor* VSTEqualizerAudioProcessor::createEditor()
{
    return new VSTEqualizerAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VSTEqualizerAudioProcessor();
}
