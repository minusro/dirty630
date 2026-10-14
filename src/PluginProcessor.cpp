#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

//==============================================================================
Dirty630Processor::Dirty630Processor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",     AudioChannelSet::stereo(), true)
                          .withOutput ("Output",    AudioChannelSet::stereo(), true)
                          .withInput  ("Sidechain", AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

AudioProcessorValueTreeState::ParameterLayout Dirty630Processor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto ratioRange = NormalisableRange<float> (1.0f, 100.0f, 0.01f, 0.3f);
    auto ratioText = [] (float v, int) { return v >= 99.0f ? String (CharPointer_UTF8 ("\xe2\x88\x9e:1")) : String (v, v < 10.0f ? 1 : 0) + ":1"; };

    auto msText = [] (float v, int) { return v < 1.0f ? String (v, 2) + " ms" : (v < 1000.0f ? String (v, v < 10.0f ? 1 : 0) + " ms" : String (v / 1000.0f, 2) + " s"); };

    // One set of controls drives both channels.  The IDs keep their old "A"
    // suffix (ids::ch (name, 0)) so early sessions and automation still load.
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::ch (ids::threshold, 0), 1 }, "Threshold",
                    NormalisableRange<float> (-40.0f, 20.0f, 0.1f), 0.0f, AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::ch (ids::ratio, 0), 1 }, "Ratio",
                    ratioRange, 4.0f, AudioParameterFloatAttributes().withStringFromValueFunction (ratioText)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::ch (ids::attack, 0), 1 }, "Attack",
                    NormalisableRange<float> (0.1f, 200.0f, 0.01f, 0.3f), 10.0f, AudioParameterFloatAttributes().withStringFromValueFunction (msText)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::ch (ids::release, 0), 1 }, "Release",
                    NormalisableRange<float> (50.0f, 3000.0f, 1.0f, 0.4f), 300.0f, AudioParameterFloatAttributes().withStringFromValueFunction (msText)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::ch (ids::output, 0), 1 }, "Output",
                    NormalisableRange<float> (-20.0f, 20.0f, 0.1f), 0.0f, AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<AudioParameterBool>  (ParameterID { ids::ch (ids::peak, 0), 1 }, "Peak", false));
    layout.add (std::make_unique<AudioParameterBool>  (ParameterID { ids::ch (ids::soft, 0), 1 }, "Soft Knee", false));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::ch (ids::gateThr, 0), 1 }, "Gate Threshold",
                    NormalisableRange<float> (-70.0f, 10.0f, 0.1f), -70.0f,
                    AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return v <= -70.0f ? String ("Off") : String (v, 1) + " dB"; })));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::ch (ids::gateRate, 0), 1 }, "Gate Rate",
                    NormalisableRange<float> (20.0f, 2000.0f, 1.0f, 0.4f), 100.0f, AudioParameterFloatAttributes().withStringFromValueFunction (msText)));
    layout.add (std::make_unique<AudioParameterBool>  (ParameterID { ids::ch (ids::bypass, 0), 1 }, "Bypass", false));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::inputTrim, 1 }, "Input Trim",
                    NormalisableRange<float> (-12.0f, 24.0f, 0.1f), 0.0f, AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::calib, 1 }, "0 dBFS =",
                    NormalisableRange<float> (10.0f, 24.0f, 0.5f), 18.0f, AudioParameterFloatAttributes().withLabel ("dBu")));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::thump, 1 }, "Thump",
                    NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::noise, 1 }, "Noise",
                    NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::mismatch, 1 }, "Mismatch",
                    NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));
    layout.add (std::make_unique<AudioParameterBool>  (ParameterID { ids::extSc, 1 }, "Ext Sidechain", false));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::oversamp, 1 }, "Oversampling",
                    StringArray { "1x", "2x", "4x" }, 1));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::mix, 1 }, "Mix",
                    NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f, AudioParameterFloatAttributes().withLabel ("%")));

    return layout;
}

//==============================================================================
bool Dirty630Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    if (in != AudioChannelSet::stereo() && in != AudioChannelSet::mono()) return false;

    // Sidechain, if enabled, must be mono or stereo.
    const auto& sc = layouts.getChannelSet (true, 1);
    return sc.isDisabled() || sc == AudioChannelSet::stereo() || sc == AudioChannelSet::mono();
}

void Dirty630Processor::rebuildOversampling (int factorIndex)
{
    const int factor = 1 << factorIndex;
    oversampling = std::make_unique<dsp::Oversampling<float>> (2, factorIndex,
                        dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    oversampling->initProcessing ((size_t) maxBlock);
    oversampling->reset();
    model.prepare (hostSampleRate * factor);
    currentOversampIndex = factorIndex;
    setLatencySamples ((int) std::ceil (oversampling->getLatencyInSamples()));
    dryWet.setWetLatency (oversampling->getLatencyInSamples());
}

void Dirty630Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    hostSampleRate = sampleRate;
    maxBlock = samplesPerBlock;
    currentOversampIndex = -1;
    dryWet.prepare ({ sampleRate, (uint32) samplesPerBlock, 2 });
    rebuildOversampling ((int) apvts.getRawParameterValue (ids::oversamp)->load());
    stereoBuffer.setSize (2, samplesPerBlock);
    scBuffer.setSize (2, samplesPerBlock);
    updateParameters();
}

