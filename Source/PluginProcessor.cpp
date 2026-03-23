#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>
#include <algorithm>

VSTEqualizerAudioProcessor::VSTEqualizerAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    apvts.addParameterListener(PARAM_OUTPUT_GAIN, this);
    apvts.addParameterListener(PARAM_INPUT_GAIN, this);

    for (int i = 0; i < DSP::FilterBank::NUM_BANDS; ++i)
    {
        apvts.addParameterListener(juce::String(PARAM_FREQ_PREFIX) + juce::String(i), this);
        apvts.addParameterListener(juce::String(PARAM_GAIN_PREFIX) + juce::String(i), this);
        apvts.addParameterListener(juce::String(PARAM_Q_PREFIX) + juce::String(i), this);
        apvts.addParameterListener(juce::String(PARAM_ENABLED_PREFIX) + juce::String(i), this);
        apvts.addParameterListener(juce::String(PARAM_TYPE_PREFIX) + juce::String(i), this);
    }
}

VSTEqualizerAudioProcessor::~VSTEqualizerAudioProcessor()
{
    apvts.removeParameterListener(PARAM_OUTPUT_GAIN, this);
    apvts.removeParameterListener(PARAM_INPUT_GAIN, this);

    for (int i = 0; i < DSP::FilterBank::NUM_BANDS; ++i)
    {
        apvts.removeParameterListener(juce::String(PARAM_FREQ_PREFIX) + juce::String(i), this);
        apvts.removeParameterListener(juce::String(PARAM_GAIN_PREFIX) + juce::String(i), this);
        apvts.removeParameterListener(juce::String(PARAM_Q_PREFIX) + juce::String(i), this);
        apvts.removeParameterListener(juce::String(PARAM_ENABLED_PREFIX) + juce::String(i), this);
        apvts.removeParameterListener(juce::String(PARAM_TYPE_PREFIX) + juce::String(i), this);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout VSTEqualizerAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Input/Output Gain
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        PARAM_INPUT_GAIN, "Input Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f),
        0.0f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        PARAM_OUTPUT_GAIN, "Output Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f),
        0.0f, "dB"));

    // Per-band parameters
    for (int i = 0; i < DSP::FilterBank::NUM_BANDS; ++i)
    {
        // Frequency (logarithmic range)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::String(PARAM_FREQ_PREFIX) + juce::String(i),
            juce::String("Band ") + juce::String(i + 1) + juce::String(" Frequency"),
            juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f),  // logarithmic skew
            1000.0f * (float)std::pow(2.0, i), "Hz"));

        // Gain
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::String(PARAM_GAIN_PREFIX) + juce::String(i),
            juce::String("Band ") + juce::String(i + 1) + juce::String(" Gain"),
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f),
            0.0f, "dB"));

        // Q Factor
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::String(PARAM_Q_PREFIX) + juce::String(i),
            juce::String("Band ") + juce::String(i + 1) + juce::String(" Q"),
            juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f),
            1.0f));

        // Band Enabled
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::String(PARAM_ENABLED_PREFIX) + juce::String(i),
            juce::String("Band ") + juce::String(i + 1) + juce::String(" Enabled"),
            true));

        // Filter Type (0=HighPass, 1=LowPass, 2=LowShelf, 3=HighShelf, 4=Bell)
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::String(PARAM_TYPE_PREFIX) + juce::String(i),
            juce::String("Band ") + juce::String(i + 1) + juce::String(" Type"),
            juce::StringArray("High-Pass", "Low-Pass", "Low-Shelf", "High-Shelf", "Bell"),
            i == 0 ? 0 : i == DSP::FilterBank::NUM_BANDS - 1 ? 1 : 4));  // Default: High-Pass, Low-Pass, Bell
    }

    return {params.begin(), params.end()};
}

void VSTEqualizerAudioProcessor::prepareToPlay(double newSampleRate, int newSamplesPerBlock)
{
    sampleRate = newSampleRate;
    samplesPerBlock = newSamplesPerBlock;

    filterBank.prepare(sampleRate);
    analyzer.prepare(sampleRate);

    // Setup smoothers
    for (auto& smoother : frequencySmoothers)
        smoother.reset(sampleRate, 0.05);  // 50ms ramp time
    for (auto& smoother : gainSmoothers)
        smoother.reset(sampleRate, 0.05);
    for (auto& smoother : qSmoothers)
        smoother.reset(sampleRate, 0.05);

    inputGainSmoother.reset(sampleRate, 0.05);
    outputGainSmoother.reset(sampleRate, 0.05);

    needsUpdate = true;
}

