#include "PluginEditor.h"
#include "BuildInfo.h"

using namespace juce;

// Colours taken from the 3630's own front panel: a near-black brushed
// fascia, white silkscreen, black knurled caps with a white pointer, and
// the green/amber/red LED ladders.
namespace
{
    const Colour kPanel    (0xff121214);
    const Colour kPanelHi  (0xff1b1c1f);
    const Colour kLegend   (0xfff2f2f0);
    const Colour kLegendDim(0xff9a9a97);
    const Colour kKnobCap  (0xff1a1a1c);
    const Colour kKnobEdge (0xff3c3c40);
    const Colour kKnobRing (0xff6e6e73);
    const Colour kPointer  (0xfff6f6f4);
    const Colour kLedRed   (0xffe8261c);
    const Colour kLedAmber (0xffe8a81c);
    const Colour kLedGreen (0xff35c43a);
    const Colour kLedOff   (0xff241c1a);
    const Colour kButtonOn (0xffe8261c);
    const Colour kBrand    (0xff3fb6e0);   // the panel logo's cyan

    constexpr int kHeaderH = 40;   // branding band at the top of the fascia
}

//==============================================================================
// The MINUS wordmark, after the logo the hardware wears: a geometric face
// leaning forward, its initial at full cap height and the rest of the word
// about three quarters of it.  The letters are built as centre lines and then
// stroked with round caps and joins, which is what gives the logo its soft
// terminals - drawing them as filled outlines would mean describing every
// one of those arcs by hand.
static Path minusWordmark()
{
    constexpr float H     = 14.0f;   // cap height of the initial
    // The stroke has to clear the counters of the small letters: the S bowls
    // are only a quarter of the word's height each, so a weight that looks
    // right on the M closes them up into a figure eight.
    constexpr float sw    = 2.5f;    // stroke weight
    constexpr float small = 0.80f;   // height of the letters after the first
    constexpr float gap   = 1.7f;    // space between adjacent outer edges
    constexpr float slant = 0.19f;   // tangent of the forward lean, about 11 degrees

    // Centre lines sit half a stroke inside the letter's outer bounds.
    constexpr float top  = sw * 0.5f;
    constexpr float base = H - sw * 0.5f;
    constexpr float sTop = H * (1.0f - small) + sw * 0.5f;   // top of the small letters

    // Widths measured centre to centre; the stroke adds sw to each.
    constexpr float wM = 11.0f, wN = 7.0f, wU = 7.0f, wS = 7.0f;

    const float smallH = base - sTop;
    Path k;
    float x = 0.0f;

    k.startNewSubPath (x, base);                                    // M, full height
    k.lineTo (x, top);
    k.lineTo (x + wM * 0.5f, top + (base - top) * 0.62f);
    k.lineTo (x + wM, top);
    k.lineTo (x + wM, base);
    x += wM + sw + gap;

    k.startNewSubPath (x, sTop);                                    // I
    k.lineTo (x, base);
    x += sw + gap;

    k.startNewSubPath (x, base);                                    // N
    k.lineTo (x, sTop);
    k.lineTo (x + wN, base);
    k.lineTo (x + wN, sTop);
    x += wN + sw + gap;

    k.startNewSubPath (x, sTop);                                    // U
    k.lineTo (x, sTop + smallH * 0.55f);
    k.quadraticTo (x, base, x + wU * 0.5f, base);
    k.quadraticTo (x + wU, base, x + wU, sTop + smallH * 0.55f);
    k.lineTo (x + wU, sTop);
    x += wU + sw + gap;

    k.startNewSubPath (x + wS, sTop + smallH * 0.20f);              // S
    k.quadraticTo (x + wS, sTop, x + wS * 0.5f, sTop);
    k.quadraticTo (x, sTop, x, sTop + smallH * 0.26f);
    k.quadraticTo (x, sTop + smallH * 0.46f, x + wS * 0.52f, sTop + smallH * 0.5f);
    k.quadraticTo (x + wS, sTop + smallH * 0.54f, x + wS, sTop + smallH * 0.76f);
    k.quadraticTo (x + wS, base, x + wS * 0.5f, base);
    k.quadraticTo (x, base, x, sTop + smallH * 0.82f);

    Path p;
    PathStrokeType (sw, PathStrokeType::curved, PathStrokeType::rounded).createStrokedPath (p, k);
    // Lean the finished outline; shearing against y puts the top edge ahead.
    p.applyTransform (AffineTransform::shear (-slant, 0.0f).translated (slant * H, 0.0f));
    return p;
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

    // Silkscreened tick scale around the pot, as printed on the fascia: a
    // longer mark at each end and at centre, shorter ones between.
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (endAngle - startAngle) * (float) i / 10.0f;
        const bool major = (i % 5) == 0;
        const auto p1 = centre.getPointOnCircumference (radius + 2.0f, a);
        const auto p2 = centre.getPointOnCircumference (radius + (major ? 7.0f : 5.0f), a);
        g.setColour (major ? kLegend.withAlpha (0.75f) : kLegendDim.withAlpha (0.55f));
        g.drawLine (Line<float> (p1, p2), major ? 1.4f : 1.0f);
    }

    // Knurled black cap: skirt, fluted edge, domed top.
    const float capR = radius * 0.80f;
    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillEllipse (Rectangle<float> (capR * 2.0f, capR * 2.0f).withCentre (centre.translated (0.0f, 2.0f)));

    g.setColour (kKnobEdge);
    g.fillEllipse (Rectangle<float> (capR * 2.0f, capR * 2.0f).withCentre (centre));

    // Flutes: short radial notches around the skirt catch the light.
    for (int i = 0; i < 24; ++i)
    {
        const float a = MathConstants<float>::twoPi * (float) i / 24.0f;
        const auto q1 = centre.getPointOnCircumference (capR * 0.86f, a);
        const auto q2 = centre.getPointOnCircumference (capR, a);
        g.setColour (Colours::black.withAlpha (0.55f));
        g.drawLine (Line<float> (q1, q2), 1.6f);
    }

    const float topR = capR * 0.84f;
    g.setGradientFill (ColourGradient (kKnobCap.brighter (0.22f), centre.x, centre.y - topR,
                                       kKnobCap,                  centre.x, centre.y + topR, false));
    g.fillEllipse (Rectangle<float> (topR * 2.0f, topR * 2.0f).withCentre (centre));
    g.setColour (kKnobRing.withAlpha (0.45f));
    g.drawEllipse (Rectangle<float> (topR * 2.0f, topR * 2.0f).withCentre (centre), 1.0f);

    // White pointer line running from the centre out over the skirt.
    const auto p1 = centre.getPointOnCircumference (topR * 0.10f, angle);
    const auto p2 = centre.getPointOnCircumference (capR * 0.98f, angle);
    g.setColour (kPointer);
    g.drawLine (Line<float> (p1, p2), 2.2f);
}

