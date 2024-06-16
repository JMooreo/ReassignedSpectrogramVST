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
    juce::ComboBox useReassignmentComboBox; // TODO: This should not be a combo box.

    juce::AudioProcessorValueTreeState::SliderAttachment noiseFloorSliderAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment despecklingCutoffSliderAttachment;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment fftSizeComboBoxAttachment;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment useReassignmentComboBoxAttachment;

    juce::Label noiseFloorSliderLabel;
    juce::Label despecklingCutoffLabel;
    juce::Label fftSizeComboBoxLabel;
    juce::Label useReassignmentComboBoxLabel;

    void updateSpectrogram();

    void updateSpectrogramReassigned();

    void timerCallback();

    void drawSpectrogram(juce::Graphics& g, juce::Rectangle<int> area);

    void initializeColorMap();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrogramVSTAudioProcessorEditor)
};
