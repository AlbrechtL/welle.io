#pragma once

#include "input_factory.h"
#include <sdrplay_api.h>

#include <atomic>
#include <complex>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// ---------- SPSC ring buffer (overwrite s drop counterom) ----------
class CplxRingSPSC {
public:
    explicit CplxRingSPSC(size_t capacity_pow2 = (1u << 22)) { // ~32MB
        cap_ = 1;
        while (cap_ < capacity_pow2) cap_ <<= 1;
        mask_ = cap_ - 1;
        buf_.resize(cap_);
    }

    void clearUnsafe() {
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
        dropped_.store(0, std::memory_order_relaxed);
    }

    void flushSafe() {
        const size_t head = head_.load(std::memory_order_acquire);
        tail_.store(head, std::memory_order_release);
    }

    size_t available() const {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t head = head_.load(std::memory_order_acquire);
        size_t used = head - tail;
        if (used > cap_) used = cap_;
        return used;
    }

    void pushBlock(const std::complex<float>* in, size_t n) {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t h = head & mask_;
        size_t cont = (n < (cap_ - h)) ? n : (cap_ - h);

        std::memcpy(&buf_[h], in, cont * sizeof(std::complex<float>));
        size_t rem = n - cont;
        if (rem) std::memcpy(&buf_[0], in + cont, rem * sizeof(std::complex<float>));

        head_.store(head + n, std::memory_order_release);
    }

    size_t popBlock(std::complex<float>* out, size_t want) {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t head = head_.load(std::memory_order_acquire);

        size_t used = head - tail;
        if (used > cap_) {
            size_t over = used - cap_;
            dropped_.fetch_add(over, std::memory_order_relaxed);
            tail = head - cap_;
            used = cap_;
        }
        if (used == 0) return 0;

        size_t n = (want < used) ? want : used;
        size_t t = tail & mask_;
        size_t cont = (n < (cap_ - t)) ? n : (cap_ - t);

        std::memcpy(out, &buf_[t], cont * sizeof(std::complex<float>));
        size_t rem = n - cont;
        if (rem) std::memcpy(out + cont, &buf_[0], rem * sizeof(std::complex<float>));

        tail_.store(tail + n, std::memory_order_release);
        return n;
    }

    uint64_t dropped() const { return dropped_.load(std::memory_order_relaxed); }

private:
    size_t cap_{0}, mask_{0};
    std::vector<std::complex<float>> buf_;
    std::atomic<size_t> head_{0}, tail_{0};
    std::atomic<uint64_t> dropped_{0};
};

class SDRPlayInput : public CVirtualInput {
public:
    SDRPlayInput();
    ~SDRPlayInput() override;

    bool restart() override;
    void stop() override;
    int32_t getSamples(std::complex<float>* buffer, int32_t size) override;
    void setFrequency(int frequency) override;
    int getFrequency() const override;
    float setGain(int gain) override;
    float getGain() const override;
    int getGainCount() override;
    void setAgc(bool agc) override;
    bool is_ok() override;
    void reset() override;
    std::vector<std::complex<float>> getSpectrumSamples(int size) override;
    int32_t getSamplesToRead() override;
    std::string getDescription() override;
    CDeviceID getID() override;

    static void streamCallback(short *xi, short *xq,
                               sdrplay_api_StreamCbParamsT *params,
                               unsigned int numSamples,
                               unsigned int reset,
                               void *cbContext);

    static void eventCallback(sdrplay_api_EventT eventId,
                              sdrplay_api_TunerSelectT tuner,
                              sdrplay_api_EventParamsT *params,
                              void *cbContext);

private:
    void controlThreadMain();
    void applyTuningAndGainUnsafe(double rfHz);

    // env/config
    int  antennaSel_ = 0;                // WELLE_SDRPLAY_ANT (0/1/2)
    bool rfNotch_ = true;                //FM
    bool invertQ_ = false;               // WELLE_SDRPLAY_INV_Q
    bool swapIQ_  = false;               // WELLE_SDRPLAY_SWAP_IQ
    double loOffsetHz_ = 0.0;            // WELLE_SDRPLAY_LO_OFFS_HZ
    double ppm_ = 0.0;                   // WELLE_SDRPLAY_PPM
    double srPpm_ = 0.0;                 // WELLE_SDRPLAY_SR_PPM

    int agcSetpointDbfs_ = -30;          // WELLE_SDRPLAY_AGC_SETPOINT
    int agcHz_ = 100;                    // WELLE_SDRPLAY_AGC_HZ
    int lnaState_ = 8;                   // WELLE_SDRPLAY_LNA_STATE
    int grDb_ = 25;                      // WELLE_SDRPLAY_GRDB
    int discardMs_ = 600;                // WELLE_SDRPLAY_DISCARD_MS
    bool forceBulk_ = true;              // WELLE_SDRPLAY_BULK
    int  retuneDebounceMs_ = 120;        // WELLE_SDRPLAY_RETUNE_DEBOUNCE_MS

    bool releaseOnStop_ = true;          // WELLE_SDRPLAY_RELEASE_ON_STOP (default 1)

    // SDRplay
    sdrplay_api_DeviceT chosenDevice{};
    sdrplay_api_DeviceParamsT* deviceParams = nullptr;
    sdrplay_api_CallbackFnsT cbs_{};

    std::mutex apiMtx_;                 
    std::atomic<bool> isStreaming_{false};
    bool apiSelected_ = false;

    std::atomic<int>  currentFrequency_{0};
    std::atomic<int>  requestedFrequency_{0};
    std::atomic<bool> retunePending_{false};

    // overload handling 
std::atomic<bool> stopping_{false};            
std::atomic<bool> overloadAckPending_{false};  

bool apiReady_ = false;



    std::atomic<int>  currentGainIndex_{0};
    float denominator_ = 2048.0f;

    // ring + notification
    CplxRingSPSC ring_;
    std::vector<std::complex<float>> cbTmp_;
    std::mutex cvMtx_;
    std::condition_variable cvData_;

    // discard gate
    std::atomic<int64_t> discardSamples_{0};

    // NCO
    std::complex<float> ncoPhase_{1.0f, 0.0f};
    std::complex<float> ncoStep_{1.0f, 0.0f};
    std::atomic<bool> resetNco_{false};

    // control thread
    std::atomic<bool> ctrlRun_{false};
    std::thread ctrlThread_;
};
