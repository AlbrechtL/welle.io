/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *
 *    This file is part of the welle.io.
 *    Many of the ideas as implemented in welle.io are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are recognized.
 *
 *    welle.io is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    welle.io is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with welle.io; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <chrono>
#include <condition_variable>
#include <deque>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include <cstdint>
#include <lame/lame.h>
#include "backend/radio-receiver.h"
#include "various/Socket.h"
#include "various/channels.h"

class ProgrammeSender {
    private:
        Socket s;

        bool running = true;
        std::condition_variable cv;
        std::mutex mutex;

    public:
        ProgrammeSender(Socket&& s);
        bool send_mp3(const std::vector<uint8_t>& mp3data);
        void wait_for_termination();
        void cancel();
};

struct Lame {
    lame_t lame;

    Lame() {
        lame = lame_init();
    }

    Lame(const Lame& other) = delete;
    Lame& operator=(const Lame& other) = delete;
    Lame(Lame&& other) = default;
    Lame& operator=(Lame&& other) = default;

    ~Lame() {
        lame_close(lame);
    }
};

enum class MOTType { JPEG, PNG, Unknown };

class WebProgrammeHandler : public ProgrammeHandlerInterface {
    private:
        uint32_t serviceId;

        bool lame_initialised = false;
        Lame lame;

        int last_frameErrors = -1;
        int last_rsErrors = -1;
        int last_aacErrors = -1;

        mutable std::mutex senders_mutex;
        std::list<ProgrammeSender*> senders;

        mutable std::mutex pad_mutex;

        bool last_label_valid = false;
        std::chrono::time_point<std::chrono::system_clock> time_label;
        std::string last_label;

        bool last_mot_valid = false;
        std::chrono::time_point<std::chrono::system_clock> time_mot;
        std::vector<uint8_t> last_mot;
        MOTType last_subtype = MOTType::Unknown;

    public:
        bool stereo = false;
        int rate = 0;
        int last_audioLevel_L = -1;
        int last_audioLevel_R = -1;

        WebProgrammeHandler(uint32_t serviceId);
        WebProgrammeHandler(WebProgrammeHandler&& other);

        void registerSender(ProgrammeSender *sender);
        void removeSender(ProgrammeSender *sender);
        bool needsToBeDecoded() const;
        void cancelAll();

        struct dls_t {
            std::string label;
            time_t time = -1; };
        dls_t getDLS() const;

        struct mot_t {
            std::string data;
            MOTType subtype = MOTType::Unknown;
            time_t time = -1; };
        mot_t getMOT_base64() const;

        virtual void onFrameErrors(int frameErrors) override;
        virtual void onNewAudio(std::vector<int16_t>&& audioData,
                int sampleRate, bool isStereo) override;
        virtual void onRsErrors(int rsErrors) override;
        virtual void onAacErrors(int aacErrors) override;
        virtual void onNewDynamicLabel(const std::string& label) override;
        virtual void onMOT(const std::vector<uint8_t>& data, int subtype) override;
};

class WebRadioInterface : public RadioControllerInterface {
    public:
        enum class DecodeStrategy {
            OnDemand, // Decode services only on-demand
            All,      // Decode all services simultaneously
            Carousel  // Decode all services one by one, for 30s
        };
        WebRadioInterface(CVirtualInput& in, int port, DecodeStrategy ds);
        ~WebRadioInterface();
        WebRadioInterface(const WebRadioInterface&) = delete;
        WebRadioInterface& operator=(const WebRadioInterface&) = delete;

        void serve();

        virtual void onSNR(int snr) override;
        virtual void onFrequencyCorrectorChange(int fine, int coarse) override;
        virtual void onSyncChange(char isSync) override;
        virtual void onSignalPresence(bool isSignal) override;
        virtual void onServiceDetected(uint32_t sId, const std::string& label) override;
        virtual void onNewEnsembleName(const std::string& name) override;
        virtual void onDateTimeUpdate(const dab_date_time_t& dateTime) override;
        virtual void onFIBDecodeSuccess(bool crcCheckOk, const uint8_t* fib) override;
        virtual void onNewImpulseResponse(std::vector<float>&& data) override;
        virtual void onNewNullSymbol(std::vector<DSPCOMPLEX>&& data) override;
        virtual void onConstellationPoints(std::vector<DSPCOMPLEX>&& data) override;
        virtual void onMessage(message_level_t level, const std::string& text) override;
        virtual void onTIIMeasurement(tii_measurement_t&& m) override;

    private:
        void retune(const std::string& channel);

        bool dispatch_client(Socket&& client);
        // Send the index.html file
        bool send_index(Socket& s);

        // Generate and send the mux.json
        bool send_mux_json(Socket& s);

        // Send an mp3 stream containing the selected programme.
        // stream is a service id, either in hex with 0x prefix or
        // in decimal
        bool send_mp3(Socket& s, const std::string& stream);

        // Send the Fast Information Channel as a stream.
        // Every FIB is 32 bytes long, there three FIBs per 24ms interval,
        // which gives 32000 bits/s
        bool send_fic(Socket& s);

        // Send the impulse response, in dB, as a sequence of float values.
        bool send_impulseresponse(Socket& s);

        // Send the signal spectrum, in dB, as a sequence of float values.
        bool send_spectrum(Socket& s);
        bool send_null_spectrum(Socket& s);

        // Send the constellation points, a sequence of phases between -180 and 180 .
        bool send_constellation(Socket& s);

        // Send the currently tuned channel
        bool send_channel(Socket& s);

        // Handle a POST to /channel that will tune the receiver
        bool handle_channel_post(Socket& s, const std::string& request);

        void handle_phs();
        void check_decoders_required();
        std::list<tii_measurement_t> getTiiStats();

        std::thread programme_handler_thread;
        bool running = true;

        Channels channels;
        DABParams dabparams;
        CVirtualInput& input;
        common_fft spectrum_fft_handler;

        DecodeStrategy decode_strategy = DecodeStrategy::OnDemand;

        mutable std::mutex data_mut;
        bool synced = 0;
        int last_snr = 0;
        int last_fine_correction = 0;
        int last_coarse_correction = 0;
        dab_date_time_t last_dateTime;
        std::deque<std::pair<message_level_t, std::string> > pending_messages;

        mutable std::mutex plotdata_mut;
        std::vector<float> last_CIR;
        std::vector<DSPCOMPLEX> last_NULL;
        std::vector<DSPCOMPLEX> last_constellation;

        mutable std::mutex fib_mut;
        std::condition_variable new_fib_block_available;
        std::deque<std::vector<uint8_t> > fib_blocks;

        using comb_pattern_t = std::pair<int, int>;

        std::chrono::time_point<std::chrono::steady_clock> time_last_tiis_clean;
        std::map<comb_pattern_t, std::list<tii_measurement_t> > tiis;

        Socket serverSocket;

        mutable std::mutex rx_mut;
        std::unique_ptr<RadioReceiver> rx;

        using SId_t = uint32_t;
        std::map<SId_t, WebProgrammeHandler> phs;
        std::map<SId_t, bool> programmes_being_decoded;
        std::condition_variable phs_changed;

        SId_t current_carousel_service = 0;
        std::chrono::time_point<std::chrono::steady_clock> time_carousel_change;
};
