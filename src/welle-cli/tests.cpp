/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is based on SDR-J
 *    Copyright (C) 2010, 2011, 2012
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
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

#include "tests.h"
#include "backend/radio-receiver.h"
#include "raw_file.h"
#include "various/profiling.h"
#include <algorithm>
#include <numeric>
#include <random>
#include <condition_variable>
#include <deque>
#include <iostream>
#include <utility>
#include <cstdio>

using namespace std;

static random_device rd;
static mt19937 random_generator(rd());

class ChannelSimulator : public CVirtualInput
{
    private:
        CVirtualInput* parentInput;
        normal_distribution<> distr;

    public:
        size_t num_samps = 0;

        ChannelSimulator(unique_ptr<CVirtualInput>& parent, double stddev) :
            parentInput(parent.get()),
            distr(0.0, stddev) {}

        virtual ~ChannelSimulator() {}

        virtual CDeviceID getID(void) { return parentInput->getID(); }
        virtual void setFrequency(int frequency)
            { parentInput->setFrequency(frequency); }

        virtual int getFrequency(void) const
            { return parentInput->getFrequency(); }

        virtual bool restart(void)
            { return parentInput->restart(); }
        virtual bool is_ok(void)
            { return parentInput->is_ok(); }
        virtual void stop(void)
            { return parentInput->stop(); }
        virtual void reset(void)
            { return parentInput->reset(); }

        virtual int32_t getSamples(DSPCOMPLEX* buffer, int32_t size)
        {
            int32_t r = parentInput->getSamples(buffer, size);

            num_samps += r;

            // Add gaussian noise, with stddev on I and Q
            // This is not equivalent to complex noise of the same stddev!
            for (int32_t i = 0; i < size; i++) {
                buffer[0] += distr(random_generator);
                buffer[1] += distr(random_generator);
            }

            return r;
        }

        virtual vector<DSPCOMPLEX> getSpectrumSamples(int size)
            { return parentInput->getSpectrumSamples(size); }

        virtual int32_t getSamplesToRead(void)
            { return parentInput->getSamplesToRead(); }

        virtual float getGain() const
            { return parentInput->getGain(); }

        virtual float setGain(int gain)
            { return parentInput->setGain(gain); }

        virtual int getGainCount(void)
            { return parentInput->getGainCount(); }

        virtual void setAgc(bool agc)
            { return parentInput->setAgc(agc); }

        virtual std::string getDescription(void)
            { return parentInput->getDescription() + " with ChannelSimulator"; }
};

class TestRadioInterface : public RadioControllerInterface {
    private:
        struct FILEDeleter{ void operator()(FILE* fd){ if (fd) fclose(fd); }};
        std::unique_ptr<FILE, FILEDeleter> cirFile;
        bool first_sync_time_set = false;

    public:
        void openCIRdumpfile(const std::string& fname)
        {
            FILE *fd = fopen(fname.c_str(), "w");
            if (fd) {
                cirFile.reset(fd);
            }
        }

        virtual void onSNR(int snr) override { (void)snr; }
        virtual void onFrequencyCorrectorChange(int fine, int coarse) override { (void)fine; (void)coarse; }
        virtual void onSyncChange(char isSync) override
        {
            if (isSync) {
                num_syncs++;

                if (not first_sync_time_set) {
                    first_sync_time = chrono::steady_clock::now();
                }
            }
            else num_desyncs++;
        }
        virtual void onSignalPresence(bool isSignal) override { (void)isSignal; }
        virtual void onServiceDetected(uint32_t sId) override
        {
            cout << "New Service: 0x" << hex << sId << dec << endl;
        }

        virtual void onNewEnsemble(uint16_t eId) override
        {
            cout << "Ensemble id is: " << eId << endl;
        }

        virtual void onSetEnsembleLabel(DabLabel& label) override
        {
            cout << "Ensemble label: " << label.utf8_label() << endl;
        }

