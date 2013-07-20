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

#ifndef __MACROS86_H_
#define __MACROS86_H_

#ifndef __ASM__
#  define __ASM__
#endif
#include "config.h"

.macro SIM_INT n,rtnlabel
        pushw %ds
        pushw $0
        popw %ds
        pushf
        pushw %cs
        pushw $\rtnlabel-bios_f000
        pushw (4*\n)+2
        pushw (4*\n)
        lret
        \rtnlabel: popw %ds
.endm

.macro FILL_OPCODE x,v
.rept \x
\v
.endr
.endm

#define FILL_BYTE(x,v) FILL_OPCODE x, .byte v
#define FILL_DWORD(x,v) FILL_OPCODE x, .int v
#define FILL_WORD(x,v) FILL_OPCODE x, .word v
#define FILL_SHORT(x,v) FILL_OPCODE x, .short v
#define FILL_LONG(x,v) FILL_OPCODE x, .long v

/* NOTE: we need the following only for prefixing string instructions
 *       (such as lodsb, lodsw, lodsl, etc.)
 *       In all other cases prefixing can be imbedded in the operant itself
 */
#ifndef SEGES
#define SEGES .byte	0x26;
#endif
#define SEGCS .byte	0x2e;
#define SEGSS .byte	0x36;
#define SEGDS .byte	0x3e;
#define SEGFS .byte	0x64;
#define SEGGS .byte	0x65;


#endif
