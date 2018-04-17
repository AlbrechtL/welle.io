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

#include <vector>
#include <deque>
#include <chrono>
#include <list>
#include <map>
#include <memory>
#include <cstdint>
#include "backend/radio-receiver.h"
#include "various/Socket.h"


#if 0
class WebProgrammeHandler : public ProgrammeHandlerInterface {
    private:
        Socket s;
    public:
        virtual void onFrameErrors(int frameErrors) override;
        virtual void onNewAudio(std::vector<int16_t>&& audioData, int sampleRate, bool isStereo) override;


        virtual void onRsErrors(int rsErrors) override { (void)rsErrors; }
        virtual void onAacErrors(int aacErrors) override { (void)aacErrors; }
        virtual void onNewDynamicLabel(const std::string& label) override
        {
            cout << "DLS: " << label << endl;
        }

        virtual void onMOT(const std::vector<uint8_t>& data, int subtype) override { (void)data; (void)subtype; }
};
#endif

class WebRadioInterface : public RadioControllerInterface {
    public:
        WebRadioInterface(CVirtualInput& in, int port);

        void serve();

        virtual void onSNR(int snr) override;
        virtual void onFrequencyCorrectorChange(int fine, int coarse) override;
        virtual void onSyncChange(char isSync) override;
        virtual void onSignalPresence(bool isSignal) override;
        virtual void onServiceDetected(uint32_t sId, const std::string& label) override;
        virtual void onNewEnsembleName(const std::string& name) override;
        virtual void onDateTimeUpdate(const dab_date_time_t& dateTime) override;
        virtual void onFICDecodeSuccess(bool isFICCRC) override;
        virtual void onNewImpulseResponse(std::vector<float>&& data) override;
        virtual void onNewNullSymbol(std::vector<DSPCOMPLEX>&& data) override;
        virtual void onConstellationPoints(std::vector<DSPCOMPLEX>&& data) override;
        virtual void onMessage(message_level_t level, const std::string& text) override;
        virtual void onTIIMeasurement(tii_measurement_t&& m) override;

    private:
        bool dispatch_client(Socket s);
        std::list<tii_measurement_t> getTiiStats();

        mutable std::mutex mut;
        int last_snr = 0;
        int last_fine_correction = 0;
        int last_coarse_correction = 0;
        dab_date_time_t last_dateTime;
        std::vector<float> last_CIR;
        std::vector<DSPCOMPLEX> last_NULL;
        std::vector<DSPCOMPLEX> last_constellation;
        std::deque<std::pair<message_level_t, std::string> > pending_messages;

        using comb_pattern_t = std::pair<int, int>;

        std::chrono::time_point<std::chrono::steady_clock> time_last_tiis_clean;
        std::map<comb_pattern_t, std::list<tii_measurement_t> > tiis;

        Socket serverSocket;

        std::unique_ptr<RadioReceiver> rx;
};