        virtual void onDateTimeUpdate(const dab_date_time_t& dateTime) override { (void)dateTime; }
        virtual void onFIBDecodeSuccess(bool crcCheckOk, const uint8_t* fib) override { (void)crcCheckOk; (void)fib; }
        virtual void onNewImpulseResponse(std::vector<float>&& data) override
        {
            if (data.size() != 2048) {
                cout << "CIR is not 2048!" << endl;
            }
            else if (cirFile) {
                fwrite(data.data(), sizeof(data[0]), data.size(), cirFile.get());
            }
        }

        virtual void onNewNullSymbol(std::vector<DSPCOMPLEX>&& data) override { (void)data; }
        virtual void onConstellationPoints(std::vector<DSPCOMPLEX>&& data) override { (void)data; }
        virtual void onMessage(message_level_t level, const std::string& text, const std::string& text2 = std::string()) override
        {
            std::string fullText;
            if (text2.empty())
                fullText = text;
            else
                fullText = text + text2;

            switch (level) {
                case message_level_t::Information:
                    cerr << "Info: " << fullText << endl;
                    break;
                case message_level_t::Error:
                    cerr << "Error: " << fullText << endl;
                    break;
            }
        }

        virtual void onTIIMeasurement(tii_measurement_t&& m) override { (void)m; }
        size_t num_syncs = 0;
        size_t num_desyncs = 0;
        chrono::steady_clock::time_point first_sync_time;
};

class TestProgrammeHandler: public ProgrammeHandlerInterface {
    private:
        int rate = 0;

    public:
        vector<int> frameErrorStats;
        vector<int> aacErrorStats;
        vector<int> rsErrorStats;

        virtual void onFrameErrors(int frameErrors) override {
            frameErrorStats.push_back(frameErrors);
        }

        virtual void onNewAudio(std::vector<int16_t>&& audioData, int sampleRate, const string& mode) override {
            (void)audioData;
            (void)mode;

            if (rate != sampleRate) {
                cout << "rate " << sampleRate << endl;
            }
            rate = sampleRate;
        }

        virtual void onRsErrors(bool uncorrectedErrors, int numCorrectedErrors) override {
            rsErrorStats.push_back(uncorrectedErrors);
            (void)numCorrectedErrors;
        }

        virtual void onAacErrors(int aacErrors) override { aacErrorStats.push_back(aacErrors); }
        virtual void onNewDynamicLabel(const std::string& label) override {
            cout << "DLS: " << label << endl;
        }

        virtual void onMOT(const mot_file_t& mot_file) override { (void)mot_file;}
        virtual void onPADLengthError(size_t announced_xpad_len, size_t xpad_len) override {
            cout << "X-PAD length mismatch, expected: " << announced_xpad_len << " got: " << xpad_len << endl;
        }
};

Tests::Tests(std::unique_ptr<CVirtualInput>& interface, RadioReceiverOptions rro) :
    input_interface(interface),
    rro(rro) {}

void Tests::test_with_noise_iteration(double stddev)
{
    cerr << "Setup test0" << endl;
    TestProgrammeHandler tph;

    ChannelSimulator s(input_interface, stddev);
    TestRadioInterface ri;
    RadioReceiver rx(ri, s, rro);

    cerr << "Restart rx" << endl;
    rx.restart(false);

    bool service_selected = false;
    string dumpFileName = "";

    while (not service_selected) {
        this_thread::sleep_for(chrono::seconds(1));

        for (const auto s : rx.getServiceList()) {
            if (rx.playSingleProgramme(tph, dumpFileName, s) == false) {
                cerr << "Tune to " << s.serviceLabel.utf8_label() << " failed" << endl;
            }
            else {
                service_selected = true;
                break;
            }
        }
    }

    cerr << "Wait for completion" << endl;
    while (not dynamic_cast<CRAWFile&>(*input_interface).endWasReached()) {
        this_thread::sleep_for(chrono::milliseconds(120));
    }

    cerr << endl;
    cerr << "STDDEV " << stddev << endl;
    cerr << "Num samps processed: " << s.num_samps << endl;
    cerr << "Num syncs/desyncs: " << ri.num_syncs << "/" << ri.num_desyncs << endl;
    cerr << "frameErrorStats (" << tph.frameErrorStats.size() << ") : " <<
        std::accumulate(tph.frameErrorStats.begin(), tph.frameErrorStats.end(), 0)
        << endl;
    cerr << "aacErrorStats (" << tph.aacErrorStats.size() << ") : " <<
        std::accumulate(tph.aacErrorStats.begin(), tph.aacErrorStats.end(), 0)
        << endl;
    cerr << "rsErrorStats (" << tph.rsErrorStats.size() << ") : " <<
        std::accumulate(tph.rsErrorStats.begin(), tph.rsErrorStats.end(), 0)
        << endl;
    cerr << endl;
}

