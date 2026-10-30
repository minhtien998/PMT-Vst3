#pragma once
#include <JuceHeader.h>

class CleanMixEQAudioProcessor : public juce::AudioProcessor
{
public:
    CleanMixEQAudioProcessor();
    ~CleanMixEQAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "CleanMix EQ"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    void pushAnalyserSamples(const juce::AudioBuffer<float>& buffer);
    bool getSpectrumSnapshot(std::array<float, fftSize / 2>& destination);
    juce::String getMudReport() const;

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coefficients = juce::dsp::IIR::Coefficients<float>;
    std::array<juce::dsp::ProcessorDuplicator<Filter, Coefficients>, 8> eqFilters;
    juce::dsp::ProcessSpec spec {};

    juce::dsp::FFT forwardFFT { fftOrder };
    juce::dsp::WindowingFunction<float> window { fftSize, juce::dsp::WindowingFunction<float>::hann };
    std::array<float, fftSize> fftData {};
    std::array<float, fftSize / 2> spectrum {};
    std::array<float, fftSize / 2> smoothedSpectrum {};
    std::array<float, fftSize / 2> publishedSpectrum {};
    int fftWritePosition = 0;
    int samplesSinceFFT = 0;
    mutable juce::SpinLock spectrumLock;

    void updateEQ();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CleanMixEQAudioProcessor)
};
