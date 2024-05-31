/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SpectrogramVSTAudioProcessorEditor::SpectrogramVSTAudioProcessorEditor (SpectrogramVSTAudioProcessor& p)
    :   AudioProcessorEditor(&p),
        audioProcessor(p)
{
    setSize(512, 512);
    startTimerHz(refreshRateHz);
    spectrogramBuffer.setSize(1, spectrogramProcessingSize);

    // Calculate the number of frames and the size of each inner vector
    size_t numFrames = (spectrogramProcessingSize / (audioProcessor.fftDataGenerator.fftSize / 2)) + 1;
    size_t innerVectorSize = (audioProcessor.fftDataGenerator.fftSize / 2) + 1;

    magnitudes.resize(numFrames, std::vector<float>(innerVectorSize, 0.0f));
    frequencies.resize(numFrames, std::vector<float>(innerVectorSize, 0.0f));
    times.resize(numFrames, std::vector<float>(innerVectorSize, 0.0f));
}

SpectrogramVSTAudioProcessorEditor::~SpectrogramVSTAudioProcessorEditor()
{
}

//==============================================================================
void SpectrogramVSTAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(juce::Colours::black);
    auto area = getLocalBounds();
    drawSpectrogram(g, area);
}

void SpectrogramVSTAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
