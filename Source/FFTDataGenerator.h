#pragma once
#include <JuceHeader.h>

class FFTDataGenerator
{
public:
    int fftSize;

    FFTDataGenerator(int _fftSize, int _sampleRate);

    void reassignedSpectrogram(
        juce::AudioBuffer<float>& buffer,
        std::vector<float>& times,
        std::vector<float>& frequencies,
        std::vector<float>& magnitudes
    );

    void updateTimeWeightedWindow();

    void updateDerivativeWindow();

    void updateDerivativeTimeWeightedWindow();

    std::vector<std::complex<float>> doFFT(
        const juce::AudioBuffer<float>&inputBuffer, 
        std::vector<float>& window
    );

    void FFTDataGenerator::ensureFFTResultSpace(std::vector<float>& vector);

    void FFTDataGenerator::updateParameters(float despeckleCutoff, float fftSize);

private:
    int sampleRate;
    std::vector<float> standardWindow;
    std::vector<float> derivativeWindow;
    std::vector<float> timeWeightedWindow;
    std::vector<float> derivativeTimeWeightedWindow;
    juce::dsp::FFT fft;
    float despecklingCutoff;
};