#include "PluginEditor.h"

AQEQAudioProcessorEditor::AQEQAudioProcessorEditor (AQEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setupKnob (knobs[0],  "LC_FREQ",  "FREQ", 0, 0);

    setupKnob (knobs[1],  "LS_FREQ",  "FREQ", 1, 0);
    setupKnob (knobs[2],  "LS_GAIN",  "GAIN", 1, 1);

    setupKnob (knobs[3],  "LM_FREQ",  "FREQ", 2, 0);
    setupKnob (knobs[4],  "LM_GAIN",  "GAIN", 2, 1);
    setupKnob (knobs[5],  "LM_Q",     "Q",    2, 2);

    setupKnob (knobs[6],  "HM_FREQ",  "FREQ", 3, 0);
    setupKnob (knobs[7],  "HM_GAIN",  "GAIN", 3, 1);
    setupKnob (knobs[8],  "HM_Q",     "Q",    3, 2);

    setupKnob (knobs[9],  "HS_FREQ",  "FREQ", 4, 0);
    setupKnob (knobs[10], "HS_GAIN",  "GAIN", 4, 1);

    setupKnob (knobs[11], "HC_FREQ",  "FREQ", 5, 0);

    setupKnob (knobs[12], "OUT_GAIN", "GAIN", 6, 1);

    setSize (940, 640);
    startTimerHz (24);
}

void AQEQAudioProcessorEditor::setupKnob (Knob& k, const juce::String& paramId,
                                          const juce::String& caption, int column, int row)
{
    k.column = column;
    k.row = row;

    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
    k.slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff0e6f94));
    k.slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::white.withAlpha (0.45f));
    k.slider.setColour (juce::Slider::thumbColourId, juce::Colours::white);
    k.slider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    k.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (k.slider);

    k.label.setText (caption, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (juce::Font (11.0f, juce::Font::bold));
    k.label.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (k.label);

    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), paramId, k.slider);
}

void AQEQAudioProcessorEditor::drawCurve (juce::Graphics& g) const
{
    const auto r = curveArea.toFloat();

    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.fillRoundedRectangle (r, 14.0f);
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.drawRoundedRectangle (r, 14.0f, 1.2f);

    const float minDb = -18.0f;
    const float maxDb = 18.0f;

    auto dbToY = [&] (float db)
    {
        return r.getY() + r.getHeight() * (maxDb - db) / (maxDb - minDb);
    };
    auto freqToX = [&] (float f)
    {
        return r.getX() + r.getWidth() * std::log (f / 20.0f) / std::log (1000.0f);
    };

    // Rejilla horizontal (dB)
    for (float db : { -12.0f, -6.0f, 0.0f, 6.0f, 12.0f })
    {
        g.setColour (juce::Colours::white.withAlpha (db == 0.0f ? 0.55f : 0.20f));
        g.drawHorizontalLine (juce::roundToInt (dbToY (db)), r.getX() + 8.0f, r.getRight() - 8.0f);
    }

    // Rejilla vertical (Hz)
    const float gridFreqs[] = { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f };
    for (float f : gridFreqs)
    {
        g.setColour (juce::Colours::white.withAlpha (0.20f));
        g.drawVerticalLine (juce::roundToInt (freqToX (f)), r.getY() + 8.0f, r.getBottom() - 8.0f);
    }

    // Etiquetas de frecuencia
    g.setFont (juce::Font (10.0f));
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.drawText ("100", juce::roundToInt (freqToX (100.0f)) - 20, curveArea.getBottom() - 18, 40, 12,
                juce::Justification::centred);
    g.drawText ("1k", juce::roundToInt (freqToX (1000.0f)) - 20, curveArea.getBottom() - 18, 40, 12,
                juce::Justification::centred);
    g.drawText ("10k", juce::roundToInt (freqToX (10000.0f)) - 20, curveArea.getBottom() - 18, 40, 12,
                juce::Justification::centred);

    // Curva de respuesta
    const double fs = audioProcessor.getCurrentSampleRate() > 0.0 ? audioProcessor.getCurrentSampleRate() : 44100.0;
    const auto chain = aqeq::buildChain (audioProcessor.getSettings(), fs);

    juce::Path path;
    const int w = juce::jmax (1, juce::roundToInt (r.getWidth()));

    for (int x = 0; x <= w; x += 2)
    {
        const double freq = 20.0 * std::pow (1000.0, static_cast<double> (x) / static_cast<double> (w));
        double mag = 1.0;

        for (int b = 0; b < aqeq::kNumBands; ++b)
            if (chain.on[b])
                mag *= aqeq::magnitudeAt (chain.c[b], fs, freq);

        const float db = juce::jlimit (minDb, maxDb,
                                       static_cast<float> (20.0 * std::log10 (juce::jmax (mag, 1.0e-6))));
        const float px = r.getX() + static_cast<float> (x);
        const float py = dbToY (db);

        if (x == 0)
            path.startNewSubPath (px, py);
        else
            path.lineTo (px, py);
    }

    juce::Graphics::ScopedSaveState saved (g);
    g.reduceClipRegion (curveArea);
    g.setColour (juce::Colours::white);
    g.strokePath (path, juce::PathStrokeType (2.4f));
}

void AQEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8fe9f8), 0.0f, 0.0f,
                                             juce::Colour (0xff1f9bc0), 0.0f, static_cast<float> (getHeight()),
                                             false));
    g.fillAll();

    // Panel de vidrio
    const auto panel = b.reduced (18.0f);
    g.setColour (juce::Colours::white.withAlpha (0.20f));
    g.fillRoundedRectangle (panel, 24.0f);
    g.setColour (juce::Colours::white.withAlpha (0.65f));
    g.drawRoundedRectangle (panel, 24.0f, 1.5f);

    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (30.0f, juce::Font::bold));
    g.drawFittedText ("AQEQ", 25, 18, getWidth() - 50, 42, juce::Justification::centred, 1);

    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.setFont (juce::Font (11.0f));
    g.drawFittedText ("LIQUID EQUALIZER  -  AERO FILTERS", 25, 56, getWidth() - 50, 20,
                      juce::Justification::centred, 1);

    drawCurve (g);

    // Columnas de bandas
    const char* titles[numColumns] = { "LOW CUT", "LOW SHELF", "LOW MID", "HIGH MID", "HIGH SHELF", "HIGH CUT", "OUTPUT" };
    const int colW = (getWidth() - 60) / numColumns;
    const int top = curveArea.getBottom() + 10;
    const int bottom = getHeight() - 30;

    for (int i = 0; i < numColumns; ++i)
    {
        const int x = 30 + i * colW;

        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.fillRoundedRectangle (static_cast<float> (x + 2), static_cast<float> (top),
                                static_cast<float> (colW - 4), static_cast<float> (bottom - top), 10.0f);

        g.setColour (juce::Colours::white);
        g.setFont (juce::Font (12.0f, juce::Font::bold));
        g.drawFittedText (titles[i], x, top + 2, colW, 18, juce::Justification::centred, 1);
    }
}

void AQEQAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (30);
    area.removeFromTop (70);
    curveArea = area.removeFromTop (190);
    area.removeFromTop (10);
    area.removeFromTop (20); // titulos de banda

    const int colW = area.getWidth() / numColumns;
    const int rowH = area.getHeight() / 3;

    for (auto& k : knobs)
    {
        juce::Rectangle<int> cell (area.getX() + k.column * colW,
                                   area.getY() + k.row * rowH,
                                   colW, rowH);

        k.label.setBounds (cell.removeFromTop (14));
        k.slider.setBounds (cell.reduced (4, 0));
    }
}
