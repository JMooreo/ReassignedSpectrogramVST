/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Fifo.h"
#include "SingleChannelSampleFifo.h"
#include "Channel.h"
#include "ChainSettings.h"
#include "FFTDataGenerator.h"

//==============================================================================
SpectrogramVSTAudioProcessor::SpectrogramVSTAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

SpectrogramVSTAudioProcessor::~SpectrogramVSTAudioProcessor()
{
}

//==============================================================================
const juce::String SpectrogramVSTAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SpectrogramVSTAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SpectrogramVSTAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SpectrogramVSTAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SpectrogramVSTAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SpectrogramVSTAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SpectrogramVSTAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SpectrogramVSTAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SpectrogramVSTAudioProcessor::getProgramName (int index)
{
    return {};
}

void SpectrogramVSTAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void SpectrogramVSTAudioProcessor::updateSettings()
{
    //auto chainSettings = getChainSettings(apvts);

    //updateLowCutFilters(chainSettings);
    //updatePeakFilter(chainSettings);
    //updateHighCutFilters(chainSettings);
}

//==============================================================================
void SpectrogramVSTAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;

    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;

    // updateSettings()

    leftChannelFifo.prepare(samplesPerBlock);
    rightChannelFifo.prepare(samplesPerBlock);

    leftChannelFFTDataGenerator.changeFFTSize(FFTOrder::order1024);

    // Create an oscillator that will produce a test frequency for us.
    osc.initialise([](float x) { return std::sin(x); });

    spec.numChannels = getTotalNumOutputChannels();
    osc.prepare(spec);
    osc.setFrequency(440);
}

void SpectrogramVSTAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SpectrogramVSTAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SpectrogramVSTAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    leftChannelFifo.update(buffer);
    rightChannelFifo.update(buffer);
}

//==============================================================================
bool SpectrogramVSTAudioProcessor::hasEditor() const {
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SpectrogramVSTAudioProcessor::createEditor() {
    //return new juce::GenericAudioProcessorEditor(*this);
    return new SpectrogramVSTAudioProcessorEditor (*this);
}

//==============================================================================
void SpectrogramVSTAudioProcessor::getStateInformation (juce::MemoryBlock& destData) {
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void SpectrogramVSTAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

juce::AudioProcessorValueTreeState::ParameterLayout SpectrogramVSTAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>("Low Frequency",
        "Low Frequency",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        20.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("High Frequency",
        "High Frequency",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.9f),
        20000.f));

    juce::StringArray fftOptions;
    for (int i = 9; i < 12; i++) {
        juce::String str;

        str << (1 << i); // 2^i

        fftOptions.add(str);
    }

    layout.add(std::make_unique<juce::AudioParameterChoice>("FFT Size", "FFT Size", fftOptions, 0));


    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpectrogramVSTAudioProcessor();
}
