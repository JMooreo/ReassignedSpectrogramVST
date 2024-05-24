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
        audioProcessor(p),
        forwardFFT(FFTOrder::order1024),
        window(1024, juce::dsp::WindowingFunction<float>::hann)
{
    setSize(800, 600);
    startTimerHz(144);
    monoBuffer.setSize(1, 1024);
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
