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

/* This is in implementation of int 17, the printer service interrupt
   It is based on Bochs' implementation, and purely communicates
   with the BIOS data area and the printer ports */

#include "config.h"
#include "emu.h"
#include "bios.h"
#include "port.h"
#include "lpt.h"
#include "timers.h"
#ifdef X86_EMULATOR
#include "cpu-emu.h"
#endif

int int17(void)
{
  int timeout, val8;
  ioport_t addr;

  CARRY;
  if (_DX >= 3 || _AH >= 3)
    return 1;
  addr = READ_WORD(BIOS_ADDRESS_LPT1 + _DX * 2);
  if (addr == 0)
    return 1;
  timeout = READ_BYTE(BIOS_LPT1_TIMEOUT + _DX) << 8;

  reset_idle(2);

  switch(_AH)
  {
  case 0:
    port_outb(addr, _AL);
    val8 = port_inb(addr + 2);
    port_outb(addr + 2, val8 | 0x01); // send strobe
    port_outb(addr + 2, val8 & ~0x01);
    while (((val8 = port_inb(addr + 1)) & LPT_STAT_NOT_ACK) && timeout)
      timeout--;
    break;
  case 1:
    val8 = port_inb(addr + 2);
    port_outb(addr + 2, val8 & ~0x04); // send init
    port_outb(addr + 2, val8 | 0x04);
    val8 = port_inb(addr + 1);
    break;
  default: /* case 2: */
    val8 = port_inb(addr + 1);
    break;
  }
  _AH = val8 ^ (LPT_STAT_NOT_ACK | LPT_STAT_NOIOERR);
  if (!timeout) _AH |= LPT_STAT_TIMEOUT;
  NOCARRY;

  return 1;
}

void
printer_mem_setup(void)
{
  int i;
  for (i = 0; i < 3; i++) {
    /* set the port address for each printer in bios */
    WRITE_WORD(BIOS_ADDRESS_LPT1 + i * 2, lpt[i].base_port);
    WRITE_BYTE(BIOS_LPT1_TIMEOUT + i, 20);
  }
}
