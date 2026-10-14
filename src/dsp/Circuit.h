// Circuit-level building blocks for the dirty630 model.
//
// Everything in this file works in *volts* at the circuit node, so that the
// clipping points, detector constants and VCA feedthrough can be taken from
// the schematic / datasheets rather than tuned by ear.
//
// Reference points used throughout:
//   0 dBu  = 0.775 Vrms
//   Rails  = +/-15 V, LF347 output swing ~ +/-13.5 V into 2k
#pragma once

#include <cmath>
#include <cstdint>
#include <algorithm>

namespace dirty630
{

constexpr double kPi      = 3.14159265358979323846;
constexpr double kDbuRef  = 0.775;          // volts rms at 0 dBu
constexpr double kDbuRef2 = kDbuRef * kDbuRef;

inline double dbToLin (double db)  { return std::pow (10.0, db / 20.0); }
inline double linToDb (double lin) { return 20.0 * std::log10 (std::max (lin, 1e-12)); }

// One-pole coefficient for a time constant in seconds.
inline double tcCoef (double seconds, double fs)
{
    if (seconds <= 0.0) return 1.0;
    return 1.0 - std::exp (-1.0 / (seconds * fs));
}

//==============================================================================
// One-pole parameter smoother for gains the user can ride or automate
// (input trim, output).  Keeps block-rate parameter jumps from zippering;
// not part of the circuit model.
//==============================================================================
struct Smoothed
{
    void prepare (double tauSec, double fs) { coef = tcCoef (tauSec, fs); }
    void setTarget (double t) { tgt = t; }
    void snap() { cur = tgt; }
    inline double next() { cur += (tgt - cur) * coef; return cur; }

    double cur = 1.0, tgt = 1.0, coef = 1.0;
};

//==============================================================================
// Op-amp output stage (LF347 / TL07x class).
//
// The op-amp itself is linear until it runs into the rails; the output
// transistors then saturate within ~1.5 V of the supply.  We model that as a
// linear region up to kSoftStart and a tanh-shaped approach to kRail above it.
// The gain stages also have a small amount of open-loop distortion which is
// negligible next to the VCA, so it is ignored.
//==============================================================================
struct OpAmpStage
{
    double rail      = 13.5;   // volts, hard ceiling
    double softStart = 12.0;   // volts, where saturation begins

    inline double process (double v) const
    {
        const double a = std::abs (v);
        if (a <= softStart)
            return v;
        const double range = rail - softStart;
        const double y = softStart + range * std::tanh ((a - softStart) / range);
        return v < 0.0 ? -y : y;
    }
};

//==============================================================================
// DC blocking capacitor + load resistor (e.g. 10 uF into 100 k -> 0.16 Hz).
// Used on the VCA output so that control-voltage feedthrough shows up as a
// thump rather than a permanent DC offset, exactly as on the hardware.
//==============================================================================
struct CouplingCap
{
    void prepare (double cutoffHz, double fs)
    {
        const double w = 2.0 * kPi * cutoffHz / fs;
        r = 1.0 - w;
        x1 = y1 = 0.0;
    }

    inline double process (double x)
    {
        const double y = x - x1 + r * y1;
        x1 = x; y1 = y;
        return y;
    }

    void reset() { x1 = y1 = 0.0; }

    double r = 0.9999, x1 = 0.0, y1 = 0.0;
};

//==============================================================================
// dbx-style log-domain RMS detector (the "RMS level sensor" the 3630 inherits
// from dbx, 2252 family).
//
// The hardware rectifies the input current, converts it to a log voltage and
// averages *in the log domain* with a single timing capacitor.  Writing the
// capacitor ODE out and substituting y = exp(Vc/Vt) turns it into a linear
// one-pole on the mean-square:
//
//      dy/dt = (x^2 - y) / tau
//
// so the detector is a true-RMS averager whose output, read in dB, has the
// classic dbx asymmetry: dB level rises quickly on a step up and decays along
// an exponential-of-a-log on the way down.  tau is set by the timing cap.
//
// "Peak" mode on the 3630 adds a hold capacitor in front of the sensor so it
// sees a peak-rectified, slowly leaking version of the signal.
//==============================================================================
struct RmsDetector
{
    void prepare (double fs, double rmsTauSeconds, double peakHoldSeconds)
    {
        rmsCoef  = tcCoef (rmsTauSeconds,  fs);
        peakCoef = tcCoef (peakHoldSeconds, fs);
        reset();
    }

    void reset()
    {
        y   = kDbuRef2 * 1e-9;   // -90 dBu: the detector's own noise floor
        pk  = 0.0;
    }

    // Returns the detected level in dBu.
    inline double process (double v, bool peakMode)
    {
        double in = v;
        if (peakMode)
        {
            const double a = std::abs (v);
            // Ideal diode into the hold cap: instant charge, RC leak.
            if (a > pk) pk = a;
            else        pk += (a - pk) * peakCoef;
            in = pk;
        }

        const double sq = in * in;
        y += (sq - y) * rmsCoef;

        // Mean-square relative to 0 dBu, in dB.
        return 10.0 * std::log10 (std::max (y, 1e-18) / kDbuRef2);
    }

    // Mean-square state, exposed for the stereo link (dbx ties the detector
    // outputs together, which averages the two mean-squares).
    double y = 0.0;
    double pk = 0.0;
    double rmsCoef = 0.0, peakCoef = 0.0;
};

//==============================================================================
// dbx 2150-class VCA.
//
// Gain law: 6 mV/dB on the control port, i.e. gain is exact in the dB domain.
// The imperfections that matter audibly:
//
//   * Symmetry: the class-AB log/antilog core has a trim that cancels even
//     harmonics.  On a budget unit it is set once at the factory, so a small
//     2nd-harmonic term remains.
//   * Odd-order distortion rises with gain reduction (the datasheet THD curves
//     climb from ~0.02 % at 0 dB to a few tenths of a percent at -20 dB).
//   * Control feedthrough: the same symmetry error turns a change of control
//     voltage into a DC step at the output.  After the coupling cap that is
//     the "thump" people hear on fast attack settings.
//
// Levels: audio is in volts at the VCA's I/V output, so the distortion
// coefficients below are defined relative to a 10 V reference.
//==============================================================================
struct Vca
{
    double symmetryErr   = 0.003;   // 2nd-harmonic coefficient (dimensionless)
    double oddBase       = 0.010;   // 3rd-harmonic at 0 dB gain reduction
    double oddPerDb      = 0.0045;  // extra 3rd-harmonic per dB of reduction
    double feedthroughVPerDb = 0.0; // volts of DC offset per dB of gain change

    static constexpr double kRef = 10.0; // volts

    // gainDb is the total gain applied (negative = reduction).
    inline double process (double v, double gainDb) const
    {
        const double reduction = std::max (0.0, -gainDb);
        const double n  = v / kRef;
        const double a3 = oddBase + oddPerDb * reduction;
        const double shaped = v + kRef * (symmetryErr * n * n + a3 * n * n * n);
        const double g = dbToLin (gainDb);
        return g * shaped + feedthroughVPerDb * (-gainDb);
    }
};

//==============================================================================
// Tiny deterministic noise source (for the VCA/op-amp noise floor).
//==============================================================================
struct Noise
{
    inline double next()
    {
        // xorshift32 -> uniform in [-1,1], summed twice for a rough gaussian.
        auto u = [this]
        {
            s ^= s << 13; s ^= s >> 17; s ^= s << 5;
            return (double) s / 2147483648.0 - 1.0;
        };
        return 0.5 * (u() + u());
    }
    uint32_t s = 0x9E3779B9u;
};

} // namespace dirty630
