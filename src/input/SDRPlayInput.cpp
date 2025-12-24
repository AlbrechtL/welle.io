#include "SDRPlayInput.h"

#include <iostream>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <thread>

#include <string>
#include <cstring>

#include <mutex>

namespace {
  std::mutex g_apiMx;
  int  g_apiRefcnt = 0;
  bool g_apiOpened = false;

  bool SdrplayApiAcquire()
  {
    std::lock_guard<std::mutex> lk(g_apiMx);

    if (g_apiOpened) { 
      g_apiRefcnt++;
      return true;
    }

    auto err = sdrplay_api_Open();
    if (err != sdrplay_api_Success) {
      std::cerr << "[SDRPlay] Open failed: " << sdrplay_api_GetErrorString(err) << "\n";
      return false;
    }

    
    float runtimeVer = 0.0f;
    err = sdrplay_api_ApiVersion(&runtimeVer);
    if (err != sdrplay_api_Success) {
      std::cerr << "[SDRPlay] ApiVersion failed: " << sdrplay_api_GetErrorString(err) << "\n";
      sdrplay_api_Close(); 
      return false;
    }

#ifdef SDRPLAY_API_VERSION
    
    if (std::fabs(runtimeVer - SDRPLAY_API_VERSION) > 0.001f) {
      std::cerr << "[SDRPlay] API version mismatch. "
                << "runtime=" << runtimeVer << " compiled=" << SDRPLAY_API_VERSION << "\n";
      sdrplay_api_Close();
      return false;
    }
#endif

    std::cout << "[SDRPlay] API ready. Version: " << runtimeVer << "\n";

    g_apiOpened = true;
    g_apiRefcnt = 1;
    return true;
  }

  void SdrplayApiRelease()
  {
    std::lock_guard<std::mutex> lk(g_apiMx);

    if (g_apiRefcnt > 0) g_apiRefcnt--;

    if (g_apiRefcnt == 0 && g_apiOpened) {
      auto err = sdrplay_api_Close();
      if (err != sdrplay_api_Success) {
        std::cerr << "[SDRPlay] Close failed: " << sdrplay_api_GetErrorString(err) << "\n";
      }
      g_apiOpened = false;
      g_apiRefcnt = 0;
    }

  }
} // namespace


#ifndef sdrplay_api_RSPdx
#define sdrplay_api_RSPdx (4)
#endif
#ifndef sdrplay_api_RSPdxR2
#define sdrplay_api_RSPdxR2 (7)
#endif

#ifndef sdrplay_api_RSP1
#define sdrplay_api_RSP1   (1)
#endif
#ifndef sdrplay_api_RSP2
#define sdrplay_api_RSP2   (2)
#endif
#ifndef sdrplay_api_RSPduo
#define sdrplay_api_RSPduo (3)
#endif
#ifndef sdrplay_api_RSP1A
#define sdrplay_api_RSP1A  (255)
#endif
#ifndef sdrplay_api_RSP1B
#define sdrplay_api_RSP1B  (6)
#endif


static inline int bitsForDeviceAndFs(const sdrplay_api_DeviceT& dev, double fsHz)
{
    int bits = 12;
    switch (dev.hwVer) {
        case sdrplay_api_RSPdx:
        case sdrplay_api_RSPdxR2:
        case sdrplay_api_RSPduo:
        case sdrplay_api_RSP1A:
        case sdrplay_api_RSP1B:
            bits = 14;
            break;
        default:
            bits = 12;
            break;
    }

    if (bits == 14) {
        if (fsHz > 9.216e6)      bits = 8;
        else if (fsHz > 8.064e6) bits = 10;
        else if (fsHz > 6.048e6) bits = 12;
    }
    return bits;
}

static inline float denomFromBits(int bits)
{
    return (float)(1u << (bits - 1));
}


#ifndef sdrplay_api_BULK
#define sdrplay_api_BULK (1)
#endif

#ifndef MAX_BB_GR
#define MAX_BB_GR 59
#endif


static inline const char* sdrErr(sdrplay_api_ErrT e) {
    return sdrplay_api_GetErrorString(e);
}

static inline int envInt(const char* name, int defv) {
    const char* v = std::getenv(name);
    if (!v || !*v) return defv;
    return std::atoi(v);
}
static inline double envDouble(const char* name, double defv) {
    const char* v = std::getenv(name);
    if (!v || !*v) return defv;
    return std::atof(v);
}

