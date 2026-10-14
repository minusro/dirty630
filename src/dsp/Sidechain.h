// Control-voltage path of one 3630 channel: threshold/ratio/knee network,
// the diode-steered attack/release RC, and the gate.
//
// All quantities here are in dB of gain reduction (positive = quieter), which
// is what the hardware's control voltage represents (6 mV/dB at the VCA).
#pragma once

#include "Circuit.h"

namespace dirty630
{

//==============================================================================
// Threshold / ratio / knee.
//
// The 3630 derives the control voltage from (detector - threshold) through an
// op-amp whose gain is set by the RATIO pot.  The HARD/SOFT switch swaps a
// diode clamp for a resistor blend (R83/R17 on the board), which spreads the
// onset of compression over roughly 10 dB around threshold.
//==============================================================================
struct GainComputer
{
    double thresholdDbu = 0.0;
    double ratio        = 4.0;    // >= 1; use a large number for inf:1
    bool   softKnee     = false;

    static constexpr double kSoftWidthDb = 10.0;
    static constexpr double kHardWidthDb = 0.5;   // even the "hard" knee has a diode's worth of rounding

    inline double reductionDb (double levelDbu) const
    {
        const double slope = 1.0 - 1.0 / std::max (ratio, 1.0);
        const double w     = softKnee ? kSoftWidthDb : kHardWidthDb;
        const double over  = levelDbu - thresholdDbu;

        if (over <= -0.5 * w) return 0.0;
        if (over >=  0.5 * w) return slope * over;
        const double t = over + 0.5 * w;
        return slope * t * t / (2.0 * w);
    }
};

//==============================================================================
// Diode-steered attack / release network.
//
// The control voltage sits on a capacitor; the ATTACK pot charges it through
// one diode when the target rises and the RELEASE pot discharges it through
// another when it falls.  Two consequences we keep:
//
//   * the time constants are fully independent (and very wide: 0.1 ms - 200 ms
//     attack, 50 ms - 3 s release on the front panel);
//   * near the target neither diode conducts, so there is a small dead band
//     where the CV simply holds.  It is only a fraction of a dB but it is a
//     real part of the grain on sustained material.
//==============================================================================
struct TimingNetwork
{
    void prepare (double sampleRate) { fs = sampleRate; reset(); }
    void reset() { cv = 0.0; }

    void setTimes (double attackSeconds, double releaseSeconds)
    {
        attackCoef  = tcCoef (attackSeconds,  fs);
        releaseCoef = tcCoef (releaseSeconds, fs);
    }

    inline double process (double targetDb)
    {
        const double diff = targetDb - cv;
        if (diff > kDeadBandDb)       cv += diff * attackCoef;
        else if (diff < -kDeadBandDb) cv += diff * releaseCoef;
        return cv;
    }

    static constexpr double kDeadBandDb = 0.15;

    double fs = 48000.0;
    double cv = 0.0;
    double attackCoef = 1.0, releaseCoef = 1.0;
};

//==============================================================================
// Noise gate.
//
// The 3630 gate drives the same VCA with its own control voltage.  It opens
// almost instantly, closes at the RATE pot's speed, and has only about a dB
// of hysteresis, which is why it chatters on anything that hovers around the
// threshold.  "Off" is modelled as a threshold below any real signal.
//==============================================================================
struct Gate
{
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        openCoef = tcCoef (kOpenSeconds, fs);
        reset();
    }

    void reset() { open = true; cv = 0.0; }

    void setRate (double closeSeconds) { closeCoef = tcCoef (closeSeconds, fs); }

    double thresholdDbu = -70.0;   // <= -70 means off
    static constexpr double kHysteresisDb = 1.0;
    static constexpr double kDepthDb      = 80.0;
    static constexpr double kOpenSeconds  = 0.0005;

    // Returns gain reduction in dB (positive).
    inline double process (double levelDbu)
    {
        if (thresholdDbu <= -70.0)
        {
            open = true;
            cv += (0.0 - cv) * openCoef;
            return cv;
        }

        if (open  && levelDbu < thresholdDbu - kHysteresisDb) open = false;
        if (!open && levelDbu > thresholdDbu)                 open = true;

        const double target = open ? 0.0 : kDepthDb;
        cv += (target - cv) * (open ? openCoef : closeCoef);
        return cv;
    }

    double fs = 48000.0;
    bool   open = true;
    double cv = 0.0;
    double openCoef = 1.0, closeCoef = 1.0;
};

} // namespace dirty630
