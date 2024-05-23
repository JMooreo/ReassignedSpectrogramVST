#pragma once
struct ChainSettings
{
    int fftSize{ 1024 };
    float minFrequency{ 20.f }, maxFrequency{ 20000.f };
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);