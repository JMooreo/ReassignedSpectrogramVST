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

    void reassignedSpectrogram(
        juce::AudioBuffer<float>& buffer,
        std::vector<std::vector<float>>& times,
        std::vector<std::vector<float>>& frequencies,
        std::vector<std::vector<float>>& magnitudes
    );

    void updateTimeWeightedWindow();

    void updateDerivativeWindow();

    void updateDerivativeTimeWeightedWindow();

    std::vector<std::vector<std::complex<float>>> stft(
        const juce::AudioBuffer<float>&inputBuffer, 
        std::vector<float>& window
    );

    void ensureEnoughVectorSpace(
        std::vector<std::vector<std::complex<float>>>& fftResult,
        std::vector<std::vector<float>>& times,
        std::vector<std::vector<float>>& frequencies,
        std::vector<std::vector<float>>& magnitudes
    );

    void resize2dVectorIfNeeded(std::vector<std::vector<float>>& vector, int numRows, int numColumns);


private:
    int sampleRate;
    std::vector<float> standardWindow;
    std::vector<float> derivativeWindow;
    std::vector<float> timeWeightedWindow;
    std::vector<float> derivativeTimeWeightedWindow;
    juce::dsp::FFT fft;
};