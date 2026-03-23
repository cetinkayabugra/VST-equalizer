#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class FrequencyResponseComponent : public juce::Component
{
public:
    FrequencyResponseComponent(VSTEqualizerAudioProcessor& processor)
        : processor(processor)
    {
        setSize(800, 300);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.fillAll(juce::Colour(30, 30, 35));

        // Grid
        g.setColour(juce::Colour(60, 60, 65));
        
        // Vertical frequency grid lines (log scale)
        float logMinFreq = std::log10(20.0f);
        float logMaxFreq = std::log10(20000.0f);
        
        for (float octave = 0.0f; octave <= 10.0f; ++octave)
        {
            float freq = std::pow(10.0f, logMinFreq + (logMaxFreq - logMinFreq) * octave / 10.0f);
            float x = bounds.getX() + (octave / 10.0f) * bounds.getWidth();
            
            if (octave > 0.0f && octave < 10.0f)
            {
                g.drawVerticalLine(static_cast<int>(x), bounds.getY(), bounds.getBottom());
            }
        }

        // Horizontal gain grid lines
        for (float gain = -24.0f; gain <= 24.0f; gain += 6.0f)
        {
            float y = bounds.getBottom() - ((gain + 24.0f) / 48.0f) * bounds.getHeight();
            g.drawHorizontalLine(static_cast<int>(y), bounds.getX(), bounds.getRight());
        }

        // EQ curve
        g.setColour(juce::Colour(100, 200, 255));
        juce::Path curve;
        
        const int pointCount = static_cast<int>(bounds.getWidth());
        for (int x = 0; x < pointCount; ++x)
        {
            float normalized = x / bounds.getWidth();
            float frequency = std::pow(10.0f, logMinFreq + (logMaxFreq - logMinFreq) * normalized);
            
            double response = processor.getAPVTS().state.getProperty("frequencyResponse", 1.0);
            auto& filterBank = processor.getAPVTS();  // Note: This is a simplified version
            
            // Simplified response calculation
            float gain = processor.getAPVTS().getRawParameterValue("gain0")->load() * 0.1f;  // placeholder
            float magnitudeDb = 20.0f * std::log10(std::max(1.0f + gain * 0.01f, 0.01f));
            
            float y = bounds.getBottom() - ((magnitudeDb + 24.0f) / 48.0f) * bounds.getHeight();
            y = juce::jlimit(bounds.getY(), bounds.getBottom(), y);
            
            if (x == 0)
                curve.startNewSubPath(x, y);
            else
                curve.lineTo(x, y);
        }

        g.strokePath(curve, juce::PathStrokeType(2.0f));

        // Center line (0dB)
        g.setColour(juce::Colour(100, 100, 100));
        float centerY = bounds.getBottom() - (24.0f / 48.0f) * bounds.getHeight();
        g.drawHorizontalLine(static_cast<int>(centerY), bounds.getX(), bounds.getRight());

        // Analyzer (background)
        g.setColour(juce::Colour(80, 80, 85).withAlpha(0.3f));
        auto& analyzer = processor.getAnalyzer();
        auto& magnitudes = analyzer.getOutputMagnitudes();
        
        if (!magnitudes.empty())
        {
            juce::Path analyzerPath;
            for (size_t i = 0; i < magnitudes.size(); ++i)
            {
                float normalized = i / static_cast<float>(magnitudes.size());
                float frequency = std::pow(10.0f, logMinFreq + (logMaxFreq - logMinFreq) * normalized);
                
                float x = bounds.getX() + normalized * bounds.getWidth();
                float magnitudeDb = magnitudes[i];
                float y = bounds.getBottom() - ((magnitudeDb + 24.0f) / 48.0f) * bounds.getHeight();
                y = juce::jlimit(bounds.getY(), bounds.getBottom(), y);
                
                if (i == 0)
                    analyzerPath.startNewSubPath(x, y);
                else
                    analyzerPath.lineTo(x, y);
            }
            
            g.strokePath(analyzerPath, juce::PathStrokeType(1.0f));
        }

        // Border
        g.setColour(juce::Colour(100, 100, 100));
        g.drawRect(bounds, 1.0f);

        // Frequency labels
        g.setColour(juce::Colour(150, 150, 150));
        g.setFont(10.0f);
        
        for (float octave = 0.0f; octave <= 10.0f; octave += 2.0f)
        {
            float freq = std::pow(10.0f, logMinFreq + (logMaxFreq - logMinFreq) * octave / 10.0f);
            float x = bounds.getX() + (octave / 10.0f) * bounds.getWidth();
            
            juce::String freqLabel;
            if (freq < 1000.0f)
                freqLabel = juce::String(static_cast<int>(freq)) + "Hz";
            else
                freqLabel = juce::String(freq / 1000.0f, 1) + "kHz";
            
            g.drawText(freqLabel, static_cast<int>(x - 20), static_cast<int>(bounds.getBottom() + 5),
                      40, 15, juce::Justification::centred);
        }

        // Gain labels
        g.drawText("+24dB", static_cast<int>(bounds.getX() - 40), static_cast<int>(bounds.getY()), 35, 15, juce::Justification::right);
        g.drawText("0dB", static_cast<int>(bounds.getX() - 40), static_cast<int>(centerY - 8), 35, 15, juce::Justification::right);
        g.drawText("-24dB", static_cast<int>(bounds.getX() - 40), static_cast<int>(bounds.getBottom() - 15), 35, 15, juce::Justification::right);
    }

private:
    VSTEqualizerAudioProcessor& processor;
};

