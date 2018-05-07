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

#include <string>
#include <iostream>
#include <memory>
#include "radio-receiver.h"

using namespace std;

static constexpr size_t AUDIOBUFFERSIZE = 32768;

RadioReceiver::RadioReceiver(
                RadioControllerInterface& rci,
                InputInterface& input) :
    rci(rci),
    input(input),
    mscHandler(params, false),
    ficHandler(rci),
    ofdmProcessor(input,
        params,
        rci,
        mscHandler,
        ficHandler,
        3)
{ }

void RadioReceiver::restart(bool doScan)
{
    ofdmProcessor.set_scanMode(doScan);
    mscHandler.stopProcessing();
    ficHandler.clearEnsemble();
    ofdmProcessor.coarseCorrectorOn();
    ofdmProcessor.reset();
}

bool RadioReceiver::playSingleProgramme(ProgrammeHandlerInterface& handler,
        const std::string& dumpFileName, const Service& s)
{
    return playProgramme(handler, s, dumpFileName, true);
}

bool RadioReceiver::addServiceToDecode(ProgrammeHandlerInterface& handler,
        const std::string& dumpFileName, const Service& s)
{
    return playProgramme(handler, s, dumpFileName, false);
}

bool RadioReceiver::removeServiceToDecode(const Service& s)
{
    const auto comps = ficHandler.fibProcessor.getComponents(s);
    for (const auto& sc : comps) {
        if (sc.transportMode() == TransportMode::Audio) {
            const auto& subch = ficHandler.fibProcessor.getSubchannel(sc);
            if (subch.valid()) {
                return mscHandler.removeSubchannel(subch);
            }
        }
    }
    return false;
}

bool RadioReceiver::playProgramme(ProgrammeHandlerInterface& handler,
        const Service& s, const std::string& dumpFileName, bool unique)
{
    const auto comps = ficHandler.fibProcessor.getComponents(s);
    for (const auto& sc : comps) {
        if (sc.transportMode() == TransportMode::Audio) {
            const auto& subch = ficHandler.fibProcessor.getSubchannel(sc);

            if (subch.valid()) {
                if (unique) {
                    mscHandler.stopProcessing();
                }

                if (sc.audioType() == AudioServiceComponentType::DAB ||
                    sc.audioType() == AudioServiceComponentType::DABPlus) {
                    mscHandler.addSubchannel(
                            handler, sc.audioType(), dumpFileName, subch);
                    return true;
                }
            }
        }
    }

    return false;
}

uint16_t RadioReceiver::getEnsembleId(void) const
{
    return ficHandler.fibProcessor.getEnsembleId();
}

DabLabel RadioReceiver::getEnsembleLabel(void) const
{
    return ficHandler.fibProcessor.getEnsembleLabel();
}

std::vector<Service> RadioReceiver::getServiceList(void) const
{
    return ficHandler.fibProcessor.getServiceList();
}

std::list<ServiceComponent> RadioReceiver::getComponents(const Service& s) const
{
    return ficHandler.fibProcessor.getComponents(s);
}

Subchannel RadioReceiver::getSubchannel(const ServiceComponent& sc) const
{
    return ficHandler.fibProcessor.getSubchannel(sc);
}
