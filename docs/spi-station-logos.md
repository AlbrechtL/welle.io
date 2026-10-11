# Station logos (SPI)

welle.io shows a station's logo left of its name in the station list. The
logos are received over DAB itself, from the *Service and Programme
Information* (SPI, formerly called EPG) of the ensemble. No internet
connection is used.

This document describes how the logos are transmitted and decoded, and
where the implementation is. It was written with the help of an AI
assistant (Claude Code).

## Specifications

| Specification | Content |
|---|---|
| ETSI TS 102 818 | SPI XML format: `service/mediaDescription/multimedia` with the logos, logo sizes |
| ETSI TS 102 371 | Binary encoding of the SPI documents and their transport by MOT |
| ETSI EN 301 234 | MOT (Multimedia Object Transfer), directory mode |
| ETSI EN 300 401 | DAB: packet mode, MSC data groups, FIG 0/2, 0/3, 0/8, 0/13 |
| ETSI TS 101 756 | User application types (0x007 = SPI) |

## How logos are transmitted

1. **Signalling (FIC).** An SPI service is a data service with a packet mode
   service component:
   - FIG 0/2 gives the SId and the SCId of the component (TMId 3).
   - FIG 0/3 gives, for that SCId, the subchannel, the packet address,
     DSCTy 60 (MOT) and the DG flag.
   - FIG 0/8 gives the SCIdS of the component.
   - FIG 0/13 gives the user application type 0x007 (SPI, called "EPG")
     with the profile (0x01 basic, 0x02 advanced).

   No FIG links an audio service to its SPI service. The SI document tells
   which services the SPI covers. It can also cover services of other
   ensembles.
2. **Packet mode.** The subchannel carries packets of 24, 48, 72 or 96
   bytes. Each has a 3-byte header (continuity index, first/last flags,
   10-bit address, useful data length) and a CRC. One subchannel can carry
   several components with different addresses. Packets of address 1022
   carry the Reed-Solomon parity of the packet FEC (EN 300 401 clause
   5.3.5). The packets of one address form MSC data groups.
3. **MOT directory mode.** The data groups carry a MOT carousel. The MOT
   directory (data group type 6) lists all objects with their headers. The
   bodies (data group type 4) are matched to the entries by their
   TransportId. The carousel is repeated continuously.
4. **SPI objects.** The objects with ContentType 7 are SPI documents: 7/0 is
   SI (service information), 7/1 is PI (programme information, the
   schedule) and 7/2 is GI. They are binary encoded (a tag-length-value tree
   with optional string tokens), see TS 102 371 annexes D to F. The logos
   are ordinary image objects (ContentType 2) in the same carousel.
5. **Logos.** Each `service` of the SI document has a `bearer` with ECC,
   EId and SId, and up to several `multimedia` elements. The `url` of such
   an element is the MOT ContentName of the logo object. For broadcast,
   TS 102 818 requires four PNG logos per service: 32x32
   (`logo_colour_square`), 112x32 (`logo_colour_rectangle`), 128x128 and
   320x240. URLs starting with `http://` refer to logos on the internet.

Example: on channel 11D (WDR NRW), the SPI service "WDR-EPG" (SId
0xE0D01004) is an 8 kbit/s subchannel with packet address 1 and packet FEC.
Its carousel has about 126 objects: SI and PI documents (`*.EIB`, `*.EHB`)
and the logos (`d391-….png`). A complete cycle takes several minutes.

## Implementation

