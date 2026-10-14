#include "PluginEditor.h"
#include "BuildInfo.h"

using namespace juce;

namespace
{
    const Colour kPanel    (0xff1c1d20);
    const Colour kPanelHi  (0xff26282c);
    const Colour kLegend   (0xffe8e2d0);
    const Colour kLegendDim(0xff8e8a7e);
    const Colour kKnobCap  (0xff3a4250);
    const Colour kKnobRing (0xff10a7d6);
    const Colour kPointer  (0xfff3f0e6);
    const Colour kLedRed   (0xffff3b2f);
    const Colour kLedAmber (0xffffb02e);
    const Colour kLedOff   (0xff3a2a28);
    const Colour kButtonOn (0xffd9c78b);
}

//==============================================================================
Dirty630LookAndFeel::Dirty630LookAndFeel()
{
    setColour (Slider::textBoxTextColourId, kLegend);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour (Label::textColourId, kLegend);
    setColour (ComboBox::backgroundColourId, kPanelHi);
    setColour (ComboBox::textColourId, kLegend);
    setColour (ComboBox::outlineColourId, kLegendDim.withAlpha (0.4f));
    setColour (ComboBox::arrowColourId, kLegend);
    setColour (PopupMenu::backgroundColourId, kPanelHi);
    setColour (PopupMenu::textColourId, kLegend);
    setColour (PopupMenu::highlightedBackgroundColourId, kKnobRing.withAlpha (0.4f));
}

Font Dirty630LookAndFeel::getLabelFont (Label&)
{
    return Font (FontOptions (11.5f, Font::bold));
}

void Dirty630LookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                            float startAngle, float endAngle, Slider&)
{
    const auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float radius = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre  = bounds.getCentre();
    const float angle  = startAngle + pos * (endAngle - startAngle);

    // Scale ring with tick marks, like a silk-screened legend.
    Path track;
    track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour (kLegendDim.withAlpha (0.35f));
    g.strokePath (track, PathStrokeType (1.5f));

    Path value;
    value.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, angle, true);
    g.setColour (kKnobRing);
    g.strokePath (value, PathStrokeType (2.0f));

    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (endAngle - startAngle) * (float) i / 10.0f;
        const auto p1 = centre.getPointOnCircumference (radius + 3.0f, a);
        const auto p2 = centre.getPointOnCircumference (radius + 6.0f, a);
        g.setColour (kLegendDim);
        g.drawLine (Line<float> (p1, p2), 1.0f);
    }

    // Cap
    const float capR = radius * 0.72f;
    g.setColour (Colours::black.withAlpha (0.5f));
    g.fillEllipse (Rectangle<float> (capR * 2.0f, capR * 2.0f).withCentre (centre.translated (0.0f, 1.5f)));
    g.setGradientFill (ColourGradient (kKnobCap.brighter (0.25f), centre.x, centre.y - capR,
                                       kKnobCap.darker (0.4f),   centre.x, centre.y + capR, false));
    g.fillEllipse (Rectangle<float> (capR * 2.0f, capR * 2.0f).withCentre (centre));

    // Pointer
    const auto p1 = centre.getPointOnCircumference (capR * 0.25f, angle);
    const auto p2 = centre.getPointOnCircumference (capR * 0.92f, angle);
    g.setColour (kPointer);
    g.drawLine (Line<float> (p1, p2), 2.5f);
}

void Dirty630LookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();

    g.setColour (Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (r.translated (0.0f, 1.0f), 3.0f);
    g.setColour (on ? kButtonOn : (highlighted ? kPanelHi.brighter (0.15f) : kPanelHi));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (on ? kButtonOn.darker (0.6f) : kLegendDim.withAlpha (0.5f));
    g.drawRoundedRectangle (r, 3.0f, 1.0f);

    g.setColour (on ? kPanel : kLegend);
    g.setFont (Font (FontOptions (10.5f, Font::bold)));
    g.drawText (b.getButtonText(), r, Justification::centred);
}

