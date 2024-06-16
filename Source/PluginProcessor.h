/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "FFTDataGenerator.h"

//==============================================================================
/**
*/
class SpectrogramVSTAudioProcessor  : public juce::AudioProcessor
{
public:
    FFTDataGenerator fftDataGenerator;

    SpectrogramVSTAudioProcessor();
    ~SpectrogramVSTAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    void SpectrogramVSTAudioProcessor::pushIntoFFTBuffer(juce::AudioBuffer<float>& buffer);

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::vector<float> times;
    std::vector<float> frequencies;
    std::vector<float> magnitudes;
    std::vector<float> standardFFTResult;

    float noiseFloorDb = -48.f;
    float despecklingCutoff = 1.f;
    float fftSize = 1024.f;

    juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "Parameters", createParameterLayout() };

private:
    juce::AudioBuffer<float> fftBuffer;
    juce::dsp::Oscillator<float> osc;
    juce::dsp::Gain<float> gain;
    std::vector<int> fftChoiceOrders;
    void SpectrogramVSTAudioProcessor::updateParameters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrogramVSTAudioProcessor)
};
