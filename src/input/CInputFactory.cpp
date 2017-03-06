/*
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

#include "CInputFactory.h"
#include "CNullDevice.h"

#include "CRTL_TCP_Client.h"
#include "CRAWFile.h"

#ifdef HAVE_RTLSDR
#include "CRTL_SDR.h"
#endif

#ifdef HAVE_AIRSPY
#include "CAirspy.h"
#endif


CVirtualInput *CInputFactory::GetDevice(QString Device)
{
    CVirtualInput *InputDevice = NULL;

    fprintf(stderr, "Input device: %s\n", Device.toStdString().c_str());

    if(Device == "auto")
        InputDevice = GetAutoDevice();
    else
        InputDevice = GetManualDevice(Device);

    // Fallback if no device is found or an error occured
    if(InputDevice == NULL)
    {
        fprintf(stderr, "No valid device found use Null device instead.\n");
        InputDevice = new CNullDevice();
    }

    return InputDevice;
}

CVirtualInput *CInputFactory::GetAutoDevice()
{
    CVirtualInput *InputDevice = NULL;

    // Try to find a input device
    for(int i=0;i<2;i++) // At the moment two devices are supported
    {
        try
        {
            switch(i)
            {
#ifdef HAVE_AIRSPY
            case 0: InputDevice = new CAirspy(); break;
#endif
#ifdef HAVE_RTLSDR
            case 1: InputDevice = new CRTL_SDR(); break;
#endif
            }
        }

        // Catch all exceptions
        catch(...)
        {
            // An error occured. Maybe the device isn't present.
            // Just try the next input device
        }

        // Break loop if we found a device
        if(InputDevice != NULL)
            break;
    }

    return InputDevice;
}

CVirtualInput *CInputFactory::GetManualDevice(QString Device)
{
    CVirtualInput *InputDevice = NULL;

    try
    {
#ifdef HAVE_AIRSPY
        if (Device == "airspy")
            InputDevice = new CAirspy();
        else
#endif
        if (Device == "rtl_tcp")
            InputDevice = new CRTL_TCP_Client();
        else
#ifdef HAVE_RTLSDR
        if (Device == "rtl_sdr")
            InputDevice = new CRTL_SDR();
        else
#endif
        if (Device == "rawfile")
            InputDevice = new CRAWFile();
        else
            fprintf(stderr, "Unknown device \"%s\".\n", Device.toStdString().c_str());
    }

    // Catch all exceptions
    catch(...)
    {
        fprintf(stderr, "Error while opening device \"%s\".\n", Device.toStdString().c_str());
    }

    return InputDevice;
}
