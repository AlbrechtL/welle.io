# Backend implementation remarks

welle-io can be roughly partitioned into a frontend part (Qt GUI, or welle-cli command-line or web-based frontend) and a backend part, where all the signal processing for the reception is done. The backend can be instantiated with several options, some of which visible through the GUI.

These options are defined in the `struct RadioRecevierOptions` in `src/radio-receiver-options.h`:

## FFT Window Placement

`int ofdmProcessorThreshold`

After finding the NULL symbol, the backend contains two algorithms for deciding where to place the FFT window used to demodulate the OFDM signal. Reminder: In DAB Transmission Mode 1, the FFT size is 2048, and the cyclic prefix has length 504 (at the native sample rate of 2048000 samples per second) which implies that the time difference between the first and last signal (in a multipath or single-frequency network scenario) reaching the receiver can be up to 504/2048000=246 us. For an intersymbol interference-free reception, the FFT window must be placed such that it encompasses all the signal components or a single symbol.

The original algorithm does a correlation with the phase reference symbol, and chooses the strongest component for the FFT window placement. It skips a frame if the strongest component is not above a given threshold compared to the average value, so as to avoid locking in the absence of a signal. The problem of this approach is that it ignores an earlier but weaker signal, and places the FFT window in a way that creates inter-symbol interference. When this happens, you will see: 1) a strong signal over noise ratio, 2) at least two peaks in the channel impulse response, the first one being smaller than the second, 3) a noisy constellation plot.

The new algorithm tries to find the first peak instead, and will not show the ISI problems, however, it might take longer to acquire lock on some signals.


## TII decoding

`bool decodeTII`

The backend can detect and identify the TII signal in the NULL symbol. Since this is not needed for normal DAB reception, it is disabled to save CPU resources.

When enabled, the Qt GUI will print it out to the console using `qDebug`.

**What does the km stand for?**

km is calculated from the delay, which is measured in samples. One sample is 1/2048000 seconds. However, the measurement itself is not robust yet, and I've noticed it was wrong in many cases. What's also missing is a good criterion to decide how good a measurement is. If you look at the error, you'll see that the value is more or less the same for all entries. And sometimes you get loads of inexisting comb/pattern pairs.

Also, keep in mind it is not an absolute distance measurement! It is a relative measurement of time between several transmitters, converted to it's equivalent in distance. The receiver doesn't know where the transmitters are.

## Coarse Frequency Corrector
`bool disable_coarse_corrector`

Hardware dongles with an accurate clock (less than 100Hz frequency error on the frequency being received) do not need the coarse frequency corrector, which works in 1kHz steps. Disabling it can accelerate lock.

When enabled, the `FreqsyncMethod freqsyncMethod` selects which algorithm is used to find the coarse corrector. The algorithms are explained in `OFDMProcessor::processPRS()`
