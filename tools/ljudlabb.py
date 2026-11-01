"""Ljudlabb: analyserar ett bibliotek med ljudfiler och listar typiska kvalitetsfel."""
import sys, os, subprocess, glob, io
import numpy as np
from scipy.io import wavfile
from scipy.signal import resample_poly, butter, sosfilt

def load(p):
    raw = subprocess.run(["ffmpeg","-loglevel","error","-i",p,"-f","wav","-c:a","pcm_f32le","-"],capture_output=True).stdout
    sr, x = wavfile.read(io.BytesIO(raw)); x = x.astype(np.float64)
    if x.ndim == 1: x = x[:,None]
    return sr, x

def db(v): return 20*np.log10(max(v,1e-12))

def kweight_loud(x, sr):
    # förenklad LUFS (K-vikt + max 400 ms-fönster), räcker för att jämföra ljud
    s1 = butter(2, 1500, 'highpass', fs=sr, output='sos'); s2 = butter(2, 60, 'highpass', fs=sr, output='sos')
    y = sosfilt(s2, x, axis=0) + 0.6*sosfilt(s1, x, axis=0)
    w = int(0.4*sr); e = (y**2).sum(axis=1)
    if len(e) < w: return 10*np.log10(max(e.mean(),1e-12)) - 0.691
    c = np.convolve(e, np.ones(w)/w, 'valid')
    return 10*np.log10(max(c.max(),1e-12)) - 0.691

rows = []
for p in sorted(glob.glob(os.path.join(sys.argv[1], "*.*"))):
    sr, x = load(p); n = len(x); m = x.mean(axis=1)
    peak = np.abs(x).max()
    tp = np.abs(resample_poly(x, 4, 1, axis=0)).max()
    dc = abs(m.mean())
    first = np.abs(x[0]).max()
    lead = int(np.argmax(np.abs(m) > peak*0.02)) / sr * 1000
    tail = np.sqrt((x[-int(0.002*sr):]**2).mean()) if n > sr*0.01 else peak
    hf = sosfilt(butter(4, 18000, 'highpass', fs=sr, output='sos'), m)
    hfr = db(np.sqrt((hf**2).mean())) - db(np.sqrt((m**2).mean()))
    fel = []
    if tp > 1.0: fel.append(f"true peak {db(tp):+.1f} dB")
    if dc > 0.003: fel.append(f"DC {dc:.3f}")
    if first > 0.02: fel.append(f"start-klick ({db(first):.0f} dB)")
    if db(tail) - db(peak) > -45: fel.append(f"avhugget slut ({db(tail)-db(peak):.0f} dB)")
    if lead > 3: fel.append(f"tystnad före ({lead:.0f} ms)")
    if hfr > -25 and os.path.basename(p)[0] in "BS": fel.append(f"fräs >18k ({hfr:.0f} dB)")
    rows.append((os.path.basename(p), sr, n/sr, db(peak), kweight_loud(x, sr), fel))

print(f"{'fil':38s} {'sr':>6s} {'sek':>5s} {'topp':>6s} {'LUFS':>6s}  fel")
for r in rows: print(f"{r[0][:38]:38s} {r[1]:6d} {r[2]:5.2f} {r[3]:6.1f} {r[4]:6.1f}  {', '.join(r[5])}")
cats = {}
for r in rows: cats.setdefault(r[0][0], []).append(r[4])
print("\nLoudness per grupp (min / median / max):")
for k,v in sorted(cats.items()): print(f"  {k}: {min(v):6.1f} {np.median(v):6.1f} {max(v):6.1f}  ({len(v)} st)")
from collections import Counter
c = Counter(f.split(' (')[0].split(' ')[0] + (' ' + f.split(' ')[1] if f.startswith('true') else '') for r in rows for f in r[5])
print("\nFel totalt:", dict(c), " av", len(rows), "ljud")
