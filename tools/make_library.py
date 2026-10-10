"""BETONG ljudbibliotek. Syntetiserar ~100 techno-ljud från grunden.

Allt räknas i 96 kHz och decimeras till 48 kHz (mindre aliasing),
sparas som 24-bit mono FLAC. Tonala ljud ligger i F-moll.

Kör:  python3 tools/make_library.py assets/library
"""
import os, sys, subprocess, tempfile
import numpy as np
from scipy.signal import butter, sosfilt, fftconvolve, resample_poly
from scipy.io import wavfile

FS = 96000          # intern samplingsfrekvens
OUT_FS = 48000
BPM = 130
BEAT = 60 / BPM
S16 = BEAT / 4
BAR = BEAT * 4
OUT = sys.argv[1] if len(sys.argv) > 1 else "assets/library"
rng = np.random.default_rng(2026)

def mtof(m): return 440.0 * 2 ** ((m - 69) / 12)
F1, F2, F3 = 29, 41, 53  # MIDI: F1 (43.7 Hz), F2, F3

def N(sec): return int(round(sec * FS))
def t(sec): return np.arange(N(sec)) / FS
def filt(x, kind, f, order=2):
    return sosfilt(butter(order, f, btype=kind, fs=FS, output="sos"), x)
def env(sec, decay, attack=0.0005):
    tt = t(sec); a = np.minimum(tt / max(attack, 1e-6), 1.0)
    return a * np.exp(-tt / decay)
def adsr(sec, a, d, s, r, gate):
    x = t(sec)
    att = x / a
    dec = 1 - (1 - s) * (x - a) / d
    rel = s * np.exp(-(x - gate) / r)
    return np.where(x < a, att, np.where(x < a + d, dec, np.where(x < gate, s, rel)))
def noise(sec): return rng.standard_normal(N(sec))
def sine_sweep(f, sec):
    f = np.broadcast_to(f, N(sec)) if np.isscalar(f) else f
    return np.sin(2 * np.pi * np.cumsum(f) / FS)
def saw(f, sec, phase=0.0):
    f = np.full(N(sec), f) if np.isscalar(f) else f
    ph = (np.cumsum(f) / FS + phase) % 1.0
    # polyBLEP mot aliasing
    dt = f / FS; y = 2 * ph - 1
    m = ph < dt; tt = ph[m] / dt[m]; y[m] -= tt + tt - tt * tt - 1
    m = ph > 1 - dt; tt = (ph[m] - 1) / dt[m]; y[m] -= tt * tt + tt + tt + 1
    return y
def square(f, sec, pw=0.5):
    f = np.full(N(sec), f) if np.isscalar(f) else f
    pw = np.full(len(f), pw) if np.isscalar(pw) else pw
    a = saw(f, sec); b = saw(f, sec, phase=0.5)
    # skillnad mellan två fasförskjutna sågtänder = puls; fasen styrs av pw
    ph2 = (np.cumsum(f) / FS + pw) % 1.0
    b = 2 * ph2 - 1
    return np.clip(a - b + (2 * pw - 1), -1.5, 1.5) * 0.66
def tanhd(x, d): return np.tanh(x * d) / np.tanh(d)
def crush(x, bits=12, down=4):
    y = np.repeat(x[::down], down)[: len(x)]; q = 2 ** (bits - 1); return np.round(y * q) / q
def fade(x, ms=4):
    n = min(len(x), int(FS * ms / 1000)); x = x.copy(); x[-n:] *= np.linspace(1, 0, n); return x
def fadein(x, ms=2):
    n = min(len(x), int(FS * ms / 1000)); x = x.copy(); x[:n] *= np.linspace(0, 1, n); return x
def fit(x, n): return np.pad(x, (0, max(0, n - len(x))))[:n]
def place(sec, hits):
    out = np.zeros(int(sec * FS))
    for ts, snd, g in hits:
        s = int(ts * FS); e = min(len(out), s + len(snd))
        if s < len(out): out[s:e] += snd[: e - s] * g
    return out

def svf_lp(x, cut, res, passes=2):
    """State-variable lowpass per sampel (2 pass ≈ 24 dB). cut i Hz (array)."""
    y = x.astype(np.float64)
    cut = np.broadcast_to(cut, len(x)).astype(np.float64)
    q = 1.0 - res
    for _ in range(passes):
        low = band = 0.0; out = np.empty_like(y)
        for i in range(len(y)):
            f = 2 * np.sin(np.pi * min(cut[i], FS * 0.12) / FS)
            low += f * band; high = y[i] - low - q * band; band += f * high
            out[i] = low
        y = out
    return y

