#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cmath>

// Små DSP-byggstenar för BETONG. Allt här körs i ljudtråden: inga allokeringar.
namespace betong
{
inline float fastTanh (float x) noexcept
{
    // Padé-approximation, mjuk och billig, klipps vid ±3
    x = juce::jlimit (-3.0f, 3.0f, x);
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

//==============================================================================
// Zavalishin TPT state-variable filter (stabil vid snabb modulering och hög resonans)
struct Svf
{
    enum Mode { LP, HP, BP, Notch };

    void reset() noexcept { ic1 = ic2 = 0.0f; }

    void setParams (float cutoffHz, float resonance01, double sampleRate) noexcept
    {
        cutoffHz = juce::jlimit (16.0f, (float) (sampleRate * 0.45), cutoffHz);
        g = std::tan (juce::MathConstants<float>::pi * cutoffHz / (float) sampleRate);
        // resonans 0..1 -> k 2..0.02 (nära självsvängning i toppen)
        k = 2.0f - 1.98f * juce::jlimit (0.0f, 1.0f, resonance01);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    float process (float x, Mode mode) noexcept
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        switch (mode)
        {
            case LP:    return v2;
            case HP:    return x - k * v1 - v2;
            case BP:    return v1;
            case Notch: return x - k * v1;
        }
        return v2;
    }

    float g = 0, k = 2, a1 = 0, a2 = 0, a3 = 0, ic1 = 0, ic2 = 0;
};

struct OnePole
{
    float z = 0.0f, a = 0.0f;
    void setCutoff (float hz, double sr) noexcept { a = std::exp (-juce::MathConstants<float>::twoPi * hz / (float) sr); }
    float lp (float x) noexcept { z = x + a * (z - x); return z; }
    float hp (float x) noexcept { return x - lp (x); }
};

//==============================================================================
// Ett spårs kedja: drive -> techno-filter (med envelope) -> crush. Körs 2x översamplat.
class LaneChain
{
public:
    enum FilterType { LP24, LP12, HP12, BP12, Notch, Off };

    void prepare (double sampleRate, int maxBlock)
    {
        sr = sampleRate;
        oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
            2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
        oversampler->initProcessing ((size_t) maxBlock);
        for (auto& f : svfA) f.reset();
        for (auto& f : svfB) f.reset();
        env = 0.0f;
        holdL = holdR = 0.0f; holdCount = 0.0f;
    }

    float getLatency() const { return oversampler ? oversampler->getLatencyInSamples() : 0.0f; }

    void trigger (float amount) noexcept { env = juce::jmax (env, amount); }

    struct Params
    {
        int type = LP24;
        float cutoff = 18000.0f, resonance = 0.1f, drive = 0.0f;
        float envAmount = 0.0f, envDecayMs = 150.0f, crush = 0.0f;
    };

    void process (juce::AudioBuffer<float>& buffer, int numSamples, const Params& p)
    {
        if (numSamples <= 0)
            return;

        juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), 2, (size_t) numSamples);
        auto up = oversampler->processSamplesUp (block);
        const double osr = sr * 2.0;
        const int n = (int) up.getNumSamples();
        auto* L = up.getChannelPointer (0);
        auto* R = up.getChannelPointer (1);

        const float driveGain = 1.0f + p.drive * p.drive * 7.0f; // max ca +18 dB in i mättningen
        const float makeUp = 1.0f / std::sqrt (driveGain);
        const float envCoef = std::exp (-1.0f / (0.001f * juce::jmax (5.0f, p.envDecayMs) * (float) osr));
        const auto mode = p.type == HP12 ? Svf::HP : p.type == BP12 ? Svf::BP : p.type == Notch ? Svf::Notch : Svf::LP;
        // hög resonans: sänk nivån in i filtret (som en klassisk ladder) så toppen inte tar över
        const float resIn = 1.0f - 0.55f * p.resonance * p.resonance;

        for (int i = 0; i < n; ++i)
        {
            if ((i & 15) == 0 && p.type != Off)
            {
                const float octaves = p.envAmount * env * 6.0f;
                const float cut = p.cutoff * std::exp2 (octaves);
                // steg 1 bär resonansen, steg 2 (bara LP 24) är odämpat Butterworth-likt
                svfA[0].setParams (cut, p.resonance, osr); svfA[1] = copyCoeffs (svfA[1], svfA[0]);
                svfB[0].setParams (cut, 0.3f, osr); svfB[1] = copyCoeffs (svfB[1], svfB[0]);
            }
            env *= envCoef;

            float l = L[i], r = R[i];
            if (p.drive > 0.001f)
            {
                l = fastTanh (l * driveGain) * makeUp;
                r = fastTanh (r * driveGain) * makeUp;
            }
            if (p.type != Off)
            {
                l = svfA[0].process (l * resIn, mode);
                r = svfA[1].process (r * resIn, mode);
                if (p.type == LP24)
                {
                    l = svfB[0].process (l, mode);
                    r = svfB[1].process (r, mode);
                }
                // resonansen får dista, som på en gammal modulärsynt
                l = fastTanh (l * 1.2f) / 1.2f;
                r = fastTanh (r * 1.2f) / 1.2f;
            }
            L[i] = l; R[i] = r;
        }

        oversampler->processSamplesDown (block);

        if (p.crush > 0.001f)
        {
            // bitdjup 16 -> 4 och sample-and-hold ner mot ~4 kHz
            const float bits = 16.0f - p.crush * 12.0f;
            const float q = std::exp2 (bits - 1.0f);
            const float holdLen = 1.0f + p.crush * p.crush * (float) (sr / 4000.0 - 1.0);
            auto* l = buffer.getWritePointer (0);
            auto* r = buffer.getWritePointer (1);
            for (int i = 0; i < numSamples; ++i)
            {
                holdCount += 1.0f;
                if (holdCount >= holdLen) { holdCount -= holdLen; holdL = l[i]; holdR = r[i]; }
                l[i] = std::round (holdL * q) / q;
                r[i] = std::round (holdR * q) / q;
            }
        }
    }

private:
    static Svf copyCoeffs (Svf target, const Svf& src) noexcept
    {
        target.g = src.g; target.k = src.k; target.a1 = src.a1; target.a2 = src.a2; target.a3 = src.a3;
        return target;
    }

