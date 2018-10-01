/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *
 *    Copyright (C) 2017
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

#include <iostream>

#include "input_factory.h"
#include "null_device.h"
#include "rtl_tcp.h"
#include "raw_file.h"

#ifdef HAVE_RTLSDR
#include "rtl_sdr.h"
#endif

#ifdef HAVE_AIRSPY
#include "airspy_sdr.h"
#endif

#ifdef HAVE_SOAPYSDR
#include "soapy_sdr.h"
#endif

CVirtualInput *CInputFactory::GetDevice(RadioControllerInterface& radioController, const std::string& device)
{
    CVirtualInput *InputDevice = nullptr;

    std::clog << "InputFactory:" << "Input device:" << device << std::endl;

    if (device == "auto")
        InputDevice = GetAutoDevice(radioController);
    else
        InputDevice = GetManualDevice(radioController, device);

    // Fallback if no device is found or an error occured
    if (InputDevice == nullptr) {
        std::string text;

        if (device == "auto")
            text = "No valid device found use Null device instead.";
        else
            text = "Error while opening device";

        radioController.onMessage(message_level_t::Error, text);
        InputDevice = new CNullDevice();
    }

    return InputDevice;
}

CVirtualInput* CInputFactory::GetAutoDevice(RadioControllerInterface& radioController)
{
    (void)radioController;
    CVirtualInput *inputDevice = nullptr;

    // Try to find a input device
    for (int i = 0; i <= 2; i++) {
        try {
            switch(i) {
#ifdef HAVE_AIRSPY
            case 0: inputDevice = new CAirspy(); break;
#endif
#ifdef HAVE_RTLSDR
            case 1: inputDevice = new CRTL_SDR(radioController); break;
#endif
#ifdef HAVE_SOAPYSDR
            case 2: inputDevice = new CSoapySdr(); break;
#endif
            }
        }
        catch (...) {
            // An error occured. Maybe the device isn't present.
            // Just try the next input device
        }

        // Break loop if we found a device
        if (inputDevice != nullptr)
            break;
    }

    return inputDevice;
}

CVirtualInput* CInputFactory::GetManualDevice(RadioControllerInterface& radioController, const std::string& device)
{
    CVirtualInput *InputDevice = nullptr;

    try {
#ifdef HAVE_AIRSPY
        if (device == "airspy")
            InputDevice = new CAirspy();
        else
#endif
        if (device == "rtl_tcp")
            InputDevice = new CRTL_TCP_Client(radioController);
        else
#ifdef HAVE_RTLSDR
        if (device == "rtl_sdr")
            InputDevice = new CRTL_SDR(radioController);
        else
#endif
#ifdef HAVE_SOAPYSDR
        if (device == "soapysdr")
            InputDevice = new CSoapySdr();
        else
#endif
        if (device == "rawfile")
            InputDevice = new CRAWFile(radioController);
        else
            std::clog << "InputFactory:"
                "Unknown device \"" << device << "\"." << std::endl;
    }
    catch (...) {
        std::clog << "InputFactory:"
            "Error while opening device \"" << device << "\"." << std::endl;
    }

    return InputDevice;
}