//==============================================================================
void GrMeter::paint (Graphics& g)
{
    // 12-segment ladder: 1 2 3 4 5 6 8 10 12 15 20 30 dB, as on the front panel.
    static const float steps[] = { 1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 20, 30 };
    auto r = getLocalBounds().toFloat();
    const int n = (int) std::size (steps);
    const float segW = r.getWidth() / (float) n;

    for (int i = 0; i < n; ++i)
    {
        auto seg = Rectangle<float> (r.getX() + segW * (float) i, r.getY(), segW, r.getHeight() - 12.0f).reduced (1.5f, 0.0f);
        const bool lit = value >= steps[i];
        Colour c = i >= 9 ? kLedRed : kLedAmber;
        g.setColour (lit ? c : kLedOff);
        g.fillRoundedRectangle (seg, 2.0f);
        if (lit)
        {
            g.setColour (c.withAlpha (0.35f));
            g.fillRoundedRectangle (seg.expanded (1.5f), 3.0f);
        }
        g.setColour (kLegendDim);
        g.setFont (Font (FontOptions (8.5f)));
        g.drawText (String ((int) steps[i]), seg.withY (r.getBottom() - 11.0f).withHeight (11.0f), Justification::centred);
    }
}

//==============================================================================
Dirty630Editor::Dirty630Editor (Dirty630Processor& p)
    : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);

    auto& s = strip;
    makeKnob (s.threshold, ids::ch (ids::threshold, 0), "THRESHOLD");
    makeKnob (s.ratio,     ids::ch (ids::ratio, 0),     "RATIO");
    makeKnob (s.attack,    ids::ch (ids::attack, 0),    "ATTACK");
    makeKnob (s.release,   ids::ch (ids::release, 0),   "RELEASE");
    makeKnob (s.output,    ids::ch (ids::output, 0),    "OUTPUT");
    makeKnob (s.gateThr,   ids::ch (ids::gateThr, 0),   "GATE THRESH");
    makeKnob (s.gateRate,  ids::ch (ids::gateRate, 0),  "GATE RATE");
    makeToggle (s.peak,   ids::ch (ids::peak, 0),   "PEAK");
    makeToggle (s.soft,   ids::ch (ids::soft, 0),   "SOFT");
    makeToggle (s.bypass, ids::ch (ids::bypass, 0), "BYPASS");
    addAndMakeVisible (s.meter);

    makeKnob (inputTrim, ids::inputTrim, "INPUT");
    makeKnob (calib,     ids::calib,     "0 dBFS =");
    makeKnob (thump,     ids::thump,     "THUMP");
    makeKnob (noise,     ids::noise,     "NOISE");
    makeKnob (mismatch,  ids::mismatch,  "MISMATCH");
    makeKnob (mix,       ids::mix,       "MIX");
    makeToggle (extSc,  ids::extSc,  "EXT SC");
    makeToggle (modded, ids::modded, "MODDED");

    oversamp.addItemList ({ "1x", "2x", "4x" }, 1);
    addAndMakeVisible (oversamp);
    oversampAttachment = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, ids::oversamp, oversamp);
    oversampLabel.setText ("OVERSAMPLE", dontSendNotification);
    oversampLabel.setJustificationType (Justification::centred);
    addAndMakeVisible (oversampLabel);

    dirtLabel.setText ("DIRT", dontSendNotification);
    dirtLabel.setFont (Font (FontOptions (13.0f, Font::bold)));
    dirtLabel.setColour (Label::textColourId, kKnobRing);
    addAndMakeVisible (dirtLabel);

    setSize (980, 300);
    startTimerHz (30);
}

Dirty630Editor::~Dirty630Editor()
{
    setLookAndFeel (nullptr);
}

void Dirty630Editor::makeKnob (Knob& k, const String& paramId, const String& text)
{
    k.slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (Slider::TextBoxBelow, false, 64, 16);
    k.slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    addAndMakeVisible (k.slider);
    k.label.setText (text, dontSendNotification);
    k.label.setJustificationType (Justification::centred);
    k.label.setColour (Label::textColourId, kLegendDim);
    addAndMakeVisible (k.label);
    k.attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramId, k.slider);
}

