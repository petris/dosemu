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

#ifndef LPT_H
#define LPT_H 1

struct p_fops {
  int (*open) (int prtnum);
  int (*write) (int prtnum, int data);
  int (*close) (int prtnum);
  int (*realwrite) (int prnum, int data);
};

struct printer {
  char *dev;
  char *prtcmd;
  int delay;
  ioport_t base_port;		/* Base port address handled by device */

  /* end of user-set options */
  FILE *file;
  int remaining;

  struct p_fops fops;

  Bit8u data, status, control;
};

/* public functions */
int printer_open(int prnum);
int printer_close(int prnum);
int printer_flush(int prnum);
int printer_write(int prnum, int outchar);
void printer_config(int prnum, struct printer *pptr);
void printer_print_config(int prnum, void (*print)(const char *, ...));

/* status bits */
#define LPT_STAT_TIMEOUT	0x1
#define LPT_STAT_NOT_IRQ	0x4
#define LPT_STAT_NOIOERR	0x8
#define LPT_STAT_ONLINE		0x10
#define LPT_STAT_NOPAPER	0x20
#define LPT_STAT_NOT_ACK	0x40
#define LPT_STAT_NOT_BUSY	0x80

/* control bits */
#define LPT_CTRL_INPUT		0x20
#define LPT_CTRL_IRQ_EN		0x10
#define LPT_CTRL_SELECT		0x8
#define LPT_CTRL_NOT_INIT	0x4
#define LPT_CTRL_AUTOLF		0x2
#define LPT_CTRL_STROBE		0x1

#define NUM_PRINTERS 3
extern struct printer lpt[NUM_PRINTERS];

#endif /* LPT_H */
