#pragma once

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

    FFTDataGenerator(int _fftSize, int _sampleRate): 
        fftSize(_fftSize),
        fft(std::log2(fftSize)),
        standardWindow(fftSize, 0.0f),
        derivativeWindow(fftSize, 0.0f),
        timeWeightedWindow(fftSize, 0.0f),
        sampleRate(_sampleRate)
    {
        updateFFTSize(fftSize);
    }

    void updateFFTSize(int size) {
        fftSize = size;
        juce::dsp::WindowingFunction<float>::fillWindowingTables(standardWindow.data(), fftSize, juce::dsp::WindowingFunction<float>::hann, false);
        fft = juce::dsp::FFT(std::log2(size));
        updateTimeWeightedWindow();
        updateDerivativeWindow();
    }

    void reassignedSpectrogram(
        juce::AudioBuffer<float>& buffer,
        std::vector<std::vector<float>>& times,
        std::vector<std::vector<float>>& frequencies,
        std::vector<std::vector<float>>& magnitudes
    ) {
        float negativeInfinity = -96.f;
        int bufferSize = buffer.getNumSamples();

        auto spectrumHann = stft(buffer, standardWindow);
        auto spectrumHannDerivative = stft(buffer, derivativeWindow);
        auto spectumHannTimeWeighted = stft(buffer, timeWeightedWindow);

        int numFrames = spectrumHann.size();

        if (numFrames == 0) {
            return;
        }

        float currentFrequency = 0;
        float frequencyCorrection = 0;
        float timeCorrection = 0;
        float magnitude = 0;
        float currentTime = 0;
        float fftBinSize = (float)sampleRate / (float)fftSize;
        float pi = 3.14159265358979;
        float totalTimeSeconds = bufferSize / sampleRate;
        float normalizedTimeStep = totalTimeSeconds / (float)numFrames;

        for (int timeIndex = 0; timeIndex < numFrames; timeIndex++) {
            currentTime = (timeIndex * normalizedTimeStep) - totalTimeSeconds;

            for (int frequencyBin = 0; frequencyBin < spectrumHann[0].size(); frequencyBin++) {
                currentFrequency = (frequencyBin * fftBinSize);
                magnitude = std::abs(spectrumHann[timeIndex][frequencyBin]);

                frequencyCorrection = -(std::imag(spectrumHannDerivative[timeIndex][frequencyBin] / spectrumHann[timeIndex][frequencyBin])) * 0.5 * sampleRate / pi;
                timeCorrection = std::real(spectumHannTimeWeighted[timeIndex][frequencyBin] / spectrumHann[timeIndex][frequencyBin]) / sampleRate;

                times[timeIndex][frequencyBin] = currentTime + timeCorrection; // in seconds
                frequencies[timeIndex][frequencyBin] = currentFrequency + frequencyCorrection; // in Hz
                magnitudes[timeIndex][frequencyBin] = magnitude; // in Gain
            }
        }
    }

    void updateTimeWeightedWindow() {
        int halfWidth = fftSize / 2;
        int index = 0;

        // The center should be 0. If the size is even, then split the middle.
        for (int time = -halfWidth; time < halfWidth; time++) {
            index = time + halfWidth;
            timeWeightedWindow[index] = standardWindow[index] * (time + 0.5);
        }
    }

    void updateDerivativeWindow() {
        int previousIndex = 0;
        int nextIndex = 0;

        for (int i = 0; i < fftSize; ++i)
        {
            previousIndex = (i - 1 + fftSize) % fftSize;
            nextIndex = (i + 1) % fftSize;
            derivativeWindow[i] = (standardWindow[nextIndex] - standardWindow[previousIndex]) / 2.0;
        }
    }

    std::vector<std::vector<std::complex<float>>> stft(const juce::AudioBuffer<float>& inputBuffer, std::vector<float>& window) {
        int hopLength = fftSize / 4;
        int numFrames = (inputBuffer.getNumSamples() - fftSize) / hopLength + 1;
        int numOutputChannels = inputBuffer.getNumChannels();

        // Temporary buffers
        std::vector<std::complex<float>> frame(fftSize, 0.0f);
        std::vector<std::complex<float>> frameFFTResult(fftSize, 0.0f);
        std::vector<std::vector<std::complex<float>>> allFrameResults(numFrames, std::vector<std::complex<float>>(fftSize / 2 + 1));
        std::vector<float> spectrum(fftSize, 0.0f);

        for (int channel = 0; channel < numOutputChannels; channel++) {
            const float* inputChannelData = inputBuffer.getReadPointer(channel);
            
            for (int i = 0; i < numFrames; i++) {
                int start = i * hopLength;

                for (int j = 0; j < fftSize; j++) {
                    frame[j] = inputChannelData[start + j] * window[j];
                }

                // Perform FFT
                fft.perform(frame.data(), frameFFTResult.data(), false);

                // Copy the result into the 2d array.
                std::memcpy(
                    allFrameResults[i].data(), // Destination
                    frameFFTResult.data(),     // Source
                    (fftSize / 2 + 1) * sizeof(std::complex<float>) // Number of bytes to copy
                );
            }
        }

        return allFrameResults;
    }

private:
    int sampleRate;
    std::vector<float> standardWindow;
    std::vector<float> derivativeWindow;
    std::vector<float> timeWeightedWindow;
    juce::dsp::FFT fft;
};