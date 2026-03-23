#include "PluginEditor.h"

//==============================================================================
// Colour palette
//==============================================================================
namespace EQColours
{
    static const juce::Colour background  { 0xFF1A1A2E };
    static const juce::Colour surface     { 0xFF16213E };
    static const juce::Colour accent      { 0xFF0F3460 };
    static const juce::Colour highlight   { 0xFFE94560 };
    static const juce::Colour gridLine    { 0xFF2D2D4A };
    static const juce::Colour curveFill   { 0x33E94560 };
    static const juce::Colour curveStroke { 0xFFE94560 };
    static const juce::Colour textLight   { 0xFFEEEEEE };
    static const juce::Colour textMuted   { 0xFF888899 };
}

//==============================================================================
// FrequencyResponseComponent
//==============================================================================
FrequencyResponseComponent::FrequencyResponseComponent (VSTEqualizerAudioProcessor& p)
    : processor (p)
{
    // Pre-compute log-spaced frequency points
    freqPoints.resize (NUM_POINTS);
    for (int i = 0; i < NUM_POINTS; ++i)
    {
        const double t = static_cast<double> (i) / static_cast<double> (NUM_POINTS - 1);
        freqPoints[static_cast<size_t> (i)] =
            MIN_FREQ_HZ * std::pow (static_cast<double> (MAX_FREQ_HZ) / MIN_FREQ_HZ, t);
    }

    startTimerHz (30); // repaint at ~30 fps
}

FrequencyResponseComponent::~FrequencyResponseComponent()
{
    stopTimer();
}

void FrequencyResponseComponent::timerCallback()
{
    repaint();
}

float FrequencyResponseComponent::freqToX (float freq) const noexcept
{
    const float w = static_cast<float> (getWidth());
    return w * std::log (freq / MIN_FREQ_HZ)
             / std::log (MAX_FREQ_HZ / MIN_FREQ_HZ);
}

float FrequencyResponseComponent::dbToY (float db) const noexcept
{
    const float h = static_cast<float> (getHeight());
    // Centre at 0 dB; positive dB goes up
    return h * 0.5f - (db / (MAX_DB - MIN_DB)) * h;
}

void FrequencyResponseComponent::resized() {}

void FrequencyResponseComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background
    g.setColour (EQColours::surface);
    g.fillRoundedRectangle (bounds, 4.0f);

    // --- Grid lines --------------------------------------------------------
    g.setColour (EQColours::gridLine);

    // Horizontal dB lines
    for (float db : { -24.0f, -18.0f, -12.0f, -6.0f, 0.0f, 6.0f, 12.0f, 18.0f, 24.0f })
    {
        const float y = dbToY (db);
        g.drawHorizontalLine (juce::roundToInt (y), bounds.getX(), bounds.getRight());

        // Label
        if (std::abs (db) < 0.1f || std::fmod (std::abs (db), 6.0f) < 0.1f)
        {
            g.setColour (EQColours::textMuted);
            g.setFont (10.0f);
            g.drawText (juce::String (juce::roundToInt (db)) + " dB",
                        2, juce::roundToInt (y) - 6, 36, 12,
                        juce::Justification::left);
            g.setColour (EQColours::gridLine);
        }
    }

    // Vertical frequency lines
    for (float freq : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
    {
        const float x = freqToX (freq);
        g.drawVerticalLine (juce::roundToInt (x), bounds.getY(), bounds.getBottom());

        // Label
        g.setColour (EQColours::textMuted);
        g.setFont (10.0f);
        juce::String freqStr = freq >= 1000.0f
                                   ? juce::String (juce::roundToInt (freq / 1000.0f)) + "k"
                                   : juce::String (juce::roundToInt (freq));
        g.drawText (freqStr, juce::roundToInt (x) - 12, static_cast<int> (bounds.getBottom()) - 14,
                    24, 12, juce::Justification::centred);
        g.setColour (EQColours::gridLine);
    }

    // 0 dB centre line (slightly brighter)
    g.setColour (EQColours::gridLine.brighter (0.3f));
    g.drawHorizontalLine (juce::roundToInt (dbToY (0.0f)),
                          bounds.getX(), bounds.getRight());

    // --- EQ curve ----------------------------------------------------------
    const auto magnitudesDb = processor.getFrequencyResponse (freqPoints);

    juce::Path curvePath;
    bool firstPoint = true;

    for (int i = 0; i < NUM_POINTS; ++i)
    {
        const float x = freqToX (static_cast<float> (freqPoints[static_cast<size_t> (i)]));
        const float db = juce::jlimit (MIN_DB, MAX_DB,
                                       static_cast<float> (magnitudesDb[static_cast<size_t> (i)]));
        const float y = dbToY (db);

        if (firstPoint)
        {
            curvePath.startNewSubPath (x, y);
            firstPoint = false;
        }
        else
        {
            curvePath.lineTo (x, y);
        }
    }

    // Filled area under the curve
    juce::Path fillPath (curvePath);
    const float zeroY = dbToY (0.0f);
    fillPath.lineTo (freqToX (static_cast<float> (freqPoints.back())), zeroY);
    fillPath.lineTo (freqToX (static_cast<float> (freqPoints.front())), zeroY);
    fillPath.closeSubPath();
    g.setColour (EQColours::curveFill);
    g.fillPath (fillPath);

    // Stroke
    g.setColour (EQColours::curveStroke);
    g.strokePath (curvePath, juce::PathStrokeType (2.0f));

    // Border
    g.setColour (EQColours::accent);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);

    // TODO: Add live input/output spectrum analyser overlay (v2)
}

