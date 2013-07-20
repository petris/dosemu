/*
 *  (C) Copyright 1992, ..., 2007 the "DOSEMU-Development-Team".
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License, version 2, as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

void LibpacketInit(void);
int OpenNetworkLink(char *);
void CloseNetworkLink(void);
int GetDeviceHardwareAddress(char *, unsigned char *);
int GetDeviceMTU(char *);

void pkt_io_select(void(*)(void *), void *);
ssize_t pkt_read(void *buf, size_t count);
ssize_t pkt_write(const void *buf, size_t count);
