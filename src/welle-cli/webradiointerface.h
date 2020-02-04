/*
 *    Copyright (C) 2020
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
#pragma once

#include <atomic>
#include <thread>
#include <utility>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include <cstdint>
#include <cstddef>
#include "backend/dab-constants.h"
#include "backend/radio-controller.h"
#include "various/fft.h"
#include "various/Socket.h"
#include "various/channels.h"
#include "webprogrammehandler.h"
#include "radio-receiver-options.h"

class CVirtualInput; // from input/virtual_input.h
class RadioReceiver; // from backend/radio_receiver.h

class WebRadioInterface : public RadioControllerInterface {
    public:
        enum class DecodeStrategy {
            /* Decode services only on-demand */
            OnDemand,

            /* Decode all services simultaneously */
            All,

            /* Decode all services one by one, for 10s */
            Carousel10,

            /* Decode all services one by one, switch when
             * DLS and slide were decoded, stay at most 80s on one service.  */
            CarouselPAD
        };

        struct DecodeSettings {
            DecodeStrategy strategy = DecodeStrategy::OnDemand;
            int num_decoders_in_carousel = 0;
        };

        WebRadioInterface(
                CVirtualInput& in,
                int port,
                DecodeSettings cs,
                RadioReceiverOptions rro);
        ~WebRadioInterface();
        WebRadioInterface(const WebRadioInterface&) = delete;
        WebRadioInterface& operator=(const WebRadioInterface&) = delete;

        void serve();

        virtual void onSNR(int snr) override;
        virtual void onFrequencyCorrectorChange(int fine, int coarse) override;
        virtual void onSyncChange(char isSync) override;
        virtual void onSignalPresence(bool isSignal) override;
        virtual void onServiceDetected(uint32_t sId) override;
        virtual void onNewEnsemble(uint16_t eId) override;
        virtual void onSetEnsembleLabel(DabLabel& label) override;
        virtual void onDateTimeUpdate(const dab_date_time_t& dateTime) override;
        virtual void onFIBDecodeSuccess(bool crcCheckOk, const uint8_t* fib) override;
        virtual void onNewImpulseResponse(std::vector<float>&& data) override;
        virtual void onNewNullSymbol(std::vector<DSPCOMPLEX>&& data) override;
        virtual void onConstellationPoints(std::vector<DSPCOMPLEX>&& data) override;
        virtual void onMessage(message_level_t level, const std::string& text, const std::string& text2 = std::string()) override;
        virtual void onTIIMeasurement(tii_measurement_t&& m) override;
        virtual void onInputFailure() override;

    private:
        std::mutex retune_mut;
        void retune(const std::string& channel);

        bool dispatch_client(Socket&& client);
        // Send a file
        bool send_file(Socket& s,
                const std::string& filename,
                const std::string& content_type);

        // Generate and send the mux.json
        bool send_mux_json(Socket& s);

        // Send an mp3 stream containing the selected programme.
        // stream is a service id, either in hex with 0x prefix or
        // in decimal
        bool send_mp3(Socket& s, const std::string& stream);

        // Send the slide for the selected programme.
        // stream is a service id, either in hex with 0x prefix or
        // in decimal
        bool send_slide(Socket& s, const std::string& stream);

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

        // Handle a POSTs
        bool handle_fft_window_placement_post(Socket& s, const std::string& request);
        bool handle_coarse_corrector_post(Socket& s, const std::string& request);

        // Handle a POST to /channel that will tune the receiver
        bool handle_channel_post(Socket& s, const std::string& request);

        void handle_phs();
        void check_decoders_required();
        std::list<tii_measurement_t> getTiiStats();

        std::thread programme_handler_thread;
        std::atomic<bool> running = ATOMIC_VAR_INIT(true);

        Channels channels;
        DABParams dabparams;
        CVirtualInput& input;
        fft::Forward spectrum_fft_handler;

        RadioReceiverOptions rro;
        DecodeSettings decode_settings;

        mutable std::mutex data_mut;
        bool synced = 0;
        int last_snr = 0;
        int last_fine_correction = 0;
        int last_coarse_correction = 0;
        dab_date_time_t last_dateTime;

        struct pending_message_t {
            message_level_t level;
            std::string text;
            std::chrono::time_point<std::chrono::system_clock> timestamp;
        };

        std::deque<pending_message_t> pending_messages;

        mutable std::mutex plotdata_mut;
        std::vector<float> last_CIR;
        std::vector<DSPCOMPLEX> last_NULL;
        std::vector<DSPCOMPLEX> last_constellation;

        mutable std::mutex fib_mut;
        size_t num_fic_crc_errors = 0;
        std::condition_variable new_fib_block_available;
        std::deque<std::vector<uint8_t> > fib_blocks;

        using comb_pattern_t = std::pair<int, int>;

        std::chrono::time_point<std::chrono::steady_clock> time_last_tiis_clean;
        std::map<comb_pattern_t, std::list<tii_measurement_t> > tiis;

        Socket serverSocket;

        mutable std::mutex rx_mut;
        std::chrono::time_point<std::chrono::system_clock> time_rx_created;
        std::unique_ptr<RadioReceiver> rx;

        using SId_t = uint32_t;
        std::map<SId_t, WebProgrammeHandler> phs;
        std::map<SId_t, bool> programmes_being_decoded;
        std::condition_variable phs_changed;

        std::list<SId_t> carousel_services_available;
        struct ActiveCarouselService {
            explicit ActiveCarouselService(SId_t sid) : sid(sid) {
                time_change = std::chrono::steady_clock::now();
            }
            SId_t sid;
            std::chrono::time_point<std::chrono::steady_clock> time_change;
        };
        std::list<ActiveCarouselService> carousel_services_active;
};
