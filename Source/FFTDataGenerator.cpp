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
    std::vector<float>& times,
    std::vector<float>& frequencies,
    std::vector<float>& magnitudes
) {
    int bufferSize = buffer.getNumSamples();
    auto fftResult = doFFT(buffer, standardWindow);
    auto fftWindowDerivativeResult = doFFT(buffer, derivativeWindow);
    auto fftTimeWeightedWindowResult = doFFT(buffer, timeWeightedWindow);
    auto fftTimeWeightedWindowDerivativeResult = doFFT(buffer, derivativeTimeWeightedWindow);

    resizeFFTResultVectorIfNeeded(magnitudes);
    resizeFFTResultVectorIfNeeded(frequencies);
    resizeFFTResultVectorIfNeeded(times);

    float currentFrequency = 0.f;
    float frequencyCorrectionRadians = 0.f;
    float frequencyCorrectionHz = 0.f;
    float correctedTimeSeconds = 0.f;
    float magnitude = 0.f;
    float currentTimeSeconds = 0.f;
    float fftBinSize = (float)sampleRate / (float)fftSize;
    float pi = 3.14159265358979;
    float totalTimeSeconds = (float)bufferSize / (float)sampleRate;
    float mixedPartialPhaseDerivative = 0.f;
    float magnitudeSquared = 0.f;

    std::complex<float> X;
    std::complex<float> X_Dh;
    std::complex<float> X_Th;
    std::complex<float> X_T_Dh;
    std::complex<float> demonimator;
    std::vector<float> mixedDerivative(fftSize / 2, 0.f);

    // Only go to half the FFT size because we only want the positive frequency information.
    for (int frequencyBin = 0; frequencyBin < fftSize / 2; frequencyBin++) {
        currentFrequency = frequencyBin * fftBinSize;
        X = fftResult[frequencyBin];
        X_Dh = fftWindowDerivativeResult[frequencyBin];
        X_Th = fftTimeWeightedWindowResult[frequencyBin];
        X_T_Dh = fftTimeWeightedWindowDerivativeResult[frequencyBin];
        magnitude = std::abs(X);
        magnitudeSquared = std::norm(X);
            
        if (magnitude == 0) {
            // Skip unnecessary calculations
            continue;
        }

        float t1 = std::real(X_T_Dh * std::conj(X) / magnitudeSquared);
        float t2 = std::real(X_Th * X_Dh / magnitudeSquared);

        mixedPartialPhaseDerivative = t1 - t2;
        mixedDerivative[frequencyBin] = mixedPartialPhaseDerivative;

        // We expect that these values should be close to 0 it they are.
        bool isImpulseComponent = std::abs(mixedPartialPhaseDerivative + 1) < 1;
        bool isSinusoidalComponent = std::abs(mixedPartialPhaseDerivative) < 1;

        if (!isImpulseComponent || !isSinusoidalComponent) {
            magnitude = 0; // filter it out.
        }

        frequencyCorrectionRadians = -std::imag((X_Dh * std::conj(X)) / magnitudeSquared);
        frequencyCorrectionHz = frequencyCorrectionRadians * sampleRate / (2 * pi);
        correctedTimeSeconds = 0; // currentTimeSeconds - std::real(X_Th * std::conj(X) / magnitudeSquared) / sampleRate;

        times[frequencyBin] = correctedTimeSeconds;
        frequencies[frequencyBin] = currentFrequency + frequencyCorrectionHz;

        // in Gain, such that a known reassigned sine wave at an amplitude of 1 gets a magnitude of 1.
        // I'm not sure where the other factor of 2 is coming from.
        magnitudes[frequencyBin] = 2 * magnitude; 
    }

    return;
}

void FFTDataGenerator::resizeFFTResultVectorIfNeeded(std::vector<float>& vector) {
    // When we do an FFT, we get both the positive and negative frequency information, which is mirrored around the center.
    // We only want the positive frequency information, so, we only care about the first half of the FFT result.
    if (vector.size() != fftSize / 2) {
        vector.resize(fftSize / 2);
    }
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
    // Use linear approximation to get the derivative of the window function.
    // f'(x) = f(x + h) - f(x - h) / 2h

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

std::vector<std::complex<float>> FFTDataGenerator::doFFT(const juce::AudioBuffer<float>& inputBuffer, std::vector<float>& window) {
    int channel = 0; // Left channel only for now

    std::vector<std::complex<float>> windowedSignal(fftSize, 0.0f);
    std::vector<std::complex<float>> result(fftSize, 0.0f);

    const float* inputChannelData = inputBuffer.getReadPointer(0);
            
    // Multiply by the windowing function to prevent FFT artifacts.
    for (int i = 0; i < fftSize; i++) {
        windowedSignal[i] = inputChannelData[i] * window[i];
    }

    // Here we are getting the complex result from the FFT, not just the magnitudes.
    // This is because we need the imaginary parts for phase info.
    fft.perform(windowedSignal.data(), result.data(), false);

    // Normalize the result by the length of the FFT.
    // E.g. if a test signal with an amplitude of 1 goes in, we should see an amplitude of 1 in the corresponding POSITIVE frequency bin.
    // The FFT result splits the amplitude over the positive and negative frequency components, so, this is why we divide by half the FFT size.
    for (int i = 0; i < fftSize; i++) {
        result[i] /= (fftSize / 2.0);
    }

    return result;
}