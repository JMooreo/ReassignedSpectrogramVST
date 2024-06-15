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
        refreshRateHz(240),
        spectrogramImagePos(0),
        despecklingCutoffSliderAttachment(audioProcessor.apvts, "Despeckling Cutoff", despecklingCutoffSlider),
        noiseFloorSliderAttachment(audioProcessor.apvts, "Noise Floor", noiseFloorSlider),
        fftSizeComboBoxAttachment(audioProcessor.apvts, "FFT Size", fftSizeComboBox)
{

    addAndMakeVisible(noiseFloorSlider);
    addAndMakeVisible(despecklingCutoffSlider);
    addAndMakeVisible(fftSizeComboBox);

    addAndMakeVisible(noiseFloorSliderLabel);
    addAndMakeVisible(despecklingCutoffLabel);
    addAndMakeVisible(fftSizeComboBoxLabel);

    fftSizeComboBox.addItem("1024", 1);
    fftSizeComboBox.addItem("2048", 2);
    fftSizeComboBox.addItem("4096", 3);
    fftSizeComboBox.addItem("8192", 4);

    noiseFloorSliderLabel.setText("Noise Floor (dB)", juce::dontSendNotification);
    despecklingCutoffLabel.setText("Despeckling Cutoff", juce::dontSendNotification);
    fftSizeComboBoxLabel.setText("FFT Size", juce::dontSendNotification);

    noiseFloorSliderLabel.attachToComponent(&noiseFloorSlider, true);
    despecklingCutoffLabel.attachToComponent(&despecklingCutoffSlider, true);
    fftSizeComboBoxLabel.attachToComponent(&fftSizeComboBox, true);

    setSize(862, 512);
    initializeColorMap();
    startTimerHz(refreshRateHz);
}

SpectrogramVSTAudioProcessorEditor::~SpectrogramVSTAudioProcessorEditor()
{
}

//==============================================================================
void SpectrogramVSTAudioProcessorEditor::paint (juce::Graphics& g)
{
    drawSpectrogram(g, juce::Rectangle(0, 0, 512, 512));
}

inline int mapFrequencyToPixel(float frequency, float minFreq, float maxFreq, int minHeight, int maxHeight) {
    // Uses a logarithmic transform instead of a linear one.
    return static_cast<int>((std::log(frequency / minFreq) / std::log(maxFreq / minFreq)) * (maxHeight - minHeight) + minHeight);
}

void SpectrogramVSTAudioProcessorEditor::updateSpectrogram() {
    // Define the height and width of the spectrogram image
    int spectrogramHeight = spectrogramImage.getHeight();
    int spectrogramWidth = spectrogramImage.getWidth();

    if (spectrogramHeight == 0 || spectrogramWidth == 0) {
        return;
    }

    float minFrequency = 20.f;
    float maxFrequency = 24000.f;
    float minMagnitudeDb = audioProcessor.noiseFloorDb;
    float maxMagnitudeDb = -14.9f;
    float pixelsPerSecond = 1 / refreshRateHz;
    std::vector<float> largestMagnitudeForY(spectrogramHeight, 0.f);
    int x;
    int y;

    // Clear out the old pixels
    for (y = 0; y < spectrogramHeight; y++) {
        spectrogramImage.setPixelAt(spectrogramImagePos, y, juce::Colour::greyLevel(0));
    }

    // Draw the new stuff
    for (int i = 0; i < audioProcessor.magnitudes.size(); i++) {
        x = spectrogramImagePos + audioProcessor.times[i] / pixelsPerSecond;
        x %= spectrogramWidth;
        y = spectrogramHeight - mapFrequencyToPixel(audioProcessor.frequencies[i], minFrequency, maxFrequency, 0, spectrogramHeight - 1);

        if (x >= 0 && x < spectrogramWidth && y >= 0 && y < spectrogramHeight)
        {
            float magnitude = juce::jlimit(minMagnitudeDb, maxMagnitudeDb, audioProcessor.magnitudes[i]);
            float normalizedMagnitude = juce::jmap<float>(magnitude, minMagnitudeDb, maxMagnitudeDb, 0.0f, 1.0f);

            if (normalizedMagnitude > 0 && normalizedMagnitude > largestMagnitudeForY[y]) {
                spectrogramImage.setPixelAt(x, y, infernoGradient.getColourAtPosition(normalizedMagnitude));
                largestMagnitudeForY[y] = normalizedMagnitude;
            }
        }
    }

    spectrogramImagePos += 1;

    if (spectrogramImagePos >= spectrogramWidth) {
        spectrogramImagePos = 0;
    }
}

void SpectrogramVSTAudioProcessorEditor::timerCallback()
{
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

void SpectrogramVSTAudioProcessorEditor::initializeColorMap() {
    infernoGradient.addColour(0.0, juce::Colour::fromRGB(0, 0, 4));
    infernoGradient.addColour(0.14, juce::Colour::fromRGB(40, 11, 84));
    infernoGradient.addColour(0.29, juce::Colour::fromRGB(101, 21, 110));
    infernoGradient.addColour(0.43, juce::Colour::fromRGB(159, 42, 99));
    infernoGradient.addColour(0.57, juce::Colour::fromRGB(212, 72, 66));
    infernoGradient.addColour(0.71, juce::Colour::fromRGB(245, 125, 21));
    infernoGradient.addColour(0.88, juce::Colour::fromRGB(250, 193, 39));
    infernoGradient.addColour(1.f, juce::Colour::fromRGB(252, 255, 164));
}


void SpectrogramVSTAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    auto slidersArea = bounds.removeFromRight(200).removeFromLeft(175).removeFromTop(500);

    noiseFloorSlider.setBounds(slidersArea.removeFromTop(75));
    despecklingCutoffSlider.setBounds(slidersArea.removeFromTop(50));
    fftSizeComboBox.setBounds(slidersArea.removeFromTop(75).removeFromBottom(30));
}
