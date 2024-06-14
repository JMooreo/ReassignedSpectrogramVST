/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SpectrogramVSTAudioProcessor::SpectrogramVSTAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : 
        AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
        fftDataGenerator(2048, 48000)
#endif
{
    for (int i = 10; i < 13; i++) {
        fftChoiceOrders.push_back(i);
    }
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

//==============================================================================
void SpectrogramVSTAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2;
    spec.sampleRate = sampleRate;

    osc.initialise([](float x) { return std::sin(x); });
    osc.prepare(spec);
    osc.setFrequency(40);

    gain.setGainLinear(0.1f);
    updateParameters();
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
    /*
        juce::dsp::AudioBlock<float> block(buffer);
        buffer.clear();
        juce::dsp::ProcessContextReplacing<float> stereoContext(block);
        osc.process(stereoContext);
        gain.process(stereoContext);
    */

    pushIntoFFTBuffer(buffer);
    updateParameters(); // TODO: This is pretty expensive and we don't have to do this every time!
    fftDataGenerator.reassignedSpectrogram(fftBuffer, times, frequencies, magnitudes);
}

void SpectrogramVSTAudioProcessor::pushIntoFFTBuffer(juce::AudioBuffer<float>& buffer) {
    int size = buffer.getNumSamples();

    for (int channel = 0; channel < 2; channel++) {
        // Move the existing data back by the size of the new buffer
        juce::FloatVectorOperations::copy( 
            fftBuffer.getWritePointer(channel, 0), // destination
            fftBuffer.getReadPointer(channel, size), // source
            fftBuffer.getNumSamples() - size // num values
        );

        // Put the new data into the buffer
        juce::FloatVectorOperations::copy(
            fftBuffer.getWritePointer(channel, fftBuffer.getNumSamples() - size), // destination
            buffer.getReadPointer(channel, 0), // source
            size
        );
    }
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
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);

    if (tree.isValid()) {
        apvts.replaceState(tree);
        updateParameters();
    }
}

void SpectrogramVSTAudioProcessor::updateParameters() {
    noiseFloorDb = apvts.getRawParameterValue("Noise Floor")->load();
    despecklingCutoff = apvts.getRawParameterValue("Despeckling Cutoff")->load();

    int fftIndex = apvts.getRawParameterValue("FFT Size")->load();
    fftSize = 1 << fftChoiceOrders[fftIndex];

    fftBuffer.setSize(2, fftSize);
    fftDataGenerator.updateParameters(fftSize, despecklingCutoff);
}

juce::AudioProcessorValueTreeState::ParameterLayout 
SpectrogramVSTAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(
        std::make_unique<juce::AudioParameterFloat>(
            "Noise Floor",
            "Noise Floor",
            -96.f, // min
            -15.f, // max
            -48.f // default
        )
    );

    layout.add(
        std::make_unique<juce::AudioParameterFloat>(
            "Despeckling Cutoff",
            "Despeckling Cutoff",
            0.f, // min
            10.f, // max
            1.f // default
        )
    );

    juce::StringArray fftChoices;

    for (int i = 10; i < 13; i++) {
        juce::String str;
        str << (1 << i);
        fftChoices.add(str);
    }

    layout.add(
        std::make_unique<juce::AudioParameterChoice>(
            "FFT Size",
            "FFT Size",
            fftChoices,
            0
        )
    );

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpectrogramVSTAudioProcessor();
}
