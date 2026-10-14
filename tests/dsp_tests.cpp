// Plain C++ tests for the dirty630 model.  No JUCE required:
//   g++ -std=c++17 -O2 -I src tests/dsp_tests.cpp -o dirty630_tests && ./dirty630_tests
#include "dsp/Compressor3630.h"

#include <cstdio>
#include <cmath>
#include <vector>
#include <string>

using namespace dirty630;

static int failures = 0;

#define CHECK(cond, ...)                                                       \
    do {                                                                       \
        if (!(cond)) { ++failures; std::printf ("  FAIL: " __VA_ARGS__); std::printf ("\n"); } \
        else         { std::printf ("  ok:   " __VA_ARGS__); std::printf ("\n"); } \
    } while (0)

static constexpr double kFs = 96000.0;

// Feed a sine of the given dBu rms level for `seconds`, return the last-second
// mean gain reduction and output rms (in dBu).
struct RunResult { double grDb; double outDbu; double thd; };

static RunResult runSine (Compressor3630& comp, double levelDbu, double seconds, double hz = 1000.0,
                          bool measureThd = false)
{
    const int n = (int) (seconds * kFs);
    const int tail = (int) (0.5 * kFs);
    const double amp = kDbuRef * std::sqrt (2.0) * dbToLin (levelDbu);
    // convert dBu to full-scale at 18 dBu calibration
    const double fsAmp = amp / (kDbuRef * std::sqrt (2.0) * dbToLin (18.0));

    double grSum = 0.0, sq = 0.0;
    int count = 0;
    // Goertzel-ish THD estimate: accumulate fundamental and total power.
    double fundRe = 0.0, fundIm = 0.0;

    for (int i = 0; i < n; ++i)
    {
        const double t = i / kFs;
        double l = fsAmp * std::sin (2.0 * kPi * hz * t), r = l;
        comp.processSample (l, r);
        if (i >= n - tail)
        {
            grSum += comp.gainReductionDb (0);
            sq += l * l;
            if (measureThd)
            {
                fundRe += l * std::cos (2.0 * kPi * hz * t);
                fundIm += l * std::sin (2.0 * kPi * hz * t);
            }
            ++count;
        }
    }
    const double rmsFs  = std::sqrt (sq / count);
    const double outDbu = 20.0 * std::log10 (rmsFs * (kDbuRef * std::sqrt (2.0) * dbToLin (18.0)) / kDbuRef + 1e-20);
    double thd = 0.0;
    if (measureThd)
    {
        const double fundPow = 2.0 * (fundRe * fundRe + fundIm * fundIm) / (count * (double) count);
        const double total   = sq / count;
        thd = std::sqrt (std::max (0.0, total - fundPow) / fundPow);
    }
    return { grSum / count, outDbu, thd };
}

static Compressor3630 makeComp (const ChannelParams& p, const GlobalParams& g = GlobalParams{})
{
    Compressor3630 c;
    c.prepare (kFs);
    GlobalParams gg = g;
    gg.noise = 0.0;
    c.setGlobalParams (gg);
    c.setChannelParams (0, p);
    c.setChannelParams (1, p);
    return c;
}

