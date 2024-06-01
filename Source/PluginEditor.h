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
    int spectrogramProcessingSize;
    std::vector<std::vector<float>> times;
    std::vector<std::vector<float>> frequencies;
    std::vector<std::vector<float>> magnitudes;
    juce::Image spectrogramImage;
    SpectrogramVSTAudioProcessor& audioProcessor;
    juce::AudioBuffer<float> spectrogramBuffer;

    juce::Colour interpolateColor(float t, juce::Colour start, juce::Colour end);

    juce::Colour getColorForLevel(float level);

    void updateSpectrogram();

    void timerCallback();

    void drawSpectrogram(juce::Graphics& g, juce::Rectangle<int> area);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrogramVSTAudioProcessorEditor)
};
