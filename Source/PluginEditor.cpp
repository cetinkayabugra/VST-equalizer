#include "PluginEditor.h"

VSTEqualizerAudioProcessorEditor::VSTEqualizerAudioProcessorEditor(VSTEqualizerAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p),
      frequencyResponseComponent(p),
      bandControls({
          BandControlComponent(p, 0),
          BandControlComponent(p, 1),
          BandControlComponent(p, 2),
          BandControlComponent(p, 3),
          BandControlComponent(p, 4)
      })
{
    setSize(1200, 700);

    // Frequency response display
    addAndMakeVisible(frequencyResponseComponent);

    // Input/Output gain sliders
    inputGainSlider.setRange(-24.0, 24.0, 0.1);
    inputGainSlider.setSliderStyle(juce::Slider::Rotary);
    inputGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(inputGainSlider);

    outputGainSlider.setRange(-24.0, 24.0, 0.1);
    outputGainSlider.setSliderStyle(juce::Slider::Rotary);
    outputGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(outputGainSlider);

    inputGainLabel.setText("Input Gain", juce::dontSendNotification);
    inputGainLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(inputGainLabel);

    outputGainLabel.setText("Output Gain", juce::dontSendNotification);
    outputGainLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(outputGainLabel);

    // Add band controls
    for (auto& bandControl : bandControls)
    {
        addAndMakeVisible(bandControl);
    }

    startTimer(100);  // Update UI every 100ms
}

VSTEqualizerAudioProcessorEditor::~VSTEqualizerAudioProcessorEditor()
{
    stopTimer();
}

void VSTEqualizerAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(30, 30, 35));
}

void VSTEqualizerAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    // Frequency response graph at top (40% of height)
    frequencyResponseComponent.setBounds(area.removeFromTop(getHeight() * 2 / 5).reduced(10));

    // Control section
    auto controlArea = area.reduced(10);
    
    // Input/Output gain on left
    auto gainArea = controlArea.removeFromLeft(120);
    inputGainLabel.setBounds(gainArea.removeFromTop(20));
    inputGainSlider.setBounds(gainArea.removeFromTop(70));
    
    auto outputArea = controlArea.removeFromLeft(120);
    outputGainLabel.setBounds(outputArea.removeFromTop(20));
    outputGainSlider.setBounds(outputArea.removeFromTop(70));

    // Band controls in a row
    auto bandArea = controlArea;
    int bandWidth = bandArea.getWidth() / DSP::FilterBank::NUM_BANDS;
    
    for (auto& bandControl : bandControls)
    {
        bandControl.setBounds(bandArea.removeFromLeft(bandWidth).reduced(5));
    }
}

void VSTEqualizerAudioProcessorEditor::timerCallback()
{
    processorRef.getAnalyzer().performAnalysis();
    frequencyResponseComponent.repaint();
}
