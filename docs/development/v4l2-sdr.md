# Linux kernel SDR input (V4L2)

The input `CV4L2Sdr` (`src/input/v4l2_sdr.{h,cpp}`, `CDeviceID::V4L2`)
receives with SDR devices that have a Linux kernel driver, through the
V4L2 SDR capture interface (`/dev/swradioN`). It needs only the kernel
headers (`linux/videodev2.h`). User documentation: [Linux SDR
(V4L2)](../devices/v4l2.md).

This document was written with the help of an AI assistant (Claude Code).

## The kernel interface

Kernel documentation: *Software Defined Radio Interface (SDR)* in the
V4L2 user space API (`Documentation/userspace-api/media/v4l/dev-sdr.rst`).

| Item | Use |
|---|---|
| `VIDIOC_QUERYCAP` | `V4L2_CAP_SDR_CAPTURE`, `V4L2_CAP_TUNER`, `V4L2_CAP_STREAMING` are required |
| Tuner 0, type `V4L2_TUNER_ADC` | Sample rate: `VIDIOC_ENUM_FREQ_BANDS`, `VIDIOC_S/G_FREQUENCY` |
| Tuner 1, type `V4L2_TUNER_RF` | RF frequency (`V4L2_TUNER_CAP_1HZ`: Hz, otherwise 62.5 kHz units) |
| `VIDIOC_ENUM_FMT`, `VIDIOC_S_FMT` (`V4L2_BUF_TYPE_SDR_CAPTURE`) | Sample format |
| `VIDIOC_REQBUFS`, `QBUF`, `DQBUF`, `STREAMON` | 8 mmap buffers, read by a thread with `poll()` |
| `VIDIOC_S_EXT_CTRLS` (`V4L2_CID_RF_TUNER_*`) | Gain |

## Sample formats

In the order of preference: `CU8` (rtl2832_sdr, the same bytes as
librtlsdr), `CS8` (hackrf, msi2500), `CU16LE`, `CS14LE` (msi2500) and the
real format `RU12LE` (airspy). `V4L2SdrFormats::convertComplex()` and
`convertReal()` convert them to float with full scale 1.

## Sample rate

The input chooses the lowest rate from 2.048 MS/s (complex) or 4.096 MHz
(real) that the ADC bands allow. Other rates than 2.048 MS/s are converted
by `RationalResampler` (as for the Airspy input).

The **airspy** driver never sends a sample rate command to the device: the
firmware runs at its default rate, but the driver reports a fixed 20 MHz.
`restart()` therefore measures the data rate of the first 0.5 s (before the
reader thread starts) and uses the nearest of the known ADC rates (R2: 20
and 5 MHz, Mini: 12 and 6 MHz). The Airspy Mini runs at 12 MHz (measured
11.997 MHz), i.e. its 6 MS/s mode.

## Real samples (airspy)

The airspy driver delivers the real 12 bit samples of the ADC. The wanted
band is at a quarter of the ADC rate. `RealIqConverter` converts them to
complex samples:

1. Shift by -fs/4: sample *n* is multiplied by exp(-j·π·n/2), i.e. by 1,
   -j, -1, j. Even samples become real, odd ones imaginary.
2. Half-band low pass (Kaiser window, 80 dB, length 4m + 3) and decimation
   by 2. Every second coefficient of a half-band filter is zero, so I is
   filtered with the even coefficients and Q is only delayed (centre
   coefficient). The output has a gain of 2, so a real tone keeps its
   amplitude.
3. From 10 MHz on (Mini 12 MHz, R2 20 MHz) a second, complex half-band
   stage decimates by 2 again (Mini: 3 MS/s). It costs less than it saves
   in the resampler (26 instead of 51 taps per output).

The filters work per buffer: the loop over the coefficients is outside,
the loop over the samples inside, so the compiler vectorises it (without
`-ffast-math`). A filter loop per sample with its 4–6 coefficient pairs was
four times slower.

