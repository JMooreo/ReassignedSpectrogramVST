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
    updateFFTSize(_fftSize);
}

void FFTDataGenerator::updateFFTSize(int size) {
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
    int bufferSize = buffer.getNumSamples();
    auto spectrumHann = stft(buffer, standardWindow);
    auto spectrumHannDerivative = stft(buffer, derivativeWindow);
    auto spectumHannTimeWeighted = stft(buffer, timeWeightedWindow);
    int hopLength = fftSize / 4;
    int numFrames = spectrumHann.size();

    if (numFrames == 0) {
        return;
    }

    ensureEnoughVectorSpace(spectrumHann, times, frequencies, magnitudes);

    float currentFrequency = 0.f;
    float frequencyCorrection = 0.f;
    float timeCorrection = 0.f;
    float magnitude = 0.f;
    float currentTime = 0.f;
    float fftBinSize = (float)sampleRate / (float)fftSize;
    float pi = 3.14159265358979;
    float totalTimeSeconds = (float)bufferSize / (float)sampleRate;
    float normalizedTimeStep = (float)totalTimeSeconds / (float)numFrames;
    std::complex<float> demonimator;

    for (int timeIndex = 0; timeIndex < numFrames; timeIndex++) {
        currentTime = (timeIndex * normalizedTimeStep) - totalTimeSeconds;

        for (int frequencyBin = 0; frequencyBin < spectrumHann[0].size(); frequencyBin++) {
            currentFrequency = frequencyBin * fftBinSize;
            magnitude = std::abs(spectrumHann[timeIndex][frequencyBin]);

            demonimator = spectrumHann[timeIndex][frequencyBin];
            
            if (std::real(demonimator) == 0 || std::imag(demonimator) == 0) {
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

void FFTDataGenerator::resize2dVectorIfNeeded(std::vector<std::vector<float>>& vector, int numRows, int numColumns) {
    if (vector.size() != numRows || vector[0].size() != numColumns) {
        vector.resize(numRows);

        for (auto& vec : vector) {
            if (vec.size() != numColumns) {
                vec.resize(numColumns, 0.0f);
            }
        }
    }
}

void FFTDataGenerator::ensureEnoughVectorSpace(
    std::vector<std::vector<std::complex<float>>>& fftResult,
    std::vector<std::vector<float>>& times,
    std::vector<std::vector<float>>& frequencies,
    std::vector<std::vector<float>>& magnitudes
) {
    // Calculate the number of frames and the size of each inner vector
    size_t numRows = fftResult.size();
    size_t numColumns = fftResult[0].size();

    resize2dVectorIfNeeded(magnitudes, numRows, numColumns);
    resize2dVectorIfNeeded(frequencies, numRows, numColumns);
    resize2dVectorIfNeeded(times, numRows, numColumns);
}

void FFTDataGenerator::updateTimeWeightedWindow() {
    int halfWidth = fftSize / 2;
    int index = 0;

    // The center should be 0. If the size is even, then split the middle.
    for (int time = -halfWidth; time < halfWidth; time++) {
        index = time + halfWidth;
        timeWeightedWindow[index] = standardWindow[index] * (time + 0.5);
    }
}

void FFTDataGenerator::updateDerivativeWindow() {
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
    int hopLength = fftSize / 4;
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