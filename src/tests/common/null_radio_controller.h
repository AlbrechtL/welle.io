/*
 *    Copyright (C) 2026
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is part of the welle.io.
 *    This file was generated with an AI assistant (Claude Code).
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

#ifndef NULL_RADIO_CONTROLLER_H
#define NULL_RADIO_CONTROLLER_H

#include "radio-controller.h"

// RadioControllerInterface that ignores every callback. Tests derive from it
// and override only the callbacks they are interested in.
class NullRadioController : public RadioControllerInterface {
public:
    void onSNR(float) override {}
    void onFrequencyCorrectorChange(int, int) override {}
    void onSyncChange(char) override {}
    void onSignalPresence(bool) override {}
    void onServiceDetected(uint32_t) override {}
    void onNewEnsemble(uint16_t) override {}
    void onSetEnsembleLabel(DabLabel&) override {}
    void onDateTimeUpdate(const dab_date_time_t&) override {}
    void onFIBDecodeSuccess(bool, const uint8_t*) override {}
    void onNewImpulseResponse(std::vector<float>&&) override {}
    void onConstellationPoints(std::vector<DSPCOMPLEX>&&) override {}
    void onNewNullSymbol(std::vector<DSPCOMPLEX>&&) override {}
    void onTIIMeasurement(tii_measurement_t&&) override {}
    void onMessage(message_level_t, const std::string&, const std::string& = std::string()) override {}
};

#endif // NULL_RADIO_CONTROLLER_H
