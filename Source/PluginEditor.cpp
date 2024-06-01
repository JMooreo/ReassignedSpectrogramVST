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
    std::cout << "SpectrogramVSTProcessorEditor constructor" << std::endl;

    setSize(512, 512);
    startTimerHz(refreshRateHz);
    spectrogramBuffer.setSize(1, spectrogramProcessingSize);

    jassert(audioProcessor.fftDataGenerator.fftSize > 0);

    // Calculate the number of frames and the size of each inner vector
    size_t numFrames = (static_cast<size_t>(spectrogramProcessingSize / (audioProcessor.fftDataGenerator.fftSize / 4))) + 1;
    size_t innerVectorSize = (static_cast<size_t>(audioProcessor.fftDataGenerator.fftSize / 2)) + 1;

    jassert(numFrames > 0);
    jassert(innerVectorSize > 0);

    magnitudes.resize(numFrames);
    frequencies.resize(numFrames);
    times.resize(numFrames);

    for (auto& innerVec : magnitudes) {
        innerVec.resize(innerVectorSize);
    }

    for (auto& innerVec : frequencies) {
        innerVec.resize(innerVectorSize);
    }

    for (auto& innerVec : times) {
        innerVec.resize(innerVectorSize);
    }
}

SpectrogramVSTAudioProcessorEditor::~SpectrogramVSTAudioProcessorEditor()
{
}

//==============================================================================
void SpectrogramVSTAudioProcessorEditor::paint (juce::Graphics& g)
{
    std::cout << "SpectrogramVSTProcessorEditor paint" << std::endl;

    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(juce::Colours::black);
    auto area = getLocalBounds();
    drawSpectrogram(g, area);
}

void SpectrogramVSTAudioProcessorEditor::resized()
{
    std::cout << "SpectrogramVSTProcessorEditor resized" << std::endl;

    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