    double sr = 44100.0;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    Svf svfA[2], svfB[2];
    float env = 0.0f;
    float holdL = 0, holdR = 0, holdCount = 0;
};

//==============================================================================
// Dub-delay: ping-pong, filter i återkopplingen, tejp-wobble och freeze.
class DubDelay
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        const int size = (int) (sr * 4.0) + 8;
        buf.setSize (2, size);
        buf.clear();
        writePos = 0;
        for (auto& f : hp) { f.setCutoff (180.0f, sr); f.z = 0; }
        for (auto& f : lp) { f.setCutoff (3000.0f, sr); f.z = 0; }
        phase = 0;
        smoothedDelay.reset (sr, 0.25);
    }

    // delaySec: tid, fb 0..0.98, tone 0..1 (mörk..ljus), wobble 0..1
    void process (const float* inL, const float* inR, float* outL, float* outR, int n,
                  float delaySec, float fb, float tone, float wobble, bool freeze)
    {
        const int size = buf.getNumSamples();
        auto* bl = buf.getWritePointer (0);
        auto* br = buf.getWritePointer (1);
        const float lpHz = 800.0f + tone * tone * 7000.0f;
        const float hpHz = 120.0f + tone * 400.0f;
        for (auto& f : lp) f.setCutoff (lpHz, sr);
        for (auto& f : hp) f.setCutoff (hpHz, sr);
        smoothedDelay.setTargetValue (juce::jlimit (0.01f, 3.5f, delaySec));
        const float inGain = freeze ? 0.0f : 1.0f;
        const float feedback = freeze ? 1.0f : juce::jlimit (0.0f, 0.98f, fb);
        const float lfoInc = juce::MathConstants<float>::twoPi * 0.55f / (float) sr;

        for (int i = 0; i < n; ++i)
        {
            phase += lfoInc;
            if (phase > juce::MathConstants<float>::twoPi) phase -= juce::MathConstants<float>::twoPi;
            const float wob = wobble * 0.004f * std::sin (phase) + wobble * 0.0015f * std::sin (phase * 2.7f);
            const float d = (smoothedDelay.getNextValue() + wob) * (float) sr;

            float rp = (float) writePos - d;
            while (rp < 0) rp += (float) size;
            const int i0 = (int) rp;
            const int i1 = (i0 + 1) % size;
            const float fr = rp - (float) i0;
            const float dl = bl[i0] + fr * (bl[i1] - bl[i0]);
            const float dr = br[i0] + fr * (br[i1] - br[i0]);

            outL[i] = dl;
            outR[i] = dr;

            // ping-pong: vänster matar höger och tvärtom
            float fl = freeze ? dr : lp[0].lp (hp[0].hp (dr));
            float frr = freeze ? dl : lp[1].lp (hp[1].hp (dl));
            fl = fastTanh (fl * feedback * 1.1f) / 1.1f;
            frr = fastTanh (frr * feedback * 1.1f) / 1.1f;

            const float mono = 0.5f * (inL[i] + inR[i]) * inGain;
            bl[writePos] = mono + fl;
            br[writePos] = frr;
            if (++writePos >= size) writePos = 0;
        }
    }

