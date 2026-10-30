#include "PluginProcessor.h"
#include "PluginEditor.h"

CleanMixEQAudioProcessor::CleanMixEQAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout CleanMixEQAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (int i = 0; i < 8; ++i)
    {
        const auto id = juce::String(i + 1);
        layout.add(std::make_unique<juce::AudioParameterBool>("band" + id + "On", "Band " + id + " On", i < 4));
        layout.add(std::make_unique<juce::AudioParameterFloat>("band" + id + "Freq", "Band " + id + " Frequency",
            juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.25f), 100.0f * std::pow(2.0f, (float)i)));
        layout.add(std::make_unique<juce::AudioParameterFloat>("band" + id + "Gain", "Band " + id + " Gain",
            juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f), 0.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>("band" + id + "Q", "Band " + id + " Q",
            juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f), 1.0f));
    }
    layout.add(std::make_unique<juce::AudioParameterFloat>("output", "Output",
        juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>("analyser", "Analyzer", true));
    return layout;
}

bool CleanMixEQAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo())
        && in == out;
}

void CleanMixEQAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
    spec.numChannels = (juce::uint32)getTotalNumOutputChannels();

    for (auto& f : eqFilters)
        f.prepare(spec);

    updateEQ();
    fftData.fill(0.0f);
    spectrum.fill(0.0f);
    smoothedSpectrum.fill(0.0f);
}

void CleanMixEQAudioProcessor::updateEQ()
{
    for (int i = 0; i < 8; ++i)
    {
        const auto id = juce::String(i + 1);
        const bool on = parameters.getRawParameterValue("band" + id + "On")->load() > 0.5f;
        const float freq = parameters.getRawParameterValue("band" + id + "Freq")->load();
        const float gain = parameters.getRawParameterValue("band" + id + "Gain")->load();
        const float q = parameters.getRawParameterValue("band" + id + "Q")->load();

        const float linearGain = (!on || std::abs(gain) < 0.001f)
            ? 1.0f : juce::Decibels::decibelsToGain(gain);
        auto coeff = Coefficients::makePeakFilter(spec.sampleRate,
            juce::jlimit(20.0f, (float)(spec.sampleRate * 0.45), freq),
            q, linearGain);
        *eqFilters[(size_t)i].state = *coeff;
    }
}

void CleanMixEQAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    updateEQ();

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    for (auto& f : eqFilters)
        f.process(context);

    const float outDb = parameters.getRawParameterValue("output")->load();
    buffer.applyGain(juce::Decibels::decibelsToGain(outDb));

    if (parameters.getRawParameterValue("analyser")->load() > 0.5f)
        pushAnalyserSamples(buffer);
}

void CleanMixEQAudioProcessor::pushAnalyserSamples(const juce::AudioBuffer<float>& buffer)
{
    const int channels = buffer.getNumChannels();
    for (int n = 0; n < buffer.getNumSamples(); ++n)
    {
        float mono = 0.0f;
        for (int c = 0; c < channels; ++c)
            mono += buffer.getReadPointer(c)[n];
        mono /= (float)juce::jmax(1, channels);

        fftData[(size_t)fftWritePosition++] = mono;
        if (fftWritePosition >= fftSize) fftWritePosition = 0;

        if (++samplesSinceFFT >= fftSize / 4)
        {
            samplesSinceFFT = 0;
            // JUCE's real-only FFT routine expects a 2 * fftSize work buffer.
            std::array<float, fftSize * 2> frame {};
            for (int i = 0; i < fftSize; ++i)
                frame[(size_t)i] = fftData[(size_t)((fftWritePosition + i) % fftSize)];
            window.multiplyWithWindowingTable(frame.data(), fftSize);
            forwardFFT.performFrequencyOnlyForwardTransform(frame.data());

            for (int i = 0; i < fftSize / 2; ++i)
            {
                const float db = juce::Decibels::gainToDecibels(frame[(size_t)i] / (float)fftSize, -100.0f);
                smoothedSpectrum[(size_t)i] = 0.78f * smoothedSpectrum[(size_t)i] + 0.22f * db;
            }
            const juce::SpinLock::ScopedLockType lock(spectrumLock);
            publishedSpectrum = smoothedSpectrum;
        }
    }
}

bool CleanMixEQAudioProcessor::getSpectrumSnapshot(std::array<float, fftSize / 2>& destination)
{
    const juce::SpinLock::ScopedTryLockType lock(spectrumLock);
    if (!lock.isLocked()) return false;
    destination = publishedSpectrum;
    return true;
}

juce::String CleanMixEQAudioProcessor::getMudReport() const
{
    std::array<float, fftSize / 2> local {};
    {
        const juce::SpinLock::ScopedLockType lock(spectrumLock);
        local = publishedSpectrum;
    }

    if (spec.sampleRate <= 0.0) return "Play audio to analyse the mix.";
    const int lo = juce::jmax(1, (int)(180.0 * fftSize / spec.sampleRate));
    const int hi = juce::jmin(fftSize / 2 - 1, (int)(500.0 * fftSize / spec.sampleRate));
    float sum = 0.0f, peak = -100.0f;
    int peakBin = lo;
    for (int i = lo; i <= hi; ++i)
    {
        sum += local[(size_t)i];
        if (local[(size_t)i] > peak) { peak = local[(size_t)i]; peakBin = i; }
    }
    const float average = sum / (float)juce::jmax(1, hi - lo + 1);
    const float hz = (float)peakBin * (float)spec.sampleRate / (float)fftSize;
    if (peak < -75.0f) return "Low signal: play a louder section to analyse.";
    if (peak - average > 7.0f)
        return "Resonance candidate near " + juce::String((int)hz) + " Hz. Sweep and listen; try a narrow cut.";
    return "No strong narrow peak in 180-500 Hz. Check overall low-mid balance by ear.";
}

void CleanMixEQAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}

void CleanMixEQAudioProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* CleanMixEQAudioProcessor::createEditor()
{
    return new CleanMixEQAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CleanMixEQAudioProcessor();
}
