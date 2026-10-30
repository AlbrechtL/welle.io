# welle-pi volume

welle-pi changes the volume of the sound card when an input source sends
`VOLUME_UP` or `VOLUME_DOWN`, and mutes it on `MUTE`: the terminal sends
the volume from `+`, `=` and `-`, a remote control plugin from its volume and
mute keys (see [welle-pi input sources](welle-pi-input.md)). Where those
actions come from does not matter, they all end in the same place.

## The card, not the audio

welle-pi does not scale the decoded audio: it turns the mixer of the PCM
device the audio output plays on (`-o`, `default` by default), the same
control `alsamixer` shows as `Master`, `PCM` or `Digital`. That keeps the
audio path untouched (no gain applied to samples, no clipping, no second
place that can get the level wrong), it follows the card when the sound is
taken from headphones or a line output, and the card remembers the level
itself.

`libasound` is already linked for the audio output, so this adds no
dependency.

## Configuration

The `"volume"` entry of `welle-pi.json` (same file as `"input"` and
`"display"`, see [Configuration file](welle-pi-input.md#configuration-file)):

```json
{
    "volume": {"element": "Digital", "step": 5}
}
```

| Entry | Meaning | Default |
|---|---|---|
| `element` | Mixer element to turn, by the name `alsamixer` shows | the first element of the device with a playback volume |
| `step` | Percent of the element's range per key press | `5` |

Both are optional. `element` is worth setting on cards where the first
element is not the one that carries the sound, for instance a card with
`Mic`, `PCM` and `Master`. A name without a playback volume is logged and
the first usable element is taken instead. Without `"volume"`, or without
the file, the defaults apply.

## Behaviour

* One key press is one step of `step` percent of the element's range,
  always at least one raw step, clamped at the ends. Pressing up and down
  again returns exactly to the level before, because the step is computed
  from the raw value of the control, not from a percentage.
* The element is unmuted while changing it: a muted card would otherwise
  stay silent whatever the level says.
* The new level goes to stderr (`Volume: 45 %`), like the mute state
  (`Muted`, `Unmuted`), and with it to the journal. Instead of the DLS, the
  last line of the screen is the level while the keys are used and for a
  second after the last one: the whole line is a bar of U+2588 blocks,
  e.g. at 45 % on 16 columns (the rest stays empty):

      ███████

  (a level above zero keeps one block, so that the line never looks like a
  screen without the level). `volumeLine()` in `ui_manager.cpp` builds it,
  `UIManager::showVolume()` gets it from the main loop, so the input and
  display plugins do not have to know about each other. While muted the
  line is `Muted`. The framebuffer plugin and the terminal have U+2588; the
  character LCD has no block in its ROM, so the LCD plugin defines one as a
  user defined character (CGRAM) and the bar is made of those.
* A level changed with `alsamixer` or `amixer` between presses is picked up
  before the next step.
* A device the mixer cannot attach to is reported once (`volume: cannot
  attach a mixer to 'hw:2': …`), and the volume and mute keys do nothing. A
  `plughw:` device is tried again as the `hw:` device of the same card, as
  the control interface has no plug layer. Cards without a playback volume
  control are reported the same way.

## Mute

`MUTE` toggles the playback switch of the same element: the card is silent
and `alsamixer` shows `[off]`. The state therefore belongs to the card and
not to a level of zero that the next volume press would undo; a volume key
takes the mute off again.

Not every element has a switch; one without it reports
`volume: 'PCM' on 'default' has no mute control` once and does nothing,
rather than pretending. Outputs that have neither a volume nor a switch can
be given both with `softvol` (below).

This is also what makes a pair of rotary encoders a complete control: the
detents of the first are `VOLUME_UP` and `VOLUME_DOWN`, its push button is
`MUTE`, the detents of the second are `UP` and `DOWN`, its button `ENTER`.

## Persistence

Nothing to store in welle-pi: the card keeps the level and the mute state,
and `alsa-restore` puts both back at boot. After setting the level with the
remote for the first time, save it once:

```sh
sudo alsactl store
```

If the image has the service disabled, enable it (`systemctl enable
alsa-restore`); without it the level is back to the card default after a
reboot.

## Cards without a control

Some outputs have a fixed level and no mixer element to turn, and no switch
to mute (HDMI is the usual case). ALSA's `softvol` plugin adds a control in
front of such a device, which the volume and mute keys then find. Add to
`/etc/asound.conf`:

```
pcm.softvol {
    type softvol
    slave.pcm "hw:0,0"
    control {
        name "Softvol"
        card 0
    }
}
```

and start welle-pi with `-o softvol`. The level is applied by ALSA before
the samples reach the card; nothing else changes.

## Code

`VolumeControl` in `src/welle-pi/volume.{hpp,cpp}`, created next to the
audio output in `welle-pi.cpp`, handled in the main loop with the other
input actions. The step arithmetic (`volumeStep()`) is separate from the
mixer and covered by `unit.volume`
(`src/tests/volume_tests.cpp`); the mixer itself needs a sound card and is
checked on the device.
