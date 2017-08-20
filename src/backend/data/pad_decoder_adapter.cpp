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

#include "pad_decoder_adapter.h"
#include "charsets.h"

PADDecoderAdapter::PADDecoderAdapter(CRadioController *radioController)
{
    padDecoder = new PADDecoder(this, true);
    // Enable loose mode
    /* If the announced X-PAD length of a DAB+ service does not match the available
    X-PAD length i.e. if it falls below, a red `[X-PAD len]` message is shown and
    the X-PAD is discarded. However not all X-PADs may be affected and hence it may
    happen that the Dynamic Label can be processed but the MOT Slideshow cannot. To
    anyhow process affected X-PADs, a loose mode can be enabled by using `-L`. Thus
    the mentioned message will be shown in yellow then.*/

    this->radioController = radioController;

    connect (this, SIGNAL (showLabel (QString)),
             radioController, SLOT (showLabel (QString)));

    connect (this, SIGNAL (the_picture (QByteArray, int, QString)),
             radioController, SLOT (showMOT (QByteArray, int, QString)));
}

void PADDecoderAdapter::PADChangeDynamicLabel(const DL_STATE& DynamicLabel)
{
    // Convert it
    QString DynamicLabelText = toQStringUsingCharset (
                                               (const char *)&DynamicLabel.raw[0],
                                               (CharacterSet) DynamicLabel.charset,
                                               DynamicLabel.raw.size());

    emit showLabel(DynamicLabelText);

    //qDebug("PADChangeDynamicLabel: %s\n", DynamicLabelText.toStdString().c_str());
}

void PADDecoderAdapter::PADChangeSlide(const MOT_FILE& motFile)
{
    QByteArray Data((const char*) motFile.data.data(), (int) motFile.data.size());

    emit the_picture(Data, motFile.content_sub_type, motFile.content_name.c_str());

    //qDebug("PADChangeSlide: Type: %i content_name: %s click_through_url: %s\n", motFile.content_sub_type, motFile.content_name.c_str(), motFile.click_through_url.c_str());
}

void PADDecoderAdapter::processPAD(uint8_t *theAU)
{
    // Get PAD length
    uint8_t pad_start = 2;
    uint8_t pad_len = theAU[1];
    if (pad_len == 255) {
        pad_len += theAU[2];
        pad_start++;
    }

    // Adapt length to PADDecoder
    uint8_t FPAD_LEN = 2;
    size_t xpad_len = pad_len - FPAD_LEN;

    // Adapt FPAD to PADDecoder
    uint8_t *fpad = theAU + pad_start + pad_len - FPAD_LEN;
    uint16_t fpad_value = fpad[0] << 8 | fpad[1];

    // Adapt AU data to PADDecoder
    uint8_t xpad_data[256];
    uint8_t *xpad = theAU + pad_start;

    // Undo reversed byte order
    size_t used_xpad_len = std::min(xpad_len, sizeof(xpad_data));
    for(size_t i = 0; i < used_xpad_len; i++)
        xpad_data[i] = xpad[xpad_len - 1 - i];

    // Run PADDecoder
    padDecoder->Process(xpad_data, xpad_len, true, fpad_value);
}
