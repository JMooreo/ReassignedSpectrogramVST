/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
//==============================================================================
/**
*/
class SpectrogramVSTAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    SpectrogramVSTAudioProcessorEditor(SpectrogramVSTAudioProcessor&);
    ~SpectrogramVSTAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    float sampleRate;
    float refreshRateHz;
    int spectrogramImagePos;
    int scrollSpeed;
    juce::Image spectrogramImage;
    SpectrogramVSTAudioProcessor& audioProcessor;
    juce::ColourGradient infernoGradient;
    juce::Slider noiseFloorSlider;
    juce::Slider despecklingCutoffSlider;
    juce::ComboBox fftSizeComboBox;
    juce::AudioProcessorValueTreeState::SliderAttachment noiseFloorSliderAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment despecklingCutoffSliderAttachment;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment fftSizeComboBoxAttachment;

    juce::Colour interpolateColor(float t, juce::Colour start, juce::Colour end);

    juce::Colour getColorForLevel(float level);

    void updateSpectrogram();

    void timerCallback();

    void drawSpectrogram(juce::Graphics& g, juce::Rectangle<int> area);

    void initializeColorMap();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrogramVSTAudioProcessorEditor)
};