def reverb_ir(sec=2.0, decay=0.6, lp=6000):
    tt = t(sec); return filt(rng.standard_normal(len(tt)), "low", lp) * np.exp(-tt / decay) * 0.02
def dub_delay(x, dsec, fb=0.6, lo=300, hi=2500, tail=3.0):
    y = np.concatenate([x, np.zeros(int(tail * FS))]); d = int(dsec * FS); out = y.copy(); tap = y.copy()
    for _ in range(int(tail / dsec) + 1):
        tap = filt(np.concatenate([np.zeros(d), tap[:-d]]), "band", [lo, hi]) * fb; out += tap
    return out

lib = {}
def add(name, x):
    lib[name] = x

# ---------------------------------------------------------------- KICKS
def kick(f0=230, f1=48, pdec=0.035, dec=0.35, length=0.6, click=0.6, drive=1.5, sub=0.0):
    tt = t(length)
    freq = f1 + (f0 - f1) * np.exp(-tt / pdec)
    body = np.sin(2 * np.pi * np.cumsum(freq) / FS) * np.exp(-tt / dec)
    if sub: body += sub * np.sin(2 * np.pi * f1 * tt) * np.exp(-tt / (dec * 1.6))
    c = filt(noise(length), "high", 2500) * np.exp(-tt / 0.003) * click
    return tanhd(body + c, drive)

kk = kick()
add("K01_909_Punch", kk)
add("K02_Deep_Long", kick(180, 43.65, 0.05, 0.85, 1.3, 0.3, 1.2))
add("K03_Distorted", filt(kick(260, 50, 0.03, 0.4, 0.6, 0.6, 7), "low", 9000))
add("K04_Minimal_Tight", kick(300, 60, 0.02, 0.11, 0.3, 1.0, 1.2))
add("K05_Lofi_12bit", crush(kick(240, 50, 0.035, 0.45), 12, 4))
ind = kick(250, 48, 0.03, 0.42, 0.6, 0.8, 4)
ind = ind - 0.5 * filt(filt(ind, "band", [230, 320]), "band", [230, 320])
add("K06_Industrial_Crushed", crush(ind, 10, 3))
hc = kick(280, 52, 0.025, 0.38, 0.55, 0.9, 3); add("K07_Hard_Clipped", np.clip(hc * 2.2, -1, 1))
dry = kick(230, 48, 0.035, 0.3, 1.4, 0.5, 1.6)
wet = fftconvolve(dry, reverb_ir(2.0, 0.7))[: len(dry)]
wet = filt(tanhd(filt(wet, "low", 150) * 6, 2), "low", 110)
duck = np.minimum(t(1.4) / 0.22, 1) ** 2
add("K08_Rumble_Combo", dry + 1.8 * wet / (np.max(np.abs(wet)) + 1e-9) * duck * 0.5)
add("K09_Sub_Boom_F", kick(120, 43.65, 0.08, 1.2, 1.8, 0.15, 1.1, sub=0.6))
add("K10_Click_Thump", kick(400, 70, 0.012, 0.09, 0.25, 1.4, 2))

# ---------------------------------------------------------------- HATS
def metal(length, dec, hp=7000, fr=(205.3, 304.4, 369.6, 522.7, 540.0, 800.0)):
    tt = t(length)
    m = sum(square(f * 1.0, length) for f in fr)
    m = filt(m, "band", [hp, 18000]) + 0.4 * filt(noise(length), "high", hp)
    return m * env(length, dec)
add("H01_Closed", metal(0.16, 0.03))
add("H02_Closed_Short", metal(0.08, 0.011))
add("H03_Open", metal(0.75, 0.2))
add("H04_Ride", metal(1.8, 0.7, 4500) + 0.25 * np.sin(2 * np.pi * 3150 * t(1.8)) * env(1.8, 0.6))
add("H05_Closed_12bit", crush(metal(0.16, 0.035), 12, 4))
ih = crush(metal(0.25, 0.04), 6, 4)
add("H06_Industrial_6bit", ih + 0.3 * fftconvolve(ih, reverb_ir(0.4, 0.08, 9000))[: len(ih)])
rp = metal(0.5, 0.15, 6000, tuple(f * 1.5 for f in (205.3, 304.4, 369.6, 522.7, 540.0, 800.0)))
rp = fit(rp, int(2.5 * FS))
add("H07_Ride_Pitched_Verb", rp + 0.7 * fftconvolve(rp, reverb_ir(2.5, 0.9, 9000))[: len(rp)] / 0.02 * 0.02)
add("H08_Open_Long_Metal", metal(1.4, 0.45, 5000, (180, 263, 411, 587, 702, 913)))