Cost per second of signal for the Airspy Mini (12 MHz), one core at
3.8 GHz: format conversion 0.7 %, converter 2.1 %, resampler 5.2 %, AGC
measurement 0.5 %.

The tuner of the Airspy inverts the spectrum (`invertSpectrum`), derived
from the behaviour of libairspy and confirmed with real signals: with the
other orientation no ensemble synchronises. libairspy's code can't be
used, its license is limited to the Airspy ecosystem.

The level of the software AGC is measured on the ADC values (as pairs, so
that `SoftwareAgc` needs no change), with the level of the Airspy input
(-25 dBFS).

## Gain

* LNA and mixer gain with automatic mode (airspy, e4000): automatic, the
  IF gain is the gain of the AGC and of the slider (like the hybrid mode of
  the Airspy input). The gain table is in dB (1 dB per unit of the control;
  airspy: VGA index, nominal 3.4 dB); `SoftwareAgc` learns the real steps.
* Without IF gain: the LNA gain, else the RF gain.
* rtl2832_sdr with R820T/R828D has no gain control: `getGainCount()` is 0,
  the tuner uses its own AGC.

## Known limitations

* **RTL-SDR Blog V4**: mainline `rtl28xxu` configures the R828D with a
  16 MHz crystal, the Blog V4 has 28.8 MHz, and has no support for its
  input switching. The stick receives at 1.8 × the requested frequency
  + 2.86 MHz (crystal ratio 28.8/16, plus 0.8 × the IF of 3.57 MHz).
  Measured: requested 114.022 MHz received the ensemble on 9D
  (208.064 MHz) with a frequency correction of -32 kHz; the formula gives
  208.096 MHz. The samples themselves are fine: such a recording decodes.
  Not worked around in welle.io (no frequency scaling), use the rtl-sdr
  input.
* No bias tee (none of the drivers has a control for it).
* RAW recording only with `CU8` at 2.048 MS/s (the format of the rtl-sdr
  input).

## Tests

| Test | Content |
|---|---|
| `unit.v4l2_formats` | Conversion of every format |
| `unit.real_iq_converter` | Tone at fs/4 + d becomes +d (or -d inverted) with gain 1, mirror below -75 dB, aliases of the second stage below -75 dB, output rates, block sizes don't matter (6, 12, 20 MHz) |
| `hw.v4l2` | The real devices, only with `WELLE_TEST_V4L2=1`: rate 2.048 MS/s ± 2 %, retuning while streaming, higher gain gives a higher noise level, stop and restart. Needs no antenna. |
| `gui.ui.devices` | *Linux SDR (V4L2)* in the device box, settings page, device list, kept after a restart |

```sh
WELLE_TEST_V4L2=1 ctest --test-dir build -L hw --output-on-failure
```

Only one program can stream from a device: don't run `hw.v4l2` while
welle.io uses it.

The tests that may open a real receiver (`gui.ui.airspy`, `gui.ui.devices`,
`hw.v4l2`) share the CTest `RESOURCE_LOCK` `receiver`, so they never run at
the same time. libairspy detaches the kernel driver when it opens an Airspy:
after `gui.ui.airspy` with a connected Airspy, `/dev/swradioN` of the Airspy
is gone until it is plugged in again (seen in the kernel log as
`airspy ...: URB failed -71` if it was streaming).

Status, tested with:

* airspy driver, Airspy Mini: `hw.v4l2` passes (rate 2.048 MS/s, IF gain
  from -53 to -4 dBFS noise level). With an antenna six ensembles were
  received in Band III (5C, 5D, 9D, 11B, 11D, 12D), audio without AAC
  errors.
* rtl2832_sdr driver, RTL-SDR Blog V4: `hw.v4l2` passes; reception only
  with the frequency error above.

Not tested: Airspy R2, hackrf, msi2500, rtl-sdr sticks with R820T.
