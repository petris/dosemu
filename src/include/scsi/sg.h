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

/* this is a copied and cut down version of linux/include/scsi/sg.h
 * (contains only what we need, if you need more, add it here)
 */

/*
   History:
    Started: Aug 9 by Lawrence Foard (entropy@world.std.com), to allow user
     process control of SCSI devices.
    Development Sponsored by Killy Corp. NY NY
*/

#ifndef _SCSI_GENERIC_H
#define _SCSI_GENERIC_H

/*
 An SG device is accessed by writing "packets" to it, the replies
 are then read using the read call. The same header is used for
 reply, just ignore reply_len field.
*/

struct sg_header
 {
  int pack_len;    /* length of incoming packet <4096 (including header) */
  int reply_len;   /* maximum length <4096 of expected reply */
  int pack_id;     /* id number of packet */
  int result;      /* 0==ok, otherwise refer to errno codes */
  unsigned int twelve_byte:1; /* Force 12 byte command length for group 6 & 7 commands  */
  unsigned int other_flags:31;			/* for future use */
  unsigned char sense_buffer[16]; /* used only by reads */
  /* command follows then data for command */
 };

/* ioctl's */
#define SG_SET_TIMEOUT 0x2201  /* set timeout *(int *)arg==timeout */

#endif
