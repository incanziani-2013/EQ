#pragma once
#include <JuceHeader.h>
#include <array>
#include "PluginProcessor.h"

class AQEQAudioProcessorEditor : public juce::AudioProcessorEditor,
                                 private juce::Timer
{
public:
    explicit AQEQAudioProcessorEditor (AQEQAudioProcessor&);
    ~AQEQAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override { repaint (curveArea); }

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        int column = 0;
        int row = 0;
    };

    void setupKnob (Knob&, const juce::String& paramId, const juce::String& caption, int column, int row);
    void drawCurve (juce::Graphics&) const;

    AQEQAudioProcessor& audioProcessor;
    std::array<Knob, 13> knobs;
    juce::Rectangle<int> curveArea;

    static constexpr int numColumns = 7;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AQEQAudioProcessorEditor)
};