# ---------------------------------------------------------------- CLAPS / SNARES
def clap(dec=0.18):
    L = 0.6; x = np.zeros(int(L * FS)); n = filt(noise(L), "band", [900, 2600])
    for i, off in enumerate([0, 0.011, 0.022, 0.032]):
        s = int(off * FS); x[s:] += n[s:] * env((len(x) - s) / FS, 0.006 if i < 3 else dec)
    return x
cl = clap()
add("C01_Clap_909ish", cl)
g = fftconvolve(cl, reverb_ir(0.6, 0.3, 7000))[: len(cl)]
gate = (t(0.6) < 0.18).astype(float); gate = filt(gate, "low", 60, 1)
add("C02_Clap_Gated", cl + 1.5 * g / (np.max(np.abs(g)) + 1e-9) * gate)
tt = t(0.4)
add("C03_Snare_Tight", np.sin(2 * np.pi * 185 * tt) * env(0.4, 0.06) + 0.8 * filt(noise(0.4), "band", [1500, 9000]) * env(0.4, 0.12))
add("C04_Snare_Noise_Industrial", tanhd(filt(noise(0.35), "band", [600, 7000]) * env(0.35, 0.09, 0.004) * 1.5, 3))
tt = t(0.09)
add("C05_Rim", (np.sin(2 * np.pi * 1700 * tt) + 0.6 * np.sin(2 * np.pi * 480 * tt)) * env(0.09, 0.007))
cv = fit(cl, int(1.6 * FS)); add("C06_Clap_Layer_Verb", cv + 0.6 * fftconvolve(cv, reverb_ir(1.6, 0.5))[: len(cv)] / 0.02 * 0.02)

# ---------------------------------------------------------------- PERC
def tom(f0, f1, dec=0.3):
    L = dec * 3; tt = t(L); fr = f1 + (f0 - f1) * np.exp(-tt / 0.05)
    return np.sin(2 * np.pi * np.cumsum(fr) / FS) * env(L, dec) + 0.12 * filt(noise(L), "band", [f1 * 2, f1 * 8]) * env(L, 0.02)
def fm(fc, fm_, idx, dec, L=0.6, idec=0.04):
    tt = t(L); i = idx * np.exp(-tt / idec)
    return np.sin(2 * np.pi * fc * tt + i * np.sin(2 * np.pi * fm_ * tt)) * env(L, dec)
add("P01_Tom_Low", tom(140, 70))
add("P02_Tom_Mid", tom(210, 110, 0.22))
add("P03_Tom_High", tom(320, 170, 0.18))
add("P04_Metal_FM", fm(420, 1130, 6, 0.12))
add("P05_Bell_FM", fm(880, 1234, 4, 0.35, 1.0))
add("P06_Wood", fm(600, 1500, 2, 0.04, 0.3))
add("P07_Woodblock_6bit", crush(fm(1100, 2200, 1.5, 0.03, 0.25), 6, 4))
add("P08_Shaker", filt(noise(0.22), "band", [5000, 14000]) * env(0.22, 0.04, 0.015))
add("P09_Blip", np.sin(2 * np.pi * 2200 * t(0.1)) * env(0.1, 0.01))
add("P10_Blip_Low", np.sin(2 * np.pi * 880 * t(0.12)) * env(0.12, 0.016))
add("P11_Conga_Low", tom(260, 190, 0.16))
add("P12_Conga_High", tom(420, 330, 0.12))
tt = t(0.5); cb = square(540, 0.5) + square(800, 0.5)
add("P13_Cowbell_Dark", filt(cb, "band", [400, 2500]) * env(0.5, 0.09))
add("P14_Clave", np.sin(2 * np.pi * 2500 * t(0.08)) * env(0.08, 0.012))
mh = fm(160, 437, 12, 0.25, 0.9, 0.08) + 0.4 * filt(noise(0.9), "band", [2000, 6000]) * env(0.9, 0.05)
add("P15_Metal_Hit", tanhd(mh, 2))
st = tom(90, 55, 0.2); spring = fftconvolve(st, filt(noise(1.0), "band", [300, 3000]) * env(1.0, 0.25) * 0.03)[: int(1.0 * FS)]
add("P16_Spring_Thud", fit(st, int(1.0 * FS)) + spring)

