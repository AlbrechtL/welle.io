# Multiplex reconfiguration and MP2 error concealment

Some broadcasters change the layout of their ensemble while it is on air.
BBC National DAB (12B), for example, moves most of its subchannels at the
full hour and changes the bitrate of some services (Radio 3 160 ↔ 192
kbit/s, Radio 4 80 ↔ 128 kbit/s). Before welle.io supported this, the
audio of every moved service stopped or turned into noise after the change
(issue #587).

This document describes how a reconfiguration is signalled and handled,
and the Scale Factor error check of MPEG Audio Layer II (DAB) frames. It
was written with the help of an AI assistant (Claude Code).

## Specifications

| Specification | Content |
|---|---|
| ETSI EN 300 401 clause 6.5 | Multiplex reconfiguration |
| ETSI EN 300 401 clause 6 | FIG 0/0 (CIF count, change flags, occurrence change), C/N flag |
| ETSI TS 103 466 clauses B.3, C.5 | ScF-CRC of DAB audio frames, error concealment |

## How a reconfiguration is signalled

* Every CIF (24 ms of the MSC) has a CIF count. FIG 0/0 carries it for the
  CIF its FIB belongs to. In transmission mode I a transmission frame has
  4 CIFs, the FIBs 0–2 belong to the first CIF, 3–5 to the second, and so
  on. BBC sends FIG 0/0 only in the FIBs of the first CIF of a frame.
* Some seconds before a change (about 6 s at the BBC), FIG 0/0 has
  change flags ≠ 0 and the *occurrence change*: the lower part of the CIF count from which the new
  configuration is used.
* During this time the new configuration is sent in addition to the
  current one, as FIG 0/1 (subchannel organisation) and FIG 0/2 (service
  organisation) with the C/N flag set to 1 ("next").

## Implementation

* `FIBProcessor` (`fib-processor.cpp`) stores FIG 0/1 with C/N = 1 in a
  separate table, `nextSubChannels`. Before, it overwrote the current
  subchannels, so a service selected during the announcement was decoded
  with the new layout too early.
* `FicHandler` calls `FIBProcessor::processCifEnd()` after the three FIBs
  of each CIF, also if their CRC failed. The CIF count is taken from
  FIG 0/0 and counted on from there, so the change also happens at the
  right CIF if the FIG 0/0 of that frame is lost.
* At the CIF of the change, the next subchannels are taken over and the
  reconfiguration callback (`setReconfigurationCallback()`, set by
  `RadioReceiver`) passes them to `MscHandler::reconfigure()`, with the
  index of that CIF in the current transmission frame. The FIC of a
  frame is decoded before its CIFs.
* `MscHandler` changes its streams before it cuts that CIF:
  * a subchannel that only moved (same size and protection) keeps its
    decoder, so the audio continues without a gap;
  * a subchannel with a new size or protection gets a new decoder
    (`SelectedStream::createHandler`), there is a short gap.
* Subchannels that are not signalled in the next configuration are kept;
  the FIG 0/1 of the current configuration updates them afterwards.
* If the announcement ends before the change was seen (e.g. the CIF count
  was not known yet), the change is made at that moment.

Not supported: changes of the service organisation, i.e. a service that
moves to another subchannel. FIG 0/2 with C/N = 1 is handled like before.

### Logging

```
fib-processor: Reconfiguration announced at CIF count 4844, change at 90
fib-processor: Reconfiguration at CIF count 90: subchannel 4 308+96 -> 332+58; ...
msc-handler: subchannel 4 changed from 128 kbit/s UEP 3 to 80 kbit/s UEP 3
```

## Scale Factor error check (ScF-CRC)

The CRC of an MPEG Audio Layer II frame covers only the header, the bit
allocation and the ScFSI. DAB adds the ScF-CRC: 2 or 4 CRC-8 words in
front of the F-PAD of frame *n − 1*, each covering the 3 MSbs of the Scale
Factors of one group of sub-bands of frame *n* (sub-bands 0–3, 4–7, 8–15,
16–26/29).

A Scale Factor step is 2 dB, so a bit error in the MSb of a Scale Factor
changes the level of a sub-band by 64 dB. Without the check, a weak signal
therefore sounds like crackling and full scale noise bursts, although the
MPEG CRC of the frames is correct.

`Mp2ScfCrc` (`mp2_scf_crc.cpp`) checks the Scale Factors of every frame
before mpg123 decodes it and mutes the Scale Factors of the groups with an
error (Scale Factor index 62), as recommended by TS 103 466 clause C.5.
`MP2Decoder::DecodeFrame()` calls it and counts a frame with muted groups
as frame error. After a frame with an MPEG CRC error, the next frame is
not checked, because the ScF-CRC words in front of it are missing.

Measured with the BBC recording of issue #587 (SNR about 10 dB): 15 % of
the frames had Scale Factor errors. With the check, the full scale samples
per service dropped from 3,600–12,000 to 0–290 in two minutes. Clean
recordings have practically no ScF-CRC errors (2 of about 22,000 frames).

## Tests

* `unit.fib`: `reconfiguration` and `reconfigurationWithoutCifCount`.
* `unit.mp2_scf_crc`: CRC-8 and muting of a group with an error.
* `backend.recording.uk_12b_reconfig_2024`: 30 s of BBC National DAB
  around the reconfiguration at 17:00; every service has to decode audio
  after the change.
* `backend.recording.uk_12b_bbc_2020` contains an announcement (the change
  is after the end of the clip): Radio 4 has to stay at 80 kbit/s.
