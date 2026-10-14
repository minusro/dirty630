// Full two-channel 3630 model.  Pure C++, sample-rate agnostic, no JUCE.
//
// Signal path per channel (mirrors the board):
//
//   in -> calibration -> input buffer (LF347, clips at the rails)
//      -> VCA (2150: gain, distortion, CV feedthrough) + noise
//      -> coupling cap -> output level -> output buffer (clips) -> out
//
// Sidechain (shared):
//
//   mono sum of the two buffer outputs (or of the external SC)
//      -> [peak hold] -> RMS log-domain sensor (one per channel, same input)
//      -> gain computer (threshold / ratio / knee)
//      -> diode-steered attack/release RC  -> CV
//   RMS level -> gate -> CV
//
// One set of panel controls drives both VCAs; the channels only drift apart
// by component spread (Mismatch).
#pragma once

#include "Circuit.h"
#include "Sidechain.h"

namespace dirty630
{

struct ChannelParams
{
    double thresholdDbu = 0.0;
    double ratio        = 4.0;
    double attackSec    = 0.010;
    double releaseSec   = 0.300;
    double outputDb     = 0.0;
    bool   peakMode     = false;
    bool   softKnee     = false;
    double gateThreshDbu = -70.0;   // <= -70 = off
    double gateRateSec  = 0.100;
    bool   bypass       = false;
};

struct GlobalParams
{
    double inputTrimDb   = 0.0;
    double calibDbuAt0dBFS = 18.0;  // what 0 dBFS (peak) means in dBu at the input jack
    double thump         = 0.5;     // 0..1, control-voltage feedthrough amount
    double noise         = 0.5;     // 0..1, VCA/op-amp noise floor
    double mismatch      = 0.5;     // 0..1, channel-to-channel component spread
    bool   externalSidechain = false;
};

//==============================================================================
class Channel
{
public:
    void prepare (double fs, double rmsTauSec, double peakHoldSec)
    {
        sampleRate = fs;
        detector.prepare (fs, rmsTauSec, peakHoldSec);
        timing.prepare (fs);
        gate.prepare (fs);
        coupling.prepare (0.16, fs);   // 10 uF into 100 k
        reset();
    }

    void reset()
    {
        detector.reset();
        timing.reset();
        gate.reset();
        coupling.reset();
        lastReductionDb = 0.0;
    }

    void setParams (const ChannelParams& p)
    {
        params = p;
        computer.thresholdDbu = p.thresholdDbu;
        computer.ratio        = p.ratio;
        computer.softKnee     = p.softKnee;
        timing.setTimes (p.attackSec, p.releaseSec);
        gate.thresholdDbu = p.gateThreshDbu;
        gate.setRate (p.gateRateSec);
        outputGain = dbToLin (p.outputDb);
    }

    // Step 1: run the detector on the sidechain source.  Separated from the
    // audio path so the stereo link can combine the two detectors in between.
    inline void detect (double scVolts)
    {
        levelDbu = detector.process (scVolts, params.peakMode);
    }

    // Step 2: compute the control voltage from a given gain computer / timing
    // (channel B borrows channel A's computer, since one panel drives both).
    inline double controlDb (const GainComputer& gc)
    {
        const double target = gc.reductionDb (levelDbu);
        const double comp   = timing.process (target);
        const double g      = gate.process (levelDbu);
        lastReductionDb = comp + g;
        return -lastReductionDb;
    }

    // Step 3: audio through the VCA and output stage.  v is the input-buffer
    // output in volts; returns volts at the output jack.
    inline double audio (double v, double gainDb, double noiseV)
    {
        if (params.bypass)
            return v;

        double y = vca.process (v, gainDb) + noiseV;
        y = coupling.process (y);
        y *= outputGain;
        return stage.process (y);
    }

    double gainReductionDb() const { return lastReductionDb; }
    double levelDbuValue()   const { return levelDbu; }

