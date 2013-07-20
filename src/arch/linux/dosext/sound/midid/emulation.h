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

/* Set emulation type */

#ifndef _EMULATION_H
#define _EMULATION_H

#include "device.h"

/* Current instrument mapping needed for emulation */
extern int *imap;

extern int imap_default[128], imap_mt2gm[128];

/* Sets emulation mode to new_mode */
void emulation_set(Device *dev, Emumode new_mode);

#endif
