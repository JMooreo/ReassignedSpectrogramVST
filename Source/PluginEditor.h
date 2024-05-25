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

    void drawNextFrameOfSpectrogram(std::vector<float> fftData, int fftSize)
    {
        auto maxLevel = 10.f;
        auto height = spectrogramImage.getHeight();
        int prevY = juce::jmap(std::log10(1.0 + 0), 0.0, std::log10(1.0 + fftSize / 2), 0.0, (double)height);

        for (int i = 1; i < fftSize / 2; ++i)
        {
            float level = 0;

            if (maxLevel > -96.f)
            {
                level = juce::jmap(fftData[i], -96.f, maxLevel, 0.0f, 1.0f);
            }

            // Map frequency index to logarithmic scale
            int y = juce::jmap(std::log10(1.0 + i), 0.0, std::log10(1.0 + fftSize / 2), 0.0, (double)height);

            // Ensure y is within the bounds of the image height
            y = juce::jlimit(0, height - 1, y);

            // Fill the gap between prevY and y
            for (int fillY = std::min(prevY, y); fillY <= std::max(prevY, y); ++fillY)
            {
                spectrogramImage.setPixelAt(spectrogramImagePos, height - fillY - 1, getColorForLevel(level));
            }

            prevY = y;
        }

        // Update the spectrogram image position
        if (++spectrogramImagePos >= spectrogramImage.getWidth())
        {
            spectrogramImagePos = 0;
        }
    }

    void timerCallback() override
    {

        juce::AudioBuffer<float> tempIncomingBuffer;
        std::vector<float> fftData;

        while (audioProcessor.leftChannelFifo.getNumCompleteBuffersAvailable() > 0)
        {
            if (audioProcessor.leftChannelFifo.getAudioBuffer(tempIncomingBuffer))
            {
                auto size = tempIncomingBuffer.getNumSamples();

                juce::FloatVectorOperations::copy(
                    monoBuffer.getWritePointer(0, 0),
                    monoBuffer.getReadPointer(0, size),
                    monoBuffer.getNumSamples() - size
                );

                juce::FloatVectorOperations::copy(
                    monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
                    tempIncomingBuffer.getReadPointer(0, 0),
                    size
                );

                audioProcessor.leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -96.f);
            }
        }

        if (audioProcessor.leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0)
        {
            audioProcessor.leftChannelFFTDataGenerator.getFFTData(fftData);
            drawNextFrameOfSpectrogram(fftData, audioProcessor.leftChannelFFTDataGenerator.getFFTSize());
            repaint();
        }
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
