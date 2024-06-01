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
    float sampleRate = 48000;
    float refreshRateHz = 60;
    int spectrogramImagePos = 0;
    int scrollSpeed = 1;
    int spectrogramProcessingSize = 2048;
    int spectralBufferRowSize;
    std::vector<std::vector<float>> times;
    std::vector<std::vector<float>> frequencies;
    std::vector<std::vector<float>> magnitudes;
    juce::Image spectrogramImage;
    SpectrogramVSTAudioProcessor& audioProcessor;
    juce::AudioBuffer<float> spectrogramBuffer;

    juce::Colour interpolateColor(float t, juce::Colour start, juce::Colour end)
    {
        return start.interpolatedWith(end, t);
    }

    juce::Colour getColorForLevel(float level)
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

    void updateSpectrogram() {
        // Define the height and width of the spectrogram image
        int spectrogramHeight = spectrogramImage.getHeight();
        int spectrogramWidth = spectrogramImage.getWidth();

        jassert(spectrogramHeight > 0);
        jassert(spectrogramWidth > 0);
        jassert(refreshRateHz > 0);
        // Spectrogram image position = (# pixels)
        // Times (seconds)
        // Frequency (Hz)
        // Magnitude (raw gain), eventually decibels

        // Clear the current column
        for (int y = 0; y < spectrogramHeight; ++y) {
            spectrogramImage.setPixelAt(spectrogramImagePos, y, juce::Colours::black);
        }

        // Define the min and max frequencies for the log scale
        float minFrequency = 20.f;  // Minimum frequency to display
        float maxFrequency = sampleRate / 2; // Nyquist frequency

        // Find the minimum and maximum magnitude values for normalization
        float minMagnitude = 0;
        float maxMagnitude = 4;
        float maxTimeSeconds = spectrogramWidth * (1 / refreshRateHz);

        jassert(maxTimeSeconds > 0);


        /*
        for (int i = 0; i < magnitudes.size(); i++) {
            for (int j = 0; j < magnitudes[i].size(); j++) {
                if (magnitudes[i][j] > maxMagnitude && magnitudes[i][j] < 1e4) maxMagnitude = magnitudes[i][j];
            }
        }
        */

        // Clear out all the pixels
        for (int i = 0; i < spectrogramHeight; i++) {
            for (int j = 0; j < scrollSpeed; j++) {
                spectrogramImage.setPixelAt(spectrogramImagePos - j, i, juce::Colour::greyLevel(0));
            }
        }

        for (int i = 0; i < magnitudes.size(); i++)
        {
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
                    
                    for (int i = 0; i < scrollSpeed; i++) {
                        spectrogramImage.setPixelAt(x - i, y, getColorForLevel(normalizedMagnitude));
                    }
                }
            }
        }

        // Update the position for the next 
        spectrogramImagePos += scrollSpeed;

        if (spectrogramImagePos >= spectrogramWidth) {
            spectrogramImagePos = 0;
        }
    }

    void timerCallback() override
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

    void drawSpectrogram(juce::Graphics& g, juce::Rectangle<int> area)
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrogramVSTAudioProcessorEditor)
};
