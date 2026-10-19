#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

//==============================================================================
// Rack-unit look: charcoal panel, cream legends, blue-grey knob caps, red
// LED gain-reduction ladder like the original.
class Dirty630LookAndFeel : public juce::LookAndFeel_V4
{
public:
    Dirty630LookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    juce::Font getLabelFont (juce::Label&) override;
};

//==============================================================================
class GrMeter : public juce::Component
{
public:
    // Instant attack, ~40 dB/s fall at the 30 Hz editor timer, so short
    // transient bites stay visible instead of flickering.
    void setValue (float grDb)
    {
        value = juce::jmax (grDb, value - 1.4f);
        repaint();
    }
    void paint (juce::Graphics&) override;
private:
    float value = 0.0f;
};

//==============================================================================
class Dirty630Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Dirty630Editor (Dirty630Processor&);
    ~Dirty630Editor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    struct Toggle
    {
        juce::ToggleButton button;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
    };

    void makeKnob (Knob&, const juce::String& paramId, const juce::String& text);
    void makeToggle (Toggle&, const juce::String& paramId, const juce::String& text);

    Dirty630Processor& proc;
    Dirty630LookAndFeel lnf;

    struct ChannelStrip
    {
        Knob threshold, ratio, attack, release, output, gateThr, gateRate;
        Toggle peak, soft, clean;      // the three switches beside the knobs
        GrMeter meter;
    } strip;

    Knob inputTrim, calib, thump, noise, mismatch, mix;
    Toggle extSc, bypass;              // the global row's pair
    juce::ComboBox oversamp;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> oversampAttachment;
    juce::Label oversampLabel, dirtLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Dirty630Editor)
};
