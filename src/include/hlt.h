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

#ifndef _EMU_HLT_H
#define _EMU_HLT_H

#if 0
#include "emu_defs.h"
#endif
#include "bios.h"

typedef void (* emu_hlt_func)(Bit32u offs, void *arg);

typedef struct {
  emu_hlt_func  func;
  const char   *name;
  Bit32u        start_addr;
  int           len;
  void         *arg;
} emu_hlt_t;

extern void hlt_init(void);
extern Bit32u hlt_register_handler(emu_hlt_t handler);
extern void hlt_handle(void);

#endif /* _EMU_HLT_H */
