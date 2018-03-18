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

#include <iostream>
#include "backend/radio-receiver.h"

using namespace std;

class RadioInterface : public RadioControllerInterface {
    public:
        virtual void onFrameErrors(int frameErrors) override { }
        virtual void onNewAudio(std::vector<int16_t>&& audioData, int sampleRate) override { }
        virtual void onStereoChange(bool isStereo) override { }
        virtual void onRsErrors(int rsErrors) override { }
        virtual void onAacErrors(int aacErrors) override { }
        virtual void onNewDynamicLabel(const std::string& label) override { }
        virtual void onMOT(const std::vector<uint8_t>& data, int subtype) override { }
        virtual void onSNR(int snr) override { }
        virtual void onFrequencyCorrectorChange(int fine, int coarse) override { }
        virtual void onSyncChange(char isSync) override { }
        virtual void onSignalPresence(bool isSignal) override { }
        virtual void onServiceDetected(uint32_t sId, const std::string& label) override { }
        virtual void onNewEnsembleName(const std::string& name) override { }
        virtual void onDateTimeUpdate(const dab_date_time_t& dateTime) override { }
        virtual void onFICDecodeSuccess(bool isFICCRC) override { }
        virtual void onNewImpulseResponse(std::vector<float>&& data) override { }
};

int main(int argc, char **argv)
{
    cerr << "Hello this is welle-cli" << endl;

    /*
    RadioInterface ri;
    auto in = CInputFactory::GetDevice(ri, "auto");
    RadioReceiver rx(ri, in);
    */

    return 0;
}