//==============================================================================
// BandControlComponent
//==============================================================================
BandControlComponent::BandControlComponent (VSTEqualizerAudioProcessor& p, int bandIndex)
    : processor (p), band (bandIndex)
{
    auto& apvts = processor.apvts;

    // Band label
    const juce::String bandNames[] =
        { "HP", "Low Shelf", "Bell 1", "Bell 2", "High Shelf", "LP" };
    bandLabel.setText (bandIndex < static_cast<int> (std::size (bandNames))
                           ? bandNames[bandIndex]
                           : "Band " + juce::String (bandIndex + 1),
                       juce::dontSendNotification);
    bandLabel.setColour (juce::Label::textColourId, EQColours::textLight);
    bandLabel.setFont (juce::Font (12.0f, juce::Font::bold));
    addAndMakeVisible (bandLabel);

    // Enable button
    enableButton.setButtonText ({});
    enableButton.setColour (juce::ToggleButton::tickColourId, EQColours::highlight);
    enableButton.setColour (juce::ToggleButton::tickDisabledColourId, EQColours::textMuted);
    addAndMakeVisible (enableButton);
    enableAttach = std::make_unique<ButtonAttachment> (
        apvts, ParamIDs::bandEnabled (band), enableButton);

    // Type combo
    for (int t = 0; t < FILTER_TYPE_NAMES.size(); ++t)
        typeCombo.addItem (FILTER_TYPE_NAMES[t], t + 1);
    typeCombo.setColour (juce::ComboBox::backgroundColourId, EQColours::accent);
    typeCombo.setColour (juce::ComboBox::textColourId,       EQColours::textLight);
    typeCombo.setColour (juce::ComboBox::outlineColourId,    EQColours::gridLine);
    addAndMakeVisible (typeCombo);
    typeAttach = std::make_unique<ComboAttachment> (
        apvts, ParamIDs::bandType (band), typeCombo);

    // Helper lambda to style a rotary slider
    auto styleSlider = [this] (juce::Slider& s, const juce::String& suffix)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);
        s.setColour (juce::Slider::rotarySliderFillColourId, EQColours::highlight);
        s.setColour (juce::Slider::rotarySliderOutlineColourId, EQColours::accent);
        s.setColour (juce::Slider::thumbColourId,  EQColours::highlight);
        s.setColour (juce::Slider::textBoxTextColourId, EQColours::textLight);
        s.setColour (juce::Slider::textBoxBackgroundColourId, EQColours::surface);
        s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        s.setTextValueSuffix (suffix);
        addAndMakeVisible (s);
    };

    styleSlider (freqSlider, " Hz");
    styleSlider (gainSlider, " dB");
    styleSlider (qSlider,    "");

    // Freq label
    freqLabel.setText ("Freq", juce::dontSendNotification);
    freqLabel.setColour (juce::Label::textColourId, EQColours::textMuted);
    freqLabel.setFont (juce::Font (10.0f));
    freqLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (freqLabel);

    gainLabel.setText ("Gain", juce::dontSendNotification);
    gainLabel.setColour (juce::Label::textColourId, EQColours::textMuted);
    gainLabel.setFont (juce::Font (10.0f));
    gainLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (gainLabel);

    qLabel.setText ("Q", juce::dontSendNotification);
    qLabel.setColour (juce::Label::textColourId, EQColours::textMuted);
    qLabel.setFont (juce::Font (10.0f));
    qLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (qLabel);

    // APVTS attachments
    freqAttach = std::make_unique<SliderAttachment> (apvts, ParamIDs::bandFreq (band), freqSlider);
    gainAttach = std::make_unique<SliderAttachment> (apvts, ParamIDs::bandGain (band), gainSlider);
    qAttach    = std::make_unique<SliderAttachment> (apvts, ParamIDs::bandQ    (band), qSlider);
}

BandControlComponent::~BandControlComponent() {}

