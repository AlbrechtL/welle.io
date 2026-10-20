# Iterative DAB+ decoding

DAB+ protects the audio twice: the convolutional code (Viterbi decoder) of
the subchannel and a Reed-Solomon code RS(120, 110) over each superframe
(5 logical frames, 120 ms). For superframes with uncorrectable RS codewords,
welle.io decodes the 5 logical frames again with the Viterbi decoder, with the
bytes of the correct codewords as known bits, and then decodes RS again. This
gives about 0.5 dB more sensitivity for DAB+, with both demodulators (see
[Coherent demodulation](coherent-demodulation.md)), also in welle-cli. MP2
services have no RS code and are unchanged.

This document was written with the help of an AI assistant (Claude Code).

## Where the audio is lost

Measured on the clip `lu_7d_2026` (LUX-NATIONAL1, 7D, about 3.3 dB SNR, 12
DAB+ services, EEP 3-A), coherent demodulator, before this change:

| | |
|---|---|
| Superframes with uncorrectable RS codewords | 51 %, on average 8.9 of 12 codewords |
| Superframes with a broken header (fire code) | 19 % |
| AUs of broken headers that the AU CRCs could find | 1 % |
| Soft bit scale of the Viterbi input | 16 is already the best |

An AU is practically only correct when all RS codewords of its superframe
are: the errors of the Viterbi decoder are spread over the whole superframe
by the RS interleaving (codeword *i* has every 12th byte, from byte *i*). So
neither a faster superframe sync nor the fire code nor a search for the AUs
helps: the RS decoding is the limit.

## Method

A Viterbi decoder makes errors in bursts. The correct RS codewords give
every 12th byte (for 96 kbit/s) of the superframe as known. With these bits
fixed, the decoder can't run a wrong path through them: the bursts become
shorter, more codewords become correctable, which gives more known bytes for
the next pass.

1. `SuperframeFilter::Feed()` (`dabplus_decoder.cpp`) decodes RS as before.
   If codewords are uncorrectable and the superframe is at the known
   alignment (5 logical frames after the last one with a correct fire code),
   `Redecode()` makes up to 3 passes:
   - the bytes of the correct codewords are marked as known,
   - `FrameRedecoder::redecodeFrame()` decodes each of the 5 frames again,
   - RS again, until all codewords are correct or no more become correct.
2. `DabAudio` (`dab-audio.cpp`) implements `FrameRedecoder`
   (`dab-processor.h`): it keeps the input of the Viterbi decoder (the time
   de-interleaved soft bits) of the last 5 frames. Known bytes become known
   bits (most significant bit first, with the energy dispersal sequence
   removed), the frame goes through `Protection::deconvolve(..., known)`
   again.
3. `Viterbi::deconvolve(input, output, known)` (`viterbi.cpp`): after each
   step with a known bit, the states with the other input bit (the lowest
   bit of the state) get the best metric of the allowed states plus 16000.
   That is more than any metric difference within the constraint length
   (6 × 1020), so no path through them survives; it isn't added up, so
   several known bits in a row can't overflow the 16 bit metrics.

Everything runs in the thread of `DabAudio::run()`. Before the first correct
superframe, and after 10 superframes without a correct fire code (alignment
lost), there is no re-decoding.

**RS statistics:** after a superframe with a broken header the filter
searches the alignment again with 4 shifted attempts. These used to be
reported as uncorrectable superframes (`FECInfo`, the RS error display):
almost twice as many superframes as there are on weak signals. Now only
superframes at the alignment, or with a correct fire code, are reported,
with the result after re-decoding.

**Cost:** only superframes with uncorrectable codewords are decoded again,
at most 3 × 5 Viterbi passes each. Strong signals: no change. `lu_7d_2026`
with all 12 services decoded at once: 6–12 s more CPU time for 30 s of
signal, about 3 % of one core per service.

## Results

The offline prototype (a dump of the Viterbi input of `lu_7d_2026`) gave
50.5 → 62.0 → 65.2 → 66.7 % correct AUs after 0, 1, 2, 3 passes. Pinning
also the bytes of AUs with a correct CRC adds nothing (such AUs are only
correct when all codewords are).

In welle.io:

| `lu_7d_2026` (30 s) | Before | After |
|---|---|---|
| Coherent: audio of the 12 stations | 174.5 s (12.9–16.2 s each) | 233.7 s (18.7–20.0 s each) |
| Coherent: frame errors | 18.7 % | 10.6 % |
| Differential: audio | 2.6 s | 59.2 s |

Error-free DAB+ audio of `fr_paris_6b_2026` with added noise (seconds per
service of about 14.3 s; SNR as in the channel model of the tests):

| SNR | Differential before | Differential after | Coherent before | Coherent after |
|---|---|---|---|---|
| 4.5 dB | 0 | 0 | 0.3 | 2.1 |
| 5.0 dB | 0 | 0 | 2.1 | 10.1 |
| 5.5 dB | 0 | 0 | 8.8 | 14.0 |
| 6.0 dB | 0 | 0.8 | 13.8 | 14.2 |
| 6.5 dB | 0.5 | 4.3 | 14.2 | 14.2 |
| 7.0 dB | 3.1 | 11.9 | 14.3 | 14.3 |
| 7.5 dB | 9.7 | 14.3 | 14.3 | 14.3 |

The point with half of the audio moves by about 0.55 dB with both
demodulators (coherent from 5.4 to 4.85 dB, differential from 7.3 to
6.75 dB). The audio threshold of DAB+ is steep, so at the threshold this is
much more audio. MP2 (`odr_dabmod`) decodes identically.

All 34 recordings of the test data with both demodulators (`demod_compare
--clips-only`, before and after): 64 of 68 runs unchanged, the others
(`lu_7d_2026` in both modes, `de_heidelberg_9d` differential) with more
audio, none with less audio, more frame errors or a missing service.

## Tests

| Test | What |
|---|---|
| `unit.viterbi` | DAB convolutional code: decoding without errors, known bits are in the result (also when they contradict the signal), fewer errors of the other bits with every 12th byte known |
| `unit.superframe` | Synthetic superframes (fire code, AU CRCs, RS): a superframe with 6 uncorrectable codewords is decoded again with the bytes of the 6 correct ones and is then correct; no re-decoding before the first sync; RS statistics without the shifted attempts |
| `backend.sensitivity` | `odr_dabmod`: DAB+ at 1 dB (differential) and 0.5 dB (coherent) with at most 2 % frame errors (before: 7.4 % and 16.3 %) |
| `backend.coherent.lu_7d_2026` | At least 17 s of audio per station (before 13–16 s), at most 15 % frame errors |
