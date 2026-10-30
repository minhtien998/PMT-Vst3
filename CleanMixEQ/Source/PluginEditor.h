#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class CleanMixEQAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    explicit CleanMixEQAudioProcessorEditor(CleanMixEQAudioProcessor&);
    ~CleanMixEQAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    CleanMixEQAudioProcessor& audioProcessor;
    juce::Label title, report;
    juce::ComboBox bandSelect;
    juce::Slider frequency, gain, q, output;
    juce::ToggleButton enabled, analyser;
    juce::TextButton scanButton;
    std::array<float, CleanMixEQAudioProcessor::fftSize / 2> spectrum {};
    int selectedBand = 0;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> freqAttachment, gainAttachment, qAttachment, outputAttachment;
    std::unique_ptr<ButtonAttachment> enabledAttachment, analyserAttachment;

    void attachBand(int band);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CleanMixEQAudioProcessorEditor)
};