void Dirty630Editor::makeToggle (Toggle& t, const String& paramId, const String& text)
{
    t.button.setButtonText (text);
    addAndMakeVisible (t.button);
    t.attachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, paramId, t.button);
}

void Dirty630Editor::paint (Graphics& g)
{
    g.fillAll (kPanel);

    // Brushed header stripe and a rack-rail edge above the global row.  The
    // rail stops short of the right edge, where the big MIX knob spans both
    // rows.
    g.setColour (kPanelHi);
    g.fillRect (0, 0, getWidth(), 34);
    g.fillRect (0, 196, getWidth() - 160, 2);

    g.setColour (kLegend);
    g.setFont (Font (FontOptions (20.0f, Font::bold)));
    g.drawText ("dirty630", 16, 4, 200, 26, Justification::centredLeft);
    g.setColour (kLegendDim);
    g.setFont (Font (FontOptions (11.0f)));
    g.drawText ("version " DIRTY630_VERSION "  -  " DIRTY630_BUILD_TIME "  -  minusro", 130, 4, 500, 26, Justification::centredLeft);
}

void Dirty630Editor::resized()
{
    auto layoutStrip = [this] (ChannelStrip& s, int top)
    {
        const int knobW = 86, knobH = 96, labelH = 14;
        int x = 24;
        Knob* knobs[] = { &s.threshold, &s.ratio, &s.attack, &s.release, &s.output };
        for (auto* k : knobs)
        {
            k->label.setBounds (x, top + 4, knobW, labelH);
            k->slider.setBounds (x, top + 18, knobW, knobH);
            x += knobW;
        }
        x += 10;
        s.peak.button.setBounds (x, top + 30, 64, 24);
        s.soft.button.setBounds (x, top + 62, 64, 24);
        s.bypass.button.setBounds (x, top + 94, 64, 24);
        x += 84;
        Knob* gate[] = { &s.gateThr, &s.gateRate };
        for (auto* k : gate)
        {
            k->label.setBounds (x, top + 4, knobW, labelH);
            k->slider.setBounds (x, top + 18, knobW, knobH);
            x += knobW;
        }
        x += 10;
        s.meter.setBounds (x, top + 50, getWidth() - x - 16, 40);
    };

    layoutStrip (strip, 40);

    // Bottom row: global and "dirt" controls.
    const int top = 206, knobW = 80, knobH = 68;
    int x = 16;
    extSc.button.setBounds  (x, top + 6,  70, 24);
    modded.button.setBounds (x, top + 38, 70, 24); x += 90;
    for (auto* k : { &inputTrim, &calib })
    {
        // 4 px higher than the knob row suggests: puts the label base at the
        // same distance from the 12-o'clock tick as on the big knobs.
        k->label.setBounds (x, top - 4, knobW, 14);
        k->slider.setBounds (x, top + 10, knobW, knobH);
        x += knobW;
    }
    x += 20;
    dirtLabel.setBounds (x, top + 24, 44, 18); x += 48;
    for (auto* k : { &thump, &noise, &mismatch })
    {
        k->label.setBounds (x, top - 4, knobW, 14);
        k->slider.setBounds (x, top + 10, knobW, knobH);
        x += knobW;
    }
    x += 20;
    oversampLabel.setBounds (x, top, 90, 14);
    oversamp.setBounds (x + 10, top + 22, 70, 24);

    // Big dry/wet knob in the bottom-right corner, spanning both rows.
    mix.label.setBounds (getWidth() - 140, 162, 120, 14);
    mix.slider.setBounds (getWidth() - 140, 176, 120, 112);
}

void Dirty630Editor::timerCallback()
{
    // One ladder for both VCAs; they only differ by the Mismatch spread.
    strip.meter.setValue (jmax (proc.grMeter[0].load(), proc.grMeter[1].load()));
}
