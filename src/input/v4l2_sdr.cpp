/******************************************************************************\
 * Description:
 * Input of SDR devices with a Linux kernel driver (V4L2 SDR capture), see
 * v4l2_sdr.h.
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

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include <dirent.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <linux/videodev2.h>

#include "v4l2_sdr.h"

// For Qt translation if Qt is existing
#ifdef QT_CORE_LIB
    #include <QtGlobal>
#else
    #define QT_TRANSLATE_NOOP(x,y) (y)
#endif

/********************
 * Sample formats   *
 ********************/

namespace {

// 16 bit little endian, independent of the byte order of the host
inline uint16_t le16(const uint8_t* p)
{
    return uint16_t(p[0] | (p[1] << 8));
}

// Size and value with full scale 1 of one value of each format
struct CU8 {
    static constexpr size_t step = 1;
    static float value(const uint8_t* p) { return (int(p[0]) - 128) / 128.0f; }
};
struct CS8 {
    static constexpr size_t step = 1;
    static float value(const uint8_t* p) { return int8_t(p[0]) / 128.0f; }
};
struct CU16LE {
    static constexpr size_t step = 2;
    static float value(const uint8_t* p) { return (int(le16(p)) - 32768) / 32768.0f; }
};
// 14 bit in the low bits, sign extended (the two high bits are padding)
struct CS14LE {
    static constexpr size_t step = 2;
    static float value(const uint8_t* p) { return (int16_t(le16(p) << 2) >> 2) / 8192.0f; }
};
// 12 bit unsigned in the low bits
struct RU12LE {
    static constexpr size_t step = 2;
    static float value(const uint8_t* p) { return (int(le16(p) & 0x0fff) - 2048) / 2048.0f; }
};

template <typename F>
size_t complexLoop(const uint8_t* data, size_t bytes, std::complex<float>* out)
{
    const size_t samples = bytes / (2 * F::step);
    for (size_t i = 0; i < samples; i++) {
        const uint8_t* p = data + 2 * F::step * i;
        out[i] = { F::value(p), F::value(p + F::step) };
    }
    return samples;
}

}

namespace V4L2SdrFormats {

bool isComplex(SdrFormat format)
{
    return format != SdrFormat::RU12LE;
}

size_t bytesPerValue(SdrFormat format)
{
    return (format == SdrFormat::CU8 || format == SdrFormat::CS8) ? 1 : 2;
}

size_t convertComplex(SdrFormat format, const uint8_t* data, size_t bytes,
                      std::complex<float>* out)
{
    switch (format) {
        case SdrFormat::CU8: return complexLoop<CU8>(data, bytes, out);
        case SdrFormat::CS8: return complexLoop<CS8>(data, bytes, out);
        case SdrFormat::CU16LE: return complexLoop<CU16LE>(data, bytes, out);
        case SdrFormat::CS14LE: return complexLoop<CS14LE>(data, bytes, out);
        case SdrFormat::RU12LE: break;
    }
    return 0;
}

size_t convertReal(SdrFormat format, const uint8_t* data, size_t bytes,
                   float* out)
{
    // RU12LE is the only real format
    if (format != SdrFormat::RU12LE)
        return 0;

    const size_t values = bytes / 2;
    for (size_t i = 0; i < values; i++) {
        out[i] = RU12LE::value(data + 2 * i);
    }
    return values;
}

}

/***********************************
 * Real to complex (RealIqConverter) *
 ***********************************/

// Modified Bessel function of the first kind, order 0 (for the Kaiser window)
static double besselI0(double x)
{
    double sum = 1;
    double term = 1;
    for (int k = 1; k < 50; k++) {
        term *= (x / (2 * k)) * (x / (2 * k));
        sum += term;
    }
    return sum;
}

