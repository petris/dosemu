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
 * BIOS video modes
 * This file provides the basis to implement a VGA/EGA/CGA/MDA BIOS emulator.
 * Contains currently supported video modes.
 */

/*
 * This file is currently used by the mouse driver to obtain
 * certain video mode parameters.
 */

#include "emu.h"
#include "video.h"
#include "bios.h"

/* video memory organization types */
enum {
	ORG_TEXT,
	ORG_CGA2,
	ORG_CGA4,
	ORG_EGA16,
	ORG_VGA
};

struct mousevideoinfo {
	int mode;		/* mode number (currently redundant) */
	char textgraph;		/* 'G' for graphics mode, 'T' for text mode */
	int width, height;	/* extents  */
	int bytesperline;	/* bytes per line */
	int organization;	/* ram organization method, see above */
	int offset;		/* offset from 0xA0000 of vram for this mode */
};

extern struct mousevideoinfo videomodes[], mouse_current_video;

int get_current_video_mode(int mode);
