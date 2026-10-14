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
    bool   modded        = false;   // THAT 2180 + op-amp swap: near-clean core
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
        outGain.prepare (0.010, fs);
        reset();
    }

    void reset()
    {
        detector.reset();
        timing.reset();
        gate.reset();
        coupling.reset();
        outGain.snap();
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
        outGain.setTarget (dbToLin (p.outputDb));
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
        y *= outGain.next();
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
    Smoothed outGain;
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
        inScale.prepare (0.010, fs);
        applyGlobals();
        inScale.snap();
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
        const double scale = inScale.next();

        // --- input buffers ---------------------------------------------------
        const double vL = ch[0].stage.process (left  * scale);
        const double vR = ch[1].stage.process (right * scale);

        // --- detector --------------------------------------------------------
        // Both sensors listen to the mono sum of the programme (or of the
        // external sidechain), so the channels always track together; the
        // Mismatch spread still lets B's sensor and VCA drift a little.
        const double det = globals.externalSidechain ? 0.5 * (scL + scR) * scale
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
        left  = ch[0].audio (vL, gL, nL) / scale;
        right = ch[1].audio (vR, gR, nR) / scale;
    }

    double gainReductionDb (int index) const { return ch[index].gainReductionDb(); }
    double levelDbu (int index) const        { return ch[index].levelDbuValue(); }

    Channel ch[2];

private:
    void applyGlobals()
    {
        // 0 dBFS peak -> calibDbu (peak).  Vpeak = 0.775 * sqrt(2) * 10^(dBu/20)
        const double vpeakAt0dBFS = kDbuRef * std::sqrt (2.0) * dbToLin (globals.calibDbuAt0dBFS);
        inScale.setTarget (vpeakAt0dBFS * dbToLin (globals.inputTrimDb));

        // "Modded": THAT 2180 in the VCA socket plus an op-amp swap, the
        // classic upgrade.  Laser-trimmed symmetry, specified feedthrough and
        // a quieter floor — the DIRT controls still work, but on a core error
        // ~30x smaller — and the better output stage swings nearer the rails.
        const double clean = globals.modded ? 0.03 : 1.0;

        // Feedthrough: up to ~3 mV per dB of gain change at full "thump".
        const double ft = 0.003 * globals.thump * clean;
        // Component spread: B gets a slightly different symmetry trim and
        // feedthrough, and marginally different timing-cap tolerance.
        const double mm = globals.mismatch;
        ch[0].vca.feedthroughVPerDb = ft;
        ch[1].vca.feedthroughVPerDb = ft * (1.0 + 0.6 * mm);
        ch[0].vca.symmetryErr = 0.003 * clean;
        ch[1].vca.symmetryErr = 0.003 * clean * (1.0 + 0.8 * mm);
        ch[1].detector.rmsCoef = tcCoef (kRmsTauSec * (1.0 + 0.1 * mm), sampleRate);
        for (auto& c : ch)
        {
            c.vca.oddBase    = 0.010  * clean;
            c.vca.oddPerDb   = 0.0045 * clean;
            c.stage.rail      = globals.modded ? 14.5 : 13.5;
            c.stage.softStart = globals.modded ? 13.2 : 12.0;
        }

        // Noise floor: about -90 dBu with noise at 1.0 (white, pre-coupling cap,
        // so it ends up shaped by the output stage like the real thing).
        noiseAmp = globals.noise * kDbuRef * dbToLin (-90.0) * std::sqrt (2.0) * clean;
    }

    double sampleRate = 48000.0;
    Smoothed inScale;
    double noiseAmp   = 0.0;
    GlobalParams globals;
    Noise noiseL, noiseR { 0x1234ABCDu };
};

} // namespace dirty630
