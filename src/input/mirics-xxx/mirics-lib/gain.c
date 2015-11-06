/*
 * Copyright (C) 2013 by Miroslav Slugen <thunder.m@email.cz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gain.h"

int mirisdr_set_gain (mirisdr_dev_t *p) {
    uint32_t reg1 = 1, reg6 = 6;

//    fprintf(stderr, "gain baseband: %d, lna: %d, mixer: %d\n", p->gain_baseband, p->gain_lna, p->gain_mixer);

    /* Receiver Gain Control */
    /* 0-3 => registr */
    /* 4-9 => baseband, 0 - 59, 60-63 je stejné jako 59 */
    /* 10-11 => mixer gain reduction pouze pro AM režim */
    /* 12 => mixer gain reduction -19dB */
    /* 13 => lna gain reduction -24dB */
    /* 14-16 => DC kalibrace */
    /* 17 => zrychlená DC kalibrace */
    reg1|= p->gain_baseband << 4;
    reg1|= 0x0 << 10;
    reg1|= p->gain_mixer << 12;
    reg1|= p->gain_lna << 13;
    reg1|= MIRISDR_DC_OFFSET_CALIBRATION_ONE_SHOT << 14;
    reg1|= MIRISDR_DC_OFFSET_CALIBRATION_SPEEDUP_OFF << 17;
    mirisdr_write_reg(p, 0x09, reg1);

    /* DC Offset Calibration setup */
    reg6|= 0x3F << 4;
    reg6|= 0xFFF << 10;
    mirisdr_write_reg(p, 0x09, reg6);

    return 0;
}

int mirisdr_get_tuner_gains (mirisdr_dev_t *dev, int *gains) {
    int i;

    if (gains) {
        for (i = 0; i <= 102; i++) {
            gains[i] = i;
        }
    }

    return i;
}

int mirisdr_set_tuner_gain (mirisdr_dev_t *p, int gain) {
    p->gain = gain;

    /*
     * Pro VHF režim je lna zapnutý +24dB, mixer +19dB a baseband
     * je možné nastavovat plynule od 0 - 59 dB, z toho je maximální
     * zesílení 102 dB
     */
    if (p->gain > 102) {
        p->gain = 102;
    } else if (p->gain < 0) {
        goto gain_auto;
    }

    /* Nejvyšší citlivost vždy bez redukce mixeru a lna */
    if (p->gain >= 43) {
        p->gain_lna = 0;
        p->gain_mixer = 0;
        p->gain_baseband = 59 - (p->gain - 43);
    } else if (p->gain >= 19) {
        p->gain_lna = 1;
        p->gain_mixer = 0;
        p->gain_baseband = 59 - (p->gain - 19);
    } else {
        p->gain_lna = 1;
        p->gain_mixer = 1;
        p->gain_baseband = 59 - p->gain;
    }

    return mirisdr_set_gain(p);

gain_auto:
    return mirisdr_set_tuner_gain_mode(p, 0);
}

int mirisdr_get_tuner_gain (mirisdr_dev_t *p) {
    int gain = 0;

    if (p->gain < 0) goto gain_auto;

    gain+= 59 - p->gain_baseband;

    if (!p->gain_lna) gain+= 24;
    if (!p->gain_mixer) gain+= 19;

    return gain;

gain_auto:
    return -1;
}

int mirisdr_set_tuner_gain_mode (mirisdr_dev_t *p, int mode) {
    if (!mode) {
        p->gain = -1;
        fprintf(stderr, "gain mode: auto\n");
        mirisdr_write_reg(p, 0x09, 0x014281);
        mirisdr_write_reg(p, 0x09, 0x3FFFF6);
    } else if (p->gain < 0) {
        fprintf(stderr, "gain mode: manual\n");
        p->gain = DEFAULT_GAIN;
    }

    return 0;
}

int mirisdr_get_tuner_gain_mode (mirisdr_dev_t *p) {
    return (p->gain < 0) ? 0 : 1;
}

int mirisdr_set_mixer_gain (mirisdr_dev_t *p, int gain) {
    p->gain_mixer = gain;

    return mirisdr_set_gain(p);
}

int mirisdr_set_lna_gain (mirisdr_dev_t *p, int gain) {
    p->gain_lna = gain;

    return mirisdr_set_gain(p);
}

int mirisdr_set_baseband_gain (mirisdr_dev_t *p, int gain) {
    p->gain_baseband = gain;

    return mirisdr_set_gain(p);
}

int mirisdr_get_mixer_gain (mirisdr_dev_t *p) {
    return p->gain_mixer;
}

int mirisdr_get_lna_gain (mirisdr_dev_t *p) {
    return p->gain_lna;
}

int mirisdr_get_baseband_gain (mirisdr_dev_t *p) {
    return p->gain_baseband;
}


