/******************************************************************************\
 * Copyright (c) 2017 Albrecht Lohofener <albrechtloh@gmx.de>
 *
 * Author(s):
 * Albrecht Lohofener
 *
 * Description:
 * This class adapts the dablin sources "pad_decoder.cpp" and "mot_manager.cpp" to dab-rpi
 *
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#ifndef PAD_DECODER_ADAPTER_H
#define PAD_DECODER_ADAPTER_H

#include <QObject>
#include <QString>
#include "pad_decoder.h"
#include "gui.h"

class PADDecoderAdapter: public QObject, PADDecoderObserver
{
    Q_OBJECT
public:
    PADDecoderAdapter(RadioInterface *mr);

    // from PADDecoderObserver
    void PADChangeDynamicLabel(void);
    void PADChangeSlide(void);

    // Adapter
    void processPAD(uint8_t *theAU);

private:
    PADDecoder  *padDecoder;
    RadioInterface	*radioInterface;

signals:
    void showLabel (QString);
    void the_picture (QByteArray, int, QString);

};

#endif // PAD_DECODER_ADAPTER_H
