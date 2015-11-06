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

#include "soft.h"

int mirisdr_set_soft (mirisdr_dev_t *p) {
    uint32_t reg0 = 0, reg2 = 2, reg5 = 5;
    uint64_t n, thresh, frac, lo_div = 0, fvco = 0, offset = 0, a, b, c;

    /*** registr0 - parametry pásma ***/

    /* pásmo */
    if (p->freq < 50000000) {
        /* AM režim2, antena 2, AM režim1 - 0x61 */
        reg0|= MIRISDR_MODE_AM << 4;
        reg0|= MIRISDR_UPCONVERT_MIXER_ON << 9;
        reg0|= MIRISDR_AM_PORT2 << 11;
        /* AM režim je posunutý o 5 * referenční frekvence, tj. o 120 MHz */
        lo_div = 16;
        offset+= 120000000UL;
    } else if (p->freq < 108000000) {
        /* VHF */
        reg0|= MIRISDR_MODE_VHF << 4;
        lo_div = 32;
    } else if (p->freq < 330000000) {
        /* B3 */
        reg0|= MIRISDR_MODE_B3 << 4;
        lo_div = 16;
    } else if (p->freq < 960000000) {
        /* B45 */
        reg0|= MIRISDR_MODE_B45 << 4;
        lo_div = 4;
    } else {
        /* BL */
        reg0|= MIRISDR_MODE_BL << 4;
        lo_div = 2;
    }

    /* RF syntetizer je vždy aktivní */
    reg0|= MIRISDR_RF_SYNTHESIZER_ON << 10;

    /* režim IF filtru - zatím nefunguje? */
    switch (p->if_freq) {
    case MIRISDR_IF_ZERO:
        reg0|= MIRISDR_IF_MODE_ZERO << 12;
        break;
    case MIRISDR_IF_450KHZ:
        reg0|= MIRISDR_IF_MODE_450KHZ << 12;
        break;
    case MIRISDR_IF_1620KHZ:
        reg0|= MIRISDR_IF_MODE_1620KHZ << 12;
        break;
    case MIRISDR_IF_2048KHZ:
        reg0|= MIRISDR_IF_MODE_2048KHZ << 12;
        break;
    }

    /* šířka pásma - 8 MHz, nejvyšší možná */
    switch (p->bandwidth) {
    case MIRISDR_BW_200KHZ:
        reg0|= 0x00 << 14;
        break;
    case MIRISDR_BW_300KHZ:
        reg0|= 0x01 << 14;
        break;
    case MIRISDR_BW_600KHZ:
        reg0|= 0x02 << 14;
        break;
    case MIRISDR_BW_1536KHZ:
        reg0|= 0x03 << 14;
        break;
    case MIRISDR_BW_5MHZ:
        reg0|= 0x04 << 14;
        break;
    case MIRISDR_BW_6MHZ:
        reg0|= 0x05 << 14;
        break;
    case MIRISDR_BW_7MHZ:
        reg0|= 0x06 << 14;
        break;
    case MIRISDR_BW_8MHZ:
        reg0|= 0x07 << 14;
        break;
    }

    /* xtal frekvence - nepodporujeme změnu */
    switch (p->xtal) {
    case MIRISDR_XTAL_19_2M:
        reg0|= 0x00 << 17;
        break;
    case MIRISDR_XTAL_22M:
        reg0|= 0x01 << 17;
        break;
    case MIRISDR_XTAL_24M:
    case MIRISDR_XTAL_24_576M:
        reg0|= 0x02 << 17;
        break;
    case MIRISDR_XTAL_26M:
        reg0|= 0x03 << 17;
        break;
    case MIRISDR_XTAL_38_4M:
        reg0|= 0x04 << 17;
        break;
    }

    /* 4 bity pro režimy snížené spotřeby */
    reg0|= MIRISDR_IF_LPMODE_NORMAL << 20;
    reg0|= MIRISDR_VCO_LPMODE_NORMAL << 23;

    /* vco frekvence, je lepší použít 64bitový rozsah */
    fvco = (p->freq + offset) * lo_div;

    /* posun po hlavní frekvenci */
    n = fvco / 96000000UL;

    /* hlavní registr, hrubé ladění */
    thresh = 96000000UL / lo_div;

    /* vedlejší registr, jemné ladění */
    frac = (fvco % 96000000UL) / lo_div;

    /* najdeme největší společný dělitel pro thresh a frac */
    for (a = thresh, b = frac; a != 0;) {
        c = a;
        a = b % a;
        b = c;
    }

    /* dělíme */
    thresh/= b;
    frac/= b;

    /* v této části musíme rozlišení snížit na maximální rozsah registru */
    a = (thresh + 4094) / 4095;
    thresh = (thresh + (a / 2)) / a;
    frac = (frac + (a / 2)) / a;

    reg5|= (0xFFF & thresh) << 4;
    /* rezervováno, musí být 0x28 */
    reg5|= MIRISDR_RF_SYNTHESIZER_RESERVED_PROGRAMMING << 16;

    reg2|= (0xFFF & frac) << 4;
    reg2|= (0x3F & n) << 16;
    reg2|= MIRISDR_LBAND_LNA_CALIBRATION_OFF << 22;

    /* kernel driver nastavuje až při změně frekvence */
    mirisdr_write_reg(p, 0x09, 0x0e);
    mirisdr_write_reg(p, 0x09, 0x03);

    mirisdr_write_reg(p, 0x09, reg0);
    mirisdr_write_reg(p, 0x09, reg5);
    mirisdr_write_reg(p, 0x09, reg2);

//    fprintf(stderr, "freq: %.2f MHz (offset: %.2f MHz), n: %lu, fraction: %lu/%lu\n", ((double) n + (double) frac / (double) thresh) * 96.0 / (double) lo_div, (double) offset / 1.0e6, n, frac, thresh);

    return 0;
}

