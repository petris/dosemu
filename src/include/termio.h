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

/* dos emulator, Matthias Lautner */
#ifndef TERMIO_H
#define TERMIO_H
/* Extensions by Robert Sanders, 1992-93
 *
 *
 */

/*
 * These are the defines for the int that is dosemu's kbd flagger (kbd_flags)
 * These just happen to coincide with the format of the GET EXTENDED SHIFT
 * STATE of int16h/ah=12h, and the low byte to GET SHIFT STATE int16h,ah=02h
 *
 * A lot of the RAW VC dosemu stuff was copied from the Linux kernel (0.98pl6)
 * I hope Linux dosen't mind
 *
 * Robert Sanders 12/12/92
 */

#include "extern.h"


struct screen_stat {
  int console_no,		/* our console number */
   vt_allow;			/* whether to allow VC switches */

  int current;			/* boolean: is our VC current? */

  int curadd;			/* row*80 + col */
  int dcurgeom;			/* msb: start, lsb: end */
  int lcurgeom;			/* msb: start, lsb: end */

  int mapped,			/* whether currently mapped */
   pageno;			/* current mapped text page # */

  int dorigin;			/* origin in DOS */
  int lorigin;

  unsigned int virt_address;	/* current map address in DOS memory */
  off_t phys_address;		/* current map address in /dev/mem memory */

  int old_modecr, new_modecr;
};

#endif /* TERMIO_H */
