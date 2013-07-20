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

#ifndef __LOWMEM_H
#define __LOWMEM_H

int lowmem_heap_init(void);
void *lowmem_heap_alloc(int size);
void lowmem_heap_free(void *p);
extern unsigned char *dosemu_lmheap_base;
#define DOSEMU_LMHEAP_OFFS_OF(s) \
  (((unsigned char *)(s) - dosemu_lmheap_base) + DOSEMU_LMHEAP_OFF)

int get_rm_stack(Bit16u *ss_p, Bit16u *sp_p);
void put_rm_stack(void);
void get_rm_stack_regs(struct vm86_regs *regs, struct vm86_regs *saved_regs);
void rm_stack_enter(void);
void rm_stack_leave(void);

#endif