| File | Task |
|---|---|
| `src/backend/fib-processor.cpp` | FIG 0/3 (packet mode), FIG 0/8 (SCIdS), FIG 0/13 (user applications); `getSpiComponents()` |
| `src/backend/msc-handler.cpp` | `addDataSubchannel()`: channel decoding of the subchannel by `DabAudio`, which hands the logical frames to the `SpiDecoder` |
| `src/backend/packet-decoder.cpp` | Packets → MSC data groups (CRC, address, continuity) |
| `src/backend/mot-carousel.cpp` | MOT directory mode: directory, bodies, complete objects |
| `src/backend/spi-decoder.cpp` | `SpiDecoder` (logical frames → packets → MOT → SI and logos), the binary SI decoder, and `SpiLogoCollector`, which picks one logo per service |
| `src/backend/radio-receiver.cpp` | `startSpiDecoding()`: decodes all SPI components of the ensemble |
| `src/welle-gui/radio_controller.cpp` | Starts the SPI decoding every few seconds (it doesn't restart running decoders) and forwards the logos |
| `src/welle-gui/station_logos.cpp` | Logo cache on disk, and the QML image provider `image://logo/<SId>` |
| `src/welle-gui/QML/home/StationDelegate.qml` | Shows the logo in the station list |
| `src/welle-gui/QML/expert/ServiceDetailsCard.qml` | Shows whether the ensemble provides station logos |

In expert mode, the "Service Details" view shows in the row "Station logos
(SPI)" whether the current ensemble has an SPI service: "Yes, n logo(s)
received", "Not provided by this ensemble" (after 15 s with sync and no SPI
service in the FIC), or "Searching…". It explains why some stations have a
logo and others not.

The SPI subchannels are decoded in addition to the audio service, and they
continue when another station of the same ensemble is selected. During a
station scan no SPI is decoded.

From the logos of a service, the one best suited for the list is used:
128x128, then larger square ones, then 32x32, then 320x240. The 112x32
logo isn't used. A logo is replaced when a better or changed one is received.

The logos are stored as PNG files, named by the SId in hexadecimal, in the
application data directory, e.g. `~/.local/share/welle.io/welle.io/logos/D391.png`
on Linux. They are shown after a restart and for stations of other
ensembles.

## Limits

- Logos are only available for ensembles with an SPI service. Many
  ensembles don't have one.
- Logos appear only after the carousel was received, i.e. after a few
  minutes on the ensemble.
- The packet FEC is not used, so packets with bit errors are lost and their
  objects arrive in a later cycle.
- SPI carried in the PAD of an audio service is not supported, only packet
  mode.
- PI and GI documents (the programme schedule) and advanced profile objects
  are ignored.
- Compressed MOT directories (data group type 7) are not supported; SPI
  doesn't use them.

## Tests

- `unit.fib` (`src/tests/unit/tst_fib.cpp`): FIC signalling of packet mode components.
- `unit.spi` (`src/tests/unit/tst_spi.cpp`): packet decoder, MOT carousel, SI decoder and logo choice, with synthetic data.
- `backend.spi.<id>` (`src/tests/backend/tst_spi_recordings.cpp`): replays recorded SPI subchannels from welle.io-test-data (`spi/`).
- `gui.ui.stations`: the logo in the station list and after a restart.
- `gui.ui.dashboard`: the SPI row of the service details.

`src/tests/tools/spi_capture.cpp` records the logical frames of the SPI
subchannels from an rtl_tcp server, and replays such a recording:

```sh
spi_capture <host> <port> <channel> <seconds> <output directory>
spi_capture --replay <file> <bitrate kbit/s> <packet address> <output directory>
```

## Possible extension: logos from RadioDNS

Stations of ensembles without SPI could get their logos from the internet
through RadioDNS (ETSI TS 102 818 clause 10, ETSI TS 103 270). This isn't
implemented. It would work like this:

1. Build the FQDN from the broadcast parameters:
   `<scids>.<sid>.<eid>.<gcc>.dab.radiodns.org`, where gcc is the country
   ID (the first nibble of the SId) followed by the ECC, e.g.
   `0.d391.10ec.de0.dab.radiodns.org`.
2. Resolve its CNAME to the authoritative FQDN of the broadcaster.
3. Look up the SRV record `_radioepg._tcp.<authoritative FQDN>` for host
   and port of the SPI server.
4. Download `http://<host>:<port>/radiodns/spi/3.1/SI.xml`.
5. Take the `multimedia` elements of the matching `service` (its `bearer`
   id is `dab:<gcc>.<eid>.<sid>.<scids>`) and download the logo. For IP,
   TS 102 818 requires five sizes: 32x32, 112x32, 128x128, 320x240 and
   600x600.

The downloaded logos could be stored in the same cache
(`station_logos.cpp`), so the station list would show them without
changes. It would need Qt Network, `QDnsLookup` and an XML parser, and it
should be optional, because it contacts servers on the internet.