// Even coefficients of a half-band filter (cut-off at fs/4, every second
// coefficient zero, centre coefficient 0.5) that keeps +-passbandHz free of
// aliases after the decimation by 2: the decimation folds fs/2 - f onto -f,
// so the stop band starts at fs/2 - passbandHz. The even coefficients sum up
// to 0.5 like the centre coefficient, so that the gain is 1 at 0 and 0.5 at
// fs/4. delay: of the centre coefficient in output samples.
static void halfbandCoefficients(double rate, double passbandHz, double attenuationDb,
                                 std::vector<float>& coefficients, int& delay)
{
    const double transition = (rate / 2.0 - 2 * passbandHz) / rate;
    if (transition <= 0) {
        throw std::invalid_argument("RealIqConverter: pass band too wide for the sample rate");
    }

    // Length from the Kaiser formula, rounded up to 4 * m + 3: then the
    // centre coefficient is at an odd position and the coefficients at the
    // even positions are the non-zero ones
    const int minLength = int(std::ceil((attenuationDb - 7.95) / (14.36 * transition))) + 1;
    const int m = (std::max(0, minLength - 3) + 3) / 4;
    const int N = 4 * m + 3;
    const int centre = (N - 1) / 2;
    const double beta = 0.1102 * (attenuationDb - 8.7);

    // h[n] = 0.5 * sinc((n - centre) / 2)
    const int T = 2 * m + 2;
    std::vector<double> h(T);
    double sum = 0;
    for (int j = 0; j < T; j++) {
        const int n = 2 * j;
        const double x = (n - centre) / 2.0;
        const double sinc = std::sin(M_PI * x) / (M_PI * x);
        const double r = 2.0 * n / (N - 1) - 1;
        const double window = besselI0(beta * std::sqrt(std::max(0.0, 1 - r * r))) / besselI0(beta);
        h[j] = 0.5 * sinc * window;
        sum += h[j];
    }

    coefficients.resize(T);
    for (int j = 0; j < T; j++) {
        coefficients[j] = float(h[j] * 0.5 / sum);
    }

    // y[2k] = sum of h[n] * x[2k - n]: the centre coefficient takes the odd
    // sample 2k - centre, which is the odd sample of the pair k - (m + 1)
    delay = m + 1;
}

RealIqConverter::RealIqConverter(int inputRate, bool invertSpectrum,
        double passbandHz, double attenuationDb) :
    rate(inputRate),
    invert(invertSpectrum)
{
    halfbandCoefficients(inputRate, passbandHz, attenuationDb, fir1.coefficients, delay1.length);
    // The gain of 2 restores the amplitude of the positive frequencies of a
    // real signal; the centre coefficient becomes 1
    for (float& c : fir1.coefficients) {
        c *= 2;
    }

    // The second stage as long as the output stays above 2.5 MS/s
    if (inputRate / 4 >= 2500000) {
        secondStage = true;
        halfbandCoefficients(inputRate / 2.0, passbandHz, attenuationDb, fir2Re.coefficients, delay2Re.length);
        fir2Im.coefficients = fir2Re.coefficients;
        delay2Im.length = delay2Re.length;
    }

    reset();
}

void RealIqConverter::Fir::reset()
{
    work.assign(coefficients.size() - 1, 0.0f);
}

// out[i] = sum of c[j] * x[i - j] with the symmetric coefficients: the loop
// over the coefficients is outside, the one over the samples inside
void RealIqConverter::Fir::process(const float* in, size_t n, float* out)
{
    const size_t T = coefficients.size();
    work.resize(T - 1 + n);
    std::copy(in, in + n, work.begin() + (T - 1));

    const float* w = work.data();
    std::fill(out, out + n, 0.0f);
    for (size_t k = 0; k < T / 2; k++) {
        const float c = coefficients[k];
        const float* __restrict a = w + k;
        const float* __restrict b = w + (T - 1 - k);
        float* __restrict o = out;
        for (size_t i = 0; i < n; i++) {
            o[i] += c * (a[i] + b[i]);
        }
    }

    std::copy(work.end() - (T - 1), work.end(), work.begin());
    work.resize(T - 1);
}

void RealIqConverter::Delay::reset()
{
    work.assign(length, 0.0f);
}

void RealIqConverter::Delay::process(const float* in, size_t n, float* out)
{
    work.insert(work.end(), in, in + n);
    std::copy(work.begin(), work.begin() + n, out);
    work.erase(work.begin(), work.begin() + n);
}

void RealIqConverter::reset()
{
    fir1.reset();
    delay1.reset();
    pairIndex = 0;
    hasPending = false;
    pending = 0;

    if (secondStage) {
        fir2Re.reset();
        fir2Im.reset();
        delay2Re.reset();
        delay2Im.reset();
    }
    hasPending2 = false;
}

int RealIqConverter::transientSamples() const
{
    // Filter lengths in output samples, rounded up
    const int length1 = 2 * int(fir1.coefficients.size()) - 1;
    const int length2 = secondStage ? 2 * int(fir2Re.coefficients.size()) - 1 : 0;
    return length1 / (secondStage ? 4 : 2) + length2 / 2 + 2;
}