# ---------------------------------------------------------------- BAS (F)
def acid_line(steps, step_len, base=250, mod=2600, res=0.85, sq=False, drive=2.5):
    n = int(step_len * FS); total = n * len(steps)
    freq = np.zeros(total); amp = np.zeros(total); cut = np.zeros(total); prev = None
    for k, (note, acc, slide) in enumerate(steps):
        s = k * n; tt = np.arange(n) / FS
        if note is None:
            freq[s:s + n] = freq[s - 1] if s else 100; cut[s:s + n] = base; prev = None; continue
        f = mtof(note)
        if slide and prev is not None:
            gl = np.minimum(tt / 0.06, 1); freq[s:s + n] = prev + (f - prev) * gl
            amp[s:s + n] = 1.0; cut[s:s + n] = cut[s - 1]
        else:
            freq[s:s + n] = f
            amp[s:s + n] = (tt < step_len * 0.62) * np.exp(-tt / 0.3) * (1.25 if acc else 1)
            cut[s:s + n] = base + mod * (1.8 if acc else 1) * np.exp(-tt / (0.07 if acc else 0.14))
        prev = f
    L = total / FS
    osc = square(freq, L, 0.5) if sq else saw(freq, L)
    amp = filt(amp, "low", 300, 1)
    return np.tanh(svf_lp(osc * amp, cut, res) * drive)

tt = t(0.5)
add("B01_Sub_Roll_F", np.sin(2 * np.pi * mtof(F2) * tt) * adsr(0.5, 0.004, 0.1, 0.7, 0.05, 0.35) + 0.2 * np.sin(4 * np.pi * mtof(F2) * tt) * env(0.5, 0.05))
ob = svf_lp(saw(mtof(F2), 0.35), 600 + 1800 * np.exp(-t(0.35) / 0.05), 0.5) * env(0.35, 0.12, 0.002)
add("B02_Offbeat_F", tanhd(ob, 1.5))
add("B03_Acid_Squelch_F", acid_line([(F2, 1, 0), (None, 0, 0)], BEAT * 0.75, res=0.9))
add("B04_Acid_Soft_F", acid_line([(F2, 0, 0), (None, 0, 0)], BEAT * 0.75, base=180, mod=900, res=0.7))
L = 0.9; rs = sum(saw(mtof(F2) * (1 + d), L) for d in (-0.012, -0.004, 0.004, 0.012)) / 2
add("B05_Reese_Dist_F", filt(tanhd(svf_lp(rs, 700 + 300 * np.sin(2 * np.pi * 0.7 * t(L)), 0.3), 3), "low", 4000) * adsr(L, 0.01, 0.2, 0.8, 0.1, 0.75))
add("B06_FM_Bass_F", np.sin(2 * np.pi * mtof(F2) * t(0.5) + 3 * np.exp(-t(0.5) / 0.06) * np.sin(2 * np.pi * mtof(F2) * 2 * t(0.5))) * env(0.5, 0.25))
rt = filt(tanhd(svf_lp(saw(mtof(F1), 1.4), 160, 0.5) * 2, 3), "low", 140) * adsr(1.4, 0.01, 0.3, 0.6, 0.3, 1.0)
add("B07_Rumble_Tone_F", rt)
add("B08_Saw_Stab_Bass_F", tanhd(svf_lp(saw(mtof(F2), 0.4) + saw(mtof(F3), 0.4) * 0.5, 300 + 3000 * np.exp(-t(0.4) / 0.04), 0.6), 2) * env(0.4, 0.15))
add("B09_Pluck_Bass_F", svf_lp(square(mtof(F2), 0.4, 0.3), 200 + 2500 * np.exp(-t(0.4) / 0.03), 0.4) * env(0.4, 0.18, 0.001))
add("B10_Square_Sub_F", filt(square(mtof(F1 + 12), 0.6, 0.5), "low", 300) * adsr(0.6, 0.005, 0.1, 0.8, 0.05, 0.45))

# ---------------------------------------------------------------- STABS (Fm)
Fm7 = (53, 56, 60, 63); Bbm7 = (58, 61, 65, 68); Fm = (53, 56, 60)
def chord_saw(notes, L, det=0.08):
    return sum(saw(mtof(n) * (1 + d / 100 * 12), L) for n in notes for d in (-det, det)) / len(notes)
