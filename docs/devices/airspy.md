# Airspy

The [Airspy](https://airspy.com/) is a low cost, high performance SDR that
performs better than the well known rtl-sdr.

The following devices are supported:

* Airspy R2
* Airspy Mini

Not supported, because their bandwidth is too low:

* Airspy HF+
* Airspy HF+ Discovery

If you start welle.io without any options, welle.io detects the Airspy
automatically. After enabling the *Expert mode* you can check if welle.io
detected the device successfully.

![welle.io with an Airspy](../images/welle-io-airspy.png)

## Gain

With *Automatic RF gain* (receiver settings, or the gain card of the expert
view) welle.io sets the gain of the Airspy so that the signal at the
converter stays clearly above its noise but far from overload. Without it,
the gain slider sets the gain by hand.

The *Gain mode* in the Airspy settings selects which gain of the Airspy is
set, by the automatic gain as well as by the slider:

* **Linearity** (default): the gain of LNA, mixer and IF amplifier together,
  in 22 steps, keeping the front end linear. The mode Airspy recommends to
  start with.
* **Sensitivity**: the same, but with more LNA gain earlier, for weak
  signals.
* **Hybrid (tuner AGC)**: the tuner controls the LNA and mixer gain itself,
  welle.io sets the IF gain (16 steps).

If the reception isn't stable, compare the gain modes.
