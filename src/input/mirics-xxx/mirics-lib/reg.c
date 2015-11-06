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

int mirisdr_write_reg (mirisdr_dev_t *p, uint8_t reg, uint32_t val) {
    uint16_t value = (val & 0xff) << 8 | reg;
    uint16_t index = (val >> 8) & 0xffff;

    if (!p) goto failed;
    if (!p->dh) goto failed;

#ifdef MIRISDR_DEBUG
    fprintf(stderr, "write reg: 0x%02x, val 0x%08x\n", reg, val);
#endif

    return libusb_control_transfer(p->dh, 0x42, 0x41, value, index, NULL, 0, CTRL_TIMEOUT);

failed:
    return -1;
}
