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
        sampleRate(48000),
        refreshRateHz(120),
        spectrogramProcessingSize(2048),
        scrollSpeed(1),
        spectrogramImagePos(0)
{
    setSize(512, 512);
    startTimerHz(refreshRateHz);
    spectrogramBuffer.setSize(1, spectrogramProcessingSize);
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

void SpectrogramVSTAudioProcessorEditor::updateSpectrogram() {
    // Define the height and width of the spectrogram image
    int spectrogramHeight = spectrogramImage.getHeight();
    int spectrogramWidth = spectrogramImage.getWidth();

    if (spectrogramHeight == 0 || spectrogramWidth == 0) {
        return;
    }

    // Spectrogram image position = (# pixels)
    // Times (seconds)
    // Frequency (Hz)
    // Magnitude (raw gain), eventually decibels

    // Clear the current column

    // Define the min and max frequencies for the log scale
    float minFrequency = 20.f;  // Minimum frequency to display
    float maxFrequency = (float)sampleRate / 2; // Nyquist frequency

    // Find the minimum and maximum magnitude values for normalization
    float minMagnitude = 0.f;
    float maxMagnitude = 8.f;
    float maxTimeSeconds = (float)spectrogramWidth * (1 / (float)refreshRateHz);

    // Clear out the old pixels
    for (int y = 0; y < spectrogramHeight; y++) {
        for (int duplicateIndex = 0; duplicateIndex < scrollSpeed; duplicateIndex++) {
            // I'm not sure why we have to add an offset of +5. 
            // It may have something to do with the reassigned data being shifted incorrectly.
            int x = (spectrogramImagePos - duplicateIndex + 5) % spectrogramWidth; 
            spectrogramImage.setPixelAt(x, y, juce::Colour::greyLevel(0));
        }
    }

    for (int i = 0; i < magnitudes.size(); i++) {
        for (int j = 0; j < magnitudes[i].size(); j++)
        {
            int x = spectrogramImagePos + (1 - (times[i][j] / maxTimeSeconds)) * spectrogramWidth;
            x %= spectrogramWidth;

            float logFrequency = std::log10(frequencies[i][j] / 20.0f); // 20 Hz is the lowest frequency of interest
            float maxLogFrequency = std::log10((float)sampleRate / 2.0f / 20.0f);
            int y = juce::jmap<float>(logFrequency, 0.0f, maxLogFrequency, (float)spectrogramHeight, 0.0f);

            if (x >= 0 && x < spectrogramWidth && y >= 0 && y < spectrogramHeight)
            {
                float magnitude = magnitudes[i][j];
                float normalizedMagnitude = juce::jmap<float>(magnitude, minMagnitude, maxMagnitude, 0.0f, 1.0f);

                spectrogramImage.setPixelAt(x - i, y, getColorForLevel(normalizedMagnitude));
            }
        }
    }

    // Update the position for the next 
    spectrogramImagePos += scrollSpeed;

    if (spectrogramImagePos >= spectrogramWidth) {
        spectrogramImagePos = 0;
    }
}

void SpectrogramVSTAudioProcessorEditor::timerCallback()
{
    int longBufferSize = audioProcessor.longAudioBuffer.getNumSamples();

    if (longBufferSize > spectrogramProcessingSize) {
        // Put the new data into the buffer
        juce::FloatVectorOperations::copy(
            spectrogramBuffer.getWritePointer(0, 0), // destination
            audioProcessor.longAudioBuffer.getReadPointer(0, longBufferSize - spectrogramProcessingSize), // source
            spectrogramProcessingSize // size
        );

        // Put reassigned spectogram data into the buffers
        audioProcessor.fftDataGenerator.reassignedSpectrogram(spectrogramBuffer, times, frequencies, magnitudes);
    }

    updateSpectrogram();
    repaint();
}

void SpectrogramVSTAudioProcessorEditor::drawSpectrogram(juce::Graphics& g, juce::Rectangle<int> area)
{
    auto w = area.getWidth();
    auto h = area.getHeight();

    if (spectrogramImage.isNull())
    {
        spectrogramImage = juce::Image(juce::Image::RGB, w, h, true);
        spectrogramImagePos = 0;
    }

    g.drawImage(spectrogramImage, area.toFloat());
}

juce::Colour SpectrogramVSTAudioProcessorEditor::getColorForLevel(float level)
{
    if (level <= 0.33f)
    {
        return interpolateColor(level / 0.33f, juce::Colours::black, juce::Colours::purple);
    }
    else if (level <= 0.66f)
    {
        return interpolateColor((level - 0.33f) / 0.33f, juce::Colours::purple, juce::Colours::orange);
    }
    else
    {
        return interpolateColor((level - 0.66f) / 0.34f, juce::Colours::orange, juce::Colours::yellow);
    }
}

juce::Colour SpectrogramVSTAudioProcessorEditor::interpolateColor(float t, juce::Colour start, juce::Colour end)
{
    return start.interpolatedWith(end, t);
}

void SpectrogramVSTAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
