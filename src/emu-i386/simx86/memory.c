/***************************************************************************
 *
 * All modifications in this file to the original code are
 * (C) Copyright 1992, ..., 2007 the "DOSEMU-Development-Team".
 *
 * for details see file COPYING.DOSEMU in the DOSEMU distribution
 *
 *
 *  SIMX86 a Intel 80x86 cpu emulator
 *  Copyright (C) 1997,2001 Alberto Vignani, FIAT Research Center
 *				a.vignani@crf.it
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Additional copyright notes:
 *
 * 1. The kernel-level vm86 handling was taken out of the Linux kernel
 *  (linux/arch/i386/kernel/vm86.c). This code originaly was written by
 *  Linus Torvalds with later enhancements by Lutz Molgedey and Hans Lermen.
 *
 ***************************************************************************/

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "mapping.h"
#include "dosemu_debug.h"
#include "dlmalloc.h"
#include "emu86.h"
#include "codegen.h"
#include "dpmi.h"

#define CGRAN		4		/* 2^n */
#define CGRMASK		(0xfffff>>CGRAN)

typedef struct _mpmap {
	struct _mpmap *next;
	int mega;
	unsigned int subpage[0x100000>>(CGRAN+5)];	/* 16-byte granularity, 64k bits */
} tMpMap;

static tMpMap *MpH = NULL;
unsigned int mMaxMem = 0;

static tMpMap *LastMp = NULL;

/////////////////////////////////////////////////////////////////////////////

static inline tMpMap *FindM(unsigned int addr)
{
	register int a2l = addr >> (PAGE_SHIFT+8);
	register tMpMap *M = LastMp;

	if (M && (M->mega==a2l)) return M;
	M = MpH;
	while (M) {
		if (M->mega==a2l) {
		    LastMp = M; break;
		}
		M = M->next;
	}
	return M;
}


static void AddMpMap(unsigned int addr, unsigned int aend)
{
	register int page;
	tMpMap *M;

	do {
	    page = addr >> PAGE_SHIFT;
	    M = MpH;
	    while (M) {
		if (M->mega==(page>>8)) break;
		M = M->next;
	    }
	    if (M==NULL) {
		M = (tMpMap *)calloc(1,sizeof(tMpMap));
		M->next = MpH; MpH = M;
		M->mega = (page>>8);
	    }
	    if (debug_level('e')>1) {
		if (addr > mMaxMem) mMaxMem = addr;
	    }
	    addr += PAGE_SIZE;
	} while (addr < aend);
}


/////////////////////////////////////////////////////////////////////////////


int e_markpage(unsigned int addr, size_t len)
{
	unsigned int abeg, aend;
	tMpMap *M;

	if (M == NULL) {
		AddMpMap(addr, addr+len);
		M = FindM(addr);
		if (M == NULL) return 0;
	}

	abeg = addr >> CGRAN;
	aend = (addr+len-1) >> CGRAN;

	if (debug_level('e')>1)
		dbug_printf("MARK from %08x to %08x for %08x\n",
			    abeg<<CGRAN,((aend+1)<<CGRAN)-1,addr);
	while (M && abeg <= aend) {
		set_bit(abeg&CGRMASK, M->subpage);
		abeg++;
		if ((abeg&CGRMASK) == 0)
			M = M->next;
	}
	return 1;
}

int e_querymark(unsigned int addr, size_t len)
{
	unsigned int abeg, aend;
	tMpMap *M = FindM(addr);

	if (M == NULL) return 0;

	abeg = addr >> CGRAN;
	aend = (addr+len-1) >> CGRAN;

	if (debug_level('e')>2)
		dbug_printf("QUERY MARK from %08x to %08x for %08x\n",
			    abeg<<CGRAN,((aend+1)<<CGRAN)-1,addr);
	while (M && abeg <= aend) {
		if (test_bit(abeg&CGRMASK, M->subpage)) {
			if (debug_level('e')>1)
				dbug_printf("QUERY MARK found code at "
					    "%08x to %08x for %08x\n",
					    abeg<<CGRAN, ((abeg+1)<<CGRAN)-1,
					    addr);
			return 1;
		}
		abeg++;
		if ((abeg&CGRMASK) == 0)
			M = M->next;
	}
	return 0;
}

static void e_resetonepagemarks(unsigned int addr)
{
	int i, idx;
	tMpMap *M;

	M = FindM(addr); if (M==NULL) return;
	/* reset all 256 bits=8 longs for the page */
	idx = ((addr >> PAGE_SHIFT) & 255) << 3;
	if (debug_level('e')>1) e_printf("UNMARK 256 bits at %08x (long=%x)\n",addr,idx);
	for (i=0; i<8; i++) M->subpage[idx++] = 0;
}

void e_resetpagemarks(unsigned int addr, size_t len)
{
	int i, pages;

	pages = ((addr + len - 1) >> PAGE_SHIFT) - (addr >> PAGE_SHIFT) + 1;
	for (i = 0; i < pages; i++)
		e_resetonepagemarks(addr + i * PAGE_SIZE);
}

/////////////////////////////////////////////////////////////////////////////

void mprot_init(void)
{
	MpH = NULL;
	AddMpMap(0,0);	/* first mega in first entry */
}

void mprot_end(void)
{
	tMpMap *M = MpH;

	while (M) {
	    tMpMap *M2 = M;
	    M = M->next;
	    free(M2);
	}
	MpH = LastMp = NULL;
}

/////////////////////////////////////////////////////////////////////////////