void VSTEqualizerAudioProcessor::releaseResources()
{
    filterBank.reset();
}

void VSTEqualizerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    auto scope = getTempDebugScope();
    juce::ScopedNoDenormals noDenormals;

    if (needsUpdate)
    {
        updateFilterParameters();
        needsUpdate = false;
    }

    // Apply input gain
    float inputGain = juce::Decibels::decibelsToGain(inputGainSmoother.getNextValue());
    buffer.applyGain(inputGain);

    // Update smoothers
    for (int i = 0; i < DSP::FilterBank::NUM_BANDS; ++i)
    {
        frequencySmoothers[i].getNextValue();
        gainSmoothers[i].getNextValue();
        qSmoothers[i].getNextValue();
    }

    // Process through filter bank
    filterBank.processBlock(buffer);

    // Apply output gain
    float outputGain = juce::Decibels::decibelsToGain(outputGainSmoother.getNextValue());
    buffer.applyGain(outputGain);

    // Push samples to analyzer
    const auto numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    if (numChannels > 0)
    {
        const auto* readPtr = buffer.getReadPointer(0);
        for (int i = 0; i < numSamples; ++i)
        {
            analyzer.pushOutputSample(readPtr[i]);
        }
    }
}

void VSTEqualizerAudioProcessor::updateFilterParameters()
{
    for (int i = 0; i < DSP::FilterBank::NUM_BANDS; ++i)
    {
        auto freqParam = dynamic_cast<juce::AudioParameterFloat*>(
            apvts.getParameter(juce::String(PARAM_FREQ_PREFIX) + juce::String(i)));
        auto gainParam = dynamic_cast<juce::AudioParameterFloat*>(
            apvts.getParameter(juce::String(PARAM_GAIN_PREFIX) + juce::String(i)));
        auto qParam = dynamic_cast<juce::AudioParameterFloat*>(
            apvts.getParameter(juce::String(PARAM_Q_PREFIX) + juce::String(i)));
        auto enabledParam = dynamic_cast<juce::AudioParameterBool*>(
            apvts.getParameter(juce::String(PARAM_ENABLED_PREFIX) + juce::String(i)));
        auto typeParam = dynamic_cast<juce::AudioParameterChoice*>(
            apvts.getParameter(juce::String(PARAM_TYPE_PREFIX) + juce::String(i)));

        if (freqParam && gainParam && qParam && enabledParam && typeParam)
        {
            double frequency = freqParam->get();
            double gain = gainParam->get();
            double q = qParam->get();
            bool enabled = enabledParam->get();

            DSP::BiQuadFilter::FilterType type;
            switch (typeParam->getIndex())
            {
                case 0: type = DSP::BiQuadFilter::FilterType::HighPass; break;
                case 1: type = DSP::BiQuadFilter::FilterType::LowPass; break;
                case 2: type = DSP::BiQuadFilter::FilterType::LowShelf; break;
                case 3: type = DSP::BiQuadFilter::FilterType::HighShelf; break;
                case 4: type = DSP::BiQuadFilter::FilterType::Bell; break;
                default: type = DSP::BiQuadFilter::FilterType::Bell; break;
            }

            filterBank.setBandParameters(i, type, frequency, gain, q);
            filterBank.setBandEnabled(i, enabled);
        }
    }

    auto inputGainParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PARAM_INPUT_GAIN));
    auto outputGainParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(PARAM_OUTPUT_GAIN));

    if (inputGainParam)
        inputGainSmoother.setTargetValue(inputGainParam->get());
    if (outputGainParam)
        outputGainSmoother.setTargetValue(outputGainParam->get());
}

void VSTEqualizerAudioProcessor::parameterChanged(const juce::String&, float)
{
    needsUpdate = true;
}

bool VSTEqualizerAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Support stereo in/out and mono in/out
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}

juce::AudioProcessorEditor* VSTEqualizerAudioProcessor::createEditor()
{
    return new VSTEqualizerAudioProcessorEditor(*this);
}

void VSTEqualizerAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyValueTreeToXml(true);
    copyXmlToBinary(*state, destData);
}

void VSTEqualizerAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xmlState = getXmlFromBinary(data, sizeInBytes);
    if (xmlState != nullptr)
        apvts.replaceValueTreeFromXml(*xmlState);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VSTEqualizerAudioProcessor();
}