void Tests::test_with_noise()
{
    double stddevs[] = {0.001, 0.01, 0.1};

    for (double stddev : stddevs) {
        test_with_noise_iteration(stddev);
        dynamic_cast<CRAWFile&>(*input_interface).rewind();
    }
}

void Tests::test_multipath(int test_id)
{
    cerr << "Setup test_multipath " << test_id << endl;
    TestRadioInterface ri;
    TestProgrammeHandler tph;

    // Test 1=old variant, test 2=new variant
    if (not(test_id == 1 or test_id == 2)) {
        throw logic_error("Invalid test id");
    }

    const char* cirdumpfilename = test_id == 1 ? "cir-old.dat" : "cir.dat";
    ri.openCIRdumpfile(cirdumpfilename);
    cerr << "Saving CIR to " << cirdumpfilename << endl;

    rro.fftPlacementMethod = (test_id == 1 ? FFTPlacementMethod::StrongestPeak : FFTPlacementMethod::EarliestPeakWithBinning);
    const auto start_time = chrono::steady_clock::now();
    RadioReceiver rx(ri, *input_interface.get(), rro);

    cerr << "Restart rx" << endl;
    rx.restart(false);

    bool service_selected = false;
    string dumpFileName = "";

    while (not service_selected) {
        this_thread::sleep_for(chrono::seconds(1));

        for (const auto s : rx.getServiceList()) {
            if (rx.playSingleProgramme(tph, dumpFileName, s) == false) {
                cerr << "Tune to " << s.serviceLabel.utf8_label() << " failed" << endl;
            }
            else {
                service_selected = true;
                break;
            }
        }
    }

    cerr << "Wait for completion" << endl;
    auto& intf = dynamic_cast<CRAWFile&>(*input_interface);
    while (not intf.endWasReached()) {
        this_thread::sleep_for(chrono::milliseconds(120));
    }

    FILE *fd = fopen("test1.csv", "a");
    const int pos = ftell(fd);
    if (pos == -1) {
        perror("ftell");
    }

    if (pos == 0) {
        fprintf(fd, "fname,ofdmthreshold,with_coarse,num_syncs,num_desyncs,time_to_first_sync,frameerrors,aacerrors,rserrors\n");
    }

    fprintf(fd, "%s,%s,%d,%zu,%zu,%ld,%d,%d,%d\n",
            intf.getFileName().c_str(),
            fftPlacementMethodToString(rro.fftPlacementMethod),
            rro.disableCoarseCorrector ? 0 : 1,
            ri.num_syncs, ri.num_desyncs,
            chrono::duration_cast<chrono::milliseconds>(start_time-ri.first_sync_time).count(),
            std::accumulate(tph.frameErrorStats.begin(), tph.frameErrorStats.end(), 0),
            std::accumulate(tph.aacErrorStats.begin(), tph.aacErrorStats.end(), 0),
            std::accumulate(tph.rsErrorStats.begin(), tph.rsErrorStats.end(), 0));
    fclose(fd);
}

void Tests::run_test(int test_id)
{
    rro.fftPlacementMethod = DEFAULT_FFT_PLACEMENT;

    if (test_id == 0) test_with_noise();
    else if (test_id == 1 or test_id == 2) test_multipath(test_id);
    else if (test_id == 3) test_with_noise_iteration(0);
    else cerr << "Test " << test_id << " does not exist!" << endl;
}