st = filt(chord_saw(Fm7, 0.6), "low", 1500) * env(0.6, 0.15)
add("S01_Dub_Fm7_Dry", st)
add("S02_Dub_Fm7_Echo", dub_delay(st, S16 * 3, 0.62, tail=3.5))
add("S03_Dub_Bbm7_Echo", dub_delay(filt(chord_saw(Bbm7, 0.6), "low", 1500) * env(0.6, 0.15), S16 * 3, 0.6, tail=3.5))
L = 0.5; org = sum(sum(a * np.sin(2 * np.pi * mtof(n) * h * t(L)) for h, a in ((1, 1), (2, 0.6), (3, 0.4), (4, 0.25), (6, 0.15))) for n in Fm)
add("S04_Organ_Fm", (org / 4 + 0.3 * filt(noise(L), "band", [2000, 6000]) * env(L, 0.004)) * adsr(L, 0.003, 0.05, 0.85, 0.05, 0.3))
rv = chord_saw(Fm7 + (65,), 0.5, 0.25)
add("S05_Rave_Stab_Fm", tanhd(svf_lp(rv, 400 + 6000 * np.exp(-t(0.5) / 0.05), 0.5), 1.8) * env(0.5, 0.18))
cr = crush(filt(chord_saw(Fm7, 0.6), "low", 2500) * env(0.6, 0.2), 4, 6)
add("S06_Crushed_Noise_Chord", tanhd(filt(cr, "band", [400, 3000]), 2))
L = 2.5; strings = filt(chord_saw((53, 56, 60, 65), L, 0.15), "low", 2200) * adsr(L, 0.35, 0.5, 0.7, 0.6, 1.8)
add("S07_Detroit_Strings_Fm", strings * (1 + 0.15 * np.sin(2 * np.pi * 5.5 * t(L))))
L = 1.6; ring = sum(np.sin(2 * np.pi * mtof(n) * t(L)) * np.sin(2 * np.pi * mtof(n) * 2.01 * t(L)) for n in Fm) * env(L, 0.45)
add("S08_Minor_Ring_Fm", ring / 3)
add("S09_Fifths_Stab_F", tanhd(svf_lp(chord_saw((41, 48, 53, 60), 0.5, 0.1), 300 + 4000 * np.exp(-t(0.5) / 0.06), 0.5), 1.5) * env(0.5, 0.16))
sk = sum(saw(mtof(n) * (1 + d), 0.6) for n, d in ((53, 0.0), (56, 0.009), (60, -0.013), (63, 0.021))) / 4  # avsiktligt fel stämd
add("S10_Skew_Detuned_Fm", dub_delay(filt(sk, "low", 1800) * env(0.6, 0.14), S16 * 5, 0.5, tail=2.5))

# ---------------------------------------------------------------- LEADS
L = 1.2; fdip = mtof(65) * (1 - 0.35 * np.exp(-t(L) / 0.05)) * (1 + 0.006 * np.sin(2 * np.pi * 5 * t(L)))
pw = 0.5 + 0.4 * np.sin(2 * np.pi * 0.8 * t(L))
hv = sum(square(fdip * m, L, pw) for m in (0.5, 0.995, 1.0, 1.006, 2.0)) / 3
add("L01_Hoover_F", tanhd(filt(hv, "low", 7000), 1.6) * adsr(L, 0.02, 0.2, 0.8, 0.2, 0.9))
seq_notes = [65, 65, 68, 65, 72, 65, 70, 68]
sq = np.concatenate([svf_lp(saw(mtof(n), S16), 500 + 3500 * np.exp(-t(S16) / 0.03), 0.6) * env(S16, 0.06) for n in seq_notes * 2])
add("L02_Saw_Seq_F_130", sq)
add("L03_FM_Pling_F", fm(mtof(77), mtof(77) * 3.5, 5, 0.4, 1.0, 0.03))
add("L04_Bleep_High", np.sin(2 * np.pi * 1760 * t(0.15)) * (t(0.15) < 0.09) * env(0.15, 0.5))
arp = [65, 68, 72, 75, 72, 68, 65, 63]
ap = np.concatenate([square(mtof(n), S16, 0.25) * env(S16, 0.05) for n in arp * 4])
add("L05_Arp_Fm_130", filt(ap, "low", 4000) * 0.8)
add("L06_Square_Lead_F", filt(square(mtof(77), 0.8, 0.35), "low", 5000) * adsr(0.8, 0.005, 0.1, 0.7, 0.15, 0.6))
add("L07_Metal_Pling_Detuned", fm(1047 * 1.017, 1047 * 2.73, 7, 0.5, 1.2, 0.05))
L = 0.9; sweep = mtof(77) * (2 + 3 * np.exp(-t(L) / 0.25))
ph1 = np.cumsum(np.full(len(t(L)), mtof(77))) / FS; ph2 = np.cumsum(sweep) / FS
sync = 2 * ((ph2 - np.floor(ph1) * (sweep / mtof(77))) % 1.0) - 1
add("L08_Sync_Scream_F", tanhd(filt(sync, "low", 9000), 2) * adsr(L, 0.005, 0.2, 0.7, 0.2, 0.6))