void Dirty630LookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool highlighted, bool)
{
    // Square push-switch like the panel's latching buttons: black body that
    // sits proud of the fascia, red face and a tiny lamp when latched.
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();

    g.setColour (Colours::black.withAlpha (0.7f));
    g.fillRoundedRectangle (r.translated (0.0f, 1.5f), 2.0f);

    const Colour body = on ? kButtonOn.darker (0.25f)
                           : (highlighted ? kPanelHi.brighter (0.3f) : kPanelHi.brighter (0.1f));
    g.setGradientFill (ColourGradient (body.brighter (0.18f), r.getCentreX(), r.getY(),
                                       body.darker (0.3f),    r.getCentreX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 2.0f);
    g.setColour (on ? kButtonOn.brighter (0.3f) : kKnobRing.withAlpha (0.5f));
    g.drawRoundedRectangle (r, 2.0f, 1.0f);

    g.setColour (on ? Colours::white : kLegend.withAlpha (0.85f));
    g.setFont (Font (FontOptions (9.5f, Font::bold)));
    g.drawText (b.getButtonText(), r, Justification::centred);
}

//==============================================================================
void GrMeter::paint (Graphics& g)
{
    // 12 round LEDs in a row with the scale printed underneath, as on the
    // panel's GAIN REDUCTION display: green through amber into red.
    static const float steps[] = { 1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 20, 30 };
    auto r = getLocalBounds().toFloat();
    const int n = (int) std::size (steps);
    const float segW = r.getWidth() / (float) n;
    const float ledD = jmin (segW - 4.0f, r.getHeight() - 14.0f);

    for (int i = 0; i < n; ++i)
    {
        const float cx = r.getX() + segW * ((float) i + 0.5f);
        const float cy = r.getY() + ledD * 0.5f + 1.0f;
        const bool lit = value >= steps[i];
        const Colour c = i >= 9 ? kLedRed : (i >= 6 ? kLedAmber : kLedGreen);
        auto led = Rectangle<float> (ledD, ledD).withCentre ({ cx, cy });

        if (lit)
        {
            g.setColour (c.withAlpha (0.28f));
            g.fillEllipse (led.expanded (3.0f));
        }
        g.setColour (lit ? c : kLedOff);
        g.fillEllipse (led);
        if (lit)
        {
            // Specular dot, so a lit LED reads as glass rather than a disc.
            g.setColour (Colours::white.withAlpha (0.5f));
            g.fillEllipse (Rectangle<float> (ledD * 0.3f, ledD * 0.3f)
                               .withCentre ({ cx - ledD * 0.16f, cy - ledD * 0.18f }));
        }
        g.setColour (Colours::black.withAlpha (0.6f));
        g.drawEllipse (led, 1.0f);

        g.setColour (kLegendDim);
        g.setFont (Font (FontOptions (8.0f)));
        g.drawText (String ((int) steps[i]), Rectangle<float> (cx - segW * 0.5f, r.getBottom() - 23.0f, segW, 11.0f),
                    Justification::centred);
    }

    // Panel caption with the rules either side, as silkscreened on the unit.
    g.setColour (kLegendDim);
    g.setFont (Font (FontOptions (8.5f, Font::bold)));
    const String caption = "GAIN REDUCTION dB";
    const float capW = GlyphArrangement::getStringWidth (g.getCurrentFont(), caption) + 12.0f;
    auto capR = Rectangle<float> (r.getCentreX() - capW * 0.5f, r.getBottom() - 12.0f, capW, 11.0f);
    g.drawText (caption, capR, Justification::centred);
    g.setColour (kLegendDim.withAlpha (0.45f));
    const float ruleY = capR.getCentreY();
    g.drawLine (r.getX(), ruleY, capR.getX() - 4.0f, ruleY, 1.0f);
    g.drawLine (capR.getRight() + 4.0f, ruleY, r.getRight(), ruleY, 1.0f);
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
    makeToggle (s.peak,  ids::ch (ids::peak, 0), "PEAK");
    makeToggle (s.soft,  ids::ch (ids::soft, 0), "SOFT");
    makeToggle (s.clean, ids::clean,             "CLEAN");
    addAndMakeVisible (s.meter);

    makeKnob (inputTrim, ids::inputTrim, "INPUT");
    makeKnob (calib,     ids::calib,     "0 dBFS =");
    makeKnob (thump,     ids::thump,     "THUMP");
    makeKnob (noise,     ids::noise,     "NOISE");
    makeKnob (mismatch,  ids::mismatch,  "MISMATCH");
    makeKnob (mix,       ids::mix,       "MIX");
    makeToggle (extSc,  ids::extSc,                "EXT SC");
    makeToggle (bypass, ids::ch (ids::bypass, 0),  "BYPASS");

    oversamp.addItemList ({ "1x", "2x", "4x" }, 1);
    addAndMakeVisible (oversamp);
    oversampAttachment = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, ids::oversamp, oversamp);
    oversampLabel.setText ("OVERSAMPLE", dontSendNotification);
    oversampLabel.setJustificationType (Justification::centred);
    addAndMakeVisible (oversampLabel);

    dirtLabel.setText ("DIRT", dontSendNotification);
    dirtLabel.setFont (Font (FontOptions (13.0f, Font::bold)));
    dirtLabel.setColour (Label::textColourId, kLedRed.brighter (0.2f));
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

    // Brushed-aluminium fascia: faint horizontal grain over the black panel.
    {
        Random rng (0x3630);
        for (int yy = 0; yy < getHeight(); yy += 2)
        {
            const float a = 0.012f + 0.016f * rng.nextFloat();
            g.setColour (Colours::white.withAlpha (a));
            g.drawHorizontalLine (yy, 0.0f, (float) getWidth());
        }
    }

    // Header band carrying the branding, with a hairline under it.
    g.setColour (kPanelHi);
    g.fillRect (0, 0, getWidth(), kHeaderH);
    g.setColour (Colours::black.withAlpha (0.8f));
    g.fillRect (0, kHeaderH - 1, getWidth(), 1);
    g.setColour (Colours::white.withAlpha (0.05f));
    g.fillRect (0, 0, getWidth(), 1);

    // Rack-ear screws at the far left and right of the header.
    auto screw = [&g] (float cx, float cy)
    {
        auto r = Rectangle<float> (11.0f, 11.0f).withCentre ({ cx, cy });
        g.setColour (Colours::black.withAlpha (0.75f));
        g.fillEllipse (r.expanded (1.0f));
        g.setGradientFill (ColourGradient (Colour (0xff5a5a5e), r.getX(), r.getY(),
                                           Colour (0xff232326), r.getRight(), r.getBottom(), false));
        g.fillEllipse (r);
        g.setColour (Colours::black.withAlpha (0.8f));
        g.drawLine (cx - 3.5f, cy - 1.2f, cx + 3.5f, cy + 1.2f, 1.4f);
    };
    screw (16.0f, kHeaderH * 0.5f);
    screw ((float) getWidth() - 16.0f, kHeaderH * 0.5f);

    // Brand wordmark, in the slot the manufacturer's logo occupies.
    const float logoX = 34.0f, logoY = kHeaderH * 0.5f;
    float logoRight = logoX;
    {
        const float capH = 18.0f;
        auto wm = minusWordmark();
        const auto wb = wm.getBounds();
        const float s = capH / wb.getHeight();
        wm.applyTransform (AffineTransform::translation (-wb.getX(), -wb.getY())
                               .scaled (s, s)
                               .translated (logoX, logoY - capH * 0.5f));
        g.setColour (kBrand);
        g.fillPath (wm);
        logoRight = logoX + wb.getWidth() * s;
    }

    // "DIRTY630 COMPRESSOR" in medium, then the light-weight description,
    // both letter-spaced like the silkscreen.
    auto spaced = [&g] (const String& text, float x, float y, float h, float tracking)
    {
        float cx = x;
        for (auto c : text)
        {
            const String s = String::charToString (c);
            const float w = GlyphArrangement::getStringWidth (g.getCurrentFont(), s);
            g.drawText (s, Rectangle<float> (cx, y, w + 4.0f, h), Justification::centredLeft);
            cx += w + tracking;
        }
        return cx;
    };

    const float textY = logoY - 9.0f;
    g.setColour (kLegend);
    g.setFont (Font (FontOptions (15.0f, Font::bold)));
    const float afterTitle = spaced ("DIRTY630 COMPRESSOR", logoRight + 22.0f, textY, 18.0f, 1.6f);

    g.setColour (kLegend.withAlpha (0.82f));
    g.setFont (Font (FontOptions (11.5f, Font::plain)));
    spaced ("RMS/PEAK STEREO COMPRESSOR LIMITER WITH GATE", afterTitle + 14.0f, textY + 2.0f, 16.0f, 1.3f);

    // Build stamp, right-aligned clear of the right-hand screw.
    g.setColour (kLegendDim.withAlpha (0.7f));
    g.setFont (Font (FontOptions (9.5f)));
    g.drawText ("version " DIRTY630_VERSION "  -  " DIRTY630_BUILD_TIME "  -  minus",
                0, kHeaderH, getWidth() - 20, 14, Justification::centredRight);

    // Section rail above the global row, stopping short of the MIX knob.
    g.setColour (Colours::black.withAlpha (0.65f));
    g.fillRect (0, 196, getWidth() - 160, 1);
    g.setColour (Colours::white.withAlpha (0.06f));
    g.fillRect (0, 197, getWidth() - 160, 1);
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
        s.peak.button.setBounds  (x, top + 30, 64, 24);
        s.soft.button.setBounds  (x, top + 62, 64, 24);
        s.clean.button.setBounds (x, top + 94, 64, 24);
        x += 84;
        Knob* gate[] = { &s.gateThr, &s.gateRate };
        for (auto* k : gate)
        {
            k->label.setBounds (x, top + 4, knobW, labelH);
            k->slider.setBounds (x, top + 18, knobW, knobH);
            x += knobW;
        }
        x += 10;
        s.meter.setBounds (x, top + 42, getWidth() - x - 24, 56);
    };

    layoutStrip (strip, kHeaderH + 16);

    // Bottom row: global and "dirt" controls.
    const int top = 206, knobW = 80, knobH = 68;
    int x = 16;
    extSc.button.setBounds  (x, top + 6,  70, 24);
    bypass.button.setBounds (x, top + 38, 70, 24); x += 90;
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