static inline std::string envStr(const char* name, const char* defv = "")
{
    const char* v = std::getenv(name);
    if (!v || !*v) return std::string(defv);
    return std::string(v);
}



static inline int clampInt(int v, int lo, int hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

static inline sdrplay_api_AgcControlT agcFromHz(int hz) {
#ifdef sdrplay_api_AGC_50HZ
    if (hz == 50) return sdrplay_api_AGC_50HZ;
#endif
#ifdef sdrplay_api_AGC_100HZ
    (void)hz;
    return sdrplay_api_AGC_100HZ;
#else
    return sdrplay_api_AGC_CTRL_EN;
#endif
}

SDRPlayInput::SDRPlayInput()
: ring_(1u << 22)
{
    antennaSel_ = clampInt(envInt("WELLE_SDRPLAY_ANT", 0), 0, 2);
    invertQ_    = envInt("WELLE_SDRPLAY_INV_Q", 0) != 0;
    swapIQ_     = envInt("WELLE_SDRPLAY_SWAP_IQ", 0) != 0;
    loOffsetHz_ = envDouble("WELLE_SDRPLAY_LO_OFFS_HZ", 0.0);
    ppm_        = envDouble("WELLE_SDRPLAY_PPM", 0.0);
    srPpm_      = envDouble("WELLE_SDRPLAY_SR_PPM", 0.0);

    agcSetpointDbfs_ = envInt("WELLE_SDRPLAY_AGC_SETPOINT", -30);
    agcHz_           = envInt("WELLE_SDRPLAY_AGC_HZ", 100);
    lnaState_        = envInt("WELLE_SDRPLAY_LNA_STATE", 8);
    grDb_            = envInt("WELLE_SDRPLAY_GRDB", 25);
    discardMs_       = envInt("WELLE_SDRPLAY_DISCARD_MS", 600);
    forceBulk_       = envInt("WELLE_SDRPLAY_BULK", 1) != 0;
    retuneDebounceMs_= envInt("WELLE_SDRPLAY_RETUNE_DEBOUNCE_MS", 120);

    releaseOnStop_   = envInt("WELLE_SDRPLAY_RELEASE_ON_STOP", 1) != 0;

    rfNotch_ = envInt("WELLE_SDRPLAY_RF_NOTCH", 1) != 0;

    std::cout << "[SDRPlay] cfg: ANT=" << antennaSel_
              << " INV_Q=" << (invertQ_?1:0)
              << " SWAP_IQ=" << (swapIQ_?1:0)
              << " LO_OFFS_HZ=" << loOffsetHz_
              << " PPM=" << ppm_
              << " SR_PPM=" << srPpm_
              << " AGC_SETPOINT=" << agcSetpointDbfs_
              << " AGC_HZ=" << agcHz_
              << " LNA_STATE=" << lnaState_
              << " GRDB=" << grDb_
              << " DISCARD_MS=" << discardMs_
              << " BULK=" << (forceBulk_?1:0)
              << " RETUNE_DEBOUNCE_MS=" << retuneDebounceMs_
              << " RELEASE_ON_STOP=" << (releaseOnStop_?1:0)
              << " RF_NOTCH=" << (rfNotch_ ? 1 : 0)
              << "\n";

              apiReady_ = SdrplayApiAcquire();
              if (!apiReady_) {
                std::cerr << "[SDRPlay] API init failed\n";
               
                return;
              }



    ring_.clearUnsafe();
}

SDRPlayInput::~SDRPlayInput() {
    stop();

    if (apiSelected_) {
        auto eLock = sdrplay_api_LockDeviceApi();
        if (eLock == sdrplay_api_Success) {
            auto e = sdrplay_api_ReleaseDevice(&chosenDevice);
            sdrplay_api_UnlockDeviceApi();

            if (e == sdrplay_api_Success) {
                apiSelected_ = false;
            } else {
                std::cerr << "[SDRPlay] ReleaseDevice(dtor) failed: " << sdrErr(e) << "\n";
            }
        } else {
            std::cerr << "[SDRPlay] LockDeviceApi(dtor) failed: " << sdrErr(eLock) << "\n";
        }
    }

    if (apiReady_) {
      SdrplayApiRelease();
    }

}


bool SDRPlayInput::restart() {
  if (!apiReady_) return false;
    if (isStreaming_.load()) return true;

    auto releaseSelectedDevice = [&]() {
        if (!apiSelected_) return;

        auto eLock = sdrplay_api_LockDeviceApi();
        if (eLock != sdrplay_api_Success) {
            std::cerr << "[SDRPlay] LockDeviceApi(release) failed: " << sdrErr(eLock) << "\n";
            return; 
        }

        auto e = sdrplay_api_ReleaseDevice(&chosenDevice);
        sdrplay_api_UnlockDeviceApi();

        if (e == sdrplay_api_Success) {
            apiSelected_ = false;
            deviceParams = nullptr;
        } else {
            std::cerr << "[SDRPlay] ReleaseDevice failed: " << sdrErr(e) << "\n";
           
        }
    };


    if (!apiSelected_) {
        unsigned int nDevs = 0;
        sdrplay_api_DeviceT devs[16]{};

        auto err = sdrplay_api_LockDeviceApi();
        if (err != sdrplay_api_Success) {
            std::cerr << "[SDRPlay] LockDeviceApi failed: " << sdrErr(err) << "\n";
            return false;
        }

        const unsigned int maxDevs =
            (unsigned int)(sizeof(devs) / sizeof(devs[0]));

            err = sdrplay_api_GetDevices(devs, &nDevs, maxDevs);
            if (err != sdrplay_api_Success || nDevs == 0) {
            std::cerr << "[SDRPlay] GetDevices failed/no devs: " << sdrErr(err) << "\n";
            sdrplay_api_UnlockDeviceApi();
            return false;
        }

const std::string wantSer = envStr("WELLE_SDRPLAY_SN", "");
const int wantHwVer = envInt("WELLE_SDRPLAY_HWVER", -1);

std::cout << "[SDRPlay] Available devices (" << nDevs << "):\n";
for (unsigned i = 0; i < nDevs; i++) {
    std::cout << "  [" << i << "] valid=" << int(devs[i].valid)
              << " hwVer=" << int(devs[i].hwVer)
              << " ser=\"" << devs[i].SerNo << "\"\n";
}

int pick = -1;

if (!wantSer.empty() || wantHwVer >= 0) {
    for (unsigned i = 0; i < nDevs; i++) {
        if (!devs[i].valid) continue;

        if (wantHwVer >= 0 && (int)devs[i].hwVer != wantHwVer) continue;

        if (!wantSer.empty()) {

          if (std::strncmp(devs[i].SerNo, wantSer.c_str(), sizeof(devs[i].SerNo)) != 0)
                continue;
        }

        pick = (int)i;
        break;
    }

    if (pick < 0) {
        std::cerr << "[SDRPlay] Requested device not found/available. "
                  << "WELLE_SDRPLAY_SN=\"" << wantSer << "\" "
                  << "WELLE_SDRPLAY_HWVER=" << wantHwVer << "\n";
        sdrplay_api_UnlockDeviceApi();
        return false;
    }
} else {
    for (unsigned i = 0; i < nDevs; i++) {
        if (devs[i].valid) { pick = (int)i; break; }
    }
    if (pick < 0) pick = 0; 
}

chosenDevice = devs[pick];

if (std::getenv("WELLE_SDRPLAY_LNA_STATE") == nullptr) {
    if (chosenDevice.hwVer == sdrplay_api_RSP1A || chosenDevice.hwVer == sdrplay_api_RSP1B) {
        lnaState_ = 5;
    } else {
        lnaState_ = 8;
    }
}



chosenDevice.tuner = sdrplay_api_Tuner_A;


if (chosenDevice.hwVer == sdrplay_api_RSPduo) {
    if (chosenDevice.rspDuoMode & sdrplay_api_RspDuoMode_Master) {
        chosenDevice.rspDuoMode = sdrplay_api_RspDuoMode_Single_Tuner;
    } else {
        std::cerr << "[SDRPlay] RSPduo available only as SLAVE (device in use?) -> abort\n";
        sdrplay_api_UnlockDeviceApi();
        return false;
    }
}



        err = sdrplay_api_SelectDevice(&chosenDevice);
        if (err != sdrplay_api_Success) {
            std::cerr << "[SDRPlay] SelectDevice failed: " << sdrErr(err) << "\n";
            sdrplay_api_UnlockDeviceApi();
            return false;
        }
        apiSelected_ = true;

        sdrplay_api_UnlockDeviceApi();
    }

    auto err = sdrplay_api_GetDeviceParams(chosenDevice.dev, &deviceParams);
    if (err != sdrplay_api_Success || !deviceParams || !deviceParams->rxChannelA) {
        std::cerr << "[SDRPlay] GetDeviceParams failed: " << sdrErr(err) << "\n";
        releaseSelectedDevice();
        return false;
    }


    if (!deviceParams->devParams) {
        std::cerr << "[SDRPlay] devParams is NULL (likely RSPduo SLAVE / device busy). Aborting.\n";
        releaseSelectedDevice();
        return false;
    }




    // sample rate
const double fsBase = 2048000.0;
const double fs = fsBase * (1.0 + (srPpm_ * 1e-6));

deviceParams->devParams->fsFreq.fsHz = fs;
deviceParams->devParams->ppm = ppm_;

const int bits = bitsForDeviceAndFs(chosenDevice, fs);
denominator_ = denomFromBits(bits);

std::cout << "[SDRPlay] hwVer=" << int(chosenDevice.hwVer)
          << " ser=\"" << chosenDevice.SerNo << "\""
          << " fs=" << fs << " bits=" << bits
          << " denom=" << denominator_ << "\n";


    if (forceBulk_) {
        deviceParams->devParams->mode = (sdrplay_api_TransferModeT)sdrplay_api_BULK;
    }

    switch (chosenDevice.hwVer) {

case sdrplay_api_RSPdx:
case sdrplay_api_RSPdxR2:
    deviceParams->devParams->rspDxParams.antennaSel =
        (sdrplay_api_RspDx_AntennaSelectT)antennaSel_; 
    deviceParams->devParams->rspDxParams.biasTEnable = 0;
    deviceParams->devParams->rspDxParams.rfNotchEnable = rfNotch_ ? 1 : 0;
    deviceParams->devParams->rspDxParams.rfDabNotchEnable = 0;
    break;

case sdrplay_api_RSP1A:
case sdrplay_api_RSP1B: 
    deviceParams->rxChannelA->rsp1aTunerParams.biasTEnable = 0;
    deviceParams->devParams->rsp1aParams.rfNotchEnable = 0;
    deviceParams->devParams->rsp1aParams.rfDabNotchEnable = 0;
    break;

case sdrplay_api_RSP2: {
    auto &p = deviceParams->rxChannelA->rsp2TunerParams;
    p.biasTEnable   = 0;
    p.rfNotchEnable = 0;
    p.amPortSel     = sdrplay_api_Rsp2_AMPORT_2; 
    p.antennaSel    = (antennaSel_ == 1) ? sdrplay_api_Rsp2_ANTENNA_B
                                         : sdrplay_api_Rsp2_ANTENNA_A;
    break;
}

case sdrplay_api_RSPduo: {
    auto &p = deviceParams->rxChannelA->rspDuoTunerParams;
    p.biasTEnable = 0;
    p.rfNotchEnable = 0;
    p.rfDabNotchEnable = 0;
    p.tuner1AmNotchEnable = 0;
    p.tuner1AmPortSel = sdrplay_api_RspDuo_AMPORT_2; 
    break;
}

default:
    break;
}

    deviceParams->rxChannelA->tunerParams.bwType = sdrplay_api_BW_1_536;
    deviceParams->rxChannelA->tunerParams.ifType = sdrplay_api_IF_Zero;

    // DC tracking
    deviceParams->rxChannelA->tunerParams.dcOffsetTuner.dcCal = 3;
    deviceParams->rxChannelA->tunerParams.dcOffsetTuner.speedUp = 0;
    deviceParams->rxChannelA->ctrlParams.dcOffset.DCenable = 1;
    deviceParams->rxChannelA->ctrlParams.dcOffset.IQenable = 1;

    // AGC + Gain
    deviceParams->rxChannelA->ctrlParams.agc.setPoint_dBfs = agcSetpointDbfs_;
    deviceParams->rxChannelA->ctrlParams.agc.enable = agcFromHz(agcHz_);
    deviceParams->rxChannelA->tunerParams.gain.gRdB = clampInt(grDb_, 0, MAX_BB_GR);
    deviceParams->rxChannelA->tunerParams.gain.LNAstate = clampInt(lnaState_, 0, 30);

    // NCO
    if (loOffsetHz_ != 0.0) {
        double ph = 2.0 * M_PI * (-loOffsetHz_) / fs;
        ncoStep_  = std::complex<float>((float)std::cos(ph), (float)std::sin(ph));
        ncoPhase_ = {1.0f, 0.0f};
    } else {
        ncoStep_  = {1.0f, 0.0f};
        ncoPhase_ = {1.0f, 0.0f};
    }

    cbs_ = {};
    cbs_.StreamACbFn = SDRPlayInput::streamCallback;
    cbs_.StreamBCbFn = nullptr;
    cbs_.EventCbFn   = SDRPlayInput::eventCallback;

    ring_.clearUnsafe();

    int f = currentFrequency_.load(std::memory_order_relaxed);
    if (f <= 0) f = requestedFrequency_.load(std::memory_order_relaxed);
    if (f > 0) {
        const double rfHz = (double)f + loOffsetHz_;
        deviceParams->rxChannelA->tunerParams.rfFreq.rfHz = rfHz;
        currentFrequency_.store(f, std::memory_order_relaxed);
        std::cout << "[SDRPlay] Tuning HW to: " << rfHz << " Hz\n";
    }

    discardSamples_.store((int64_t)(fs * (double)discardMs_ / 1000.0), std::memory_order_relaxed);
    resetNco_.store(true, std::memory_order_relaxed);

    {
        std::lock_guard<std::mutex> lk(apiMtx_);
        stopping_.store(false, std::memory_order_relaxed);
        overloadAckPending_.store(false, std::memory_order_relaxed);


        err = sdrplay_api_Init(chosenDevice.dev, &cbs_, (void*)this);
    }
    if (err != sdrplay_api_Success) {
    std::cerr << "[SDRPlay] Init failed: " << sdrErr(err) << "\n";
    releaseSelectedDevice();
    return false;
}

    {
        std::lock_guard<std::mutex> lk(apiMtx_);
        sdrplay_api_Update(chosenDevice.dev, chosenDevice.tuner,
                           sdrplay_api_Update_Ctrl_Agc, sdrplay_api_Update_Ext1_None);
        sdrplay_api_Update(chosenDevice.dev, chosenDevice.tuner,
                           sdrplay_api_Update_Tuner_Gr, sdrplay_api_Update_Ext1_None);
    }

    isStreaming_.store(true);

    ctrlRun_.store(true);
    ctrlThread_ = std::thread(&SDRPlayInput::controlThreadMain, this);

    return true;
}

void SDRPlayInput::stop() {
  if (!apiReady_) return; 
    stopping_.store(true, std::memory_order_relaxed);
    overloadAckPending_.store(false, std::memory_order_relaxed);

    if (ctrlRun_.exchange(false)) {
        if (ctrlThread_.joinable()) ctrlThread_.join();
    }

    if (!isStreaming_.load()) {
        if (releaseOnStop_ && apiSelected_) {

            auto eLock = sdrplay_api_LockDeviceApi();
            if (eLock == sdrplay_api_Success) {
                auto e = sdrplay_api_ReleaseDevice(&chosenDevice);
                sdrplay_api_UnlockDeviceApi();

                if (e == sdrplay_api_Success) {
                    apiSelected_ = false;
                    deviceParams = nullptr;
                } else {
                    std::cerr << "[SDRPlay] ReleaseDevice(stop) failed: " << sdrErr(e) << "\n";
                }
            } else {
                std::cerr << "[SDRPlay] LockDeviceApi(stop) failed: " << sdrErr(eLock) << "\n";
            }
        }

        stopping_.store(false, std::memory_order_relaxed);
        return;
    }


    {
        std::lock_guard<std::mutex> lk(apiMtx_);
        auto err = sdrplay_api_Uninit(chosenDevice.dev);

        if (err == sdrplay_api_StopPending) {
            std::cerr << "[SDRPlay] Uninit StopPending (slave running). Not releasing yet.\n";
            isStreaming_.store(false);
            stopping_.store(false, std::memory_order_relaxed);
            return;
        }

        if (err != sdrplay_api_Success) {
            std::cerr << "[SDRPlay] Uninit failed: " << sdrErr(err) << "\n";
        }
    }

    isStreaming_.store(false);

    if (releaseOnStop_ && apiSelected_) {
        auto eLock = sdrplay_api_LockDeviceApi();
        if (eLock == sdrplay_api_Success) {
            auto e = sdrplay_api_ReleaseDevice(&chosenDevice);
            sdrplay_api_UnlockDeviceApi();

            if (e == sdrplay_api_Success) {
                apiSelected_ = false;
                deviceParams = nullptr;
            } else {
                std::cerr << "[SDRPlay] ReleaseDevice(stop) failed: " << sdrErr(e) << "\n";
            }
        } else {
            std::cerr << "[SDRPlay] LockDeviceApi(stop) failed: " << sdrErr(eLock) << "\n";
        }
    }


    stopping_.store(false, std::memory_order_relaxed);
}


int32_t SDRPlayInput::getSamples(std::complex<float>* buffer, int32_t size) {
    if (!isStreaming_.load() || size <= 0) return 0;

    // čo je dostupné, to vráť (Soapy/RTL štýl)
    size_t got = ring_.popBlock(buffer, (size_t)size);
    if (got > 0) return (int32_t)got;

    std::unique_lock<std::mutex> lk(cvMtx_);
    cvData_.wait_for(lk, std::chrono::milliseconds(20));

    got = ring_.popBlock(buffer, (size_t)size);
    return (int32_t)got;
}

void SDRPlayInput::setFrequency(int frequency) {
    requestedFrequency_.store(frequency, std::memory_order_relaxed);
    currentFrequency_.store(frequency, std::memory_order_relaxed);
    retunePending_.store(true, std::memory_order_relaxed);
}

int SDRPlayInput::getFrequency() const {
    return currentFrequency_.load(std::memory_order_relaxed);
}

float SDRPlayInput::setGain(int gain) {
    currentGainIndex_.store(gain, std::memory_order_relaxed);
    return (float)gain;
}

float SDRPlayInput::getGain() const {
    return (float)currentGainIndex_.load(std::memory_order_relaxed);
}

int SDRPlayInput::getGainCount() { return 100; }
void SDRPlayInput::setAgc(bool agc) { (void)agc; }
bool SDRPlayInput::is_ok() { return isStreaming_.load(); }
void SDRPlayInput::reset() { stop(); restart(); }
std::vector<std::complex<float>> SDRPlayInput::getSpectrumSamples(int) { return {}; }

int32_t SDRPlayInput::getSamplesToRead() {
    return (int32_t)ring_.available();
}

std::string SDRPlayInput::getDescription() { return "SDRplay RSP (native)"; }
CDeviceID SDRPlayInput::getID() { return CDeviceID::SDRPLAY; }

void SDRPlayInput::controlThreadMain() {
    while (ctrlRun_.load(std::memory_order_relaxed)) {
      
      if (overloadAckPending_.exchange(false, std::memory_order_relaxed)) {
          if (isStreaming_.load(std::memory_order_relaxed) &&
              !stopping_.load(std::memory_order_relaxed) &&
              apiSelected_) {

              std::lock_guard<std::mutex> lk(apiMtx_);
              auto err = sdrplay_api_Update(chosenDevice.dev, chosenDevice.tuner,
                                            sdrplay_api_Update_Ctrl_OverloadMsgAck,
                                            sdrplay_api_Update_Ext1_None);
        if (err != sdrplay_api_Success) {
            std::cerr << "[SDRPlay] OverloadMsgAck failed: " << sdrErr(err) << "\n";
        } else {
            std::cerr << "[SDRPlay] OverloadMsgAck sent\n";
        }
    }
}

        if (retunePending_.exchange(false, std::memory_order_relaxed)) {
            if (retuneDebounceMs_ > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(retuneDebounceMs_));
                while (retunePending_.exchange(false, std::memory_order_relaxed)) {
                   
                }
            }

            const int f = requestedFrequency_.load(std::memory_order_relaxed);
            if (f > 0 && isStreaming_.load(std::memory_order_relaxed) && deviceParams) {
                const double rfHz = (double)f + loOffsetHz_;
                applyTuningAndGainUnsafe(rfHz);
                currentFrequency_.store(f, std::memory_order_relaxed);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

void SDRPlayInput::applyTuningAndGainUnsafe(double rfHz) {
    ring_.flushSafe();

    const double fsBase = 2048000.0;
    const double fs = fsBase * (1.0 + (srPpm_ * 1e-6));

    discardSamples_.store((int64_t)(fs * (double)discardMs_ / 1000.0), std::memory_order_relaxed);
    resetNco_.store(true, std::memory_order_relaxed);

    deviceParams->rxChannelA->tunerParams.rfFreq.rfHz = rfHz;

    std::lock_guard<std::mutex> lk(apiMtx_);

    auto err = sdrplay_api_Update(chosenDevice.dev,
                                  chosenDevice.tuner,
                                  sdrplay_api_Update_Tuner_Frf,
                                  sdrplay_api_Update_Ext1_None);
    if (err != sdrplay_api_Success) {
        std::cerr << "[SDRPlay] Update(FRF) failed: " << sdrErr(err) << "\n";
    }

    deviceParams->rxChannelA->ctrlParams.agc.setPoint_dBfs = agcSetpointDbfs_;
    deviceParams->rxChannelA->ctrlParams.agc.enable = agcFromHz(agcHz_);
    sdrplay_api_Update(chosenDevice.dev, chosenDevice.tuner,
                       sdrplay_api_Update_Ctrl_Agc, sdrplay_api_Update_Ext1_None);

    deviceParams->rxChannelA->tunerParams.gain.gRdB = clampInt(grDb_, 0, MAX_BB_GR);
    deviceParams->rxChannelA->tunerParams.gain.LNAstate = clampInt(lnaState_, 0, 30);
    sdrplay_api_Update(chosenDevice.dev, chosenDevice.tuner,
                       sdrplay_api_Update_Tuner_Gr, sdrplay_api_Update_Ext1_None);

    ring_.flushSafe();

    std::cout << "[SDRPlay] Retuned HW to: " << rfHz
              << " Hz (dropped=" << ring_.dropped() << ")\n";
}

void SDRPlayInput::streamCallback(short *xi, short *xq,
                                  sdrplay_api_StreamCbParamsT *params,
                                  unsigned int numSamples,
                                  unsigned int reset,
                                  void *cbContext) {
    (void)params;
    SDRPlayInput* self = (SDRPlayInput*)cbContext;
    if (!self) return;

    if (reset) {
        self->ring_.flushSafe();
        self->discardSamples_.store(0, std::memory_order_relaxed);
        self->resetNco_.store(true, std::memory_order_relaxed);
        return;
    }

    if (self->resetNco_.exchange(false, std::memory_order_relaxed)) {
        self->ncoPhase_ = {1.0f, 0.0f};
    }

    int64_t d = self->discardSamples_.load(std::memory_order_relaxed);
    if (d > 0) {
        d -= (int64_t)numSamples;
        if (d < 0) d = 0;
        self->discardSamples_.store(d, std::memory_order_relaxed);
        return;
    }

    if (self->cbTmp_.size() < numSamples) self->cbTmp_.resize(numSamples);

    const float k = 1.0f / self->denominator_;
    const bool useNco = (self->loOffsetHz_ != 0.0);

    for (unsigned i = 0; i < numSamples; i++) {
        float I = (float)xi[i] * k;
        float Q = (float)xq[i] * k;

        if (self->swapIQ_) { float t = I; I = Q; Q = t; }
        if (self->invertQ_) Q = -Q;

        std::complex<float> s(I, Q);

        if (useNco) {
            s *= self->ncoPhase_;
            self->ncoPhase_ *= self->ncoStep_;
        }

        self->cbTmp_[i] = s;
    }

    self->ring_.pushBlock(self->cbTmp_.data(), numSamples);
    self->cvData_.notify_one();
}

void SDRPlayInput::eventCallback(sdrplay_api_EventT eventId,
                                 sdrplay_api_TunerSelectT tuner,
                                 sdrplay_api_EventParamsT *params,
                                 void *cbContext)
{
    SDRPlayInput* self = (SDRPlayInput*)cbContext;
    (void)tuner;
    (void)params;
    if (!self) return;

    if (eventId != sdrplay_api_PowerOverloadChange) return;

    if (self->stopping_.load(std::memory_order_relaxed)) {
        return;
    }

    bool already = self->overloadAckPending_.exchange(true, std::memory_order_relaxed);
    if (!already) {
        std::cerr << "[SDRPlay] PowerOverloadChange (queued)\n";
    }
}
