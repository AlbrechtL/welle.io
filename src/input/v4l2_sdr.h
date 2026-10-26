/******************************************************************************\
 * Description:
 * Input of SDR devices with a Linux kernel driver (V4L2 SDR capture,
 * /dev/swradioN), e.g. rtl2832_sdr, airspy, hackrf and msi2500. Tuner 0 of
 * the device is the ADC (sample rate), tuner 1 the RF tuner (frequency). The
 * samples are converted to complex float samples at 2.048 MS/s: real
 * samples (airspy) with RealIqConverter, other sample rates with
 * RationalResampler. The gain is set with the RF tuner controls of the
 * driver, the software AGC is the same as for the other inputs.
 *
 * RealIqConverter converts the real samples to complex samples at half the
 * rate. The wanted band is centred at a quarter of the sample rate: the
 * samples are shifted by fs/4 (multiplication by 1, -j, -1, j), low pass
 * filtered with a half-band filter and decimated by 2. The half-band filter
 * has every second coefficient zero, so I needs only the even coefficients
 * and Q only the centre coefficient. From 10 MHz on a second half-band
 * stage decimates by 2 again, which saves more in the resampler than it
 * costs.
 *
 * The sample formats and RealIqConverter don't need a device and are tested
 * on their own (unit.v4l2_formats, unit.real_iq_converter).
 *
 * This file was generated with the help of an AI assistant (Claude Code).
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/
#ifndef V4L2_SDR_H
#define V4L2_SDR_H

#include <atomic>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "virtual_input.h"
#include "dab-constants.h"
#include "MathHelper.h"
#include "ringbuffer.h"
#include "radio-controller.h"
#include "rational_resampler.h"
#include "software_agc.h"

// The formats of linux/videodev2.h (V4L2_SDR_FMT_*) that are supported, in
// the order of preference: complex before real, 8 bit before 16 bit
// (rtl2832_sdr: CU8, hackrf: CS8, msi2500: CS8/CS14LE/CU16LE, airspy: RU12LE)
enum class SdrFormat { CU8, CS8, CU16LE, CS14LE, RU12LE };

namespace V4L2SdrFormats {

// Real formats deliver one value per sample, complex ones two (I and Q)
bool isComplex(SdrFormat format);
size_t bytesPerValue(SdrFormat format);

// Converts bytes (a multiple of the sample size) of a complex format, returns
// the number of samples written to out
size_t convertComplex(SdrFormat format, const uint8_t* data, size_t bytes,
                      std::complex<float>* out);

// Converts bytes of a real format, returns the number of values written to
// out
size_t convertReal(SdrFormat format, const uint8_t* data, size_t bytes,
                   float* out);

}

class RealIqConverter {
public:
    // inputRate: of the real samples. passbandHz: half bandwidth around fs/4
    // that has to stay free of aliases; attenuationDb: of the aliases.
    // invertSpectrum: false: fs/4 + d becomes +d, true: fs/4 + d becomes -d
    // (the tuner inverts the spectrum, e.g. the Airspy).
    RealIqConverter(int inputRate, bool invertSpectrum,
                    double passbandHz = 768e3, double attenuationDb = 80);

    void reset();

    // in: real samples, full scale is an amplitude of 1. Appends the complex
    // samples (count / 2 or count / 4, the rest is kept for the next call)
    // to out and returns their number. A real tone of amplitude 1 gives a
    // complex tone of amplitude 1.
    size_t process(const float* in, size_t count,
                   std::vector<std::complex<float>>& out);

    int outputRate() const { return rate / (secondStage ? 4 : 2); }
    // Output samples until the filters are filled (upper bound)
    int transientSamples() const;

private:
    const int rate;
    const bool invert;

    // Symmetric FIR filter: the even coefficients of a half-band filter,
    // applied per block (the loop over the samples can be vectorised)
    struct Fir {
        std::vector<float> coefficients;
        std::vector<float> work;  // the last inputs, then the new ones
        void reset();
        void process(const float* in, size_t n, float* out);
    };
    // Delay line for the centre coefficient of a half-band filter
    struct Delay {
        int length = 0;
        std::vector<float> work;
        void reset();
        void process(const float* in, size_t n, float* out);
    };

    // First stage: I is the filtered even sample, Q the delayed odd one
    Fir fir1;
    Delay delay1;
    // Number of pairs so far, modulo 2 (sign of the shift)
    int pairIndex = 0;
    bool hasPending = false;
    float pending = 0;

    // Second stage: complex half-band filter, the even samples are filtered,
    // the odd ones delayed
    bool secondStage = false;
    Fir fir2Re, fir2Im;
    Delay delay2Re, delay2Im;
    bool hasPending2 = false;
    std::complex<float> pending2;

    // Buffers of process()
    std::vector<float> evens, odds, filtered, delayed;
    std::vector<float> evensRe, evensIm, oddsRe, oddsIm, filteredRe, filteredIm, delayedRe, delayedIm;
};

class CV4L2Sdr : public CVirtualInput {
public:
    struct Info {
        std::string path;    // e.g. /dev/swradio0
        std::string card;    // e.g. "AirSpy SDR"
        std::string driver;  // e.g. "airspy"
    };

    // The SDR capture devices of the system, sorted by number
    static std::vector<Info> listDevices();

    // path: e.g. /dev/swradio1, empty: the first device that can be used.
    // Throws if no device can be opened.
    CV4L2Sdr(RadioControllerInterface& radioController, const std::string& path = "");
    ~CV4L2Sdr(void);
    CV4L2Sdr(const CV4L2Sdr&) = delete;
    CV4L2Sdr& operator=(const CV4L2Sdr&) = delete;

    // Opens another device (empty: the first one that can be used). Returns
    // false if that fails; then no device is open.
    bool selectDevice(const std::string& path);
    std::string devicePath() const { return path; }

    // Interface methods
    void setFrequency(int frequency);
    int getFrequency(void) const;
    bool restart(void);
    bool is_ok(void);
    void stop(void);
    void reset(void);
    int32_t getSamples(DSPCOMPLEX* buffer, int32_t size);
    std::vector<DSPCOMPLEX> getSpectrumSamples(int size);
    int32_t getSamplesToRead(void);
    float getGain(void) const;
    int getGainIndex(void) const;
    float setGain(int gain);
    int getGainCount(void);
    void setAgc(bool agc);
    std::string getDescription(void);

    CDeviceID getID(void);
    CRecordFormat recordFormat(void);

private:
    struct MappedBuffer {
        void* start = nullptr;
        size_t length = 0;
    };

    RadioControllerInterface& radioController;

    // Device, set up by openDevice()
    int fd = -1;
    std::string path;
    std::string card;
    std::string driver;
    SdrFormat format = SdrFormat::CU8;
    int adcRate = 0;            // samples (complex) or values (real) per second
    unsigned rfUnitHz = 1;      // unit of the RF frequency

    // Gain control (0: none) and the gain table of its values
    uint32_t gainControl = 0;
    int gainMin = 0;
    int gainStep = 1;
    std::vector<float> gainsDb;
    std::atomic<int> currentGain{0};
    std::atomic<bool> isAgc{false};
    std::mutex gainMutex;
    std::unique_ptr<SoftwareAgc> agc;
    std::thread agcThread;

    int frequency = kHz(174928);

    // Streaming
    std::vector<MappedBuffer> buffers;
    std::thread readerThread;
    std::atomic<bool> running{false};
    std::atomic<bool> unplugged{false};
    // The Airspy driver reports a fixed ADC rate, the real one is measured
    // at the first start
    bool rateMeasured = true;

    // Sample processing (reader thread)
    std::unique_ptr<RealIqConverter> converter;
    std::unique_ptr<RationalResampler> resampler;
    std::vector<float> realValues;
    std::vector<DSPCOMPLEX> complexSamples;
    std::vector<DSPCOMPLEX> resampled;
    RingBuffer<DSPCOMPLEX> sampleBuffer;
    RingBuffer<DSPCOMPLEX> spectrumSampleBuffer;

    void openDevice(const std::string& devicePath);
    void closeDevice();
    void setupFormat();
    void setupAdcRate();
    void setupRfTuner();
    void setupGain();
    void setupProcessing();
    bool startStreaming();
    void stopStreaming();
    void readLoop();
    void process(const uint8_t* data, size_t bytes);
    void measureAdcRate();
    void applyGain(int index);
    void agcTimerThread();
};

#endif
