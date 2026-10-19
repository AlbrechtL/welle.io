# Coherent demodulation

welle.io has two demodulators for the DQPSK carriers of DAB. They are
selected in the expert settings (*Demodulator*) or with
`RadioReceiverOptions::demodulationMethod`:

| Demodulator | Class | For |
|---|---|---|
| Differential (default) | `OfdmDecoder::decodeDifferential()` | every reception, also in a car |
| Coherent | `CoherentDemodulator` (`src/backend/ofdm-decoder.{h,cpp}`) | weak signals with a stationary antenna |

welle-cli always uses the differential demodulator. User documentation:
[Weak signals](../user-guide/usage.md#weak-signals).

This document was written with the help of an AI assistant (Claude Code).

## Why

DAB transmits the data as the phase difference between the same carrier of
two successive OFDM symbols (π/4-DQPSK). The differential demodulator
multiplies every carrier with the conjugate of the same carrier of the
previous symbol. The unknown channel (amplitude and phase of every carrier)
cancels, but the reference is a received symbol with as much noise as the
symbol itself: the noise counts about twice.

The coherent demodulator compares the symbols with a channel estimate that
is almost free of noise, and computes the soft bits of the phase difference
from the probabilities of the absolute phases of both symbols. The FIC and
the audio need up to 2 dB less SNR, with echoes up to 2.7 dB (see
[Results](#results)).

## Method

### Channel estimate (once per frame, `processPRS()`)

1. The phase reference symbol (PRS) is known:
   `Hraw[k] = Y0[k] · conj(PRS[k])` for every carrier k.
2. Before averaging, the average of the previous frames is turned to the
   timing and the phase of this frame. The FFT window moves by whole and
   partial samples from frame to frame (`findIndex()`, sample clock), which
   gives a phase that grows linearly with k; the oscillator gives a common
   phase. The timing difference is the maximum of the cross-correlation of
   `Hraw` and the average (IFFT of `Hraw · conj(Havg)`, then a ternary
   search for the fraction of a sample). The phase differences of
   neighbouring carriers are not usable for this: they are biased by echoes
   whose strength changes a little from frame to frame.
3. Exponential average over the frames (weight 0.15, the first frames with
   equal weights).
4. Noise of every carrier: average of `|Hraw − Havg|²`, smoothed over ±8
   carriers. The soft bits of a carrier are weighted with 2 / noise power,
   so carriers in a notch of the channel or with interference count less.
5. The impulse response is shorter than the guard interval: the IFFT of
   `Havg` keeps only the taps above the noise (64 times the median tap
   power, plus 16 taps margin), which removes most of the noise of the
   estimate. With a lower threshold, single noise taps far from the paths
   widen the window: with 16, a 7D recording at 3 dB SNR gave 15 % less
   audio, and `uk_12b_reconfig_2024` 8 times as many frame errors. The edges of the band are corrected with the response of a
   flat channel to the same window.
6. A PRS whose power differs by more than a factor of 4 from the average,
   or whose correlation with the average is below 0.25 (another ensemble,
   another antenna), starts a new average. Until two PRS are averaged,
   `OfdmDecoder` keeps demodulating differentially.

### Tracking within the frame (`demodulate()`)

The channel estimate is from the PRS at the beginning of the frame. Until
its end (96 ms), two things change:

* The residual **frequency offset** turns all carriers by the same phase
  per symbol. The decisions can't tell a rate of φ per symbol from
  φ + π/2 (about 200 Hz), but then every phase difference would be wrong.
  So the predicted rate comes from the correlation of the guard interval
  with the end of every symbol of the frame (unambiguous within ±500 Hz).
* The error of the **sample clock** moves the symbols in time: a phase
  that grows linearly with the carrier number. With 66 ppm (recording
  `si_10d`) that is 13 samples per frame. The rate is learned across
  frames.

For every symbol, both are predicted, the carriers are decided to the
nearest of the four phases, and what is left is measured: the timing from
the phase difference of carriers 256 apart (precise), with the ambiguity of
the folded decisions (π/2 over 256 carriers) resolved by carriers 32
apart; then the common phase over all carriers. The timing is followed with
a weight of 0.5 · SNR / (SNR + 2), its rate with 0.03 · SNR / (SNR + 2): at
low SNR the measurement noise would otherwise add up from symbol to symbol.

### Soft bits

Symbol l has the phase of the PRS plus l · π/4 plus m · π/2. After removing
the channel, the PRS, l · π/4 and the tracked drift, the log-likelihoods of
the four absolute phases m are `w · Re(v)`, `w · Im(v)`, `−w · Re(v)`,
`−w · Im(v)`. The probability of the phase difference n (π/4 + n · π/2) is
the sum over the previous phase m of `P(m) · P(m + n)`; the log-likelihood
ratios of the two bits follow from it. `OfdmDecoder` scales them by 16 to
the soft bits of the Viterbi decoder (±127).

The noise of the PRS doesn't contain the interference of echoes longer than
the guard interval (the PRS follows the almost empty null symbol). The
noise of the data symbols is therefore also measured (distance to the
nearest phases) and the larger of both is used. In `uk_12b_reconfig_2024`
it is 7.5 times the noise of the PRS.

### Cost

About 2 FFTs per frame and 2 `exp` and 2 `log` per carrier and symbol:
6 % of one core of a desktop PC in real time, in the decoder thread. The
probabilities are limited (log-likelihoods ±15, minimum 1e-15), because
denormal floats made the loop twice as slow.

## Limits

* **Moving reception:** the channel estimate is from the beginning of the
  frame. An echo that turns 10 times per second (Doppler 10 Hz) gives 26 %
  FIB errors at 15 dB SNR in `odr_dabmod`, where the differential
  demodulator has none, and from about 20 Hz Doppler on the audio needs more
  SNR. Use *Differential* in a car.
* **Echoes longer than the guard interval** disturb the data symbols, but
  not the PRS, which follows the almost empty null symbol: the noise is
  measured on the data symbols for this (see [Soft bits](#soft-bits)).
  `uk_12b_reconfig_2024` (paths at −470 and +466 samples) decodes with
  fewer frame errors than with the differential demodulator.
* The time synchronisation is the same for both demodulators. At about
  0 dB SNR its detection of the null symbol (an energy dip) can take
  seconds to find the frame again after losing it.

## Tests

| Test | What |
|---|---|
| `unit.coherent_demod` | Synthetic frames: right bits with an echo, changing FFT window and phase, frequency offset and clock error; more information per soft bit than differential at 2 dB SNR; a new channel restarts the estimate |
| `backend.coherent.<clip>` | Every recording of the test data with the coherent demodulator, against the same expectations (`expect_coherent` overrides them, e.g. `lu_7d_2026`) |
| `backend.sensitivity` | `odr_dabmod` through noise, an echo and an echo with Doppler shift, with both demodulators |
| `gui.ui.settings`, `gui.ui.playback` | The setting reaches the receiver and survives a restart; all stations play with the coherent demodulator |

`demod_compare <output.json>` (`src/tests/tools/`) produces the comparison
below: every recording in both modes, and SNR sweeps through the channel
model. Each decode runs in its own process (FFTW plans can't be created in
parallel threads).

## Results

Measured with `demod_compare` on 2026-10-19. SNR in dB as set in the
channel model (noise power relative to the whole received signal, including
the noise of the recording itself), in steps of 0.5 dB.

### SNR needed

SNR at which 50 % of the FIBs are correct (station list), and at which 90 %
of the audio of all services is decoded without errors (relative to the
recording without added noise). Echo: 0.9 after 20 µs (a notch every
50 kHz); Doppler: the echo turns with this frequency (moving receiver).

| Recording | Channel | FIB differential | FIB coherent | Audio differential | Audio coherent |
|---|---|---|---|---|---|
| odr_dabmod (synthetic) | noise | 1.0 | 0.4 | 2.9 | 2.2 |
| odr_dabmod (synthetic) | echo | 2.4 | 0.6 | 5.4 | 3.4 |
| odr_dabmod (synthetic) | echo, Doppler 5 Hz | 2.4 | 1.9 | 5.0 | 4.6 |
| odr_dabmod (synthetic) | echo, Doppler 20 Hz | 2.5 | 2.0 | 5.0 | 5.6 |
| odr_dabmod (synthetic) | echo, Doppler 50 Hz | 2.8 | 2.3 | 5.7 | 6.1 |
| fr_paris_6b_2026 | noise | 3.6 | 1.7 | 7.9 | 5.9 |
| fr_paris_6b_2026 | echo | 5.6 | 2.9 | > 10 | 9.4 |
| fr_paris_6b_2026 | echo, Doppler 5 Hz | 5.7 | 4.8 | > 10 | > 10 |
| fr_paris_6b_2026 | echo, Doppler 20 Hz | 5.7 | 5.2 | > 10 | > 10 |
| fr_paris_6b_2026 | echo, Doppler 50 Hz | 6.2 | 5.5 | > 10 | > 10 |
| fr_metz_8a_2026 | noise | 4.2 | 2.4 | 9.0 | 7.2 |
| fr_metz_8a_2026 | echo | 6.2 | 3.5 | > 10 | > 10 |
| fr_metz_8a_2026 | echo, Doppler 5 Hz | 6.2 | 5.7 | > 10 | > 10 |
| fr_metz_8a_2026 | echo, Doppler 20 Hz | 6.3 | 5.9 | > 10 | > 10 |
| fr_metz_8a_2026 | echo, Doppler 50 Hz | 6.8 | 6.2 | > 10 | > 10 |
| uk_11d_2020 | noise | 3.1 | 2.8 | 5.7 | 4.9 |
| uk_11d_2020 | echo | 4.8 | 2.9 | 8.6 | 6.8 |
| uk_11d_2020 | echo, Doppler 5 Hz | 4.6 | 4.0 | 8.4 | 8.6 |
| uk_11d_2020 | echo, Doppler 20 Hz | 4.7 | 4.0 | 8.5 | > 10 |
| uk_11d_2020 | echo, Doppler 50 Hz | 5.0 | 4.4 | 9.2 | > 10 |

With noise, the coherent demodulator needs 0.3–1.9 dB less SNR for the FIC
and 0.7–2.0 dB less for the audio; with a static echo (notches) 1.8–2.7 dB
less for the FIC and about 2 dB less for the audio. With a moving echo it
still decodes the FIC slightly earlier, but from about 20 Hz Doppler on the
audio needs more SNR.

### Recordings without added noise

All recordings of the test data, decoded with both demodulators (FIBs
correct/failed, seconds of audio of all services, erroneous audio frames).
Recordings with differences:

| Recording | FIBs differential | FIBs coherent | Audio differential | Audio coherent | Frame errors differential | Frame errors coherent |
|---|---|---|---|---|---|---|
| de_heidelberg_9d | 1191/33 | 1200/24 | 0.0 s | 80.2 s | 286 | 29 |
| lu_7d_2026 | 3405/327 | 3522/210 | 2.6 s | 174.5 s | 1889 (31.5 %) | 2718 (18.7 %) |
| ie_12c_2017 | 1202/34 | 1212/24 | 100.9 s | 102.0 s | 0 | 0 |
| it_10c_s16le | 129/15 | 132/12 | 5.7 s | 5.7 s | 0 | 0 |
| si_10d | 1937/31 | 1937/31 | 177.3 s | 178.2 s | 25 | 0 |
| uk_12b_sdrtouch | 1197/36 | 1212/12 | 90.5 s | 92.9 s | 0 | 0 |
| uk_welle_record_2024 | 1212/24 | 1224/12 | 101.2 s | 102.6 s | 0 | 0 |
| uk_12b_reconfig_2024 | 3696/27 | 3699/21 | 319.5 s | 319.6 s | 236 | 96 |

The other 26 recordings decode the same (within 1 % of the FIBs and 1 s of
audio); none decodes worse with the coherent demodulator. It often finds
the first correct FIB earlier (0.1 instead of 0.2–0.3 s), because the
differential demodulator needs a few frames until the frequency correction
has settled.

LUX-NATIONAL1 (7D) is the reason for the coherent demodulator: with an
indoor antenna at about −1.5 dB SNR (not in the test data), the
differential demodulator decodes no FIB at all, the coherent one 135 of
2220, enough for the ensemble, its 12 services and their labels, but no
audio. `lu_7d_2026` (about 3.3 dB, see below) gives audio only with the
coherent demodulator. `de_heidelberg_9d` (weak, 2016) gives 80 s of audio
instead of none.

### 7D at 3.3 dB SNR

A 66 s recording of 7D with a better antenna (welle.io recorder; 30 s of it
are the clip `lu_7d_2026`), at a constant SNR of 3.3 dB (PRS, frame to
frame), lies at the threshold of the audio (DAB+, EEP 3-A). The whole
recording:

| | Differential | Coherent |
|---|---|---|
| Correct FIBs | 89.7 % | 93.8 % |
| Audio of the 12 stations | 2.6 s (0–0.8 s each) | 260 s (19–24 s each) |
| Uncorrectable DAB+ superframes | 99.9 % | 84.2 % |

In the clip, the coherent demodulator decodes 13–16 s of audio per station
(of 30 s), the differential one at most 0.8 s; `backend.coherent.lu_7d_2026`
requires at least 6 s. This recording also showed that the time domain
window of the channel estimate was too wide with a threshold of 16 (15 %
less audio, see [Method](#method)).