    RmsDetector   detector;
    GainComputer  computer;
    TimingNetwork timing;
    Gate          gate;
    Vca           vca;
    OpAmpStage    stage;
    CouplingCap   coupling;
    ChannelParams params;

private:
    double sampleRate = 48000.0;
    double levelDbu   = -90.0;
    double outputGain = 1.0;
    double lastReductionDb = 0.0;
};

//==============================================================================
class Compressor3630
{
public:
    // Timing-cap constants of the RMS sensor and the peak-hold cap.  The dbx
    // sensor in this class of unit uses a timing cap giving a few tens of ms;
    // these two numbers are the first thing to refit against a real unit.
    static constexpr double kRmsTauSec     = 0.020;
    static constexpr double kPeakHoldSec   = 0.010;

    void prepare (double fs)
    {
        sampleRate = fs;
        for (auto& c : ch) c.prepare (fs, kRmsTauSec, kPeakHoldSec);
        applyGlobals();
    }

    void reset()
    {
        for (auto& c : ch) c.reset();
    }

    void setChannelParams (int index, const ChannelParams& p) { ch[index].setParams (p); }

    void setGlobalParams (const GlobalParams& g) { globals = g; applyGlobals(); }

    // Process one stereo sample pair in place.  Inputs/outputs are in
    // full-scale units (1.0 = 0 dBFS).  sc* are external sidechain inputs,
    // also full scale, used only when globals.externalSidechain is set.
    inline void processSample (double& left, double& right, double scL = 0.0, double scR = 0.0)
    {
        // --- input buffers ---------------------------------------------------
        const double vL = ch[0].stage.process (left  * inputScale);
        const double vR = ch[1].stage.process (right * inputScale);

        // --- detector --------------------------------------------------------
        // Both sensors listen to the mono sum of the programme (or of the
        // external sidechain), so the channels always track together; the
        // Mismatch spread still lets B's sensor and VCA drift a little.
        const double det = globals.externalSidechain ? 0.5 * (scL + scR) * inputScale
                                                     : 0.5 * (vL + vR);
        ch[0].detect (det);
        ch[1].detect (det);

        // One set of panel controls (channel A's) drives both VCAs; each
        // channel keeps its own timing network, within component tolerance.
        const double gL = ch[0].controlDb (ch[0].computer);
        const double gR = ch[1].controlDb (ch[0].computer);

        // --- VCAs and output stages ----------------------------------------
        const double nL = noiseAmp * noiseL.next();
        const double nR = noiseAmp * noiseR.next();
        left  = ch[0].audio (vL, gL, nL) / inputScale;
        right = ch[1].audio (vR, gR, nR) / inputScale;
    }

    double gainReductionDb (int index) const { return ch[index].gainReductionDb(); }
    double levelDbu (int index) const        { return ch[index].levelDbuValue(); }

    Channel ch[2];

private:
    void applyGlobals()
    {
        // 0 dBFS peak -> calibDbu (peak).  Vpeak = 0.775 * sqrt(2) * 10^(dBu/20)
        const double vpeakAt0dBFS = kDbuRef * std::sqrt (2.0) * dbToLin (globals.calibDbuAt0dBFS);
        inputScale = vpeakAt0dBFS * dbToLin (globals.inputTrimDb);

        // Feedthrough: up to ~3 mV per dB of gain change at full "thump".
        const double ft = 0.003 * globals.thump;
        // Component spread: B gets a slightly different symmetry trim and
        // feedthrough, and marginally different timing-cap tolerance.
        const double mm = globals.mismatch;
        ch[0].vca.feedthroughVPerDb = ft;
        ch[1].vca.feedthroughVPerDb = ft * (1.0 + 0.6 * mm);
        ch[0].vca.symmetryErr = 0.003;
        ch[1].vca.symmetryErr = 0.003 * (1.0 + 0.8 * mm);
        ch[1].detector.rmsCoef = tcCoef (kRmsTauSec * (1.0 + 0.1 * mm), sampleRate);

        // Noise floor: about -90 dBu with noise at 1.0 (white, pre-coupling cap,
        // so it ends up shaped by the output stage like the real thing).
        noiseAmp = globals.noise * kDbuRef * dbToLin (-90.0) * std::sqrt (2.0);
    }

    double sampleRate = 48000.0;
    double inputScale = 1.0;
    double noiseAmp   = 0.0;
    GlobalParams globals;
    Noise noiseL, noiseR { 0x1234ABCDu };
};

} // namespace dirty630
