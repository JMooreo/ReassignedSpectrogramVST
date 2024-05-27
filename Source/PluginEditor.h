/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <vector>
#include <complex>
#include <cmath>
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
    juce::Image spectrogramImage;
    int spectrogramImagePos = 0;
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;
    SpectrogramVSTAudioProcessor& audioProcessor;
    juce::AudioBuffer<float> monoBuffer;

    float findMaximum(const std::vector<float>& data, int size)
    {
        return *std::max_element(data.begin(), data.begin() + size);
    }

    // Function to interpolate between two colors
    juce::Colour interpolateColor(float t, juce::Colour start, juce::Colour end)
    {
        return start.interpolatedWith(end, t);
    }

    // Function to get the color based on the amplitude level
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

    void drawNextFrameOfSpectrogramReassigned(
        std::vector<float>& deltaTime,
        std::vector<float>& frequencyBins,
        std::vector<float>& magnitudes
    ) {
        // Define the height and width of the spectrogram image
        int spectrogramHeight = spectrogramImage.getHeight();
        int spectrogramWidth = spectrogramImage.getWidth();

        // Clear the current column
        for (int y = 0; y < spectrogramHeight; ++y) {
            spectrogramImage.setPixelAt(spectrogramImagePos, y, juce::Colours::black);
        }

        // Define the min and max frequencies for the log scale
        float minFrequency = 20.0f;  // Minimum frequency to display
        float maxFrequency = 24000.0f; // Nyquist frequency (half the sample rate)

        for (int i = 0; i < deltaTime.size(); ++i) {
            // Calculate the x and y positions in the spectrogram image
            float reassignedTime = (spectrogramImagePos + (int)deltaTime[i]) % spectrogramWidth;
            if (reassignedTime < 0) {
                reassignedTime += spectrogramWidth;
            }

            float reassignedFrequency = frequencyBins[i];
            float magnitude = magnitudes[i];

            // Logarithmic mapping of the frequency
            float logFrequency = std::log10(reassignedFrequency / minFrequency + 1);
            float logMaxFrequency = std::log10(maxFrequency / minFrequency + 1);

            // Normalize the log-mapped frequency to the spectrogram dimensions

            // Ensure the indices are within bounds
            int x = static_cast<int>(reassignedTime);
            int y = spectrogramHeight - i;

            if (x >= 0 && x < spectrogramWidth && y >= 0 && y < spectrogramHeight) {
                // Calculate the color intensity based on the magnitude
                juce::Colour colour = juce::Colour::fromHSV(0.0f, 0.0f, magnitude, 1.0f);

                // Set the pixel at the calculated position
                spectrogramImage.setPixelAt(x, y, colour);
            }
        }

        // Update the position for the next frame
        if (++spectrogramImagePos >= spectrogramWidth) {
            spectrogramImagePos = 0;
        }
    }

    void drawNextFrameOfSpectrogram(std::vector<float> normalizedMagnitudes, int fftSize)
    {
        auto maxLevel = 1.f;

        auto height = spectrogramImage.getHeight();

        for (int i = 0; i < height; i++)
        {
            auto level = normalizedMagnitudes[i] - 0.5;
            spectrogramImage.setPixelAt(spectrogramImagePos, height - i, getColorForLevel(level));
        }

        // Update the spectrogram image position
        if (++spectrogramImagePos >= spectrogramImage.getWidth())
        {
            spectrogramImagePos = 0;
        }
    }

    void timerCallback() override
    {
        audioProcessor.fftDataGenerator.reassignedSpectrogram(spectrogramImage, audioProcessor.longAudioBuffer);
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