int mirisdr_set_center_freq (mirisdr_dev_t *p, uint32_t freq) {
    p->freq = freq;

    return mirisdr_set_soft(p);
}

uint32_t mirisdr_get_center_freq (mirisdr_dev_t *p) {
    return p->freq;
}

int mirisdr_set_if_freq (mirisdr_dev_t *p, uint32_t freq) {
    if (!p) goto failed;

    switch (freq) {
    case 0:
        p->if_freq = MIRISDR_IF_ZERO;
        break;
    case 450000:
        p->if_freq = MIRISDR_IF_450KHZ;
        break;
    case 1620000:
        p->if_freq = MIRISDR_IF_1620KHZ;
        break;
    case 2048000:
        p->if_freq = MIRISDR_IF_2048KHZ;
        break;
    default:
        fprintf(stderr, "unsupported if frequency: %u Hz\n", freq);
        goto failed;
    }

    return mirisdr_set_soft(p);

failed:
    return -1;
}

uint32_t mirisdr_get_if_freq (mirisdr_dev_t *p) {
    if (!p) goto failed;

    switch (p->if_freq) {
    case MIRISDR_IF_ZERO:
        return 0;
    case MIRISDR_IF_450KHZ:
        return 450000;
    case MIRISDR_IF_1620KHZ:
        return 1620000;
    case MIRISDR_IF_2048KHZ:
        return 2048000;
    }

failed:
    return -1;
}

/* not supported yet */
int mirisdr_set_xtal_freq (mirisdr_dev_t *p, uint32_t freq) {
    return -1;
}

uint32_t mirisdr_get_xtal_freq (mirisdr_dev_t *p) {
    if (!p) goto failed;

    switch (p->xtal) {
    case MIRISDR_XTAL_19_2M:
        return 19200000;
    case MIRISDR_XTAL_22M:
        return 22000000;
    case MIRISDR_XTAL_24M:
    case MIRISDR_XTAL_24_576M:
        /* realně 24 MHz ??? */
        return 24000000;
    case MIRISDR_XTAL_26M:
        return 26000000;
    case MIRISDR_XTAL_38_4M:
        return 38400000;
    }

failed:
    return -1;
}

int mirisdr_set_bandwidth (mirisdr_dev_t *p, uint32_t bw) {
    if (!p) goto failed;

    switch (bw) {
    case 200000:
        p->bandwidth = MIRISDR_BW_200KHZ;
        break;
    case 300000:
        p->bandwidth = MIRISDR_BW_300KHZ;
        break;
    case 600000:
        p->bandwidth = MIRISDR_BW_600KHZ;
        break;
    case 1536000:
        p->bandwidth = MIRISDR_BW_1536KHZ;
        break;
    case 5000000:
        p->bandwidth = MIRISDR_BW_5MHZ;
        break;
    case 6000000:
        p->bandwidth = MIRISDR_BW_6MHZ;
        break;
    case 7000000:
        p->bandwidth = MIRISDR_BW_7MHZ;
        break;
    case 8000000:
        p->bandwidth = MIRISDR_BW_8MHZ;
        break;
    default:
        fprintf(stderr, "unsupported bandwidth: %u Hz\n", bw);
        goto failed;
    }

    return mirisdr_set_soft(p);

failed:
    return -1;
}

uint32_t mirisdr_get_bandwidth (mirisdr_dev_t *p) {
    if (!p) goto failed;

    switch (p->bandwidth) {
    case MIRISDR_BW_200KHZ:
        return 200000;
    case MIRISDR_BW_300KHZ:
        return 300000;
    case MIRISDR_BW_600KHZ:
        return 600000;
    case MIRISDR_BW_1536KHZ:
        return 1536000;
    case MIRISDR_BW_5MHZ:
        return 5000000;
    case MIRISDR_BW_6MHZ:
        return 6000000;
    case MIRISDR_BW_7MHZ:
        return 7000000;
    case MIRISDR_BW_8MHZ:
        return 8000000;
    }

failed:
    return -1;
}

int mirisdr_set_freq_correction (mirisdr_dev_t *p, int ppm) {
    fprintf(stderr, "frequency correction not implemented yet\n");
    return -1;
}

int mirisdr_set_direct_sampling (mirisdr_dev_t *p, int on) {
    fprintf(stderr, "direct sampling not implemented yet\n");
    return -1;
}

int mirisdr_set_offset_tuning (mirisdr_dev_t *p, int on) {
    if (!p) goto failed;

    if (on) {
        p->if_freq = MIRISDR_IF_450KHZ;
    } else {
        p->if_freq = MIRISDR_IF_ZERO;
    }

    return mirisdr_set_soft(p);

failed:
    return -1;
}

int mirisdr_set_transfer (mirisdr_dev_t *p, char *v) {
    if (!p) goto failed;

    if (!strcmp(v, "BULK")) {
        p->transfer = MIRISDR_TRANSFER_BULK;
    } else if (!strcmp(v, "ISOC")) {
        p->transfer = MIRISDR_TRANSFER_ISOC;
    } else {
        fprintf(stderr, "unsupported transfer type: %s\n", v);
        goto failed;
    }

    return 0;

failed:
    return -1;
}

const char *mirisdr_get_transfer (mirisdr_dev_t *p) {
    switch (p->transfer) {
    case MIRISDR_TRANSFER_BULK:
        return "BULK";
    case MIRISDR_TRANSFER_ISOC:
        return "ISOC";
    }

    return "";
}

