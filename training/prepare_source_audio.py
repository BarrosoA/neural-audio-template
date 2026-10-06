import os
import numpy as np
import soundfile as sf
from scipy.signal import chirp, windows, butter, sosfilt

SAMPLE_RATE = 44100
TARGET_TOTAL_SECONDS = 180.0

# single sample spike at half second mark (sharp marker to line up recorded waves in daw)
def generate_sync_pip(sr: int = SAMPLE_RATE, duration: float = 1.0) -> np.ndarray:
    pip = np.zeros(int(duration * sr))
    pip[int(0.5 * sr)] = 0.9
    return pip

# sine wave sliding from lowest bass to highest treble at 4 volumes (teaches model how gear shapes tone and distorts from quiet to loud)
def generate_chirp_sweeps(sr: int = SAMPLE_RATE, duration: float = 35.0) -> np.ndarray:
    levels_db = [-30.0, -18.0, -6.0, -1.0]
    segment_dur = duration / len(levels_db)
    n_samples = int(segment_dur * sr)
    t = np.linspace(0, segment_dur, n_samples, endpoint=False)
    segments = []
    for db in levels_db:
        amp = 10.0 ** (db / 20.0)
        sw = chirp(t, f0=20.0, t1=segment_dur, f1=20000.0, method='logarithmic')
        fade_len = int(0.01 * sr)
        fade = np.ones(n_samples)
        fade[:fade_len] = np.linspace(0, 1, fade_len)
        fade[-fade_len:] = np.linspace(1, 0, fade_len)
        segments.append(sw * amp * fade)
    return np.concatenate(segments)

# deep bass thump with sharp click at start (teaches model how gear handles heavy sub bass and kick punch)
def synth_kick(sr: int, dur: float, f_start: float, f_end: float, tau_pitch: float, tau_amp: float, amp_db: float, rng) -> np.ndarray:
    n = int(dur * sr)
    t = np.linspace(0, dur, n, endpoint=False)
    inst_freq = f_end + (f_start - f_end) * np.exp(-t / tau_pitch)
    phase = 2.0 * np.pi * np.cumsum(inst_freq) / sr
    body = np.sin(phase) * np.exp(-t / tau_amp)
    click_len = min(int(0.003 * sr), n)
    noise = rng.normal(0, 1, click_len)
    sos = butter(2, [2000, 5000], btype='bandpass', fs=sr, output='sos')
    click = sosfilt(sos, noise) * np.linspace(1, 0, click_len)
    combined = body.copy()
    combined[:click_len] += 0.4 * click
    combined = combined / (np.max(np.abs(combined)) + 1e-6)
    return combined * (10.0 ** (amp_db / 20.0))

# drum tone mixed with rattling snare wires (teaches model midrange smack and noisy fade out)
def synth_snare(sr: int, dur: float, amp_db: float, rng) -> np.ndarray:
    n = int(dur * sr)
    t = np.linspace(0, dur, n, endpoint=False)
    tone1 = np.sin(2.0 * np.pi * 185.0 * t) * np.exp(-t / 0.08)
    tone2 = np.sin(2.0 * np.pi * 330.0 * t) * np.exp(-t / 0.05)
    shell = 0.6 * tone1 + 0.4 * tone2
    noise = rng.normal(0, 1, n)
    sos = butter(2, [1500, 7000], btype='bandpass', fs=sr, output='sos')
    wires = sosfilt(sos, noise) * np.exp(-t / 0.12)
    combined = 0.5 * shell + 0.7 * wires
    combined = combined / (np.max(np.abs(combined)) + 1e-6)
    return combined * (10.0 ** (amp_db / 20.0))

# metallic ring mixed with bright air hiss (teaches model how gear tames harsh high frequencies)
def synth_cymbal(sr: int, dur: float, amp_db: float, rng) -> np.ndarray:
    n = int(dur * sr)
    t = np.linspace(0, dur, n, endpoint=False)
    freqs = [205.0, 305.0, 365.0, 420.0, 600.0, 800.0]
    cluster = np.zeros(n)
    for f in freqs:
        cluster += np.sign(np.sin(2.0 * np.pi * f * t))
    cluster /= len(freqs)
    noise = rng.normal(0, 1, n)
    sos = butter(2, 7000, btype='highpass', fs=sr, output='sos')
    air = sosfilt(sos, noise)
    combined = (0.5 * cluster + 0.7 * air) * np.exp(-t / 0.12)
    combined = combined / (np.max(np.abs(combined)) + 1e-6)
    return combined * (10.0 ** (amp_db / 20.0))

# full drum sequence of kicks snares cymbals and claps (teaches model how gear squashes and recovers from real drum hits)
def generate_drum_suite(sr: int = SAMPLE_RATE, duration: float = 55.0) -> np.ndarray:
    rng = np.random.default_rng(42)
    segments = []
    for f_start in [300.0, 250.0, 180.0]:
        for f_end in [45.0, 55.0]:
            for amp_db in [-18.0, -12.0, -6.0, -0.5]:
                k = synth_kick(sr, dur=0.5, f_start=f_start, f_end=f_end, tau_pitch=0.03, tau_amp=0.18, amp_db=amp_db, rng=rng)
                segments.append(k)
    for amp_db in [-18.0, -12.0, -6.0, -0.5]:
        for _ in range(3):
            s = synth_snare(sr, dur=0.4, amp_db=amp_db, rng=rng)
            segments.append(s)
    for amp_db in [-20.0, -14.0, -8.0, -2.0]:
        for dur in [0.15, 0.35, 0.6]:
            c = synth_cymbal(sr, dur=dur, amp_db=amp_db, rng=rng)
            segments.append(c)
    for amp_db in [-12.0, -6.0, -1.0]:
        dur = 0.35
        n = int(dur * sr)
        clap = np.zeros(n)
        for offset_ms in [0, 12, 25]:
            idx = int(offset_ms * sr / 1000.0)
            noise_burst = rng.normal(0, 1, n - idx) * np.exp(-np.linspace(0, 0.25, n - idx) / 0.04)
            clap[idx:] += noise_burst
        clap = clap / (np.max(np.abs(clap)) + 1e-6)
        segments.append(clap * (10.0 ** (amp_db / 20.0)))
    raw_suite = np.concatenate(segments)
    target_samples = int(duration * sr)
    if len(raw_suite) < target_samples:
        repeats = int(np.ceil(target_samples / len(raw_suite)))
        raw_suite = np.tile(raw_suite, repeats)
    return raw_suite[:target_samples]