int main()
{
    std::printf ("dirty630 DSP tests @ %.0f Hz\n", kFs);

    // ------------------------------------------------------------------ static curve
    {
        std::printf ("\n[static curve] thr=-20 dBu, 4:1, hard knee\n");
        ChannelParams p; p.thresholdDbu = -20; p.ratio = 4; p.attackSec = 0.001; p.releaseSec = 0.05;
        std::printf ("   in dBu   GR dB   expected\n");
        for (double in = -40; in <= 10; in += 10)
        {
            auto c = makeComp (p);
            auto r = runSine (c, in, 1.0);
            const double expected = std::max (0.0, (in + 20) * 0.75);
            std::printf ("   %6.1f  %6.2f   %6.2f\n", in, r.grDb, expected);
            CHECK (std::abs (r.grDb - expected) < 0.6, "GR at %.0f dBu within 0.6 dB", in);
        }
    }

    // ------------------------------------------------------------------ soft knee
    {
        std::printf ("\n[soft knee] thr=-20 dBu, 4:1\n");
        ChannelParams p; p.thresholdDbu = -20; p.ratio = 4; p.softKnee = true; p.attackSec = 0.001; p.releaseSec = 0.05;
        auto at = [&] (double in) { auto c = makeComp (p); return runSine (c, in, 1.0).grDb; };
        const double below = at (-22), atThr = at (-20), above = at (-16);
        std::printf ("   -22 dBu: %.2f   -20 dBu: %.2f   -16 dBu: %.2f\n", below, atThr, above);
        CHECK (below > 0.0 && below < atThr && atThr < above, "soft knee starts below threshold and is monotonic");
        CHECK (std::abs (atThr - 0.75 * 10.0 / 8.0) < 0.5, "soft knee at threshold ~ slope*W/8");
    }

    // ------------------------------------------------------------------ inf:1
    {
        std::printf ("\n[limiting] thr=-10 dBu, inf:1\n");
        ChannelParams p; p.thresholdDbu = -10; p.ratio = 1e6; p.attackSec = 0.0005; p.releaseSec = 0.05;
        auto c = makeComp (p);
        auto r = runSine (c, 5.0, 1.0);
        std::printf ("   in +5 dBu -> out %.2f dBu, GR %.2f dB\n", r.outDbu, r.grDb);
        CHECK (std::abs (r.grDb - 15.0) < 0.6, "inf:1 holds output at threshold");
    }

    // ------------------------------------------------------------------ attack / release timing
    {
        std::printf ("\n[timing] step -40 -> 0 dBu, attack 10 ms / release 300 ms\n");
        ChannelParams p; p.thresholdDbu = -20; p.ratio = 1e6; p.attackSec = 0.010; p.releaseSec = 0.300;
        auto c = makeComp (p);
        const double hz = 1000.0;
        auto feed = [&] (double dbu, double seconds, std::vector<double>* trace)
        {
            const double a = dbToLin (dbu - 18.0);
            const int n = (int) (seconds * kFs);
            for (int i = 0; i < n; ++i)
            {
                double l = a * std::sin (2.0 * kPi * hz * i / kFs), r = l;
                c.processSample (l, r);
                if (trace) trace->push_back (c.gainReductionDb (0));
            }
        };
        feed (-40, 0.5, nullptr);
        std::vector<double> att, rel;
        feed (0, 0.5, &att);
        feed (-40, 2.0, &rel);

        auto timeTo = [] (const std::vector<double>& v, double target, bool rising)
        {
            for (size_t i = 0; i < v.size(); ++i)
                if (rising ? v[i] >= target : v[i] <= target) return i / kFs;
            return -1.0;
        };
        const double finalGr = att.back();
        const double tA = timeTo (att, 0.63 * finalGr, true);
        const double tR = timeTo (rel, 0.37 * finalGr, false);
        std::printf ("   steady GR %.2f dB, 63%% attack %.1f ms, 63%% release %.0f ms\n", finalGr, tA * 1e3, tR * 1e3);
        CHECK (std::abs (finalGr - 20.0) < 0.6, "settles at 20 dB");
        CHECK (tA > 0.005 && tA < 0.040, "attack lands between pot time and pot+RMS tau");
        CHECK (tR > 0.150 && tR < 0.600, "release within range of pot time");
        CHECK (rel.back() < 0.3, "release returns to ~0 GR");
    }

    // ------------------------------------------------------------------ peak vs rms
    {
        std::printf ("\n[peak mode] 100 Hz burst, attack 0.1 ms\n");
        ChannelParams p; p.thresholdDbu = -20; p.ratio = 1e6; p.attackSec = 0.0001; p.releaseSec = 0.05;
        auto grAfter = [&] (bool peak, double ms)
        {
            p.peakMode = peak;
            auto c = makeComp (p);
            const double a = dbToLin (0.0 - 18.0);
            const int n = (int) (ms * 1e-3 * kFs);
            for (int i = 0; i < n; ++i) { double l = a * std::sin (2.0 * kPi * 100.0 * i / kFs), r = l; c.processSample (l, r); }
            return c.gainReductionDb (0);
        };
        const double rms5 = grAfter (false, 5.0), pk5 = grAfter (true, 5.0);
        std::printf ("   after 5 ms: RMS %.2f dB, PEAK %.2f dB\n", rms5, pk5);
        CHECK (pk5 > rms5, "peak mode reacts faster than RMS");
    }

    // ------------------------------------------------------------------ gate
    {
        std::printf ("\n[gate] threshold -30 dBu, rate 50 ms\n");
        ChannelParams p; p.thresholdDbu = 20; p.ratio = 1; p.gateThreshDbu = -30; p.gateRateSec = 0.05;
        auto c = makeComp (p);
        auto loud  = runSine (c, -10, 1.0);
        auto quiet = runSine (c, -50, 1.5);
        std::printf ("   loud GR %.2f dB, quiet GR %.2f dB\n", loud.grDb, quiet.grDb);
        CHECK (loud.grDb < 0.5, "gate open on loud signal");
        CHECK (quiet.grDb > 60.0, "gate closed on quiet signal");
    }

    // ------------------------------------------------------------------ unity path
    {
        std::printf ("\n[unity] thr +20, 1:1, gate off: output should match input\n");
        ChannelParams p; p.thresholdDbu = 20; p.ratio = 1;
        auto c = makeComp (p);
        auto r = runSine (c, 0.0, 1.0, 1000.0, true);
        std::printf ("   in 0 dBu -> out %.2f dBu, THD %.3f %%\n", r.outDbu, r.thd * 100.0);
        CHECK (std::abs (r.outDbu) < 0.2, "level preserved");
        CHECK (r.thd < 0.002, "THD below 0.2 %% at 0 dBu");
    }

    // ------------------------------------------------------------------ distortion rises with GR
    {
        std::printf ("\n[VCA distortion] THD at +4 dBu, 0 vs 20 dB GR\n");
        ChannelParams clean; clean.thresholdDbu = 20; clean.ratio = 1;
        ChannelParams squash; squash.thresholdDbu = -16; squash.ratio = 1e6; squash.outputDb = 20.0; squash.attackSec = 0.001;
        GlobalParams noThump; noThump.thump = 0.0;   // keep the coupling-cap settling out of the THD figure
        auto c1 = makeComp (clean, noThump), c2 = makeComp (squash, noThump);
        auto r1 = runSine (c1, 4.0, 1.0, 1000.0, true), r2 = runSine (c2, 4.0, 1.0, 1000.0, true);
        std::printf ("   0 dB GR: %.3f %%   20 dB GR: %.3f %%\n", r1.thd * 100, r2.thd * 100);
        CHECK (r2.thd > 2.0 * r1.thd, "distortion grows with gain reduction");
        CHECK (r2.thd < 0.01, "but stays under 1 %%");
    }

    // ------------------------------------------------------------------ clipping
    {
        std::printf ("\n[clipping] +24 dBu into the input buffer\n");
        ChannelParams p; p.thresholdDbu = 20; p.ratio = 1;
        auto c = makeComp (p);
        auto r = runSine (c, 24.0, 0.5, 1000.0, true);
        std::printf ("   out %.2f dBu, THD %.1f %%\n", r.outDbu, r.thd * 100);
        CHECK (r.outDbu < 23.5, "output pinned near the rails (clipped wave has higher rms than a sine)");
        CHECK (r.thd > 0.02, "visible clipping distortion");
    }

    // ------------------------------------------------------------------ thump
    {
        std::printf ("\n[feedthrough] DC step after fast 20 dB gain change\n");
        ChannelParams p; p.thresholdDbu = -20; p.ratio = 1e6; p.attackSec = 0.0001; p.releaseSec = 0.05;
        GlobalParams g; g.thump = 1.0;
        auto c = makeComp (p, g);
        double peakDc = 0.0;
        const double a = dbToLin (0.0 - 18.0);
        // A 20 kHz tone the coupling cap passes, but we look at the running
        // mean (DC) of the output over 1 ms windows.
        const int n = (int) (0.05 * kFs);
        double acc = 0.0; int w = 0;
        for (int i = 0; i < n; ++i)
        {
            double l = a * std::sin (2.0 * kPi * 20000.0 * i / kFs), r = l;
            c.processSample (l, r);
            acc += l; ++w;
            if (w == 96) { peakDc = std::max (peakDc, std::abs (acc / w)); acc = 0; w = 0; }
        }
        std::printf ("   peak 1 ms-mean offset: %.4f FS (%.1f dBFS)\n", peakDc, linToDb (peakDc));
        CHECK (peakDc > 1e-4, "feedthrough produces a thump");
    }

    // ------------------------------------------------------------------ modded
    {
        std::printf ("\n[modded] THAT 2180 + op-amp swap vs stock\n");
        // THD at unity (no GR), thump off: pure VCA nonlinearity.  Under
        // compression the dominant residual is gain-modulation distortion
        // from detector ripple, which the mod rightly does not remove.
        ChannelParams unity; unity.thresholdDbu = 20; unity.ratio = 1;
        GlobalParams g; g.thump = 0.0;
        GlobalParams gm = g; gm.modded = true;
        auto cs = makeComp (unity, g);
        auto cm = makeComp (unity, gm);
        const auto rs = runSine (cs, 4.0, 2.0, 1000.0, true);
        const auto rm = runSine (cm, 4.0, 2.0, 1000.0, true);
        std::printf ("   unity THD stock %.4f %%, modded %.4f %%\n", rs.thd * 100, rm.thd * 100);
        CHECK (rm.thd < 0.15 * rs.thd, "modded VCA is an order of magnitude cleaner");

        ChannelParams p; p.thresholdDbu = -20; p.ratio = 4;
        auto cs2 = makeComp (p, g);
        auto cm2 = makeComp (p, gm);
        const double grs = runSine (cs2, 4.0, 2.0).grDb;
        const double grm = runSine (cm2, 4.0, 2.0).grDb;
        std::printf ("   GR stock %.2f dB, modded %.2f dB\n", grs, grm);
        CHECK (std::abs (grs - grm) < 0.3, "the gain law itself is unchanged");
    }

    // ------------------------------------------------------------------ stereo link
    {
        std::printf ("\n[link] loud left, quiet right (mono-sum detector)\n");
        ChannelParams p; p.thresholdDbu = -20; p.ratio = 4; p.attackSec = 0.001; p.releaseSec = 0.05;
        GlobalParams g;
        auto c = makeComp (p, g);
        const double aL = dbToLin (0.0 - 18.0), aR = dbToLin (-40.0 - 18.0);
        for (int i = 0; i < (int) kFs; ++i)
        {
            double l = aL * std::sin (2.0 * kPi * 1000 * i / kFs), r = aR * std::sin (2.0 * kPi * 1000 * i / kFs);
            c.processSample (l, r);
        }
        std::printf ("   GR L %.2f dB, R %.2f dB\n", c.gainReductionDb (0), c.gainReductionDb (1));
        CHECK (std::abs (c.gainReductionDb (0) - c.gainReductionDb (1)) < 0.2, "linked channels share gain reduction");
        CHECK (c.gainReductionDb (1) > 10.0, "quiet channel is pulled down by the loud one");
    }

    std::printf ("\n%s (%d failure%s)\n", failures ? "FAILED" : "ALL PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
