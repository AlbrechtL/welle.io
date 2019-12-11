/*
 *    Copyright (C) 2018
 *    Albrecht Lohofener (albrechtloh@gmx.de)
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

#include <QtTest>

#include <algorithm>
#include <numeric>
#include <random>
#include <condition_variable>
#include <deque>
#include <iostream>
#include <utility>
#include <cstdio>

#include "radio-receiver.h"
#include "raw_file.h"

class TestRadioInterface : public RadioControllerInterface {
    public:
        virtual void onSNR(int snr) override { (void)snr; }
        virtual void onFrequencyCorrectorChange(int fine, int coarse) override { (void)fine; (void)coarse; }
        virtual void onSyncChange(char isSync) override { (void)isSync; }
        virtual void onSignalPresence(bool isSignal) override { (void)isSignal; }
        virtual void onServiceDetected(uint32_t sId) override { (void)sId; }

        virtual void onNewEnsemble(uint16_t /*eId*/) override { }
        virtual void onSetEnsembleLabel(DabLabel& /*label*/) override { }
        virtual void onDateTimeUpdate(const dab_date_time_t& dateTime) override { (void)dateTime; }
        virtual void onFIBDecodeSuccess(bool crcCheckOk, const uint8_t* fib) override { (void)crcCheckOk; (void)fib; }
        virtual void onNewImpulseResponse(std::vector<float>&& data) override { (void)data; }

        virtual void onNewNullSymbol(std::vector<DSPCOMPLEX>&& data) override { (void)data; }
        virtual void onConstellationPoints(std::vector<DSPCOMPLEX>&& data) override { (void)data; }
        virtual void onMessage(message_level_t level, const std::string& text, const std::string& text2 = std::string()) override { (void) level; (void)text; (void)text2;}

        virtual void onTIIMeasurement(tii_measurement_t&& m) override { (void)m; }
};

class TestProgrammeHandler: public ProgrammeHandlerInterface {

public:
    std::string label = "";

    virtual void onFrameErrors(int frameErrors) override { (void)frameErrors; }

    virtual void onNewAudio(std::vector<int16_t>&& audioData, int sampleRate, const std::string& mode) override {
        (void) audioData; (void)sampleRate; (void)mode;}

    virtual void onRsErrors(bool uncorrectedErrors, int numCorrectedErrors) override {
        (void)uncorrectedErrors; (void)numCorrectedErrors; }
    virtual void onAacErrors(int aacErrors) override { (void)aacErrors; }
    virtual void onNewDynamicLabel(const std::string& label) override
    {
        this->label = label;
    }

    virtual void onMOT(const mot_file_t& mot_file) override { (void)mot_file;}
    virtual void onPADLengthError(size_t announced_xpad_len, size_t xpad_len) override { (void)announced_xpad_len; (void) xpad_len;}
};

class BackendTests : public QObject
{
    Q_OBJECT

public:
    BackendTests() {}
    ~BackendTests() {}

private slots:
    void initTestCase() {}
    void cleanupTestCase() {}
    void testTuneToService();
    void testDLS();

private:
    void runRadio(const std::string &rawFileName,
                       const std::string &serviceName,
                       std::function<void (bool&, TestProgrammeHandler &)> work);
};

void BackendTests::runRadio(const std::string &rawFileName,
                              const std::string &serviceName,
                              std::function<void (bool &exit_test, TestProgrammeHandler &testProgrammeHandler)> work)
{
    TestRadioInterface testRadioInterface;
    RadioReceiverOptions radioReceiverOptions;
    TestProgrammeHandler testProgrammeHandler;

    CRAWFile rawFile(testRadioInterface, true, false);
    rawFile.setFileName(rawFileName, "auto");

    RadioReceiver radioReceiver(testRadioInterface, rawFile, radioReceiverOptions);

    rawFile.restart();
    radioReceiver.restart(false);

    bool service_selected = false;
    while (not service_selected) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        for (const auto service : radioReceiver.getServiceList())
        {
//            std::cout << "Found service \"" << service.serviceLabel.utf8_label() << "\""<< std::endl;

            if(service.serviceLabel.utf8_label() == serviceName)
            {
                if (radioReceiver.playSingleProgramme(testProgrammeHandler, "", service) == false)
                {
                    std::cerr << "Tune to " << service.serviceLabel.utf8_label() << " failed" << std::endl;
                }
                else
                {
                    std::cout << "Tune to " << service.serviceLabel.utf8_label() << " succesfully" << std::endl;
                    service_selected = true;
                }
                break;
            }
        }

        if(rawFile.endWasReached())
            break;
    }

    if(service_selected)
    {
        bool exit_test = false;
        while (not rawFile.endWasReached() and not exit_test) {
            std::this_thread::sleep_for(std::chrono::milliseconds(120));

            work(exit_test, testProgrammeHandler);
        }
    }
}

void BackendTests::testTuneToService()
{
    bool isOK = false;
    runRadio(
            "../../../DAB-Test/20160827_202005_5C.iq",
            "Deutschlandfunk ",
            [&isOK](bool &exit_test, TestProgrammeHandler &)
            {
                isOK = true;
                exit_test = true;
            });

     QCOMPARE(isOK, true);
}

void BackendTests::testDLS()
{
    bool isOK = false;
    runRadio(
            "../../../DAB-Test/20160827_202005_5C.iq",
            "Deutschlandfunk  ",
            [&isOK](bool &exit_test, TestProgrammeHandler &testProgrammeHandler)
            {
//                std::cout << "DLS: \"" << testProgrammeHandler.label << "\"" << std::endl;
                if(testProgrammeHandler.label == "Studio LCB")
                {
                    exit_test = true;
                    isOK = true;
                }
                else
                {
                    isOK = true;
                }
            });

    QCOMPARE(isOK, true);
}

QTEST_APPLESS_MAIN(BackendTests)

#include "backend_tests.moc"