size_t RealIqConverter::process(const float* in, size_t count,
        std::vector<std::complex<float>>& result)
{
    const size_t before = result.size();

    // Pairs of (even, odd) samples, shifted by -fs/4: sample n is multiplied
    // by exp(-j * pi * n / 2), i.e. by 1, -j, -1, j. The even sample becomes
    // real, the odd one imaginary. An odd sample of the last call starts the
    // first pair.
    const size_t pairs = (count + (hasPending ? 1 : 0)) / 2;
    evens.resize(pairs);
    odds.resize(pairs);
    size_t used = 0;
    for (size_t j = 0; j < pairs; j++) {
        float even;
        if (j == 0 && hasPending) {
            even = pending;
            hasPending = false;
        }
        else {
            even = in[used++];
        }
        const float odd = in[used++];
        const float sign = ((pairIndex + j) & 1) ? -1.0f : 1.0f;
        evens[j] = sign * even;
        odds[j] = -sign * odd;
    }
    pairIndex = int((pairIndex + pairs) & 1);
    if (used < count) {
        pending = in[used];
        hasPending = true;
    }

    filtered.resize(pairs);
    delayed.resize(pairs);
    fir1.process(evens.data(), pairs, filtered.data());
    delay1.process(odds.data(), pairs, delayed.data());
    const float qSign = invert ? -1.0f : 1.0f;

    if (!secondStage) {
        result.resize(before + pairs);
        for (size_t i = 0; i < pairs; i++) {
            result[before + i] = { filtered[i], qSign * delayed[i] };
        }
        return pairs;
    }

    // Second stage on the complex samples (filtered, qSign * delayed)
    const size_t pairs2 = (pairs + (hasPending2 ? 1 : 0)) / 2;
    evensRe.resize(pairs2);
    evensIm.resize(pairs2);
    oddsRe.resize(pairs2);
    oddsIm.resize(pairs2);
    size_t next = 0;
    for (size_t j = 0; j < pairs2; j++) {
        std::complex<float> even;
        if (j == 0 && hasPending2) {
            even = pending2;
            hasPending2 = false;
        }
        else {
            even = { filtered[next], qSign * delayed[next] };
            next++;
        }
        evensRe[j] = even.real();
        evensIm[j] = even.imag();
        oddsRe[j] = filtered[next];
        oddsIm[j] = qSign * delayed[next];
        next++;
    }
    if (next < pairs) {
        pending2 = { filtered[next], qSign * delayed[next] };
        hasPending2 = true;
    }

    filteredRe.resize(pairs2);
    filteredIm.resize(pairs2);
    delayedRe.resize(pairs2);
    delayedIm.resize(pairs2);
    fir2Re.process(evensRe.data(), pairs2, filteredRe.data());
    fir2Im.process(evensIm.data(), pairs2, filteredIm.data());
    delay2Re.process(oddsRe.data(), pairs2, delayedRe.data());
    delay2Im.process(oddsIm.data(), pairs2, delayedIm.data());

    // The centre coefficient is 0.5
    result.resize(before + pairs2);
    for (size_t i = 0; i < pairs2; i++) {
        result[before + i] = { filteredRe[i] + 0.5f * delayedRe[i], filteredIm[i] + 0.5f * delayedIm[i] };
    }
    return pairs2;
}

/********************
 * Device           *
 ********************/

namespace {

const unsigned bufferCount = 8;

// Known real ADC rates of the Airspy: R2 20 and 5 MHz, Mini 12 and 6 MHz
const int airspyRates[] = { 20000000, 12000000, 6000000, 5000000 };

int xioctl(int fd, unsigned long request, void* arg)
{
    int ret;
    do {
        ret = ioctl(fd, request, arg);
    } while (ret == -1 && errno == EINTR);
    return ret;
}

std::string errorText()
{
    return std::string(std::strerror(errno));
}

bool formatOf(uint32_t fourcc, SdrFormat& format)
{
    switch (fourcc) {
        case V4L2_SDR_FMT_CU8: format = SdrFormat::CU8; return true;
        case V4L2_SDR_FMT_CS8: format = SdrFormat::CS8; return true;
        case V4L2_SDR_FMT_CU16LE: format = SdrFormat::CU16LE; return true;
        case V4L2_SDR_FMT_CS14LE: format = SdrFormat::CS14LE; return true;
        case V4L2_SDR_FMT_RU12LE: format = SdrFormat::RU12LE; return true;
        default: return false;
    }
}

// Capabilities of the device node, false if it isn't an SDR receiver
bool isSdrReceiver(int fd, v4l2_capability& cap)
{
    std::memset(&cap, 0, sizeof(cap));
    if (xioctl(fd, VIDIOC_QUERYCAP, &cap) != 0) {
        return false;
    }
    const uint32_t caps = (cap.capabilities & V4L2_CAP_DEVICE_CAPS) ? cap.device_caps : cap.capabilities;
    const uint32_t needed = V4L2_CAP_SDR_CAPTURE | V4L2_CAP_TUNER | V4L2_CAP_STREAMING;
    return (caps & needed) == needed;
}

}

