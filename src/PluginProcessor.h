#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "dsp/Compressor3630.h"

namespace ids
{
    // One set of controls drives both channels.  Its IDs keep the "A" suffix
    // from the old two-strip layout (via ch (name, 0)) for state compatibility.
    static constexpr const char* threshold = "threshold";
    static constexpr const char* ratio     = "ratio";
    static constexpr const char* attack    = "attack";
    static constexpr const char* release   = "release";
    static constexpr const char* output    = "output";
    static constexpr const char* peak      = "peak";
    static constexpr const char* soft      = "soft";
    static constexpr const char* gateThr   = "gateThr";
    static constexpr const char* gateRate  = "gateRate";
    static constexpr const char* bypass    = "bypass";

    static constexpr const char* inputTrim = "inputTrim";
    static constexpr const char* calib     = "calib";
    static constexpr const char* thump     = "thump";
    static constexpr const char* noise     = "noise";
    static constexpr const char* mismatch  = "mismatch";
    static constexpr const char* extSc     = "extSc";
    static constexpr const char* oversamp  = "oversamp";
    static constexpr const char* mix       = "mix";
    static constexpr const char* modded    = "modded";

    inline juce::String ch (const char* base, int index) { return juce::String (base) + (index == 0 ? "A" : "B"); }
}

class Dirty630Processor : public juce::AudioProcessor
{
public:
    Dirty630Processor();
    ~Dirty630Processor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Meter values for the editor (dB of gain reduction, positive).
    std::atomic<float> grMeter[2] { 0.0f, 0.0f };
    std::atomic<float> levelMeter[2] { -90.0f, -90.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateParameters();
    void buildOversamplers();          // allocates: prepareToPlay / resize only
    void selectOversampling (int factorIndex);   // allocation-free swap

    dirty630::Compressor3630 model;
    // Dry/wet is a plugin nicety, not part of the circuit model; the mixer
    // delays the dry path by the oversampling latency so the blend stays
    // phase-aligned (64 samples of headroom covers the 4x IIR filters).
    juce::dsp::DryWetMixer<float> dryWet { 64 };
    // All three factors are built up front so flipping the combo box swaps a
    // pointer instead of allocating on the audio thread.
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplers[3];
    juce::dsp::Oversampling<float>* oversampling = nullptr;
    int currentOversampIndex = -1;
    int currentProgram = 0;
    double hostSampleRate = 48000.0;
    int maxBlock = 512;

    juce::AudioBuffer<float> stereoBuffer, scBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Dirty630Processor)
};