# ---------------------------------------------------------------- TEXTURER
L = 6.0; pad = filt(chord_saw((53, 56, 60), L, 0.2), "low", 900) * adsr(L, 1.2, 1.0, 0.8, 1.2, 4.5)
add("T01_Dark_Pad_Fm", pad + 0.4 * fftconvolve(pad, reverb_ir(3, 1.2, 3000))[: len(pad)] / 0.02 * 0.01)
L = 5.0; inn = filt(noise(L), "band", [200, 1800]) * (0.6 + 0.4 * np.sign(np.sin(2 * np.pi * 2.7 * t(L))))
add("T02_Industrial_Noise", tanhd(inn, 2) * adsr(L, 0.3, 0.2, 0.9, 0.5, 4.3))
L = 3.5; add("T03_Steam", filt(noise(L), "high", 3000) * adsr(L, 0.05, 0.3, 0.6, 0.8, 2.2))
L = 6.0; hum = sum(np.sin(2 * np.pi * 50 * k * t(L)) / k for k in (1, 2, 3, 5)) * 0.4 + 0.3 * filt(noise(L), "low", 400)
add("T04_Subway_Hum", hum * adsr(L, 1.0, 0.5, 0.9, 1.0, 4.8) * (1 + 0.3 * np.sin(2 * np.pi * 0.25 * t(L))))
L = 6.0; md = sum(fm(mtof(41) * m, mtof(41) * m * 1.41, 2, 4.0, L, 3.0) for m in (1, 2.01, 3.03))
add("T05_Metal_Drone_F", md * adsr(L, 1.5, 0.5, 0.9, 1.0, 5.0) / 3)
L = 4.0; add("T06_Tape_Hiss", filt(noise(L), "band", [2500, 12000]) * 0.4 * (1 + 0.2 * np.sin(2 * np.pi * 0.3 * t(L))) * adsr(L, 0.3, 0.1, 1, 0.5, 3.5))
d8 = place(BAR * 2, [(b * BEAT, kick(230, 48, 0.035, 0.3, 0.6), 1) for b in range(8)])
wet = filt(tanhd(filt(fftconvolve(d8, reverb_ir(2.0, 0.6))[: len(d8)], "low", 150) * 8, 2), "low", 110)
dk = np.ones(len(d8)); dl = int(BEAT * FS); sh = np.minimum(np.linspace(0, 1, dl) * 2.2, 1) ** 2
for b in range(8): dk[b * dl:(b + 1) * dl] = sh[: len(dk[b * dl:(b + 1) * dl])]
add("T07_Rumble_Bed_130", wet * dk)
L = BAR * 3; tt = t(L)
dr = 0.6 * np.sin(2 * np.pi * mtof(F1) * tt) + 0.3 * np.sin(2 * np.pi * mtof(F1) * 1.5 * tt + 0.3 * np.sin(2 * np.pi * 0.1 * tt))
add("T08_Drone_Dark_F", (dr + 0.06 * filt(noise(L), "band", [300, 700])) * np.minimum(tt / 1.5, 1))
L = 5.0; wn = noise(L); cuts = 600 + 500 * np.sin(2 * np.pi * 0.15 * t(L))
add("T09_Wind_Tunnel", svf_lp(wn * 0.3, cuts, 0.8, 1) * adsr(L, 0.8, 0.2, 1, 0.8, 4.2))
L = 5.0; mr = place(L, [(i * 0.23, fm(90, 233, 4, 0.08, 0.3), 0.6) for i in range(int(L / 0.23))]) + 0.3 * filt(noise(L), "band", [100, 800])
add("T10_Machine_Room", mr * adsr(L, 0.3, 0.1, 1, 0.6, 4.4))

