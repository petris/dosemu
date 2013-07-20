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

/* for the Linux dos emulator versions 0.49 and newer
 *
 */
#define LPT_C 1

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "emu.h"
#include "bios.h"
#include "port.h"
#include "timers.h"
#include "lpt.h"
#include "utilities.h"
#include "dos2linux.h"

static int stub_printer_write(int, int);

/* status bits, Centronics */
#define CTS_STAT_NOIOERR	LPT_STAT_NOIOERR
#define CTS_STAT_ONLINE		LPT_STAT_ONLINE
#define CTS_STAT_NOPAPER	LPT_STAT_NOPAPER
#define CTS_STAT_NOT_ACKing	LPT_STAT_NOT_ACK
#define CTS_STAT_BUSY		LPT_STAT_NOT_BUSY

/* control bits, Centronics */
#define CTS_CTRL_NOT_SELECT	LPT_CTRL_SELECT
#define CTS_CTRL_NOT_INIT	LPT_CTRL_NOT_INIT
#define CTS_CTRL_NOT_AUTOLF	LPT_CTRL_AUTOLF
#define CTS_CTRL_NOT_STROBE	LPT_CTRL_STROBE

/* inversion masks to convert LPT<-->Centronics */
#define LPT_STAT_INV_MASK (CTS_STAT_BUSY)
#define LPT_CTRL_INV_MASK (CTS_CTRL_NOT_STROBE | CTS_CTRL_NOT_AUTOLF | \
	CTS_CTRL_NOT_SELECT)

#define DEFAULT_STAT (CTS_STAT_ONLINE | CTS_STAT_NOIOERR | \
    CTS_STAT_NOT_ACKing | LPT_STAT_NOT_IRQ)
#define DEFAULT_CTRL (CTS_CTRL_NOT_INIT | CTS_CTRL_NOT_AUTOLF | \
    CTS_CTRL_NOT_STROBE)

struct printer lpt[NUM_PRINTERS] =
{
  {NULL, NULL, 5, 0x378, .control = DEFAULT_CTRL, .status = DEFAULT_STAT},
  {NULL, NULL, 5, 0x278, .control = DEFAULT_CTRL, .status = DEFAULT_STAT},
  {NULL, NULL, 10, 0x3bc, .control = DEFAULT_CTRL, .status = DEFAULT_STAT}
};

static int get_printer(ioport_t port)
{
  int i;
  for (i = 0; i < 3; i++)
    if (lpt[i].base_port <= port && port <= lpt[i].base_port + 2)
      return i;
  return -1;
}

static Bit8u printer_io_read(ioport_t port)
{
  int i = get_printer(port);
  Bit8u val;

  if (i == -1)
    return 0xff;

  switch (port - lpt[i].base_port) {
  case 0:
    val = lpt[i].data; /* simple unidirectional port */
    if (debug_level('p') >= 5)
      p_printf("LPT%d: Reading data byte %#x\n", i, val);
    break;
  case 1: /* status port, r/o */
    val = lpt[i].status ^ LPT_STAT_INV_MASK;
    /* we should really set ACK after 5 us but here we just
       use the fact that the BIOS only checks this once */
    lpt[i].status |= CTS_STAT_NOT_ACKing | LPT_STAT_NOT_IRQ;
    lpt[i].status &= ~CTS_STAT_BUSY;
    if (debug_level('p') >= 5)
      p_printf("LPT%d: Reading status byte %#x\n", i, val);
    break;
  case 2:
    val = lpt[i].control ^ LPT_CTRL_INV_MASK;
    if (debug_level('p') >= 5)
      p_printf("LPT%d: Reading control byte %#x\n", i, val);
    break;
  default:
    val = 0xff;
    break;
  }
  return val;
}

static void printer_io_write(ioport_t port, Bit8u value)
{
  int i = get_printer(port);
  if (i == -1)
    return;
  switch (port - lpt[i].base_port) {
  case 0:
    if (debug_level('p') >= 5)
      p_printf("LPT%d: Writing data byte %#x\n", i, value);
    lpt[i].data = value;
    break;
  case 1: /* status port, r/o */
    break;
  case 2:
    if (debug_level('p') >= 5)
      p_printf("LPT%d: Writing control byte %#x\n", i, value);
    value ^= LPT_CTRL_INV_MASK;		// convert to Centronics
    if (((lpt[i].control & (CTS_CTRL_NOT_STROBE | CTS_CTRL_NOT_SELECT)) == 0)
        && (value & CTS_CTRL_NOT_STROBE)) {
      /* STROBE rising */
      printer_write(i, lpt[i].data);
      lpt[i].status &= ~CTS_STAT_NOT_ACKing;
      lpt[i].status |= CTS_STAT_BUSY;
    }
    lpt[i].control = value;
    break;
  }
}

static int dev_printer_open(int prnum)
{
  int um = umask(026);
  lpt[prnum].file = fopen(lpt[prnum].dev, "a");
  umask(um);
  return 0;
}

static int pipe_printer_open(int prnum)
{
  p_printf("LPT: doing printer command ..%s..\n", lpt[prnum].prtcmd);

  lpt[prnum].file = popen(lpt[prnum].prtcmd, "w");
  if (lpt[prnum].file == NULL)
    error("system(\"%s\") in lpt.c failed, cannot print!\
  Command returned error %s\n", lpt[prnum].prtcmd, strerror(errno));
  return 0;
}

