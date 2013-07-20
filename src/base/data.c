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

#define EXTERN
#define INIT(x...)		= x

#include <termios.h>
#include <sys/types.h>
#include "config.h"
#include "emu.h"
#include "mapping.h"
#include "xms.h"
#include "disks.h"
#include "timers.h"
#include "int.h"
#include "hma.h"
#include "termio.h"
#include "machcompat.h"
#include "vc.h"
#include "../env/video/vga.h" /* NOTE: there exists also /usr/include/vga.h !! */
#include "video.h"
#include "mouse.h"
#include "bios.h"
#include "dpmi.h"
#include "pic.h"
#include "disks.h"
#include "mhpdbg.h"
#include "cmos.h"
#include "priv.h"
#include "dma.h"
#include "sound.h"
#include "serial.h"
#include "dosemu_debug.h"
#include "keyb_server.h"
#include "keyb_clients.h"
