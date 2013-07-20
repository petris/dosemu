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

#ifndef _LINUX_VM86PLUS_H
#define _LINUX_VM86PLUS_H

#ifdef __linux__
#include "emu.h"
#include <Asm/vm86.h>
#include <unistd.h>
#include <sys/syscall.h>
#endif /* __linux__ */

#define CBACK_SEG BIOS_HLT_BLK_SEG
EXTERN Bit16u CBACK_OFF;

int vm86_init(void);

#ifdef X86_EMULATOR
int e_vm86(void);

static inline int emu_vm86(struct vm86plus_struct *x)
{
    x->vm86plus.force_return_for_pic = 0;
    return e_vm86();
}
#endif

#ifdef __i386__
#define vm86_plus(function,param) syscall(SYS_vm86, function, param)

static inline int true_vm86(struct vm86plus_struct *x)
{
    int ret;
    unsigned short fs = getsegment(fs), gs = getsegment(gs);

    x->vm86plus.force_return_for_pic = 0;
    ret = vm86_plus(VM86_ENTER, x);
    /* kernel 2.4 doesn't preserve GS -- and it doesn't hurt to restore here */
    loadregister(fs, fs);
    loadregister(gs, gs);
    return ret;
}
#endif

static inline int do_vm86(struct vm86plus_struct *x)
{
#ifdef __i386__
#ifdef X86_EMULATOR
    if (config.cpuemu)
	return emu_vm86(x);
#endif
    return true_vm86(x);
#else
    return emu_vm86(x);
#endif
}

#if defined(USE_MHPDBG) && 0
/* ...hmm, this one seems not to work properly (Hans) */
  #define DO_VM86(x) (WRITE_FLAGS((READ_FLAGS() & ~TF) | mhpdbg.flags), do_vm86(x))
#else
  #define DO_VM86(x) do_vm86(x)
#endif

#endif
