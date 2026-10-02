#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    juce::NormalisableRange<float> freqRange (float lo, float hi, float centre)
    {
        juce::NormalisableRange<float> r (lo, hi, 0.01f);
        r.setSkewForCentre (centre);
        return r;
    }

    // Frecuencia normal (bandas shelf y campana).
    std::unique_ptr<juce::RangedAudioParameter> makeFreq (const juce::String& id, const juce::String& name,
                                                          float lo, float hi, float centre, float def)
    {
        return std::make_unique<juce::AudioParameterFloat> (
            id, name, freqRange (lo, hi, centre), def,
            juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction ([] (float v, int)
                {
                    return juce::String (juce::roundToInt (v)) + " Hz";
                }));
    }

    // Filtros de corte: en el extremo del recorrido quedan apagados (OFF).
    // offAtMin = true  -> Low Cut apagado en 20 Hz
    // offAtMin = false -> High Cut apagado en 20 kHz
    std::unique_ptr<juce::RangedAudioParameter> makeCut (const juce::String& id, const juce::String& name,
                                                         float lo, float hi, float centre, float def,
                                                         bool offAtMin)
    {
        return std::make_unique<juce::AudioParameterFloat> (
            id, name, freqRange (lo, hi, centre), def,
            juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction ([offAtMin] (float v, int)
                {
                    const bool off = offAtMin ? (v <= 20.5f) : (v >= 19990.0f);
                    return off ? juce::String ("OFF")
                               : juce::String (juce::roundToInt (v)) + " Hz";
                })
                .withValueFromStringFunction ([offAtMin, lo, hi] (const juce::String& t)
                {
                    if (t.containsIgnoreCase ("off"))
                        return offAtMin ? lo : hi;
                    return t.getFloatValue();
                }));
    }

    std::unique_ptr<juce::RangedAudioParameter> makeGain (const juce::String& id, const juce::String& name,
                                                          float range)
    {
        return std::make_unique<juce::AudioParameterFloat> (
            id, name, juce::NormalisableRange<float> (-range, range, 0.01f), 0.0f,
            juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction ([] (float v, int)
                {
                    return juce::String (v > 0.05f ? "+" : "") + juce::String (v, 1) + " dB";
                }));
    }

    std::unique_ptr<juce::RangedAudioParameter> makeQ (const juce::String& id, const juce::String& name)
    {
        return std::make_unique<juce::AudioParameterFloat> (
            id, name, freqRange (0.1f, 10.0f, 1.0f), 1.0f,
            juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction ([] (float v, int)
                {
                    return juce::String (v, 2);
                }));
    }
}

AQEQAudioProcessor::AQEQAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "AQEQ_STATE", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout AQEQAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back (makeCut  ("LC_FREQ",  "Low Cut",         20.0f,  1000.0f,  120.0f,    20.0f, true));

    p.push_back (makeFreq ("LS_FREQ",  "Low Shelf Freq",  20.0f,  1000.0f,  150.0f,   100.0f));
    p.push_back (makeGain ("LS_GAIN",  "Low Shelf Gain",  15.0f));

    p.push_back (makeFreq ("LM_FREQ",  "Low Mid Freq",    50.0f,  5000.0f,  500.0f,   400.0f));
    p.push_back (makeGain ("LM_GAIN",  "Low Mid Gain",    15.0f));
    p.push_back (makeQ    ("LM_Q",     "Low Mid Q"));

    p.push_back (makeFreq ("HM_FREQ",  "High Mid Freq",  200.0f, 12000.0f, 2000.0f,  2500.0f));
    p.push_back (makeGain ("HM_GAIN",  "High Mid Gain",   15.0f));
    p.push_back (makeQ    ("HM_Q",     "High Mid Q"));

    p.push_back (makeFreq ("HS_FREQ",  "High Shelf Freq", 1000.0f, 20000.0f, 6000.0f, 8000.0f));
    p.push_back (makeGain ("HS_GAIN",  "High Shelf Gain", 15.0f));

    p.push_back (makeCut  ("HC_FREQ",  "High Cut",       2000.0f, 20000.0f, 8000.0f, 20000.0f, false));

    p.push_back (makeGain ("OUT_GAIN", "Output",          12.0f));

    return { p.begin(), p.end() };
}

aqeq::Settings AQEQAudioProcessor::getSettings() const
{
    auto get = [this] (const char* id)
    {
        return apvts.getRawParameterValue (id)->load();
    };

    aqeq::Settings s;
    s.lcFreq  = get ("LC_FREQ");
    s.lsFreq  = get ("LS_FREQ");
    s.lsGain  = get ("LS_GAIN");
    s.lmFreq  = get ("LM_FREQ");
    s.lmGain  = get ("LM_GAIN");
    s.lmQ     = get ("LM_Q");
    s.hmFreq  = get ("HM_FREQ");
    s.hmGain  = get ("HM_GAIN");
    s.hmQ     = get ("HM_Q");
    s.hsFreq  = get ("HS_FREQ");
    s.hsGain  = get ("HS_GAIN");
    s.hcFreq  = get ("HC_FREQ");
    s.outGain = get ("OUT_GAIN");
    return s;
}

void AQEQAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;

    for (auto& f : filters)
        f.reset();

    lastGain = juce::Decibels::decibelsToGain (getSettings().outGain);
}

bool AQEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono()
        && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return mainOut == layouts.getMainInputChannelSet();
}

void AQEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalIn  = getTotalNumInputChannels();
    const int totalOut = getTotalNumOutputChannels();
    for (int ch = totalIn; ch < totalOut; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    const auto settings = getSettings();
    const auto chain = aqeq::buildChain (settings, currentSampleRate);

    for (int b = 0; b < aqeq::kNumBands; ++b)
    {
        if (! chain.on[b] && filters[(size_t) b].isActive())
            filters[(size_t) b].reset();

        filters[(size_t) b].setCoeffs (chain.c[b]);
        filters[(size_t) b].setActive (chain.on[b]);
    }

    const int numChannels = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples  = buffer.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);

        for (int i = 0; i < numSamples; ++i)
        {
            double x = static_cast<double> (data[i]);

            for (auto& f : filters)
                if (f.isActive())
                    x = f.process (ch, x);

            data[i] = static_cast<float> (x);
        }
    }

    const float newGain = juce::Decibels::decibelsToGain (settings.outGain);
    buffer.applyGainRamp (0, numSamples, lastGain, newGain);
    lastGain = newGain;
}

juce::AudioProcessorEditor* AQEQAudioProcessor::createEditor()
{
    return new AQEQAudioProcessorEditor (*this);
}

void AQEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        std::unique_ptr<juce::XmlElement> xml (state.createXml());
        copyXmlToBinary (*xml, destData);
    }
}

void AQEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// Punto de entrada que JUCE usa para crear el plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AQEQAudioProcessor();
}