class BandControlComponent : public juce::Component
{
public:
    BandControlComponent(VSTEqualizerAudioProcessor& processor, int band)
        : processor(processor), bandIndex(band)
    {
        // Frequency slider
        frequencySlider.setRange(20.0, 20000.0, 1.0);
        frequencySlider.setSkewFactorFromMidPoint(1000.0);
        frequencySlider.setSliderStyle(juce::Slider::Rotary);
        frequencySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        addAndMakeVisible(frequencySlider);

        // Gain slider
        gainSlider.setRange(-24.0, 24.0, 0.1);
        gainSlider.setSliderStyle(juce::Slider::Rotary);
        gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        addAndMakeVisible(gainSlider);

        // Q slider
        qSlider.setRange(0.1, 10.0, 0.1);
        qSlider.setSkewFactorFromMidPoint(1.0);
        qSlider.setSliderStyle(juce::Slider::Rotary);
        qSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        addAndMakeVisible(qSlider);

        // Type selector
        typeCombo.addItem("High-Pass", 1);
        typeCombo.addItem("Low-Pass", 2);
        typeCombo.addItem("Low-Shelf", 3);
        typeCombo.addItem("High-Shelf", 4);
        typeCombo.addItem("Bell", 5);
        addAndMakeVisible(typeCombo);

        // Enable button
        enableButton.setButtonText("ON");
        enableButton.setClickingTogglesState(true);
        addAndMakeVisible(enableButton);

        // Band title
        bandLabel = "Band " + juce::String(band + 1);

        setSize(120, 200);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(5);
        
        typeCombo.setBounds(area.removeFromTop(25));
        enableButton.setBounds(area.removeFromTop(25));
        frequencySlider.setBounds(area.removeFromTop(60));
        gainSlider.setBounds(area.removeFromTop(60));
        qSlider.setBounds(area.removeFromTop(60));
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(45, 45, 50));
        g.setColour(juce::Colour(150, 150, 150));
        g.drawRect(getLocalBounds(), 1);
        
        g.setFont(12.0f);
        g.drawText(bandLabel, 5, 5, getWidth() - 10, 20, juce::Justification::centred);
    }

    juce::Slider frequencySlider;
    juce::Slider gainSlider;
    juce::Slider qSlider;
    juce::ComboBox typeCombo;
    juce::ToggleButton enableButton;

private:
    VSTEqualizerAudioProcessor& processor;
    int bandIndex;
    juce::String bandLabel;
};

class VSTEqualizerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        public juce::Timer
{
public:
    explicit VSTEqualizerAudioProcessorEditor(VSTEqualizerAudioProcessor&);
    ~VSTEqualizerAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    VSTEqualizerAudioProcessor& processorRef;

    FrequencyResponseComponent frequencyResponseComponent;
    std::array<BandControlComponent, DSP::FilterBank::NUM_BANDS> bandControls;

    juce::Slider inputGainSlider;
    juce::Slider outputGainSlider;
    juce::Label inputGainLabel;
    juce::Label outputGainLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VSTEqualizerAudioProcessorEditor)
};