# ---------------------------------------------------------------- RÖSTER (formanter)
VOWELS = {"a": (800, 1150, 2900), "o": (450, 800, 2830), "e": (400, 2000, 2550), "u": (325, 700, 2530), "i": (300, 2200, 3000)}
def voice(vowel_seq, L, f0, breath=0.05):
    tt = t(L); f0a = np.full(len(tt), f0) * (1 + 0.004 * np.sin(2 * np.pi * 5.2 * tt))
    src = saw(f0a, L) + breath * noise(L)
    n = len(tt); seg = n // len(vowel_seq); out = np.zeros(n)
    for k, v in enumerate(vowel_seq):
        sl = slice(k * seg, n if k == len(vowel_seq) - 1 else (k + 1) * seg)
        for fc, g in zip(VOWELS[v], (1.0, 0.6, 0.3)):
            out[sl] += g * filt(src, "band", [fc * 0.9, fc * 1.1])[sl]
    return out
add("V01_Ah_F", voice("a", 1.0, mtof(53)) * adsr(1.0, 0.03, 0.2, 0.8, 0.2, 0.75))
add("V02_Oh_F", voice("o", 1.0, mtof(53)) * adsr(1.0, 0.03, 0.2, 0.8, 0.2, 0.75))
vc = voice("e", 1.0, mtof(53)); car = saw(mtof(53), 1.0)
add("V03_Vocoder_Ee_F", filt(vc * 0.7 + 0.3 * car * np.abs(vc), "high", 150) * adsr(1.0, 0.02, 0.1, 0.9, 0.2, 0.8))
add("V04_Formant_Chop", voice("a", 0.18, mtof(56)) * env(0.18, 0.07, 0.003))
add("V05_Robot_Vowels_130", np.concatenate([voice(v, S16 * 2, mtof(53)) * env(S16 * 2, 0.08, 0.003) for v in "aoeiuaoe" * 2]))
add("V06_Whisper", filt(noise(0.8), "band", [1500, 6000]) * adsr(0.8, 0.08, 0.2, 0.6, 0.2, 0.5) * 0.6
    + 0.3 * filt(noise(0.8), "band", [2600, 3200]) * adsr(0.8, 0.05, 0.1, 0.5, 0.2, 0.5))

# ---------------------------------------------------------------- FX
L = BAR * 2; add("X01_Noise_Riser", filt(noise(L) * (t(L) / L) ** 2, "high", 600))
add("X02_Downlifter", filt(noise(BAR), "high", 800) * env(BAR, 0.8))
cr = metal(2.5, 0.9, 3000) + 0.6 * filt(noise(2.5), "high", 4000) * env(2.5, 0.9)
add("X03_Crash", cr); add("X04_Crash_Reverse", cr[::-1])
add("X05_Zap_Down", sine_sweep(60 + 3000 * np.exp(-t(0.3) / 0.03), 0.3) * env(0.3, 0.08))
L = 2.0; add("X06_Siren", sine_sweep(900 + 500 * np.sin(2 * np.pi * 1.5 * t(L)), L) * adsr(L, 0.05, 0.1, 1, 0.2, 1.7) * 0.7)
add("X07_Laser", sine_sweep(4000 * np.exp(-t(0.25) / 0.05) + 200, 0.25) * env(0.25, 0.1))
L = 1.5; rsc = filt(noise(L), "band", [800, 4000]) * (0.5 + 0.5 * np.sign(np.sin(2 * np.pi * 17 * t(L)))) * np.abs(np.sin(2 * np.pi * 3 * t(L)))
add("X08_Radio_Scrape", crush(rsc, 8, 3))
src = chord_saw(Fm7, 1.0); gl = np.zeros(int(1.2 * FS)); gsz = int(0.03 * FS); win = np.hanning(gsz)
for k in range(60):
    p = rng.integers(0, len(src) - gsz); o = rng.integers(0, len(gl) - gsz)
    gl[o:o + gsz] += src[p:p + gsz] * win * rng.uniform(0.3, 1.0)
add("X09_Glitch_Granular", gl)
loop = np.tile(st, 3)[: int(1.5 * FS)]; spd = np.linspace(1, 0, len(loop)) ** 1.5
pos = np.cumsum(spd); pos = pos[pos < len(loop) - 1]
add("X10_Tape_Stop", np.interp(pos, np.arange(len(loop)), loop))
add("X11_Metal_Slam", tanhd(fm(110, 297, 15, 0.4, 1.2, 0.05) + kick(150, 50, 0.03, 0.2, 1.2), 3))

