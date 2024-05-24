/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

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

    void drawNextFrameOfSpectrogram(std::vector<float> fftData, int fftSize)
    {
        using namespace juce;

        auto maxLevel = findMaximum(fftData, fftSize / 2);

        for (int i = 1; i < fftSize / 2; ++i) {

            float level = 0;

            if (maxLevel > -96.f)
            {
                level = jmap(fftData[i], -96.f, maxLevel, 0.0f, 1.0f);
            }
             
            spectrogramImage.setPixelAt(spectrogramImagePos, (fftSize / 2) - i, Colour::fromFloatRGBA(level, level, level, 1.0f));
        }

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

                juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0),
                    monoBuffer.getReadPointer(0, size),
                    monoBuffer.getNumSamples() - size);

                juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
                    tempIncomingBuffer.getReadPointer(0, 0),
                    size);

                audioProcessor.leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -96.f);
            }
        }

        if (audioProcessor.leftChannelFFTDataGenerator.getFFTData(fftData))
        {
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