private:
    double sr = 44100.0;
    juce::AudioBuffer<float> buf;
    int writePos = 0;
    OnePole hp[2], lp[2];
    float phase = 0.0f;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedDelay;
};

//==============================================================================
// Rumble: reverb 100 % vått -> distorsion -> lowpass 24 dB -> duckas av kicken -> mono.
class Rumble
{
public:
    void prepare (double sampleRate, int maxBlock)
    {
        sr = sampleRate;
        reverb.setSampleRate (sr);
        reverb.reset();
        for (auto& f : lp) f.reset();
        hpf.z = 0;
        hpf.setCutoff (28.0f, sr);
        tmp.setSize (2, maxBlock);
        duckEnv = 0.0f;
    }

    void kickHit() noexcept { duckEnv = 1.0f; }

    void process (const float* in, float* out, int n, float size, float drive, float cutoff, float duck, double beatSec)
    {
        juce::Reverb::Parameters rp;
        rp.roomSize = 0.55f + 0.44f * size;
        rp.damping = 0.65f;
        rp.wetLevel = 1.0f;
        rp.dryLevel = 0.0f;
        rp.width = 0.0f;
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);

        auto* t = tmp.getWritePointer (0);
        std::copy (in, in + n, t);
        reverb.processMono (t, n);

        for (auto& f : lp) f.setParams (cutoff, 0.15f, sr);
        const float dg = 1.0f + drive * drive * 30.0f;
        const float makeUp = 1.0f / (0.5f + 0.5f * std::sqrt (dg));
        // ducken återhämtar sig över ungefär ett taktslag
        const float rec = std::exp (-1.0f / (float) (beatSec * 0.35 * sr));

        for (int i = 0; i < n; ++i)
        {
            float x = fastTanh (t[i] * 6.0f * dg) * makeUp;
            x = lp[0].process (x, Svf::LP);
            x = lp[1].process (x, Svf::LP);
            x = hpf.hp (x);
            duckEnv *= rec;
            out[i] = x * (1.0f - duck * duckEnv) * 0.5f;
        }
    }

private:
    double sr = 44100.0;
    juce::Reverb reverb;
    Svf lp[2];
    OnePole hpf;
    juce::AudioBuffer<float> tmp;
    float duckEnv = 0.0f;
};

//==============================================================================
// Stutter: spelar en frusen bit av master-ljudet i loop medan knappen hålls inne.
class Stutter
{
public:
    void prepare (double sampleRate)
    {
        buf.setSize (2, (int) (sampleRate * 2.0) + 4);
        buf.clear();
        writePos = 0; active = false; pos = 0; fadeIn = 0.0f;
    }

    void process (float* L, float* R, int n, bool on, int loopLen)
    {
        const int size = buf.getNumSamples();
        loopLen = juce::jlimit (32, size - 4, loopLen);
        auto* bl = buf.getWritePointer (0);
        auto* br = buf.getWritePointer (1);

        if (on && ! active)
        {
            active = true;
            start = (writePos - loopLen + size) % size;
            len = loopLen; pos = 0;
        }
        else if (! on && active)
        {
            active = false; fadeIn = 0.0f;
        }

        for (int i = 0; i < n; ++i)
        {
            if (active)
            {
                const int idx = (start + pos) % size;
                // korta toningar i loopens kanter mot klick
                const int edge = juce::jmin (64, len / 4);
                float g = 1.0f;
                if (pos < edge) g = (float) pos / (float) edge;
                else if (pos > len - edge) g = (float) (len - pos) / (float) edge;
                L[i] = bl[idx] * g;
                R[i] = br[idx] * g;
                if (++pos >= len) pos = 0;
            }
            else
            {
                bl[writePos] = L[i];
                br[writePos] = R[i];
                if (++writePos >= size) writePos = 0;
                if (fadeIn < 1.0f) { fadeIn = juce::jmin (1.0f, fadeIn + 0.01f); L[i] *= fadeIn; R[i] *= fadeIn; }
            }
        }
    }

private:
    juce::AudioBuffer<float> buf;
    int writePos = 0, start = 0, len = 0, pos = 0;
    bool active = false;
    float fadeIn = 1.0f;
};
} // namespace betong