std::vector<CV4L2Sdr::Info> CV4L2Sdr::listDevices()
{
    std::vector<int> numbers;
    if (DIR* dir = opendir("/dev")) {
        while (const dirent* entry = readdir(dir)) {
            int n;
            char rest;
            if (std::sscanf(entry->d_name, "swradio%d%c", &n, &rest) == 1) {
                numbers.push_back(n);
            }
        }
        closedir(dir);
    }
    std::sort(numbers.begin(), numbers.end());

    std::vector<Info> devices;
    for (const int n : numbers) {
        const std::string devicePath = "/dev/swradio" + std::to_string(n);
        const int f = open(devicePath.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
        if (f < 0) {
            std::clog << "V4L2: " << devicePath << ": " << errorText() << std::endl;
            continue;
        }
        v4l2_capability cap;
        if (isSdrReceiver(f, cap)) {
            devices.push_back({ devicePath,
                                reinterpret_cast<const char*>(cap.card),
                                reinterpret_cast<const char*>(cap.driver) });
        }
        close(f);
    }
    return devices;
}

CV4L2Sdr::CV4L2Sdr(RadioControllerInterface& radioController, const std::string& devicePath) :
    radioController(radioController),
    sampleBuffer(512 * 1024),
    spectrumSampleBuffer(8192)
{
    if (!selectDevice(devicePath)) {
        throw std::runtime_error("V4L2: no usable device");
    }
}

CV4L2Sdr::~CV4L2Sdr(void)
{
    stop();
    closeDevice();
}

bool CV4L2Sdr::selectDevice(const std::string& devicePath)
{
    stop();
    closeDevice();

    std::vector<std::string> candidates;
    if (devicePath.empty()) {
        for (const auto& info : listDevices()) {
            candidates.push_back(info.path);
        }
    }
    else {
        candidates.push_back(devicePath);
    }

    for (const auto& candidate : candidates) {
        try {
            openDevice(candidate);
            return true;
        }
        catch (const std::exception& e) {
            std::clog << "V4L2: " << e.what() << std::endl;
            closeDevice();
        }
    }

    std::clog << "V4L2: no usable SDR device" << std::endl;
    return false;
}

void CV4L2Sdr::openDevice(const std::string& devicePath)
{
    fd = open(devicePath.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        throw std::runtime_error(devicePath + ": " + errorText());
    }
    path = devicePath;

    v4l2_capability cap;
    if (!isSdrReceiver(fd, cap)) {
        throw std::runtime_error(devicePath + ": not an SDR receiver");
    }
    card = reinterpret_cast<const char*>(cap.card);
    driver = reinterpret_cast<const char*>(cap.driver);
    std::clog << "V4L2: open " << path << " (" << card << ", driver " << driver << ")" << std::endl;

    setupFormat();
    setupAdcRate();
    setupRfTuner();
    setupGain();
    setupProcessing();

    unplugged = false;
}

void CV4L2Sdr::closeDevice()
{
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
    gainControl = 0;
    gainsDb.clear();
}

void CV4L2Sdr::setupFormat()
{
    // The most preferred format of the device (order of SdrFormat)
    bool found = false;
    uint32_t fourcc = 0;
    for (uint32_t i = 0; ; i++) {
        v4l2_fmtdesc desc;
        std::memset(&desc, 0, sizeof(desc));
        desc.index = i;
        desc.type = V4L2_BUF_TYPE_SDR_CAPTURE;
        if (xioctl(fd, VIDIOC_ENUM_FMT, &desc) != 0) {
            break;
        }
        SdrFormat f;
        if (formatOf(desc.pixelformat, f) && (!found || f < format)) {
            format = f;
            fourcc = desc.pixelformat;
            found = true;
        }
    }
    if (!found) {
        throw std::runtime_error(path + ": no supported sample format");
    }

    v4l2_format fmt;
    std::memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_SDR_CAPTURE;
    fmt.fmt.sdr.pixelformat = fourcc;
    if (xioctl(fd, VIDIOC_S_FMT, &fmt) != 0) {
        throw std::runtime_error(path + ": VIDIOC_S_FMT: " + errorText());
    }
    std::clog << "V4L2: format " << std::string(reinterpret_cast<const char*>(&fourcc), 4)
              << ", buffer size " << fmt.fmt.sdr.buffersize << std::endl;
}

void CV4L2Sdr::setupAdcRate()
{
    v4l2_tuner tuner;
    std::memset(&tuner, 0, sizeof(tuner));
    tuner.index = 0;
    if (xioctl(fd, VIDIOC_G_TUNER, &tuner) != 0 || tuner.type != V4L2_TUNER_ADC) {
        throw std::runtime_error(path + ": no ADC tuner");
    }
    const unsigned unit = (tuner.capability & V4L2_TUNER_CAP_1HZ) ? 1 : 62500;

    // Real samples need twice the rate. The rate closest above it if the
    // device can't do it exactly.
    const int64_t wanted = V4L2SdrFormats::isComplex(format) ? INPUT_RATE : 2 * INPUT_RATE;
    int64_t best = 0;
    for (uint32_t i = 0; ; i++) {
        v4l2_frequency_band band;
        std::memset(&band, 0, sizeof(band));
        band.tuner = 0;
        band.type = V4L2_TUNER_ADC;
        band.index = i;
        if (xioctl(fd, VIDIOC_ENUM_FREQ_BANDS, &band) != 0) {
            break;
        }
        const int64_t low = int64_t(band.rangelow) * unit;
        const int64_t high = int64_t(band.rangehigh) * unit;
        if (high < wanted) {
            continue;
        }
        const int64_t candidate = std::max(low, wanted);
        if (best == 0 || candidate < best) {
            best = candidate;
        }
    }
    if (best == 0) {
        throw std::runtime_error(path + ": sample rate below " + std::to_string(wanted));
    }

    v4l2_frequency freq;
    std::memset(&freq, 0, sizeof(freq));
    freq.tuner = 0;
    freq.type = V4L2_TUNER_ADC;
    freq.frequency = uint32_t(best / unit);
    if (xioctl(fd, VIDIOC_S_FREQUENCY, &freq) != 0) {
        throw std::runtime_error(path + ": setting the sample rate: " + errorText());
    }
    if (xioctl(fd, VIDIOC_G_FREQUENCY, &freq) != 0) {
        throw std::runtime_error(path + ": reading the sample rate: " + errorText());
    }
    adcRate = int(int64_t(freq.frequency) * unit);

    // The airspy driver doesn't set the sample rate of the device, it
    // reports a fixed value
    rateMeasured = (driver != "airspy");

    std::clog << "V4L2: ADC rate " << adcRate << (rateMeasured ? "" : " (to be measured)") << std::endl;
}

void CV4L2Sdr::setupRfTuner()
{
    v4l2_tuner tuner;
    std::memset(&tuner, 0, sizeof(tuner));
    tuner.index = 1;
    if (xioctl(fd, VIDIOC_G_TUNER, &tuner) != 0 || tuner.type != V4L2_TUNER_RF) {
        throw std::runtime_error(path + ": no RF tuner");
    }
    rfUnitHz = (tuner.capability & V4L2_TUNER_CAP_1HZ) ? 1 : 62500;
    std::clog << "V4L2: RF tuner " << reinterpret_cast<const char*>(tuner.name) << ", "
              << uint64_t(tuner.rangelow) * rfUnitHz << " - "
              << uint64_t(tuner.rangehigh) * rfUnitHz << " Hz" << std::endl;
}

void CV4L2Sdr::setupGain()
{
    auto query = [this](uint32_t id, v4l2_query_ext_ctrl& q) {
        std::memset(&q, 0, sizeof(q));
        q.id = id;
        return xioctl(fd, VIDIOC_QUERY_EXT_CTRL, &q) == 0
               && !(q.flags & V4L2_CTRL_FLAG_DISABLED);
    };
    auto set = [this](uint32_t id, int value) {
        v4l2_ext_control c;
        std::memset(&c, 0, sizeof(c));
        c.id = id;
        c.value = value;
        v4l2_ext_controls cs;
        std::memset(&cs, 0, sizeof(cs));
        cs.which = V4L2_CTRL_WHICH_CUR_VAL;
        cs.count = 1;
        cs.controls = &c;
        return xioctl(fd, VIDIOC_S_EXT_CTRLS, &cs) == 0;
    };

    // The automatic LNA and mixer gain of the tuner (like the hybrid mode of
    // the Airspy input), the IF gain is the gain of the AGC and of the
    // manual gain. Without IF gain the LNA or RF gain.
    v4l2_query_ext_ctrl q;
    gainControl = 0;
    for (const uint32_t id : { V4L2_CID_RF_TUNER_IF_GAIN, V4L2_CID_RF_TUNER_LNA_GAIN, V4L2_CID_RF_TUNER_RF_GAIN }) {
        if (query(id, q) && q.maximum > q.minimum) {
            gainControl = id;
            gainMin = int(q.minimum);
            gainStep = int(std::max<uint64_t>(1, q.step));
            break;
        }
    }

    if (gainControl != V4L2_CID_RF_TUNER_LNA_GAIN && query(V4L2_CID_RF_TUNER_LNA_GAIN_AUTO, q)) {
        set(V4L2_CID_RF_TUNER_LNA_GAIN_AUTO, 1);
    }
    if (query(V4L2_CID_RF_TUNER_MIXER_GAIN_AUTO, q)) {
        set(V4L2_CID_RF_TUNER_MIXER_GAIN_AUTO, 1);
    }

    gainsDb.clear();
    if (gainControl != 0) {
        query(gainControl, q);
        const int count = int((q.maximum - q.minimum) / gainStep) + 1;
        // The drivers give the gain in dB, except airspy (index of the VGA,
        // nominal 3.4 dB, see CAirspy). The AGC learns the real steps.
        const float stepDb = (driver == "airspy") ? 3.4f : float(gainStep);
        for (int i = 0; i < count; i++) {
            gainsDb.push_back(i * stepDb);
        }
        std::clog << "V4L2: gain control " << reinterpret_cast<const char*>(q.name)
                  << " " << q.minimum << " - " << q.maximum << std::endl;

        // Start in the middle of the gain range
        currentGain = getGainCount() / 2;
        applyGain(currentGain);
    }
    else {
        std::clog << "V4L2: no gain control, the tuner uses its own AGC" << std::endl;
    }
}

void CV4L2Sdr::setupProcessing()
{
    int iqRate = adcRate;
    converter.reset();
    if (!V4L2SdrFormats::isComplex(format)) {
        // The Airspy tuner inverts the spectrum
        converter = std::make_unique<RealIqConverter>(adcRate, driver == "airspy");
        iqRate = converter->outputRate();
    }

    resampler.reset();
    if (iqRate != INPUT_RATE) {
        resampler = std::make_unique<RationalResampler>(iqRate, INPUT_RATE);
    }

    // The level of real samples is measured on the ADC values, two per
    // "sample". For the Airspy the level of the Airspy input (see CAirspy).
    if (V4L2SdrFormats::isComplex(format)) {
        agc = std::make_unique<SoftwareAgc>(200, adcRate);
    }
    else {
        agc = std::make_unique<SoftwareAgc>(200, adcRate / 2, -25.0f);
    }
}

void CV4L2Sdr::setFrequency(int newFrequency)
{
    frequency = newFrequency;
    if (fd < 0) {
        return;
    }

    v4l2_frequency freq;
    std::memset(&freq, 0, sizeof(freq));
    freq.tuner = 1;
    freq.type = V4L2_TUNER_RF;
    freq.frequency = uint32_t(uint64_t(frequency) / rfUnitHz);
    if (xioctl(fd, VIDIOC_S_FREQUENCY, &freq) != 0) {
        std::clog << "V4L2: setting the frequency " << frequency << " failed: " << errorText() << std::endl;
    }

    sampleBuffer.FlushRingBuffer();
    spectrumSampleBuffer.FlushRingBuffer();
    if (agc) {
        agc->reset();
    }
}

int CV4L2Sdr::getFrequency(void) const
{
    return frequency;
}

bool CV4L2Sdr::restart(void)
{
    if (running) {
        return true;
    }

    if (fd < 0 || unplugged) {
        const std::string last = path;
        if (!selectDevice(last)) {
            radioController.onMessage(message_level_t::Error,
                QT_TRANSLATE_NOOP("CRadioController", "Error while opening the Linux SDR device. See log for details."));
            return false;
        }
    }

    sampleBuffer.FlushRingBuffer();
    spectrumSampleBuffer.FlushRingBuffer();
    if (converter) {
        converter->reset();
    }
    if (resampler) {
        resampler->reset();
    }
    agc->reset();
    setFrequency(frequency);

    if (!startStreaming()) {
        return false;
    }

    if (!rateMeasured) {
        measureAdcRate();
    }

    running = true;
    readerThread = std::thread(&CV4L2Sdr::readLoop, this);
    agcThread = std::thread(&CV4L2Sdr::agcTimerThread, this);
    return true;
}

bool CV4L2Sdr::startStreaming()
{
    v4l2_requestbuffers req;
    std::memset(&req, 0, sizeof(req));
    req.count = bufferCount;
    req.type = V4L2_BUF_TYPE_SDR_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (xioctl(fd, VIDIOC_REQBUFS, &req) != 0) {
        std::clog << "V4L2: VIDIOC_REQBUFS: " << errorText() << std::endl;
        if (errno == EBUSY) {
            radioController.onMessage(message_level_t::Error,
                QT_TRANSLATE_NOOP("CRadioController", "The Linux SDR device is used by another program."));
        }
        return false;
    }

    buffers.clear();
    for (uint32_t i = 0; i < req.count; i++) {
        v4l2_buffer buf;
        std::memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_SDR_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (xioctl(fd, VIDIOC_QUERYBUF, &buf) != 0) {
            std::clog << "V4L2: VIDIOC_QUERYBUF: " << errorText() << std::endl;
            stopStreaming();
            return false;
        }
        MappedBuffer mapped;
        mapped.length = buf.length;
        mapped.start = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        if (mapped.start == MAP_FAILED) {
            std::clog << "V4L2: mmap: " << errorText() << std::endl;
            stopStreaming();
            return false;
        }
        buffers.push_back(mapped);

        if (xioctl(fd, VIDIOC_QBUF, &buf) != 0) {
            std::clog << "V4L2: VIDIOC_QBUF: " << errorText() << std::endl;
            stopStreaming();
            return false;
        }
    }

    v4l2_buf_type type = V4L2_BUF_TYPE_SDR_CAPTURE;
    if (xioctl(fd, VIDIOC_STREAMON, &type) != 0) {
        std::clog << "V4L2: VIDIOC_STREAMON: " << errorText() << std::endl;
        stopStreaming();
        return false;
    }
    return true;
}

void CV4L2Sdr::stopStreaming()
{
    if (fd < 0) {
        return;
    }

    v4l2_buf_type type = V4L2_BUF_TYPE_SDR_CAPTURE;
    xioctl(fd, VIDIOC_STREAMOFF, &type);

    for (const auto& b : buffers) {
        munmap(b.start, b.length);
    }
    buffers.clear();

    v4l2_requestbuffers req;
    std::memset(&req, 0, sizeof(req));
    req.count = 0;
    req.type = V4L2_BUF_TYPE_SDR_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    xioctl(fd, VIDIOC_REQBUFS, &req);
}

bool CV4L2Sdr::is_ok(void)
{
    return !unplugged;
}

void CV4L2Sdr::stop(void)
{
    if (!running) {
        return;
    }
    running = false;

    if (readerThread.joinable()) {
        readerThread.join();
    }
    if (agcThread.joinable()) {
        agcThread.join();
    }
    stopStreaming();
}

void CV4L2Sdr::readLoop()
{
    while (running) {
        pollfd p;
        p.fd = fd;
        p.events = POLLIN;
        p.revents = 0;
        const int ret = poll(&p, 1, 200);
        if (ret < 0 && errno != EINTR) {
            std::clog << "V4L2: poll: " << errorText() << std::endl;
            break;
        }
        if (ret <= 0) {
            continue;
        }

        v4l2_buffer buf;
        std::memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_SDR_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (xioctl(fd, VIDIOC_DQBUF, &buf) != 0) {
            if (errno == EAGAIN) {
                continue;
            }
            // ENODEV: unplugged
            std::clog << "V4L2: VIDIOC_DQBUF: " << errorText() << std::endl;
            unplugged = true;
            radioController.onMessage(message_level_t::Error,
                QT_TRANSLATE_NOOP("CRadioController", "The Linux SDR device is unplugged."));
            break;
        }

        if (buf.index < buffers.size()) {
            process(static_cast<const uint8_t*>(buffers[buf.index].start), buf.bytesused);
        }

        if (xioctl(fd, VIDIOC_QBUF, &buf) != 0) {
            std::clog << "V4L2: VIDIOC_QBUF: " << errorText() << std::endl;
        }
    }
}

// Airspy: the rate of the first 0.5 s decides which of the known rates is
// used. Runs before the reader thread starts; the samples are dropped.
void CV4L2Sdr::measureAdcRate()
{
    const size_t valueSize = V4L2SdrFormats::bytesPerValue(format);
    uint64_t values = 0;
    auto start = std::chrono::steady_clock::now();
    bool first = true;
    double seconds = 0;

    while (seconds < 0.5) {
        pollfd p;
        p.fd = fd;
        p.events = POLLIN;
        p.revents = 0;
        if (poll(&p, 1, 500) <= 0) {
            std::clog << "V4L2: no samples while measuring the ADC rate" << std::endl;
            return;
        }

        v4l2_buffer buf;
        std::memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_SDR_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (xioctl(fd, VIDIOC_DQBUF, &buf) != 0) {
            if (errno == EAGAIN) {
                continue;
            }
            std::clog << "V4L2: VIDIOC_DQBUF: " << errorText() << std::endl;
            return;
        }
        // QBUF sets bytesused to 0
        const uint32_t bytes = buf.bytesused;
        xioctl(fd, VIDIOC_QBUF, &buf);

        // The time starts with the first buffer, the start of the stream
        // takes longer
        if (first) {
            start = std::chrono::steady_clock::now();
            first = false;
            continue;
        }
        values += bytes / valueSize;
        seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    }

    const double rate = values / seconds;
    int nearest = airspyRates[0];
    for (const int r : airspyRates) {
        if (std::abs(rate - r) < std::abs(rate - nearest)) {
            nearest = r;
        }
    }
    std::clog << "V4L2: measured ADC rate " << rate << ", using " << nearest << std::endl;

    rateMeasured = true;
    if (nearest != adcRate) {
        adcRate = nearest;
        setupProcessing();
    }
}

void CV4L2Sdr::process(const uint8_t* data, size_t bytes)
{
    const size_t valueSize = V4L2SdrFormats::bytesPerValue(format);
    const DSPCOMPLEX* iq;
    size_t count;

    if (converter) {
        realValues.resize(bytes / valueSize);
        const size_t n = V4L2SdrFormats::convertReal(format, data, bytes, realValues.data());

        // Level measurement on the ADC values, as pairs
        agc->process(reinterpret_cast<const DSPCOMPLEX*>(realValues.data()), n / 2);

        complexSamples.clear();
        count = converter->process(realValues.data(), n, complexSamples);
        iq = complexSamples.data();
    }
    else {
        complexSamples.resize(bytes / (2 * valueSize));
        count = V4L2SdrFormats::convertComplex(format, data, bytes, complexSamples.data());
        iq = complexSamples.data();

        if (format == SdrFormat::CU8) {
            agc->process(data, 2 * count);
            // RAW recording in the format of the rtl-sdr input
            if (!resampler) {
                putIntoRecordBuffer(*const_cast<uint8_t*>(data), uint32_t(2 * count));
            }
        }
        else {
            agc->process(iq, count);
        }
    }

    if (resampler) {
        resampled.clear();
        count = resampler->process(iq, count, resampled);
        iq = resampled.data();
    }

    sampleBuffer.putDataIntoBuffer(iq, int32_t(count));
    spectrumSampleBuffer.putDataIntoBuffer(iq, int32_t(count));
    if (recordFormat() == CRecordFormat::CF32)
        putIntoRecordBuffer(iq, count);
}

void CV4L2Sdr::reset(void)
{
    sampleBuffer.FlushRingBuffer();
}

int32_t CV4L2Sdr::getSamples(DSPCOMPLEX* buffer, int32_t size)
{
    return sampleBuffer.getDataFromBuffer(buffer, size);
}

std::vector<DSPCOMPLEX> CV4L2Sdr::getSpectrumSamples(int size)
{
    std::vector<DSPCOMPLEX> buf(size);
    const int sizeRead = spectrumSampleBuffer.getDataFromBuffer(buf.data(), size);
    if (sizeRead < size) {
        buf.resize(sizeRead);
    }
    return buf;
}

int32_t CV4L2Sdr::getSamplesToRead(void)
{
    return sampleBuffer.GetRingBufferReadAvailable();
}

float CV4L2Sdr::getGain(void) const
{
    const int index = currentGain;
    return (index >= 0 && size_t(index) < gainsDb.size()) ? gainsDb[index] : 0.0f;
}

int CV4L2Sdr::getGainIndex(void) const
{
    return gainControl != 0 ? currentGain.load() : -1;
}

float CV4L2Sdr::setGain(int gain)
{
    if (gainControl == 0) {
        return 0;
    }
    currentGain = std::max(0, std::min(gain, getGainCount()));
    applyGain(currentGain);
    return getGain();
}

int CV4L2Sdr::getGainCount(void)
{
    return gainsDb.empty() ? 0 : int(gainsDb.size()) - 1;
}

void CV4L2Sdr::setAgc(bool on)
{
    isAgc = on;
    if (!on && gainControl != 0) {
        applyGain(currentGain);
    }
}

void CV4L2Sdr::applyGain(int index)
{
    std::lock_guard<std::mutex> lock(gainMutex);
    if (fd < 0 || gainControl == 0) {
        return;
    }

    v4l2_ext_control c;
    std::memset(&c, 0, sizeof(c));
    c.id = gainControl;
    c.value = gainMin + index * gainStep;
    v4l2_ext_controls cs;
    std::memset(&cs, 0, sizeof(cs));
    cs.which = V4L2_CTRL_WHICH_CUR_VAL;
    cs.count = 1;
    cs.controls = &c;
    if (xioctl(fd, VIDIOC_S_EXT_CTRLS, &cs) != 0) {
        std::clog << "V4L2: setting the gain failed: " << errorText() << std::endl;
    }

    if (agc) {
        agc->gainChanged();
    }
}

// The gain is changed here and not in the reader thread: the USB control
// transfers would block the reception
void CV4L2Sdr::agcTimerThread()
{
    while (running && !unplugged) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        if (isAgc && gainControl != 0) {
            const int index = currentGain;
            const int next = agc->nextGainIndex(gainsDb, index);
            if (next != index) {
                currentGain = next;
                applyGain(next);
            }
        }
        // Without gain control the user can't lower the gain
        else if (!isAgc && gainControl != 0 && agc->overloaded()) {
            std::string text = QT_TRANSLATE_NOOP("CRadioController", "ADC overload. Maybe you are using a too high gain.");
            std::clog << "V4L2: " << text << std::endl;
            radioController.onMessage(message_level_t::Information, text);
        }
    }
}

std::string CV4L2Sdr::getDescription(void)
{
    return card + " (" + driver + ", " + path + ")";
}

CDeviceID CV4L2Sdr::getID(void)
{
    return CDeviceID::V4L2;
}

// CU8 at 2.048 MS/s is recorded as it comes from the device, everything else
// as the complex samples passed to the decoder
CRecordFormat CV4L2Sdr::recordFormat(void)
{
    return (format == SdrFormat::CU8 && !resampler) ? CRecordFormat::U8 : CRecordFormat::CF32;
}
