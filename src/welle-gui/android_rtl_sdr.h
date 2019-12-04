/*
 *    Copyright (C) 2019
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
 *    Driver for https://github.com/martinmarinov/rtl_tcp_andro-
 */

#ifndef CANDROID_RTL_SDR_H
#define CANDROID_RTL_SDR_H

#include <QtAndroidExtras/QAndroidJniObject>
#include <QtAndroidExtras/QtAndroid>
#include <QtAndroidExtras/QAndroidActivityResultReceiver>

#include "virtual_input.h"
#include "rtl_tcp.h"

class ActivityResultReceiver;

class CAndroid_RTL_SDR : public CRTL_TCP_Client
{
public:
    CAndroid_RTL_SDR(RadioControllerInterface &RadioController);
    ~CAndroid_RTL_SDR();

    // Override
    std::string getDescription(void);
    CDeviceID getID(void);
    bool restart(void);
    bool is_ok(void);

    void setErrorMessage(QString message);
    void setLoaded(bool isLoaded);
    void setOpenInstallDialog(void);

private:
    std::unique_ptr<ActivityResultReceiver> resultReceiver;
    QString message;
    bool isLoaded = false;
    bool isPending = false;

signals:
    void showAndroidInstallDialog(QString Title, QString Text);
};

class ActivityResultReceiver : public QAndroidActivityResultReceiver
{
public:
    ActivityResultReceiver(CAndroid_RTL_SDR *Client): Android_RTL_SDR(Client){}

    virtual void handleActivityResult(int receiverRequestCode, int resultCode, const QAndroidJniObject &data);

private:
    CAndroid_RTL_SDR *Android_RTL_SDR;
};

#endif // CANDROID_RTL_SDR_H
