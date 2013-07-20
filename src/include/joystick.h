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

/*
 * Joystick support for DOSEMU
 * Copyright (c) 2002 Clarence Dang <dang@kde.org>
 */

#ifndef JOYSTICK_H
#define JOYSTICK_H

/* init functions */
void joy_init (void);
void joy_uninit (void);
void joy_term (void);		/* just calls joy_uninit() ! */
void joy_reset (void);

/* access functions */
int joy_exist (void);		/* used by src/base/init/init.c */
int joy_bios_read (void);	/* used by src/base/async/int.c */

#endif	/* JOYSTICK_H */
