"""BETONG ljudbibliotek. Syntetiserar ~150 techno-ljud från grunden.

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
def saw_ph(f, sec, phase):
    """BLEP-sågtand med fasförskjutning per sampel (array eller tal)."""
    f = np.full(N(sec), f) if np.isscalar(f) else f
    ph = (np.cumsum(f) / FS + phase) % 1.0
    dt = f / FS; y = 2 * ph - 1
    m = ph < dt; tt = ph[m] / dt[m]; y[m] -= tt + tt - tt * tt - 1
    m = ph > 1 - dt; tt = (ph[m] - 1) / dt[m]; y[m] -= tt * tt + tt + tt + 1
    return y
def square(f, sec, pw=0.5):
    """Band-begränsad puls: skillnaden mellan två BLEP-sågtänder. Ingen aliasing, inget klipp."""
    f = np.full(N(sec), f) if np.isscalar(f) else f
    pw = np.full(len(f), pw) if np.isscalar(pw) else np.asarray(pw)
    return 0.5 * (saw_ph(f, sec, 0.0) - saw_ph(f, sec, pw)) + (pw - 0.5) * 0.0
def tanhd(x, d): return np.tanh(x * d) / np.tanh(d)
def softclip(x, knee=0.72):
    """Klipper mjukt ovanför knät: slagen behåller sin attack i stället för att bli fyrkant."""
    a = np.abs(x); over = a > knee
    y = x.copy()
    y[over] = np.sign(x[over]) * (knee + (1 - knee) * np.tanh((a[over] - knee) / (1 - knee)))
    return y
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
def peak_cut(x, f, gain_db, width=0.7):
    """Enkel peak-EQ: lägger till eller drar bort ett band runt f."""
    lo, hi = f * (1 - width / 2), f * (1 + width / 2)
    return x + (10 ** (gain_db / 20) - 1) * filt(x, "band", [lo, hi])
def kick(f0=230, f1=48, pdec=0.035, dec=0.35, length=0.6, click=0.6, drive=1.5, sub=0.0, hold=None):
    """Trummaskins-kick: tvåstegs tonfall (klick + punch), hold-fas, ren beater-klick och mild mättnad."""
    tt = t(length)
    hold = 0.018 + 0.5 * pdec if hold is None else hold
    # tonfall: ett mycket snabbt steg (klicket i själva vågen) + punch-steget ner till grundtonen
    freq = f1 + (f0 - f1) * np.exp(-tt / pdec) + 6.0 * f1 * np.exp(-tt / 0.0016)
    ph = 2 * np.pi * np.cumsum(freq) / FS
    amp = np.where(tt < hold, 1.0, np.exp(-(tt - hold) / dec))
    amp *= np.minimum(tt / 0.0004, 1.0)
    body = np.sin(ph) * amp
    if sub: body += sub * np.sin(2 * np.pi * f1 * tt) * np.minimum(tt / 0.01, 1) * np.where(tt < hold, 1.0, np.exp(-(tt - hold) / (dec * 1.4)))
    # beater-klick: kort brus i övre mellanregistret, inget gnistrande över 9 kHz
    c = filt(filt(noise(length), "band", [1500, 7000], 2), "low", 9000) * np.exp(-tt / 0.0025) * click * 2.0
    c += 0.6 * click * np.sin(2 * np.pi * 1800 * tt) * np.exp(-tt / 0.0018)   # beater-knack
    x = tanhd(body + c, drive)
    x = peak_cut(x, 320, -4.0)                      # mindre kartong
    x = filt(x, "high", 24)                         # bort med ultrabas under 24 Hz
    return x

kk = kick()
add("K01_909_Punch", kk)
add("K02_Deep_Long", kick(180, 43.65, 0.05, 0.85, 1.3, 0.3, 1.2))
add("K03_Distorted", filt(kick(260, 50, 0.03, 0.4, 0.6, 0.6, 2.2), "low", 9000))
add("K04_Minimal_Tight", kick(300, 60, 0.02, 0.11, 0.3, 1.0, 1.2))
add("K05_Lofi_12bit", crush(kick(240, 50, 0.035, 0.45), 12, 4))
ind = kick(250, 48, 0.03, 0.42, 0.6, 0.8, 2.4)
ind = ind - 0.5 * filt(filt(ind, "band", [230, 320]), "band", [230, 320])
add("K06_Industrial_Crushed", crush(ind, 10, 3))
hc = kick(280, 52, 0.025, 0.38, 0.55, 0.9, 1.6); add("K07_Hard_Clipped", softclip(hc * 1.3))
dry = kick(230, 48, 0.035, 0.3, 1.4, 0.5, 1.6)
wet = fftconvolve(dry, reverb_ir(2.0, 0.7))[: len(dry)]
wet = filt(tanhd(filt(wet, "low", 150) * 6, 2), "low", 110)
duck = np.minimum(t(1.4) / 0.22, 1) ** 2
add("K08_Rumble_Combo", dry + 1.8 * wet / (np.max(np.abs(wet)) + 1e-9) * duck * 0.5)
add("K09_Sub_Boom_F", kick(120, 43.65, 0.08, 1.2, 1.8, 0.15, 1.1, sub=0.6))
add("K10_Click_Thump", kick(400, 70, 0.012, 0.09, 0.25, 1.4, 2))

# ---------------------------------------------------------------- HATS
def metal(length, dec, hp=7000, fr=(205.3, 304.4, 369.6, 522.7, 540.0, 800.0)):
    """808/909-cymbalkärna: sex pulsoscillatorer, bandpass + branta högpass, en liten brusdel."""
    m = sum(square(f, length) for f in fr) / 3.0
    m = filt(m, "band", [hp * 0.95, 12500], 2)
    m = filt(m, "high", hp, 4)
    n = filt(filt(noise(length), "high", hp, 4), "low", 12000)
    x = (m + 0.3 * n) * env(length, dec, 0.0003)
    return filt(x, "low", 13000, 2)
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
    """909-clap: fyra snabba smällar och en svans, bandpass runt 1,2 kHz med luft upptill."""
    L = 0.6; x = np.zeros(N(L))
    n = filt(noise(L), "band", [700, 2400], 2) + 0.35 * filt(noise(L), "band", [2400, 7000], 2)
    for i, off in enumerate([0, 0.0095, 0.0195, 0.029]):
        s = N(off); last = i == 3
        x[s:] += n[s:] * env((len(x) - s) / FS, dec if last else 0.0055, 0.0002) * (1.0 if last else 0.85)
    x = filt(x, "high", 350)
    return tanhd(x / (np.max(np.abs(x)) + 1e-9), 1.3)
cl = clap()
add("C01_Clap_909ish", cl)
g = fftconvolve(cl, reverb_ir(0.6, 0.3, 7000))[: len(cl)]
gate = (t(0.6) < 0.18).astype(float); gate = filt(gate, "low", 60, 1)
add("C02_Clap_Gated", cl + 1.5 * g / (np.max(np.abs(g)) + 1e-9) * gate)
tt = t(0.4)
def snare(t1=180, t2=330, tone_dec=0.06, noise_dec=0.14, L=0.45, snap=1.0):
    """909-snare: två tonande moder med tonfall + filtrerat brus (snärjan)."""
    tt = t(L); bend = 1 + 0.35 * np.exp(-tt / 0.008)
    tone = (np.sin(2 * np.pi * np.cumsum(t1 * bend) / FS) + 0.6 * np.sin(2 * np.pi * np.cumsum(t2 * bend) / FS)) * env(L, tone_dec, 0.0003)
    nz = filt(filt(noise(L), "high", 1200, 2), "low", 11000) * env(L, noise_dec, 0.0004)
    x = 0.55 * tone + 0.7 * snap * nz
    return tanhd(x / (np.max(np.abs(x)) + 1e-9), 1.5)
add("C03_Snare_Tight", snare())
add("C04_Snare_Noise_Industrial", tanhd(filt(noise(0.35), "band", [600, 7000]) * env(0.35, 0.09, 0.004) * 1.5, 3))
tt = t(0.09)
def rim(L=0.09):
    """909-rim: kort ringande bandpass på 1,7 kHz och 500 Hz plus klick."""
    tt = t(L)
    x = np.sin(2 * np.pi * 1720 * tt) * env(L, 0.011, 0.0002) + 0.7 * np.sin(2 * np.pi * 505 * tt) * env(L, 0.007, 0.0002)
    x += 0.35 * filt(noise(L), "band", [2500, 8000]) * env(L, 0.0015, 0.0001)
    return tanhd(x, 1.6)
add("C05_Rim", rim())
cv = fit(cl, int(1.6 * FS)); add("C06_Clap_Layer_Verb", cv + 0.6 * fftconvolve(cv, reverb_ir(1.6, 0.5))[: len(cv)] / 0.02 * 0.02)

# ---------------------------------------------------------------- PERC
def tom(f0, f1, dec=0.3):
    L = dec * 3; tt = t(L); fr = f1 + (f0 - f1) * np.exp(-tt / 0.04)
    ph = 2 * np.pi * np.cumsum(fr) / FS
    x = (np.sin(ph) + 0.15 * np.sin(2 * ph)) * env(L, dec, 0.0004)
    x += 0.18 * filt(noise(L), "band", [f1 * 3, f1 * 14]) * env(L, 0.012, 0.0003)
    return tanhd(x, 1.4)
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

# ---------------------------------------------------------------- RÅ & HÅRD: ÅNGVÄLTS-TECHNO
# Tyngre, smutsigare ljud för hård 90-tals-techno runt 135-140 BPM.
def tape(x, d=2.0, bias=0.15):
    """Asymmetrisk mättnad, som ett band som körs för hårt."""
    d = 1.0 + (d - 1.0) * 0.6  # nivå-genomgången: mindre platt
    return (np.tanh((x + bias) * d) - np.tanh(bias * d)) / np.tanh(d)
def hardclip(x, gain): return softclip(x * (1.0 + (gain - 1.0) * 0.45))
def norm(x): return x / (np.max(np.abs(x)) + 1e-9)
def room(x, sec=1.2, decay=0.4, lp=5000, amt=0.5):
    y = fit(x, len(x) + int(sec * FS))
    w = fftconvolve(y, reverb_ir(sec, decay, lp))[: len(y)]
    return y + amt * norm(w) * np.max(np.abs(x))
def heavy_kick(f0=320, f1=46, pdec=0.028, dec=0.42, length=0.7, drive=6, clip=1.6, click=1.0):
    tt = t(length)
    fr = f1 + (f0 - f1) * np.exp(-tt / pdec)
    body = np.sin(2 * np.pi * np.cumsum(fr) / FS)
    amp = np.exp(-tt / dec) * (1 - 0.25 * np.exp(-tt / 0.01))
    sub = 0.7 * np.sin(2 * np.pi * f1 * tt) * np.exp(-tt / (dec * 1.3)) * np.minimum(tt / 0.012, 1)
    c = filt(noise(length), "band", [1800, 9000]) * np.exp(-tt / 0.004) * click
    c += 0.6 * np.sin(2 * np.pi * 1200 * tt) * np.exp(-tt / 0.0025) * click
    x = tape(body * amp + sub + c, drive * 0.35)
    x = hardclip(x, clip)
    return filt(x, "low", 12000)
def rumble_from(dry, beat, drive=10, lp=120, length=None):
    """Kick genom reverb, dist och lowpass, duckad mot nästa slag: rumble."""
    L = length or len(dry) / FS
    d = fit(dry, N(L))
    w = fftconvolve(d, reverb_ir(2.4, 0.9, 2500))[: len(d)]
    w = filt(tanhd(filt(w, "low", 220) * drive, 3), "low", lp, 4)
    w = norm(w)
    tt = t(L); ph = (tt % beat) / beat
    duck = np.clip((ph - 0.12) / 0.35, 0, 1) ** 1.5 * np.clip((1 - ph) / 0.07, 0, 1)
    return w * duck

B138 = 60 / 138; S138 = B138 / 4; BAR138 = B138 * 4

ak = heavy_kick()
add("K11_Angvalt", ak)
kt = heavy_kick(280, 45, 0.03, 0.55, 1.6, 5, 1.4)
add("K12_Rumble_Tunnel", kt * 0.85 + 0.9 * rumble_from(kt, 1.6, 12, 110, 1.6) * (t(1.6) > 0.09))
add("K13_Stampen", heavy_kick(380, 58, 0.018, 0.16, 0.35, 5, 1.6, 1.4))
rk = heavy_kick(300, 43.65, 0.03, 0.45, 0.8, 7, 1.5)
add("K14_Rost_Kick_F", rk * 0.6 + 0.4 * crush(rk, 8, 6))
hg = kick(240, 50, 0.03, 0.32, 0.55, 1.2, 1.4)
add("K15_Hardgroove_909", hardclip(tape(hg * 1.15, 2.2, 0.2), 1.15))
mh2 = fm(140, 397, 10, 0.12, 0.7, 0.03)
add("K16_Anghammare", hardclip(heavy_kick(300, 48, 0.025, 0.4, 0.7, 6, 1.3) + 0.35 * fit(mh2, N(0.7)), 1.2))
add("K17_Bunker_Long_F", heavy_kick(220, 43.65, 0.045, 0.9, 1.6, 4, 1.25, 0.6))
add("K18_Tunga_Vals", hardclip(heavy_kick(260, 50, 0.04, 0.35, 0.6, 9, 2.0, 0.8) * 1.0, 1.0))

# rumble-basar: en takt-slinga och ett ensamt andetag
rb = place(BAR138 * 2, [(b * B138, heavy_kick(280, 45, 0.03, 0.35, 0.6), 1) for b in range(8)])
add("B11_Rumble_Angvalt_138", rumble_from(rb, B138, 14, 120))
one = fit(heavy_kick(260, 43.65, 0.035, 0.5, 0.9), N(B138))
add("B12_Rumble_Breath_F", rumble_from(one, B138, 16, 140, B138))
L = 0.6; gr = sum(saw(mtof(F1) * (1 + d), L) for d in (-0.01, 0, 0.011))
gr = svf_lp(gr, 180 + 700 * np.exp(-t(L) / 0.09), 0.55)
add("B13_Growl_Bass_F", hardclip(tape(gr, 4), 1.4) * adsr(L, 0.004, 0.15, 0.7, 0.08, 0.42))
L = 0.45; ds = saw(mtof(F2), L) + 0.7 * square(mtof(F2) * 0.5, L, 0.4)
ds = svf_lp(ds, 250 + 2500 * np.exp(-t(L) / 0.05), 0.7)
add("B14_Dist_Saw_Bass_F", hardclip(tanhd(ds, 6), 1.2) * env(L, 0.2, 0.002))
L = 0.6; tt = t(L)
fmb = np.sin(2 * np.pi * mtof(F2) * tt + (4 + 3 * np.exp(-tt / 0.08)) * np.sin(2 * np.pi * mtof(F2) * 1.5 * tt))
add("B15_Grind_FM_Bass_F", filt(hardclip(tape(fmb, 3), 1.5), "low", 3500) * adsr(L, 0.003, 0.1, 0.75, 0.06, 0.4))
L = 1.0; fdip = mtof(F2) * (1 - 0.4 * np.exp(-t(L) / 0.04))
hb = sum(square(fdip * m, L, 0.5 + 0.35 * np.sin(2 * np.pi * 1.1 * t(L))) for m in (0.5, 0.993, 1.0, 1.008)) / 2.5
add("B16_Hoover_Bass_F", hardclip(tanhd(filt(hb, "low", 3000), 2.5), 1.2) * adsr(L, 0.01, 0.2, 0.8, 0.15, 0.75))

# hats och cymbaler med grus
hh = metal(0.18, 0.035, 6000) + 0.5 * filt(noise(0.18), "high", 7000) * env(0.18, 0.025)
add("H09_Hard_Closed_Dist", hardclip(tape(norm(hh), 3), 1.3))
add("H10_Open_Trash", filt(hardclip(tanhd(norm(metal(0.9, 0.25, 5000) + 0.6 * filt(noise(0.9), "high", 5000) * env(0.9, 0.22)), 4), 1.4), "high", 2500))
rd = metal(2.0, 0.8, 4000) + 0.35 * np.sin(2 * np.pi * 3311 * t(2.0)) * env(2.0, 0.7) + 0.2 * np.sin(2 * np.pi * 5120 * t(2.0)) * env(2.0, 0.5)
add("H11_Ride_Crunch", tape(norm(rd), 2.5))
h9 = filt(metal(0.2, 0.05, 7500), "high", 6000) + 0.3 * filt(noise(0.2), "band", [8000, 16000]) * env(0.2, 0.03)
add("H12_Hat_Tape_909", tape(norm(h9), 2.2, 0.25))
add("H13_Hat_Shuffle_Fat", hardclip(tape(norm(metal(0.12, 0.02, 5500) + 0.8 * filt(noise(0.12), "band", [4000, 12000]) * env(0.12, 0.018)), 2.5), 1.2))

# clap och snare som smäller
cl2 = clap(0.14); sn = filt(noise(0.6), "band", [400, 6000]) * env(0.6, 0.11) + 0.6 * np.sin(2 * np.pi * 200 * t(0.6)) * env(0.6, 0.04)
add("C07_Clap_Smash", hardclip(tape(norm(cl2 + 0.7 * sn), 3), 1.5))
cw = room(fit(clap(0.2), N(0.5)), 1.4, 0.45, 4000, 0.9)
add("C08_Clap_Warehouse", tape(norm(cw), 2.5))
L = 0.45; rsn = np.sin(2 * np.pi * (180 + 80 * np.exp(-t(L) / 0.01)) * t(L)) * env(L, 0.05) + filt(noise(L), "band", [700, 8000]) * env(L, 0.13)
add("C09_Snare_Rost", 0.65 * hardclip(tanhd(rsn, 4), 1.3) + 0.35 * crush(hardclip(tanhd(rsn, 4), 1.3), 8, 3))
add("C10_Clap_Stack_Hard", hardclip(norm(clap(0.1) + fit(clap(0.18)[N(0.004):], N(0.6)) + 0.5 * filt(noise(0.6), "band", [2500, 9000]) * env(0.6, 0.06)) * 1.8, 1.0))

# perc från verkstadsgolvet
add("P17_Metal_Pipe", hardclip(tape(fm(311, 311 * 2.76, 9, 0.18, 0.8, 0.02) + 0.5 * fm(622, 622 * 3.1, 4, 0.1, 0.8), 2), 1.3))
an = sum(np.sin(2 * np.pi * f * t(1.2)) * env(1.2, d) for f, d in ((870, 0.5), (1943, 0.3), (2711, 0.25), (3517, 0.15)))
add("P18_Anvil", tape(norm(an + 0.5 * filt(noise(1.2), "high", 3000) * env(1.2, 0.006)), 2))
add("P19_Tom_Dist_Low", hardclip(tape(norm(tom(170, 65, 0.25)), 4), 1.4))
cnk = fm(230, 230 * 4.2, 14, 0.06, 0.4, 0.01) + filt(noise(0.4), "band", [1500, 5000]) * env(0.4, 0.012)
add("P20_Hammer_Clank", hardclip(tanhd(norm(cnk), 3), 1.3))
add("P21_Tom_Dist_Mid", hardclip(tape(norm(tom(260, 120, 0.18)), 4), 1.4))
add("P22_Rim_Crack", hardclip(norm((np.sin(2 * np.pi * 1900 * t(0.1)) + 0.8 * filt(noise(0.1), "band", [2000, 8000])) * env(0.1, 0.009)) * 1.8, 1.0))

# stabs och leads
st2 = chord_saw(Fm7, 0.5, 0.2)
add("S11_Rave_Stab_Dist_Fm", hardclip(tanhd(svf_lp(st2, 500 + 7000 * np.exp(-t(0.5) / 0.04), 0.65), 4), 1.2) * env(0.5, 0.14))
ist = crush(hardclip(tanhd(svf_lp(chord_saw((41, 48, 53, 56), 0.5, 0.3), 300 + 4000 * np.exp(-t(0.5) / 0.03), 0.6), 5), 1.2), 6, 4)
add("S12_Industrial_Stab_Fm", filt(ist, "band", [120, 6000]) * env(0.5, 0.12))
L = 0.7; f = (1 - 0.3 * np.exp(-t(L) / 0.04))
hs = sum(square(mtof(n) * f * m, L, 0.35) for n in (53, 56, 60) for m in (0.997, 1.004)) / 3
add("S13_Hoover_Stab_Fm", hardclip(tanhd(filt(hs, "low", 6000), 2.5), 1.2) * adsr(L, 0.005, 0.15, 0.6, 0.12, 0.35))
add("S14_Dub_Stab_Dirty_Fm", dub_delay(tape(filt(chord_saw(Fm7, 0.5, 0.15), "low", 1200) * env(0.5, 0.12), 3), S138 * 3, 0.65, 350, 2000, 3.0))
add("L09_Acid_Dist_F", hardclip(tanhd(acid_line([(F2 + 12, 1, 0), (None, 0, 0)], B138 * 0.75, base=200, mod=5000, res=0.93, drive=5), 3), 1.3))
L = 1.0; scr = sum(saw(mtof(77) * (1 + 0.04 * np.sin(2 * np.pi * 7 * t(L))) * m, L) for m in (1, 1.5))
add("L10_Screech_F", hardclip(tanhd(svf_lp(scr, 3000 + 2000 * np.sin(2 * np.pi * 3 * t(L)), 0.85), 4), 1.2) * adsr(L, 0.005, 0.2, 0.8, 0.2, 0.7))
L = 1.2; al = square(np.where((t(L) * 4) % 1 < 0.5, 988.0, 740.0), L, 0.5)
add("L11_Alarm", hardclip(tanhd(filt(al, "low", 5000), 3), 1.2) * adsr(L, 0.005, 0.1, 0.9, 0.1, 1.0))

# texturer och fx
L = 4 * B138 * 2; pist = place(L, [(i * S138, fm(70, 211, 6, 0.06, 0.25), 0.8 if i % 4 == 0 else 0.45) for i in range(32)])
pist += place(L, [(i * S138 + S138 * 0.5, filt(noise(0.2), "high", 2500) * env(0.2, 0.03), 0.4) for i in range(0, 32, 2)])
add("T11_Steamroller_138", hardclip(tape(norm(pist), 2.5), 1.2))
L = 2.5; imp = heavy_kick(200, 40, 0.05, 1.2, L, 4, 1.2, 1.4) + 0.7 * fit(metal(2.5, 0.8, 2500), N(L)) + 0.5 * filt(noise(L), "low", 3000) * env(L, 0.5)
add("X12_Impact_Steel", tape(norm(imp), 2))
L = BAR138 * 2; ris = filt(noise(L), "band", [400, 6000]) * (t(L) / L) ** 2
ris *= 0.6 + 0.4 * (np.sin(2 * np.pi * (2 + 14 * (t(L) / L) ** 2) * t(L)) > 0)
add("X13_Riser_Grind_138", hardclip(tanhd(ris * 2, 3), 1.0))

# loopar 138 BPM
def ploop138(hits): return place(BAR138 * 2, hits)
hg_hits = []
for s in range(32):
    ts = s * S138
    if s % 4 == 2: hg_hits.append((ts, lib["H10_Open_Trash"], 0.5))
    hg_hits.append((ts + (0.006 if s % 2 else 0), lib["H13_Hat_Shuffle_Fat"], 0.45 if s % 2 else 0.25))
    if s in (4, 12, 20, 28): hg_hits.append((ts, lib["C07_Clap_Smash"], 0.65))
    if s in (3, 7, 10, 15, 19, 23, 26, 31): hg_hits.append((ts, lib["P19_Tom_Dist_Low"] if s % 2 else lib["P21_Tom_Dist_Mid"], 0.45))
    if s in (6, 14, 22, 27): hg_hits.append((ts, lib["P22_Rim_Crack"], 0.5))
add("Lp09_Hardgroove_Perc_138", ploop138(hg_hits))
top = []
for s in range(32):
    ts = s * S138
    top.append((ts, lib["H09_Hard_Closed_Dist"], 0.5 if s % 4 == 2 else 0.28))
    if s % 8 in (3, 6): top.append((ts, lib["P17_Metal_Pipe"], 0.35))
    if s in (12, 28): top.append((ts, lib["C09_Snare_Rost"], 0.5))
    if s % 16 == 11: top.append((ts, lib["P20_Hammer_Clank"], 0.5))
add("Lp10_Angvalt_Top_138", ploop138(top))
full = ploop138([(b * B138, ak, 1.0) for b in range(8)]) + 0.9 * lib["B11_Rumble_Angvalt_138"][: N(BAR138 * 2)] * 0.6 + 0.7 * lib["Lp09_Hardgroove_Perc_138"]
add("Lp11_Angvalt_Groove_138", hardclip(full / (np.max(np.abs(full)) + 1e-9) * 1.4, 1.0))

# ---------------------------------------------------------------- KÄRNAN: RENA, FETA HUVUDTRUMMOR
# Grunden i alla kit. Rena slag, fullt tryck, ingen konstig dist. Det skeva skruvas fram i pluginet.
add("K22_Grund_Techno_F", kick(260, 43.65, 0.03, 0.34, 0.75, 0.55, 1.7, 0.25, hold=0.03))
add("K23_Peak_Punch", kick(300, 50, 0.026, 0.26, 0.55, 0.8, 2.2, 0.2, hold=0.025))
add("K24_Berlin_Deep_F", kick(200, 43.65, 0.045, 0.6, 1.3, 0.35, 1.4, 0.45, hold=0.04))
add("K25_Hardgroove_Tight", kick(340, 55, 0.02, 0.17, 0.4, 0.9, 2.4, 0.0, hold=0.015))
add("K26_Rund_Minimal", kick(170, 52, 0.035, 0.2, 0.45, 0.25, 1.2, 0.15, hold=0.02))
add("K27_909_Classic", kick(250, 49, 0.032, 0.3, 0.6, 0.7, 1.8, 0.0, hold=0.02))
add("K28_Lang_Svans_F", kick(230, 43.65, 0.04, 0.9, 1.8, 0.4, 1.5, 0.5, hold=0.05))

add("H17_Closed_Clean", metal(0.14, 0.032))
add("H18_Closed_Tight", metal(0.08, 0.014, 8000))
add("H19_Open_Clean", metal(0.7, 0.19))
rc = metal(1.8, 0.75, 5200, (271.3, 402.0, 488.4, 690.7, 713.6, 1057.0))
bell = (np.sin(2 * np.pi * 3420 * t(1.8)) + 0.6 * np.sin(2 * np.pi * 5130 * t(1.8))) * env(1.8, 0.55)
add("H20_Ride_Clean", rc + 0.12 * bell)
add("H21_Hat_Pedal", metal(0.12, 0.024, 6500) * np.minimum(t(0.12) / 0.004, 1))
add("H22_Shaker_Clean", filt(filt(noise(0.2), "high", 5500, 4), "low", 13000) * adsr(0.2, 0.018, 0.03, 0.25, 0.035, 0.06))

add("C15_Clap_Clean", clap(0.16))
cwide = fit(clap(0.2), N(0.9))
add("C16_Clap_Wide_Room", cwide + 0.5 * fftconvolve(cwide, reverb_ir(0.9, 0.22, 7000))[: len(cwide)] / 0.02 * 0.02)
add("C17_Snare_909_Clean", snare(185, 335, 0.055, 0.13))
add("C18_Snare_Fat", snare(165, 300, 0.08, 0.18, 0.55, 1.2))
add("C19_Rim_909", rim())

add("P43_Tom_909_Low", tom(150, 85, 0.24))
add("P44_Tom_909_Mid", tom(220, 125, 0.2))
add("P45_Tom_909_High", tom(320, 180, 0.16))
cb8 = filt(square(540, 0.5) + square(800, 0.5), "band", [450, 3000], 2)
add("P46_Cowbell_808", cb8 * np.where(t(0.5) < 0.012, 1.0, np.exp(-(t(0.5) - 0.012) / 0.07)))
add("P47_Clave_Clean", filt(np.sin(2 * np.pi * 2500 * t(0.07)), "band", [2000, 3200]) * env(0.07, 0.01, 0.0002))

crash = metal(3.0, 1.1, 3500, (311, 457, 563, 743, 811, 1171)) + 0.4 * filt(filt(noise(3.0), "high", 4000, 4), "low", 14000) * env(3.0, 0.9)
add("X14_Crash_Clean", crash)

# basar för basgångar (ton per steg i sequencern)
L = 0.4; tt = t(L)
tb = np.sin(2 * np.pi * mtof(F2) * tt) + 0.35 * np.sin(4 * np.pi * mtof(F2) * tt) * np.exp(-tt / 0.06)
add("B21_Techno_Bas_F", tanhd(tb, 1.3) * adsr(L, 0.002, 0.08, 0.6, 0.05, 0.22))
L = 0.3
ofb = svf_lp(saw(mtof(F2), L) + 0.5 * saw(mtof(F2) * 1.005, L), 250 + 1400 * np.exp(-t(L) / 0.04), 0.35)
add("B22_Offbeat_Ren_F", tanhd(ofb, 1.2) * env(L, 0.11, 0.0015))

# ---------------------------------------------------------------- TECH HOUSE & TRIBAL (124-126 BPM)
B125 = 60 / 125; S125 = B125 / 4; BAR125 = B125 * 4

def drum(f0, f1, pdec, dec, L, modes=((1.0, 1.0),), noise_amt=0.0, nband=(1500, 5000), ndec=0.01):
    """Skinntrumma: tonande moder med pitch-fall + slagbrus."""
    tt = t(L); fr = f1 + (f0 - f1) * np.exp(-tt / pdec)
    ph = 2 * np.pi * np.cumsum(fr) / FS
    x = sum(a * np.sin(ph * m) * np.exp(-tt / (dec / (1 + 0.6 * (m - 1)))) for m, a in modes)
    if noise_amt: x = x + noise_amt * filt(noise(L), "band", list(nband)) * np.exp(-tt / ndec)
    return x

# bongos, congas, djembe
add("P23_Bongo_High", drum(520, 430, 0.012, 0.09, 0.35, ((1, 1), (1.6, 0.35)), 0.5, (2000, 6000), 0.006))
add("P24_Bongo_Low", drum(380, 300, 0.015, 0.12, 0.4, ((1, 1), (1.58, 0.3)), 0.45, (1500, 5000), 0.007))
add("P25_Conga_Slap", drum(420, 360, 0.01, 0.06, 0.3, ((1, 0.6), (1.7, 0.4)), 1.4, (1200, 6000), 0.012))
add("P26_Conga_Open", drum(360, 320, 0.02, 0.22, 0.6, ((1, 1), (1.59, 0.25), (2.14, 0.1)), 0.25, (1500, 4000), 0.006))
add("P27_Conga_Mute", drum(340, 300, 0.01, 0.04, 0.2, ((1, 1), (1.6, 0.2)), 0.4, (1500, 4000), 0.005))
add("P28_Djembe_Bass", drum(140, 78, 0.03, 0.18, 0.6, ((1, 1), (2.1, 0.15)), 0.3, (300, 1500), 0.01))
add("P29_Djembe_Tone", drum(360, 300, 0.012, 0.11, 0.4, ((1, 1), (1.5, 0.4), (2.3, 0.15)), 0.35, (1500, 5000), 0.006))
add("P30_Djembe_Slap", drum(700, 600, 0.006, 0.05, 0.3, ((1, 0.5), (1.9, 0.3)), 1.6, (1800, 8000), 0.018))
tt = t(0.9); tim = (np.sin(2 * np.pi * 620 * tt) + 0.6 * np.sin(2 * np.pi * 1490 * tt) + 0.3 * np.sin(2 * np.pi * 2380 * tt)) * env(0.9, 0.22)
add("P31_Timbale_High", tim + 0.8 * filt(noise(0.9), "band", [2500, 8000]) * env(0.9, 0.012))
tt = t(1.0); tim2 = (np.sin(2 * np.pi * 410 * tt) + 0.6 * np.sin(2 * np.pi * 1010 * tt) + 0.3 * np.sin(2 * np.pi * 1730 * tt)) * env(1.0, 0.26)
add("P32_Timbale_Low", tim2 + 0.7 * filt(noise(1.0), "band", [2000, 7000]) * env(1.0, 0.014))
jing = np.zeros(N(0.45))
for k in range(9):
    o = int(rng.uniform(0, 0.025) * FS); jing[o:] += metal(0.45 - o / FS, 0.09, 6500, tuple(f * rng.uniform(1.8, 2.6) for f in (205.3, 304.4, 369.6, 522.7, 540.0, 800.0)))[: len(jing) - o] * rng.uniform(0.4, 1)
add("P33_Tambourine", filt(jing, "high", 5000) + 0.3 * filt(noise(0.45), "band", [800, 2500]) * env(0.45, 0.01))
def logdrum(note, dec=0.32, L=0.7):
    f = mtof(note); tt = t(L); fr = f * (1 + 0.25 * np.exp(-tt / 0.012))
    ph = 2 * np.pi * np.cumsum(fr) / FS
    return (np.sin(ph) + 0.25 * np.sin(2 * ph) * np.exp(-tt / 0.05)) * env(L, dec, 0.002)
add("P34_Log_Drum_F", logdrum(F2 + 12))
add("P35_Log_Drum_Low_F", logdrum(F2, 0.45, 0.9))
add("P36_Cabasa", filt(noise(0.18), "high", 6000) * adsr(0.18, 0.012, 0.03, 0.2, 0.03, 0.05))
gu = np.zeros(N(0.3))
for k in range(14):
    o = N(0.008 + k * 0.016); g = filt(noise(0.012), "band", [2500, 7000]) * np.hanning(N(0.012)); gu[o:o + len(g)] += g[: len(gu) - o]
add("P37_Guiro", gu)
add("P38_Agogo_High", (np.sin(2 * np.pi * 980 * t(0.6)) + 0.4 * np.sin(2 * np.pi * 2645 * t(0.6))) * env(0.6, 0.12))
add("P39_Agogo_Low", (np.sin(2 * np.pi * 735 * t(0.7)) + 0.4 * np.sin(2 * np.pi * 1985 * t(0.7))) * env(0.7, 0.14))
add("P40_Shaker_Soft", filt(noise(0.2), "band", [4000, 11000]) * adsr(0.2, 0.025, 0.04, 0.25, 0.04, 0.07))
add("P41_Woodblock_Tribal", fm(820, 1640, 1.2, 0.035, 0.25))
L = 0.6; tt = t(L); tbend = 180 * (1 + 0.35 * np.sin(np.pi * np.minimum(tt / 0.35, 1)))
add("P42_Talking_Drum", np.sin(2 * np.pi * np.cumsum(tbend) / FS) * env(L, 0.22, 0.002) + 0.3 * filt(noise(L), "band", [800, 3000]) * env(L, 0.008))

# clap, snap, snare
add("C11_Clap_House", room(fit(clap(0.12), N(0.45)), 0.6, 0.18, 6000, 0.45))
add("C12_Snap", filt(noise(0.15), "band", [1800, 4500]) * env(0.15, 0.025, 0.001) + 0.4 * np.sin(2 * np.pi * 2100 * t(0.15)) * env(0.15, 0.004))
add("C13_Clap_Tight_Room", room(fit(clap(0.08), N(0.35)), 0.35, 0.09, 8000, 0.6))
tt = t(0.35); add("C14_Snare_Tech_House", 0.5 * np.sin(2 * np.pi * 210 * tt) * env(0.35, 0.03) + filt(noise(0.35), "band", [2000, 9000]) * env(0.35, 0.07))

# hats
add("H14_Hat_Closed_Crisp", filt(metal(0.1, 0.018, 8000), "high", 7500))
add("H15_Hat_Open_House", filt(metal(0.5, 0.11, 7000), "high", 6500))
add("H16_Ride_Tech", metal(1.2, 0.35, 6000, (311, 427, 563, 711, 879, 1103)) + 0.25 * np.sin(2 * np.pi * 4120 * t(1.2)) * env(1.2, 0.3))

# kickar
add("K19_Tech_House", kick(170, 50, 0.04, 0.26, 0.45, 0.5, 1.3, 0.3))
add("K20_Round_Deep_F", kick(150, 43.65, 0.05, 0.38, 0.7, 0.3, 1.15, 0.5))
add("K21_Tribal_Tom_Kick", kick(200, 62, 0.03, 0.22, 0.5, 0.4, 1.4) + 0.4 * fit(tom(200, 100, 0.15), N(0.5)))

# bas (F)
L = 0.45; tt = t(L)
org = sum(a * np.sin(2 * np.pi * mtof(F2) * h * tt) for h, a in ((1, 1), (2, 0.55), (3, 0.3), (4, 0.15)))
add("B17_Organ_Bass_F", (org + 0.4 * filt(noise(L), "band", [1500, 5000]) * env(L, 0.003)) * adsr(L, 0.003, 0.08, 0.7, 0.05, 0.3))
add("B18_Tech_Pluck_F", svf_lp(saw(mtof(F2), 0.35), 180 + 1800 * np.exp(-t(0.35) / 0.035), 0.55) * env(0.35, 0.14, 0.002))
L = 0.4; tt = t(L); rf = mtof(F2) * (1 + 0.9 * np.exp(-tt / 0.008))
add("B19_Rubber_Bass_F", tanhd(np.sin(2 * np.pi * np.cumsum(rf) / FS), 1.6) * env(L, 0.18, 0.002))
add("B20_Sub_Sine_F", np.sin(2 * np.pi * mtof(F2) * t(0.6)) * adsr(0.6, 0.004, 0.1, 0.85, 0.06, 0.45))

# stabs och toner
L = 0.4; tt = t(L)
house = sum(sum(a * np.sin(2 * np.pi * mtof(n) * h * tt) for h, a in ((1, 1), (2, 0.7), (3, 0.5), (4, 0.35), (6, 0.2), (8, 0.12))) for n in (53, 56, 60, 63))
add("S15_Organ_Stab_House_Fm", (house / 5 + 0.3 * filt(noise(L), "band", [2000, 6000]) * env(L, 0.003)) * adsr(L, 0.002, 0.06, 0.6, 0.06, 0.22))
add("S16_Chord_Stab_Tech_Fm9", svf_lp(chord_saw((53, 56, 60, 63, 67), 0.4, 0.12), 600 + 3500 * np.exp(-t(0.4) / 0.03), 0.4) * env(0.4, 0.11))
L = 0.6; tt = t(L)
add("S17_Marimba_F", (np.sin(2 * np.pi * mtof(65) * tt) + 0.35 * np.sin(2 * np.pi * mtof(65) * 3.93 * tt) * np.exp(-tt / 0.03)) * env(L, 0.18, 0.001))

# röster
L = 0.35; tt = t(L)
hey = voice("e", L, mtof(58)) * env(L, 0.12, 0.004)
add("V07_Hey_Shout", hey + 0.3 * filt(noise(L), "band", [2000, 5000]) * env(L, 0.05))
add("V08_Chant_Oh_F", voice("o", 0.9, mtof(53)) * adsr(0.9, 0.04, 0.2, 0.75, 0.2, 0.65))

# textur
L = 5.0; ins = filt(noise(L), "band", [4500, 9000]) * (0.5 + 0.5 * (np.sin(2 * np.pi * 23 * t(L)) > 0.6))
ins += 0.4 * filt(noise(L), "band", [1200, 2400]) * (0.5 + 0.5 * np.sin(2 * np.pi * 0.4 * t(L)))
add("T12_Jungle_Night", ins * adsr(L, 0.6, 0.2, 1, 0.6, 4.3))

# loopar 125 BPM
def ploop125(hits): return place(BAR125 * 2, hits)
sw = lambda s: s * S125 + (S125 * 0.18 if s % 2 else 0)  # tech house-shuffle
th = []
for s in range(32):
    if s % 4 == 2: th.append((sw(s), lib["H15_Hat_Open_House"], 0.5))
    th.append((sw(s), lib["P40_Shaker_Soft"], 0.35 if s % 2 else 0.18))
    if s in (4, 12, 20, 28): th.append((sw(s), lib["C11_Clap_House"], 0.6))
    if s in (7, 15, 23, 26, 31): th.append((sw(s), lib["P25_Conga_Slap"], 0.35))
add("Lp12_Tech_House_Top_125", ploop125(th))
tr = []
for s in range(32):
    if s % 3 == 0: tr.append((sw(s), lib["P26_Conga_Open"], 0.5))
    if s % 5 == 2: tr.append((sw(s), lib["P24_Bongo_Low"], 0.45))
    if s % 7 == 4: tr.append((sw(s), lib["P29_Djembe_Tone"], 0.5))
    if s % 4 == 0: tr.append((sw(s), lib["P28_Djembe_Bass"], 0.45))
    tr.append((sw(s), lib["P36_Cabasa"], 0.22 if s % 2 else 0.1))
add("Lp13_Tribal_Perc_125", ploop125(tr))
af = []
for s in range(32):
    if s % 6 in (0, 3): af.append((sw(s), lib["P34_Log_Drum_F"], 0.5))
    if s % 8 == 6: af.append((sw(s), lib["P35_Log_Drum_Low_F"], 0.5))
    if s % 4 == 2: af.append((sw(s), lib["P33_Tambourine"], 0.35))
    if s % 3 == 1: af.append((sw(s), lib["P23_Bongo_High"], 0.3))
add("Lp14_Afro_3mot4_125", ploop125(af))
bl = [(sw(s), lib["P23_Bongo_High"] if s % 3 else lib["P24_Bongo_Low"], 0.5) for s in range(32) if s % 8 not in (1, 5)]
add("Lp15_Bongo_Loop_125", ploop125(bl))

# ---------------------------------------------------------------- SMEDJAN (för SMIDET)
# Städ, hammare, plåt, kedjor, härdning och klockor. Städ och klockor är stämda i F-moll.
def bar_modes(f, L, dec, ratios=(1.0, 2.756, 5.404, 8.933), amps=(1.0, 0.55, 0.32, 0.18), strike=0.6):
    """Fritt svängande stålbalk: oharmoniska moder, de höga dör fortare. Ett hårt anslag i början."""
    tt = t(L)
    x = sum(a * np.sin(2 * np.pi * f * r * tt) * np.exp(-tt / (dec / (1 + 0.8 * k))) for k, (r, a) in enumerate(zip(ratios, amps)))
    x += strike * filt(noise(L), "band", [2500, 9000]) * env(L, 0.0018, 0.0001)
    return x * np.minimum(tt / 0.0003, 1)
def anvil(note, dec=0.6, L=1.6):
    f = mtof(note)
    x = bar_modes(f, L, dec, (1.0, 1.506, 2.343, 3.17, 4.41), (1.0, 0.7, 0.45, 0.3, 0.2), 0.8)
    return tanhd(x / (np.max(np.abs(x)) + 1e-9), 1.3)

add("F01_Stad_F", anvil(77))
add("F02_Stad_Ab", anvil(80, 0.5))
add("F03_Stad_C", anvil(84, 0.45))
add("F04_Stad_Lag_F", anvil(65, 0.8, 2.0))
L = 0.6; tt = t(L)
hs = 0.8 * np.sin(2 * np.pi * (90 + 160 * np.exp(-tt / 0.01)) * tt) * env(L, 0.05) + 0.6 * bar_modes(1180, L, 0.12, strike=1.2)
add("F05_Hammare_Stal", tanhd(hs, 1.8))
sl = kick(180, 55, 0.02, 0.12, 0.8, 1.0, 2.0) + 0.5 * fit(bar_modes(520, 0.8, 0.25, strike=1.0), N(0.8))
add("F06_Slagga_Tung", tanhd(sl, 1.6))
L = 2.2; pl = filt(noise(L), "band", [300, 6000]) * env(L, 0.004)
res = sum(filt(pl, "band", [f * 0.985, f * 1.015]) * 70 for f in (233, 377, 611, 987, 1597, 2584))
add("F07_Platslag", res * env(L, 0.7, 0.0005) + 0.08 * pl)
ch = np.zeros(N(0.6))
for k in range(26):
    o = N(rng.uniform(0, 0.45) ** 1.3)
    c = bar_modes(rng.uniform(2500, 6000), 0.05, 0.012, strike=0.4) * rng.uniform(0.3, 1.0)
    ch[o:o + len(c)] += c[: len(ch) - o]
add("F08_Kedja", ch)
L = 1.2; tt = t(L); sp = np.sin(2 * np.pi * np.cumsum(160 + 900 * np.exp(-tt / 0.05) + 40 * np.sin(2 * np.pi * 9 * tt)) / FS) * env(L, 0.35)
spr = fftconvolve(sp, filt(noise(0.8), "band", [400, 4000]) * env(0.8, 0.2) * 0.03)[: N(L)]
add("F09_Fjader", sp + 0.6 * spr / (np.max(np.abs(spr)) + 1e-9))
L = 1.8; tt = t(L)
fr = filt(noise(L), "high", 3000) * (0.6 + 0.4 * np.sin(2 * np.pi * 13 * tt) ** 2) * adsr(L, 0.004, 0.3, 0.5, 0.6, 1.0)
crk = place(L, [(rng.uniform(0, 1.2), filt(noise(0.01), "high", 5000) * env(0.01, 0.002), rng.uniform(0.3, 1)) for _ in range(40)])
add("F10_Hardning", filt(fr + 0.8 * crk, "low", 11000))
L = 1.4; add("F11_Blasbalg", filt(noise(L), "band", [150, 1200]) * adsr(L, 0.45, 0.2, 0.7, 0.4, 0.9))
L = 3.0; tt = t(L); fb = mtof(65)
bellx = sum(a * np.sin(2 * np.pi * fb * r * tt) * np.exp(-tt / d) for r, a, d in ((0.5, 0.6, 1.6), (1.0, 1.0, 1.2), (1.19, 0.6, 0.9), (1.56, 0.45, 0.7), (2.0, 0.4, 0.6), (2.51, 0.3, 0.4), (3.0, 0.2, 0.3)))
add("F12_Gjutklocka_F", bellx * np.minimum(tt / 0.001, 1) + 0.3 * filt(noise(L), "band", [2000, 7000]) * env(L, 0.003))
add("F13_Ror_Stort", bar_modes(mtof(41) * 2, 1.4, 0.5, strike=0.5))
add("F14_Rorslag_Kort", bar_modes(mtof(53) * 2, 0.4, 0.09, strike=0.9))
sk = np.zeros(N(1.2))
for k in range(9):
    o = N(rng.uniform(0, 0.12)); h = bar_modes(rng.uniform(300, 2500), 1.0, rng.uniform(0.05, 0.3), strike=1.0) * rng.uniform(0.4, 1.0)
    sk[o:o + len(h)] += h[: len(sk) - o]
add("F15_Skrot", tanhd(sk / (np.max(np.abs(sk)) + 1e-9), 1.5))
L = 0.35; tt = t(L)
add("F16_Gnista", filt(noise(L), "high", 6000) * env(L, 0.006, 0.0001) + 0.4 * np.sin(2 * np.pi * 7400 * tt) * env(L, 0.004) + 0.2 * filt(noise(L), "high", 4000) * adsr(L, 0.01, 0.05, 0.3, 0.1, 0.12))
sk_k = kick(240, 43.65, 0.03, 0.32, 0.7, 0.6, 1.7, 0.2, hold=0.025)
add("K29_Stad_Kick_F", sk_k + 0.18 * fit(anvil(89, 0.15, 0.5), N(0.7)))
add("H23_Smideshat", metal(0.12, 0.025, 7500, (311.1, 466.2, 554.4, 739.9, 830.6, 1108.7)))
add("H24_Smide_Open", metal(0.6, 0.16, 7000, (311.1, 466.2, 554.4, 739.9, 830.6, 1108.7)))
add("P48_Hammarknack", bar_modes(1560, 0.15, 0.03, strike=1.0))

# ---------------------------------------------------------------- NIVÅER
# Varje kategori får en målnivå (högsta korta RMS, 50 ms), som i en färdig mix.
# Taket är -1 dBTP: om målet kräver mer än så, stannar ljudet vid taket.
TARGET = {"F": -14.0, "K": -9.0, "B": -11.0, "C": -13.0, "H": -17.0, "P": -14.0, "S": -14.0, "L": -14.0,
          "T": -15.0, "V": -14.0, "X": -14.0, "Lp": -13.0}
CEIL = 10 ** (-1.0 / 20)
def category(name): return "Lp" if name.startswith("Lp") else name[0]
def short_rms_max(y, sr):
    w = max(1, int(0.05 * sr)); p = np.convolve(y * y, np.ones(w) / w, "valid")
    return np.sqrt(p.max()) if len(p) else np.sqrt(np.mean(y * y))
def true_peak(y):
    return np.max(np.abs(resample_poly(y, 4, 1))) if len(y) > 64 else np.max(np.abs(y))
levels = []

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
    rms = short_rms_max(y, OUT_FS)
    gain = 10 ** (TARGET[category(name)] / 20) / (rms + 1e-12)
    tp = true_peak(y * gain)
    if tp > CEIL: gain *= CEIL / tp
    y = y * gain
    levels.append((name, 20 * np.log10(short_rms_max(y, OUT_FS) + 1e-12), 20 * np.log10(true_peak(y) + 1e-12)))
    w = os.path.join(tmp, name + ".wav")
    wavfile.write(w, OUT_FS, y.astype(np.float32))
    subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-i", w, "-c:a", "flac",
                    "-sample_fmt", "s32", "-bits_per_raw_sample", "24", "-compression_level", "8",
                    os.path.join(OUT, name + ".flac")], check=True)
print(len(lib), "ljud sparade i", OUT)
off = [(n, round(r, 1), round(TARGET[category(n)] - r, 1)) for n, r, p in levels if TARGET[category(n)] - r > 1.0]
print("under målet (stoppade av taket):", off)