void Dirty630Processor::updateParameters()
{
    auto get = [this] (const String& id) { return apvts.getRawParameterValue (id)->load(); };

    dirty630::ChannelParams p;
    p.thresholdDbu  = get (ids::ch (ids::threshold, 0));
    const float r   = get (ids::ch (ids::ratio, 0));
    p.ratio         = r >= 99.0f ? 1.0e6 : (double) r;
    p.attackSec     = get (ids::ch (ids::attack, 0))  * 1e-3;
    p.releaseSec    = get (ids::ch (ids::release, 0)) * 1e-3;
    p.outputDb      = get (ids::ch (ids::output, 0));
    p.peakMode      = get (ids::ch (ids::peak, 0)) > 0.5f;
    p.softKnee      = get (ids::ch (ids::soft, 0)) > 0.5f;
    p.gateThreshDbu = get (ids::ch (ids::gateThr, 0));
    p.gateRateSec   = get (ids::ch (ids::gateRate, 0)) * 1e-3;
    p.bypass        = get (ids::ch (ids::bypass, 0)) > 0.5f;
    model.setChannelParams (0, p);
    model.setChannelParams (1, p);

    dirty630::GlobalParams g;
    g.inputTrimDb       = get (ids::inputTrim);
    g.calibDbuAt0dBFS   = get (ids::calib);
    g.thump             = get (ids::thump);
    g.noise             = get (ids::noise);
    g.mismatch          = get (ids::mismatch);
    g.externalSidechain = get (ids::extSc) > 0.5f && getBus (true, 1) != nullptr && getBus (true, 1)->isEnabled();
    model.setGlobalParams (g);

    dryWet.setWetMixProportion (get (ids::mix) * 0.01f);
}

void Dirty630Processor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;

    const int osIndex = (int) apvts.getRawParameterValue (ids::oversamp)->load();
    if (osIndex != currentOversampIndex)
        rebuildOversampling (osIndex);

    updateParameters();

    const int numSamples = buffer.getNumSamples();
    auto mainIn = getBusBuffer (buffer, true, 0);
    const int numMain = mainIn.getNumChannels();

    // Hosts occasionally hand over more than they promised; keep the
    // allocation off the audio thread in the normal case.
    if (numSamples > maxBlock)
    {
        maxBlock = numSamples;
        dryWet.prepare ({ hostSampleRate, (uint32) maxBlock, 2 });
        rebuildOversampling (currentOversampIndex);
        stereoBuffer.setSize (2, maxBlock, false, false, true);
        scBuffer.setSize (2, maxBlock, false, false, true);
    }

    // Work on a stereo block; duplicate mono.
    AudioBuffer<float> stereo (stereoBuffer.getArrayOfWritePointers(), 2, numSamples);
    stereo.copyFrom (0, 0, mainIn, 0, 0, numSamples);
    stereo.copyFrom (1, 0, mainIn, numMain > 1 ? 1 : 0, 0, numSamples);

    const bool useSc = getBus (true, 1) != nullptr && getBus (true, 1)->isEnabled()
                       && apvts.getRawParameterValue (ids::extSc)->load() > 0.5f;
    AudioBuffer<float> sc (scBuffer.getArrayOfWritePointers(), 2, numSamples);
    if (useSc)
    {
        auto scIn = getBusBuffer (buffer, true, 1);
        sc.copyFrom (0, 0, scIn, 0, 0, numSamples);
        sc.copyFrom (1, 0, scIn, scIn.getNumChannels() > 1 ? 1 : 0, 0, numSamples);
    }
    else
        sc.clear();

    dsp::AudioBlock<float> block (stereo);
    dryWet.pushDrySamples (block);
    auto up = oversampling->processSamplesUp (block);

    const int factor = 1 << currentOversampIndex;
    float* l = up.getChannelPointer (0);
    float* r = up.getChannelPointer (1);
    const float* scl = sc.getReadPointer (0);
    const float* scr = sc.getReadPointer (1);
    const int n = (int) up.getNumSamples();

    for (int i = 0; i < n; ++i)
    {
        double dl = l[i], dr = r[i];
        // Sidechain is used at the host rate (zero-order hold is fine for a
        // detector with a 20 ms time constant).
        const int j = jmin (i / factor, numSamples - 1);
        model.processSample (dl, dr, scl[j], scr[j]);
        l[i] = (float) dl; r[i] = (float) dr;
    }

    oversampling->processSamplesDown (block);
    dryWet.mixWetSamples (block);

    auto mainOut = getBusBuffer (buffer, false, 0);
    for (int c = 0; c < mainOut.getNumChannels(); ++c)
        mainOut.copyFrom (c, 0, stereo, jmin (c, 1), 0, numSamples);

    for (int c = 0; c < 2; ++c)
    {
        grMeter[c].store ((float) model.gainReductionDb (c));
        levelMeter[c].store ((float) model.levelDbu (c));
    }
}

//==============================================================================
AudioProcessorEditor* Dirty630Processor::createEditor() { return new Dirty630Editor (*this); }

void Dirty630Processor::getStateInformation (MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void Dirty630Processor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Dirty630Processor(); }
