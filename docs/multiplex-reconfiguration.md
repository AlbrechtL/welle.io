# Multiplex reconfiguration

Some broadcasters change the layout of their ensemble while it is on air.
BBC National DAB (12B), for example, moves most of its subchannels at the
full hour and changes the bitrate of some services (Radio 3 160 ↔ 192
kbit/s, Radio 4 80 ↔ 128 kbit/s). Before welle.io supported this, the
audio of every moved service stopped or turned into noise after the change
(issue #587).

This document describes how a reconfiguration is signalled and handled.
It was written with the help of an AI assistant (Claude Code).

## Specifications

| Specification | Content |
|---|---|
| ETSI EN 300 401 clause 6.5 | Multiplex reconfiguration |
| ETSI EN 300 401 clause 6 | FIG 0/0 (CIF count, change flags, occurrence change), C/N flag |

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

## Tests

* `unit.fib`: `reconfiguration` and `reconfigurationWithoutCifCount`.
* `backend.recording.uk_12b_reconfig_2024`: 30 s of BBC National DAB
  around the reconfiguration at 17:00; every service has to decode audio
  after the change.
* `backend.recording.uk_12b_bbc_2020` contains an announcement (the change
  is after the end of the clip): Radio 4 has to stay at 80 kbit/s.
