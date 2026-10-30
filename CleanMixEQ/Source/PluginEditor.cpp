#include "PluginEditor.h"

CleanMixEQAudioProcessorEditor::CleanMixEQAudioProcessorEditor(CleanMixEQAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(900, 570);
    title.setText("CLEANMIX EQ", juce::dontSendNotification);
    title.setFont(juce::Font(25.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(title);

    for (int i = 1; i <= 8; ++i) bandSelect.addItem("Band " + juce::String(i), i);
    bandSelect.setSelectedId(1);
    bandSelect.onChange = [this] { selectedBand = bandSelect.getSelectedId() - 1; attachBand(selectedBand); };
    addAndMakeVisible(bandSelect);

    auto setupSlider = [this](juce::Slider& s, const juce::String& name, const juce::String& suffix)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 22);
        s.setName(name); s.setTextValueSuffix(suffix); addAndMakeVisible(s);
    };
    setupSlider(frequency, "Frequency", " Hz");
    setupSlider(gain, "Gain", " dB");
    setupSlider(q, "Q", "");
    setupSlider(output, "Output", " dB");

    enabled.setButtonText("Band enabled");
    analyser.setButtonText("Spectrum analyser");
    addAndMakeVisible(enabled);
    addAndMakeVisible(analyser);
    addAndMakeVisible(report);

    scanButton.setButtonText("Analyse low-mid / mud");
    scanButton.onClick = [this] { report.setText(audioProcessor.getMudReport(), juce::dontSendNotification); };
    addAndMakeVisible(scanButton);

    attachBand(0);
    outputAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "output", output);
    analyserAttachment = std::make_unique<ButtonAttachment>(audioProcessor.parameters, "analyser", analyser);
    startTimerHz(20);
}

void CleanMixEQAudioProcessorEditor::attachBand(int band)
{
    const auto id = juce::String(band + 1);
    freqAttachment.reset(); gainAttachment.reset(); qAttachment.reset(); enabledAttachment.reset();
    freqAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "band" + id + "Freq", frequency);
    gainAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "band" + id + "Gain", gain);
    qAttachment = std::make_unique<SliderAttachment>(audioProcessor.parameters, "band" + id + "Q", q);
    enabledAttachment = std::make_unique<ButtonAttachment>(audioProcessor.parameters, "band" + id + "On", enabled);
}

void CleanMixEQAudioProcessorEditor::timerCallback()
{
    audioProcessor.getSpectrumSnapshot(spectrum);
    repaint();
}

void CleanMixEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff11151d));
    auto area = getLocalBounds().reduced(22);
    auto graph = area.removeFromTop(300).withTrimmedTop(42);
    g.setColour(juce::Colour(0xff202a38));
    g.fillRoundedRectangle(graph.toFloat(), 10.0f);

    g.setColour(juce::Colour(0xff3b4655));
    for (int i = 1; i < 5; ++i)
        g.drawHorizontalLine(graph.getY() + graph.getHeight() * i / 5, (float)graph.getX(), (float)graph.getRight());
    for (int i = 1; i < 8; ++i)
        g.drawVerticalLine(graph.getX() + graph.getWidth() * i / 8, (float)graph.getY(), (float)graph.getBottom());

    juce::Path curve;
    for (int x = 0; x < graph.getWidth(); ++x)
    {
        const int bin = juce::jlimit(1, (int)spectrum.size() - 1,
            (int)std::exp(std::log((double)spectrum.size()) * x / (double)graph.getWidth()));
        const float db = juce::jlimit(-90.0f, 0.0f, spectrum[(size_t)bin]);
        const float y = juce::jmap(db, -90.0f, 0.0f, (float)graph.getBottom(), (float)graph.getY());
        if (x == 0) curve.startNewSubPath((float)graph.getX(), y);
        else curve.lineTo((float)(graph.getX() + x), y);
    }
    g.setColour(juce::Colour(0xff57d6c4));
    g.strokePath(curve, juce::PathStrokeType(2.0f));

    g.setColour(juce::Colours::white);
    g.setFont(13.0f);
    g.drawText("20 Hz", graph.getX() + 5, graph.getBottom() + 5, 55, 18, juce::Justification::left);
    g.drawText("20 kHz", graph.getRight() - 60, graph.getBottom() + 5, 55, 18, juce::Justification::right);
    g.drawText("Live spectrum (post EQ)", graph.getX() + 10, graph.getY() + 8, 200, 20, juce::Justification::left);
}

void CleanMixEQAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced(22);
    title.setBounds(r.removeFromTop(38));
    r.removeFromTop(300);
    auto row = r.removeFromTop(175);
    bandSelect.setBounds(row.removeFromLeft(110).removeFromTop(30));
    frequency.setBounds(row.removeFromLeft(150));
    gain.setBounds(row.removeFromLeft(150));
    q.setBounds(row.removeFromLeft(130));
    output.setBounds(row.removeFromLeft(130));
    enabled.setBounds(row.removeFromLeft(130).removeFromTop(30));
    analyser.setBounds(row.removeFromLeft(150).removeFromTop(30));
    auto bottom = r.removeFromTop(42);
    scanButton.setBounds(bottom.removeFromLeft(210));
    report.setBounds(bottom);
}
