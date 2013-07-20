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

#ifndef _IO_H
#define _IO_H

/* Output to screen and input from stream routines */

#include "device.h"

/* Special return values; must be smaller then 0 */
#define MAGIC_EOF (-1)    /* End Of File */
#define MAGIC_INV (-2)    /* invalid byte */
#define MAGIC_TIMEOUT (-3)

void getbyte_next(void);
int getbyte(void);
int getbyte_status(void);
int getbyte_data(void);

extern Device *devices;

/* File descriptor of input file */
extern int fd;		/* file descriptor */

#endif