# ---------------------------------------------------------------- LOOPAR 130 BPM (2 takter)
H = lib
def ploop(hits): return place(BAR * 2, hits)
hits = []
for s in range(32):
    ts = s * S16
    if s % 4 == 2: hits.append((ts, H["H03_Open"], 0.45))
    if s % 2 == 1: hits.append((ts + 0.008, H["H01_Closed"], 0.35))
    if s in (4, 12, 20, 28): hits.append((ts, H["C01_Clap_909ish"], 0.6))
    if s in (3, 10, 14, 19, 27, 30): hits.append((ts, H["P02_Tom_Mid"], 0.5))
    if s in (6, 22, 25): hits.append((ts, H["P04_Metal_FM"], 0.4))
    if s in (1, 9, 17): hits.append((ts, H["C05_Rim"], 0.5))
    hits.append((ts, H["P08_Shaker"], 0.18 if s % 2 else 0.1))
add("Lp01_Perc_Swedish_130", ploop(hits))
add("Lp02_Minimal_Click_130", ploop([(s * S16, H["P09_Blip"] if s % 3 else H["P10_Blip_Low"], 0.5) for s in range(32) if s % 3 == 0 or s % 7 == 0]))
ih = []
for s in range(32):
    ts = s * S16
    if s % 4 == 2: ih.append((ts, H["H06_Industrial_6bit"], 0.5))
    if s in (3, 6, 11, 14, 19, 22, 27, 30): ih.append((ts, H["P01_Tom_Low"], 0.6 if s % 8 == 3 else 0.35))
    if s in (5, 13, 21, 29): ih.append((ts, H["P07_Woodblock_6bit"], 0.4 + 0.2 * (s % 3)))
    if s % 8 == 6: ih.append((ts, H["P08_Shaker"], 0.3))
    if s in (4, 12, 20, 28): ih.append((ts, H["C04_Snare_Noise_Industrial"], 0.5))
add("Lp03_Industrial_Perc_130", ploop(ih))
bn = [F2, None, F2 + 12, F2, None]  # 5 steg
add("Lp04_Bass_5step_F_130", ploop([(s * S16, H["B09_Pluck_Bass_F"] if bn[s % 5] == F2 else H["B02_Offbeat_F"], 0.8) for s in range(32) if bn[s % 5] is not None]))
patt7 = [(F2, 1, 0), (F2, 0, 0), (F2 + 12, 0, 1), (None, 0, 0), (F2 + 3, 1, 0), (F2, 0, 0), (F2 + 10, 0, 1)]
add("Lp05_Acid_7step_F_130", acid_line([patt7[s % 7] for s in range(32)], S16, res=0.88))
ch11 = [1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0]
add("Lp06_Chord_11step_Fm_130", ploop([(s * S16, H["S01_Dub_Fm7_Dry"], 0.6) for s in range(32) if ch11[s % 11]]) )
patt16 = [(F2, 1, 0), (F2, 0, 0), (F3, 0, 1), (F2, 0, 0), (None, 0, 0), (F2 + 3, 1, 0), (F2, 0, 0), (F2 + 10, 0, 1),
          (F2, 0, 0), (None, 0, 0), (F2, 1, 0), (F2 + 5, 0, 1), (F2, 0, 0), (F2 + 3, 0, 0), (F3, 1, 1), (F2, 0, 0)]
add("Lp07_Acid_Screaming_F_130", acid_line(patt16 * 2, S16, base=150, mod=4500, res=0.93, drive=3.5))
fg = ploop([(b * BEAT, kk, 1.0) for b in range(8)]) + 0.8 * lib["Lp01_Perc_Swedish_130"]
add("Lp08_Full_Groove_130", fg)

# ---------------------------------------------------------------- SPARA
os.makedirs(OUT, exist_ok=True)
for f in os.listdir(OUT):
    if f.endswith(".flac"): os.remove(os.path.join(OUT, f))
tmp = tempfile.mkdtemp()
for name, x in lib.items():
    x = np.nan_to_num(np.asarray(x, dtype=np.float64))
    x = filt(x, "high", 18)                      # bort med DC
    x = fadein(fade(x, 5), 0.3)
    y = resample_poly(x, 1, FS // OUT_FS)        # 96 k -> 48 k med anti-alias-filter
    p = np.max(np.abs(y)); y = y / p * 10 ** (-0.5 / 20) if p > 0 else y
    w = os.path.join(tmp, name + ".wav")
    wavfile.write(w, OUT_FS, y.astype(np.float32))
    subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-i", w, "-c:a", "flac",
                    "-sample_fmt", "s32", "-bits_per_raw_sample", "24", "-compression_level", "8",
                    os.path.join(OUT, name + ".flac")], check=True)
print(len(lib), "ljud sparade i", OUT)
