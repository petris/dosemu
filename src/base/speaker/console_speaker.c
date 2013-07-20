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

#include "config.h"
#include "speaker.h"
/*
 * Console Speaker Emulation
 * =============================================================================
 */

#include <sys/ioctl.h>
#ifdef __linux__
#include <sys/kd.h>
#endif


void console_speaker_on(void *gp, unsigned ms, unsigned short period)
{
	ioctl((int)(uintptr_t)gp, KDMKTONE,
		(unsigned) ((ms & 0xffff) << 16) | (period & 0xffff));
}

void console_speaker_off(void *gp)
{
	ioctl((int)(uintptr_t)gp, KDMKTONE, 0);
}
