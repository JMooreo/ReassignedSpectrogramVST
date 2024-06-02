#pragma once
#include "FFTDataGenerator.h"

FFTDataGenerator::FFTDataGenerator(int _fftSize, int _sampleRate):
    fftSize(_fftSize),
    fft(std::log2(_fftSize)),
    standardWindow(_fftSize, 0.0f),
    derivativeWindow(_fftSize, 0.0f),
    timeWeightedWindow(_fftSize, 0.0f),
    derivativeTimeWeightedWindow(_fftSize, 0.0f),
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
    updateDerivativeTimeWeightedWindow();
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
    auto spectrumHannDerivativeTimeWeighted = stft(buffer, derivativeTimeWeightedWindow);
    int hopLength = fftSize / 4;
    int numFrames = spectrumHann.size();

    if (numFrames == 0) {
        return;
    }

    ensureEnoughVectorSpace(spectrumHann, times, frequencies, magnitudes);

    float currentFrequency = 0.f;
    float frequencyCorrectionRadians = 0.f;
    float frequencyCorrectionHz = 0.f;
    float correctedTimeSeconds = 0.f;
    float magnitude = 0.f;
    float currentTimeSeconds = 0.f;
    float fftBinSize = (float)sampleRate / (float)fftSize;
    float pi = 3.14159265358979;
    float totalTimeSeconds = (float)bufferSize / (float)sampleRate;
    float normalizedTimeStep = (float)totalTimeSeconds / (float)numFrames;
    float mixedPartialPhaseDerivative = 0.f;
    float magnitudeSquared = 0.f;

    std::complex<float> X;
    std::complex<float> X_Dh;
    std::complex<float> X_Th;
    std::complex<float> X_T_Dh;

    std::complex<float> demonimator;

    std::vector<float> mixedDerivative(spectrumHann[0].size(), 0.f);

    for (int timeIndex = 0; timeIndex < numFrames; timeIndex++) {
        currentTimeSeconds = (timeIndex * normalizedTimeStep) - totalTimeSeconds;

        for (int frequencyBin = 0; frequencyBin < spectrumHann[0].size(); frequencyBin++) {
            currentFrequency = frequencyBin * fftBinSize;
            X = spectrumHann[timeIndex][frequencyBin];
            X_Dh = spectrumHannDerivative[timeIndex][frequencyBin];
            X_Th = spectumHannTimeWeighted[timeIndex][frequencyBin];
            X_T_Dh = spectrumHannDerivativeTimeWeighted[timeIndex][frequencyBin];
            magnitude = std::abs(X);
            magnitudeSquared = magnitude * magnitude;
            
            if (magnitude == 0) {
                continue;
            }
            
            float t1 = std::real(X_T_Dh * std::conj(X) / magnitudeSquared);
            float t2 = std::real(X_Th * X_Dh / magnitudeSquared);

            mixedPartialPhaseDerivative = t1 - t2;
            mixedDerivative[frequencyBin] = mixedPartialPhaseDerivative;

            // We expect that these values should be close to 0 it they are.
            bool isImpulseComponent = std::abs(mixedPartialPhaseDerivative + 1) < 5;
            bool isSinusoidalComponent = std::abs(mixedPartialPhaseDerivative) < 5;

            if (!isImpulseComponent || !isSinusoidalComponent) {
                magnitude = 0; // filter it out.
            }

            frequencyCorrectionRadians = -std::imag((X_Dh * std::conj(X)) / magnitudeSquared);
            frequencyCorrectionHz = frequencyCorrectionRadians * sampleRate / (2 * pi);
            correctedTimeSeconds = currentTimeSeconds - std::real(X_Th * std::conj(X) / magnitudeSquared) / sampleRate;

            times[timeIndex][frequencyBin] = correctedTimeSeconds;
            frequencies[timeIndex][frequencyBin] = currentFrequency + frequencyCorrectionHz;
            magnitudes[timeIndex][frequencyBin] = magnitude; // in Gain
        }

        continue;
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
    // Calculate the number of frames and the size of each inner vector.
    // Since we're always using the same buffer size to the STFTs, actual allocations should really only happen once or twice.
    size_t numRows = fftResult.size();
    size_t numColumns = fftResult[0].size();

    resize2dVectorIfNeeded(magnitudes, numRows, numColumns);
    resize2dVectorIfNeeded(frequencies, numRows, numColumns);
    resize2dVectorIfNeeded(times, numRows, numColumns);
}

void FFTDataGenerator::updateTimeWeightedWindow() {
    timeWeightedWindow.resize(fftSize);
    int halfWidth = fftSize / 2;
    int index = 0;

    float maxValue = 0.f;

    // The center should be time = 0.
    for (int time = -halfWidth; time < halfWidth; time++) {
        index = time + halfWidth;
        float newValue = standardWindow[index] * time;
        timeWeightedWindow[index] = newValue;

        if (timeWeightedWindow[index] > maxValue) {
            maxValue = newValue;
        }
    }

    // There should be some math to calculate it without having to normalize the values.
    for (int i = 0; i < fftSize; i++) {
        timeWeightedWindow[i] = timeWeightedWindow[i] / maxValue;
    }
}

void FFTDataGenerator::updateDerivativeWindow() {
    derivativeWindow.resize(fftSize);
    int previousIndex = 0;
    int nextIndex = 0;

    for (int i = 0; i < fftSize; i++)
    {
        previousIndex = (i - 1 + fftSize) % fftSize;
        nextIndex = (i + 1) % fftSize;
        derivativeWindow[i] = (standardWindow[nextIndex] - standardWindow[previousIndex]) / 2.0;
    }
}

void FFTDataGenerator::updateDerivativeTimeWeightedWindow() {
    derivativeTimeWeightedWindow.resize(fftSize);
    for (int i = 0; i < fftSize; i++)
    {
        derivativeTimeWeightedWindow[i] = derivativeWindow[i] * i;
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