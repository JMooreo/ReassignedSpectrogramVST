#pragma once
#include <JuceHeader.h>

enum FFTOrder
{
    order1024 = 10,
    order2048 = 11,
    order4096 = 12,
    order8192 = 13
};

class FFTDataGenerator
{
public:
    int fftSize;

    FFTDataGenerator(int _fftSize, int _sampleRate);

    void updateFFTSize(int size);

    void FFTDataGenerator::reassignedSpectrogram(
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

    void resizeFFTResultVectorIfNeeded(std::vector<float>& vector);

private:
    int sampleRate;
    std::vector<float> standardWindow;
    std::vector<float> derivativeWindow;
    std::vector<float> timeWeightedWindow;
    std::vector<float> derivativeTimeWeightedWindow;
    juce::dsp::FFT fft;
};