int printer_open(int prnum)
{
  int rc;

  if (lpt[prnum].file != NULL)
    return 0;

  if (lpt[prnum].fops.open == NULL)
    return -1;

  rc = lpt[prnum].fops.open(prnum);
  /* use line buffering so we don't need to have a long wait for output */
  setvbuf(lpt[prnum].file, NULL, _IOLBF, 0);
  p_printf("LPT: opened printer %d to %s, file %p\n", prnum,
	   lpt[prnum].dev ? lpt[prnum].dev : "<<NODEV>>",
           (void *) lpt[prnum].file);
  return rc;
}

static int dev_printer_close(int prnum)
{
  if (lpt[prnum].file != NULL)
    fclose(lpt[prnum].file);
  return 0;
}

static int pipe_printer_close(int prnum)
{
  if (lpt[prnum].file != NULL)
    pclose(lpt[prnum].file);
  lpt[prnum].file = NULL;
  return 0;
}

int printer_close(int prnum)
{
  if (lpt[prnum].fops.close) {
    p_printf("LPT: closing printer %d, %s\n", prnum,
	     lpt[prnum].dev ? lpt[prnum].dev : "<<NODEV>>");

    lpt[prnum].fops.close(prnum);
    lpt[prnum].file = NULL;
    lpt[prnum].remaining = -1;
  }
  lpt[prnum].fops.write = stub_printer_write;
  return 0;
}

static int stub_printer_write(int prnum, int outchar)
{
  printer_open(prnum);

  /* from now on, use real write */
  lpt[prnum].fops.write = lpt[prnum].fops.realwrite;

  return printer_write(prnum, outchar);
}

static int file_printer_write(int prnum, int outchar)
{
  lpt[prnum].remaining = lpt[prnum].delay;

  fputc(outchar, lpt[prnum].file);
  return 0;
}

int printer_write(int prnum, int outchar)
{
  return lpt[prnum].fops.write(prnum, outchar);
}

/* DANG_BEGIN_FUNCTION printer_init
 *
 * description:
 *  Initialize printer control structures
 *
 * DANG_END_FUNCTIONS
 */
static struct p_fops dev_pfops =
{
  dev_printer_open,
  stub_printer_write,
  dev_printer_close,
  file_printer_write
};
static struct p_fops pipe_pfops =
{
  pipe_printer_open,
  stub_printer_write,
  pipe_printer_close,
  file_printer_write
};

void
printer_init(void)
{
  int i;
  emu_iodev_t io_device;

  io_device.read_portb   = printer_io_read;
  io_device.write_portb  = printer_io_write;
  io_device.read_portw   = NULL;
  io_device.write_portw  = NULL;
  io_device.read_portd   = NULL;
  io_device.write_portd  = NULL;
  io_device.handler_name = "Parallel printer";
  io_device.irq          = EMU_NO_IRQ;
  io_device.fd           = -1;

  for (i = 0; i < 3; i++) {
    p_printf("LPT: initializing printer %s\n", lpt[i].dev ? lpt[i].dev : "<<NODEV>>");
    lpt[i].file = NULL;
    lpt[i].remaining = -1;	/* mark not accessed yet */
    if (lpt[i].dev)
      lpt[i].fops = dev_pfops;
    else if (lpt[i].prtcmd)
      lpt[i].fops = pipe_pfops;
    if (i >= config.num_lpt) lpt[i].base_port = 0;

    if (lpt[i].base_port != 0 && lpt[i].fops.open) {
      io_device.start_addr = lpt[i].base_port;
      io_device.end_addr   = lpt[i].base_port + 2;
      port_register_handler(io_device, 0);
    }
  }
}

void
close_all_printers(void)
{
  int loop;

  for (loop = 0; loop < NUM_PRINTERS; loop++) {
    p_printf("LPT: closing printer %d (%s)\n", loop,
	     lpt[loop].dev ? lpt[loop].dev : "<<NODEV>>");
    printer_close(loop);
  }
}

int
printer_tick(u_long secno)
{
  int i;

  for (i = 0; i < NUM_PRINTERS; i++) {
    if (lpt[i].remaining >= 0) {
      p_printf("LPT: doing real tick for %d\n", i);
      if (lpt[i].remaining) {
        reset_idle(2);
	lpt[i].remaining--;
	if (!lpt[i].remaining)
	  printer_close(i);
	else if (lpt[i].file != NULL)
	  fflush(lpt[i].file);
      }
    }
  }
  return 0;
}

void printer_config(int prnum, struct printer *pptr)
{
  struct printer *destptr;

  if (prnum < NUM_PRINTERS) {
    destptr = &lpt[prnum];
    destptr->prtcmd = pptr->prtcmd;
    destptr->dev = pptr->dev;
    destptr->file = pptr->file;
    destptr->remaining = pptr->remaining;
    destptr->delay = pptr->delay;
  }
}

void printer_print_config(int prnum, void (*print)(const char *, ...))
{
  struct printer *pptr = &lpt[prnum];
  (*print)("LPT%d command \"%s\"  timeout %d  device \"%s\"  baseport 0x%03x\n",
	  prnum+1, (pptr->prtcmd ? pptr->prtcmd : ""), pptr->delay,
	   (pptr->dev ? pptr->dev : ""), pptr->base_port);
}

#undef LPT_C