void BandControlComponent::resized()
{
    auto area = getLocalBounds().reduced (4);

    // Left column: band label + enable button
    auto labelArea = area.removeFromLeft (50);
    enableButton.setBounds (labelArea.removeFromTop (20).withSizeKeepingCentre (20, 20));
    bandLabel.setBounds    (labelArea.removeFromTop (16));

    area.removeFromLeft (4); // gap

    // Type combo
    auto comboArea = area.removeFromLeft (90);
    typeCombo.setBounds (comboArea.withSizeKeepingCentre (88, 22));

    area.removeFromLeft (4);

    // Three rotary knobs: Freq, Gain, Q
    const int knobW = area.getWidth() / 3;

    auto freqArea = area.removeFromLeft (knobW);
    freqLabel.setBounds (freqArea.removeFromBottom (14));
    freqSlider.setBounds (freqArea);

    auto gainArea = area.removeFromLeft (knobW);
    gainLabel.setBounds (gainArea.removeFromBottom (14));
    gainSlider.setBounds (gainArea);

    auto qArea = area;
    qLabel.setBounds (qArea.removeFromBottom (14));
    qSlider.setBounds (qArea);
}

void BandControlComponent::paint (juce::Graphics& g)
{
    g.setColour (EQColours::accent.withAlpha (0.5f));
    g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 4.0f);
}

//==============================================================================
// VSTEqualizerAudioProcessorEditor
//==============================================================================
VSTEqualizerAudioProcessorEditor::VSTEqualizerAudioProcessorEditor (
    VSTEqualizerAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      responseDisplay (p)
{
    // Window size
    setSize (840, 560);
    setResizable (true, true);
    setResizeLimits (700, 460, 1400, 900);

    addAndMakeVisible (responseDisplay);

    // Band controls
    for (int i = 0; i < NUM_BANDS; ++i)
    {
        bandControls[static_cast<size_t> (i)] =
            std::make_unique<BandControlComponent> (p, i);
        addAndMakeVisible (*bandControls[static_cast<size_t> (i)]);
    }

    // Bypass button
    bypassButton.setButtonText ("Bypass");
    bypassButton.setColour (juce::ToggleButton::tickColourId, EQColours::highlight);
    bypassButton.setColour (juce::ToggleButton::tickDisabledColourId, EQColours::textMuted);
    addAndMakeVisible (bypassButton);
    bypassAttach = std::make_unique<ButtonAttachment> (
        audioProcessor.apvts, ParamIDs::bypass, bypassButton);

    // Output gain slider
    outputGainSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    outputGainSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 70, 20);
    outputGainSlider.setColour (juce::Slider::trackColourId,     EQColours::highlight);
    outputGainSlider.setColour (juce::Slider::backgroundColourId, EQColours::accent);
    outputGainSlider.setColour (juce::Slider::thumbColourId,     EQColours::highlight);
    outputGainSlider.setColour (juce::Slider::textBoxTextColourId, EQColours::textLight);
    outputGainSlider.setColour (juce::Slider::textBoxBackgroundColourId, EQColours::surface);
    outputGainSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    outputGainSlider.setTextValueSuffix (" dB");
    addAndMakeVisible (outputGainSlider);
    outputGainAttach = std::make_unique<SliderAttachment> (
        audioProcessor.apvts, ParamIDs::outputGain, outputGainSlider);

    outputGainLabel.setText ("Out Gain", juce::dontSendNotification);
    outputGainLabel.setColour (juce::Label::textColourId, EQColours::textMuted);
    outputGainLabel.setFont (juce::Font (12.0f));
    addAndMakeVisible (outputGainLabel);
}

VSTEqualizerAudioProcessorEditor::~VSTEqualizerAudioProcessorEditor() {}

void VSTEqualizerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (EQColours::background);

    // Title bar
    g.setColour (EQColours::surface);
    g.fillRect (0, 0, getWidth(), 28);
    g.setColour (EQColours::textLight);
    g.setFont (juce::Font (14.0f, juce::Font::bold));
    g.drawText ("VST Equalizer", 12, 0, 200, 28, juce::Justification::left);

    g.setColour (EQColours::highlight);
    g.fillRect (0, 27, getWidth(), 2); // accent line under title
}

void VSTEqualizerAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    // Title bar
    area.removeFromTop (30);

    // Frequency response display (top 40% of remaining)
    const int displayHeight = juce::roundToInt (area.getHeight() * 0.38f);
    responseDisplay.setBounds (area.removeFromTop (displayHeight).reduced (6, 4));

    // Band controls – equal-height rows
    const int bandAreaHeight = juce::roundToInt (area.getHeight() * 0.82f);
    auto bandArea = area.removeFromTop (bandAreaHeight).reduced (6, 2);
    const int rowH = bandArea.getHeight() / NUM_BANDS;

    for (int i = 0; i < NUM_BANDS; ++i)
        bandControls[static_cast<size_t> (i)]->setBounds (
            bandArea.removeFromTop (rowH).reduced (0, 1));

    // Bottom bar: bypass + output gain
    auto bottomBar = area.reduced (8, 2);
    bypassButton.setBounds (bottomBar.removeFromLeft (80));
    outputGainLabel.setBounds (bottomBar.removeFromLeft (60));
    outputGainSlider.setBounds (bottomBar);
}
