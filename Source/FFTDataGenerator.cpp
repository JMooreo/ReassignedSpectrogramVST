#pragma once
#include "FFTDataGenerator.h"

FFTDataGenerator::FFTDataGenerator(int _fftSize, int _sampleRate):
    fftSize(_fftSize),
    fft(std::log2(_fftSize)),
    standardWindow(_fftSize, 0.0f),
    derivativeWindow(_fftSize, 0.0f),
    timeWeightedWindow(_fftSize, 0.0f),
    sampleRate(_sampleRate)
{
    std::cout << "FFTDataGenerator constructor" << std::endl;

    updateFFTSize(_fftSize);
}

void FFTDataGenerator::updateFFTSize(int size) {
    std::cout << "FFTDataGenerator updateFFTSize" << std::endl;

    fftSize = size;
    juce::dsp::WindowingFunction<float>::fillWindowingTables(standardWindow.data(), size, juce::dsp::WindowingFunction<float>::hann, false);
    fft = juce::dsp::FFT(std::log2(size));
    updateTimeWeightedWindow();
    updateDerivativeWindow();
}

void FFTDataGenerator::reassignedSpectrogram(
    juce::AudioBuffer<float>& buffer,
    std::vector<std::vector<float>>& times,
    std::vector<std::vector<float>>& frequencies,
    std::vector<std::vector<float>>& magnitudes
) {
    std::cout << "FFTDataGenerator reassignedSpectrogram" << std::endl;

    int bufferSize = buffer.getNumSamples();
    auto spectrumHann = stft(buffer, standardWindow);
    auto spectrumHannDerivative = stft(buffer, derivativeWindow);
    auto spectumHannTimeWeighted = stft(buffer, timeWeightedWindow);
    int hopLength = fftSize / 4;
    int numFrames = (bufferSize - fftSize) / hopLength + 1;

    if (numFrames == 0 || sampleRate == 0 || fftSize == 0 || bufferSize == 0) {
        return;
    }

    jassert(numFrames > 0);
    jassert(sampleRate > 0);
    jassert(fftSize > 0);
    jassert(bufferSize > 0);

    float currentFrequency = 0;
    float frequencyCorrection = 0;
    float timeCorrection = 0;
    float magnitude = 0;
    float currentTime = 0;
    float fftBinSize = (float)sampleRate / (float)fftSize;
    float pi = 3.14159265358979;
    float totalTimeSeconds = bufferSize / sampleRate;
    float normalizedTimeStep = totalTimeSeconds / (float)numFrames;
    std::complex<float> demonimator;

    std::cout << "FFTDataGenerator created variables" << std::endl;

    for (int timeIndex = 0; timeIndex < numFrames; timeIndex++) {
        currentTime = (timeIndex * normalizedTimeStep) - totalTimeSeconds;

        for (int frequencyBin = 0; frequencyBin < spectrumHann[0].size(); frequencyBin++) {
            currentFrequency = frequencyBin * fftBinSize;
            magnitude = std::abs(spectrumHann[timeIndex][frequencyBin]);

            demonimator = spectrumHann[timeIndex][frequencyBin];
            
            if (std::abs(demonimator) == 0) {
                continue;
            }

            jassert(std::abs(demonimator) > 0);

            frequencyCorrection = -(std::imag(spectrumHannDerivative[timeIndex][frequencyBin] / demonimator)) * 0.5 * sampleRate / pi;
            timeCorrection = std::real(spectumHannTimeWeighted[timeIndex][frequencyBin] / demonimator) / sampleRate;

            times[timeIndex][frequencyBin] = currentTime + timeCorrection; // in seconds
            frequencies[timeIndex][frequencyBin] = currentFrequency + frequencyCorrection; // in Hz
            magnitudes[timeIndex][frequencyBin] = magnitude; // in Gain
        }
    }
}

void FFTDataGenerator::updateTimeWeightedWindow() {
    std::cout << "FFTDataGenerator update time weighted window" << std::endl;
    int halfWidth = fftSize / 2;
    int index = 0;

    // The center should be 0. If the size is even, then split the middle.
    for (int time = -halfWidth; time < halfWidth; time++) {
        index = time + halfWidth;
        timeWeightedWindow[index] = standardWindow[index] * (time + 0.5);
    }
}

void FFTDataGenerator::updateDerivativeWindow() {
    std::cout << "FFTDataGenerator update derivative window" << std::endl;

    int previousIndex = 0;
    int nextIndex = 0;

    for (int i = 0; i < fftSize; ++i)
    {
        previousIndex = (i - 1 + fftSize) % fftSize;
        nextIndex = (i + 1) % fftSize;
        derivativeWindow[i] = (standardWindow[nextIndex] - standardWindow[previousIndex]) / 2.0;
    }
}

std::vector<std::vector<std::complex<float>>> FFTDataGenerator::stft(const juce::AudioBuffer<float>& inputBuffer, std::vector<float>& window) {
    std::cout << "FFTDataGenerator stft" << std::endl;
    int hopLength = fftSize / 4;

    jassert(hopLength > 0);

    int numFrames = (inputBuffer.getNumSamples() - fftSize) / hopLength + 1;
    int numOutputChannels = inputBuffer.getNumChannels();

    // Temporary buffers
    std::vector<std::complex<float>> frame(fftSize, 0.0f);
    std::vector<std::complex<float>> frameFFTResult(fftSize, 0.0f);
    std::vector<std::vector<std::complex<float>>> allFrameResults(numFrames, std::vector<std::complex<float>>(fftSize / 2 + 1));

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