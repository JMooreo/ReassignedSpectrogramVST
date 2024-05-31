#pragma once
#include "juce_dsp/juce_dsp.h"
#include <cmath>

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
    void reassignedSpectrogram(
        juce::AudioBuffer<float>& buffer,
        const float fftSize,
        std::vector<std::vector<float>>& times,
        std::vector<std::vector<float>>& frequencies,
        std::vector<std::vector<float>>& magnitudes
    ) {
        float negativeInfinity = -96.f;
        float ref_power = 1e-6;
        int bufferSize = buffer.getNumSamples();

        std::vector<float> stftWindow(fftSize);
        juce::dsp::WindowingFunction<float>::fillWindowingTables(stftWindow.data(), fftSize, juce::dsp::WindowingFunction<float>::hann, false);

        auto S_h = stft(buffer, fftSize, stftWindow);
        auto S_dh = stft(buffer, fftSize, getDerivativeWindow(fftSize, stftWindow));
        auto S_th = stft(buffer, fftSize, getTimeWeightedWindow(fftSize, stftWindow));

        magnitudes.resize(S_h.size(), std::vector<float>(S_h[0].size(), 0.0f));
        frequencies.resize(S_h.size(), std::vector<float>(S_h[0].size(), 0.0f));
        times.resize(S_h.size(), std::vector<float>(S_h[0].size(), 0.0f));

        float numSTFTs = S_h.size();
        float currentFrequency = 0;
        float correctedTime = 0;
        float correctedFrequency = 0;
        float frequencyCorrection = 0;
        float timeCorrection = 0;
        float magnitude = 0;
        float stftStartTimeSeconds = 0;
        float fftBinSize = sampleRate / fftSize;
        float pi = 3.14159265358979;
        float totalTimeSeconds = bufferSize / sampleRate;
        float normalizedTimeStep = totalTimeSeconds / numSTFTs;

        for (int i = 0; i < numSTFTs; i++) {
            stftStartTimeSeconds = i * normalizedTimeStep;

            for (int j = 0; j < S_h[0].size(); j++) {
                currentFrequency = (j * fftBinSize);
                magnitude = std::abs(S_h[i][j]);

                frequencyCorrection = -(std::imag(S_dh[i][j] / S_h[i][j])) * 0.5 * sampleRate / pi;
                timeCorrection = std::real(S_th[i][j] / S_h[i][j]) / sampleRate;

                times[i][j] = stftStartTimeSeconds + timeCorrection; // in seconds
                frequencies[i][j] = currentFrequency + frequencyCorrection; // in Hz
                magnitudes[i][j] = magnitude; // in Gain
            }
        }
    }

    void drawSpectrogram(
        juce::Image spectrogramImage,
        std::vector<std::vector<float>>& magnitudes,
        std::vector<std::vector<float>>& times,
        std::vector<std::vector<float>>& frequencies,
        float maxTimeSeconds
    )
    {
        const int width = spectrogramImage.getWidth();
        const int height = spectrogramImage.getHeight();

        // Reset the image
        spectrogramImage.clear(juce::Rectangle<int>(0, 0, width, height));

        // Find the minimum and maximum magnitude values for normalization
        float minMagnitude = 0;
        float maxMagnitude = 0.5;

        for (int i = 0; i < magnitudes.size(); i++)
        {
            for (int j = 0; j < magnitudes[i].size(); j++)
            {
                int x = (1 - (times[i][j] / maxTimeSeconds)) * width; // 0 time is at the right

                float logFrequency = std::log10(frequencies[i][j] / 20.0f); // 20 Hz is the lowest frequency of interest
                float maxLogFrequency = std::log10((float)sampleRate / 2.0f / 20.0f);
                int y = juce::jmap<float>(logFrequency, 0.0f, maxLogFrequency, (float)height, 0.0f);

                if (x >= 0 && x < width && y >= 0 && y < height)
                {
                    float magnitude = magnitudes[i][j];
                    float normalizedMagnitude = juce::jmap<float>(magnitude, minMagnitude, maxMagnitude, 0.0f, 1.0f);
                    spectrogramImage.setPixelAt(x, y, juce::Colour::greyLevel(normalizedMagnitude));
                }
            }
        }
    }

    std::vector<float> getTimeWeightedWindow(int fftSize, std::vector<float>& window) {
        std::vector<float> windowTimes(fftSize, 0);
        int halfWidth = fftSize / 2;

        // The center should be 0. If the size is even, then split the middle.
        for (int time = -halfWidth; time < halfWidth; time++) {
            int index = time + halfWidth;
            windowTimes[index] = (time + 0.5) * window[index];
        }

        return windowTimes;
    }

    std::vector<float> getDerivativeWindow(int n_fft, const std::vector<float>& window) {
        int windowSize = window.size();
        std::vector<float> grad(windowSize, 0.0f);

        for (int i = 0; i < windowSize; ++i)
        {
            int prevIdx = (i - 1 + windowSize) % windowSize;
            int nextIdx = (i + 1) % windowSize;
            grad[i] = (window[nextIdx] - window[prevIdx]) / 2.0;
        }

        return grad;
    }

    std::vector<std::vector<float>> getLongAudioBufferData() const {
        int numChannels = longAudioBuffer.getNumChannels();
        int numSamples = longAudioBuffer.getNumSamples();
        std::vector<std::vector<float>> data(numChannels, std::vector<float>(numSamples));

        for (int channel = 0; channel < numChannels; ++channel) {
            const float* channelData = longAudioBuffer.getReadPointer(channel);
            std::copy(channelData, channelData + numSamples, data[channel].begin());
        }

        return data;
    }

    std::vector<std::vector<std::complex<float>>> stft(
        const juce::AudioBuffer<float>& inputBuffer,
        int n_fft,
        std::vector<float>& window,
        int hop_length = -1,
        int win_length = -1,
        bool center = true
    ) {
        if (win_length == -1) win_length = n_fft;
        if (hop_length == -1) hop_length = win_length / 4;

        if (hop_length <= 0) throw std::invalid_argument("hop_length must be a positive integer");

        // Pad the window to n_fft size
        if (n_fft > win_length) {
            window.resize(n_fft, 0.0f);
        }

        // Number of frames
        int n_frames = (inputBuffer.getNumSamples() + (center ? n_fft : 0) - n_fft) / hop_length + 1;

        // Prepare the output buffer
        int numOutputChannels = inputBuffer.getNumChannels();

        // FFT setup
        juce::dsp::FFT fft(std::log2(n_fft));

        // Temporary buffers
        std::vector<std::complex<float>> frame(n_fft, 0.0f);
        std::vector<std::complex<float>> frameFFTResult(n_fft, 0.0f);
        std::vector<std::vector<std::complex<float>>> allFrameResults(n_frames, std::vector<std::complex<float>>(n_fft / 2 + 1));
        std::vector<float> spectrum(n_fft, 0.0f);

        for (int channel = 0; channel < numOutputChannels; channel++) {
            const float* inputChannelData = inputBuffer.getReadPointer(channel);
            
            for (int i = 0; i < n_frames; i++) {
                int start = i * hop_length;

                for (int j = 0; j < n_fft; j++) {
                    frame[j] = inputChannelData[start + j] * window[j];
                }

                // Perform FFT
                fft.perform(frame.data(), frameFFTResult.data(), false);

                // Copy the result into the 2d array.
                std::memcpy(
                    allFrameResults[i].data(), // Destination
                    frameFFTResult.data(),     // Source
                    (n_fft / 2 + 1) * sizeof(std::complex<float>) // Number of bytes to copy
                );
            }
        }

        return allFrameResults;
    }

private:
    float sampleRate = 48000.f;
    juce::AudioBuffer<float> longAudioBuffer; // Holds the stereo audio for the last N seconds
};