# 3 notes played together like a chord (teaches model how notes blend and distort together)
def generate_prime_multitones(sr: int = SAMPLE_RATE, duration: float = 35.0) -> np.ndarray:
    clusters = [
        [53.0, 107.0, 223.0],
        [107.0, 223.0, 439.0],
        [223.0, 439.0, 883.0],
        [439.0, 883.0, 1777.0],
        [883.0, 1777.0, 3559.0],
    ]
    cluster_dur = duration / len(clusters)
    n_samples = int(cluster_dur * sr)
    t = np.linspace(0, cluster_dur, n_samples, endpoint=False)
    fade_len = int(0.02 * sr)
    fade = np.ones(n_samples)
    fade[:fade_len] = np.linspace(0, 1, fade_len)
    fade[-fade_len:] = np.linspace(1, 0, fade_len)
    segments = []
    for cluster in clusters:
        sig = np.zeros(n_samples)
        for freq in cluster:
            sig += (1.0 / len(cluster)) * np.sin(2.0 * np.pi * freq * t)
        segments.append(sig * 0.707 * fade)
    return np.concatenate(segments)

# quick volume spikes from super fast to slow attack (teaches model how fast the circuit reacts to sudden loud hits)
def generate_variable_slew_impulses(sr: int = SAMPLE_RATE, duration: float = 25.0) -> np.ndarray:
    rise_times_ms = [0.1, 0.5, 1.0, 2.0, 5.0, 10.0]
    levels_db = [-18.0, -12.0, -6.0, -1.0]
    repeats = 4
    total_pulses = len(rise_times_ms) * len(levels_db) * repeats
    pulse_dur = duration / total_pulses
    n_samples = int(pulse_dur * sr)
    t = np.linspace(0, pulse_dur, n_samples, endpoint=False)
    segments = []
    for _ in range(repeats):
        for rt in rise_times_ms:
            tau_rise = rt / 1000.0
            tau_decay = 0.050
            pulse = (1.0 - np.exp(-t / max(tau_rise, 1e-5))) * np.exp(-t / tau_decay)
            pulse = pulse / (np.max(pulse) + 1e-6)
            for amp_db in levels_db:
                amp = 10.0 ** (amp_db / 20.0)
                segments.append(pulse * amp)
    raw_sig = np.concatenate(segments)
    target_samples = int(duration * sr)
    return raw_sig[:target_samples]

# static hiss at different volumes plus 5 seconds of dead silence (teaches model to stay quiet on silence + eq shape)
def generate_noise_and_silence(sr: int = SAMPLE_RATE, duration: float = 30.0) -> np.ndarray:
    silence_dur = 5.0
    noise_dur = duration - silence_dur
    noise_levels_db = [-36.0, -24.0, -12.0, -6.0, 0.0]
    segment_dur = noise_dur / len(noise_levels_db)
    n_segment = int(segment_dur * sr)
    segments = []
    rng = np.random.default_rng(42)
    for db in noise_levels_db:
        amp = 10.0 ** (db / 20.0)
        noise = rng.normal(0, 0.5, n_segment)
        noise = np.clip(noise, -1.0, 1.0)
        fade_len = int(0.01 * sr)
        fade = np.ones(n_segment)
        fade[:fade_len] = np.linspace(0, 1, fade_len)
        fade[-fade_len:] = np.linspace(1, 0, fade_len)
        segments.append(noise * amp * fade)
    silence_samples = int(silence_dur * sr)
    segments.append(np.zeros(silence_samples))
    return np.concatenate(segments)

# glues all 5 test sounds into one 3-minute wav file 
def build_corpus(output_path: str = 'training/data/dry_corpus.wav'):
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    sync_pip = generate_sync_pip(SAMPLE_RATE, duration=1.0)
    chirps = generate_chirp_sweeps(SAMPLE_RATE, duration=34.0)
    drums = generate_drum_suite(SAMPLE_RATE, duration=55.0)
    multitones = generate_prime_multitones(SAMPLE_RATE, duration=35.0)
    impulses = generate_variable_slew_impulses(SAMPLE_RATE, duration=25.0)
    noise_silence = generate_noise_and_silence(SAMPLE_RATE, duration=30.0)
    corpus = np.concatenate([sync_pip, chirps, drums, multitones, impulses, noise_silence])
    total_samples = int(TARGET_TOTAL_SECONDS * SAMPLE_RATE)
    corpus = corpus[:total_samples]
    sf.write(output_path, corpus, SAMPLE_RATE, subtype='FLOAT')
    print(f'Saved updated drum-enriched corpus to {output_path}')
    print(f'Total duration: {len(corpus)/SAMPLE_RATE:.2f}s ({len(corpus)} samples at {SAMPLE_RATE} Hz)')
    print(f'Peak amplitude: {np.max(np.abs(corpus)):.4f} ({20*np.log10(np.max(np.abs(corpus))):.2f} dBFS)')

if __name__ == '__main__':
    build_corpus()
