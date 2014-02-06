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

#include "emu86.h"
#include "trees.h"
#include "codegen-arch.h"

#ifdef __i386__
#define asmlinkage static __attribute__((used)) __attribute__((cdecl))
#else
#define asmlinkage static __attribute__((used))
#endif

#ifdef HOST_ARCH_X86

/*
 * Return address of the stub function is passed into eip
 */
static void m_munprotect(unsigned int addr, unsigned int len, unsigned char *eip)
{
	/* Oops.. we hit code, maybe the stub was set up before that
	 * code was parsed. Ok, undo the patch and clear that code */
	if (debug_level('e')>1)
	    e_printf("CODE %08x hit in DATA %p patch\n",addr,eip);
	InvalidateNodePage(addr,len,eip,NULL);
	e_resetpagemarks(addr,len);
}

static void r_munprotect(unsigned int addr, unsigned int len, unsigned char *eip)
{
	if (EFLAGS & EFLAGS_DF) addr -= len;
	if (debug_level('e')>1)
	    dbug_printf("\tR_MUNPROT %08x:%08x %s\n",
		addr,addr+len,(EFLAGS&EFLAGS_DF?"back":"fwd"));
	InvalidateNodePage(addr,len,eip,NULL);
	e_resetpagemarks(addr,len);
}

#define repmovs(std,letter,cld)			       \
	asm volatile(#std" ; rep ; movs"#letter ";" #cld"\n\t" \
		     : "=&c" (ecx), "=&D" (edi), "=&S" (esi)   \
		     : "0" (ecx), "1" (edi), "2" (esi) \
		     : "memory")

#define repstos(std,letter,cld)			       \
	asm volatile(#std" ; rep ; stos"#letter ";" #cld"\n\t" \
		     : "=&c" (ecx), "=&D" (edi) \
		     : "a" (eax), "0" (ecx), "1" (edi) \
		     : "memory")

struct rep_stack {
	unsigned char *esi, *edi;
	unsigned long ecx, eflags, edx, eax;
#ifdef __x86_64__
	unsigned long eax_pad;
#endif
	unsigned char *eip;
} __attribute__((packed));


asmlinkage void rep_movs_stos(struct rep_stack *stack)
{
	unsigned char *paddr = stack->edi;
	unsigned int ecx = stack->ecx;
	unsigned char *eip = stack->eip;
	unsigned int addr;
	unsigned int len = ecx;
	unsigned char *edi;
	unsigned char op;

	addr = paddr - mem_base;
	if (*eip == 0xf3) /* skip rep */
		eip++;
	op = eip[0];
	if (*eip == 0x66) {
		len *= 2;
		op = eip[1];
	}
	else if (*eip & 1)
		len *= 4;
	if (e_querymark(addr, len))
		r_munprotect(addr, len, eip);
	edi = LINEAR2UNIX(addr);
	if ((op & 0xfe) == 0xa4) { /* movs */
		unsigned int source = stack->esi - mem_base;
		unsigned char *esi = LINEAR2UNIX(source);
		if (ecx == len) {
			if (EFLAGS & EFLAGS_DF) repmovs(std,b,cld);
			else repmovs(,b,);
		}
		else if (ecx*2 == len) {
			if (EFLAGS & EFLAGS_DF) repmovs(std,w,cld);
			else repmovs(,w,);
		}
		else {
			if (EFLAGS & EFLAGS_DF) repmovs(std,l,cld);
			else repmovs(,l,);
		}
		if (EFLAGS & EFLAGS_DF) source -= len;
		else source += len;
		stack->esi = &mem_base[source];
	}
	else { /* stos */
		unsigned int eax = stack->eax;
		if (ecx == len) {
			if (EFLAGS & EFLAGS_DF) repstos(std,b,cld);
			else repstos(,b,);
		}
		else if (ecx*2 == len) {
			if (EFLAGS & EFLAGS_DF) repstos(std,w,cld);
			else repstos(,w,);
		}
		else {
			if (EFLAGS & EFLAGS_DF) repstos(std,l,cld);
			else repstos(,l,);
		}
	}
	if (EFLAGS & EFLAGS_DF) addr -= len;
	else addr += len;
	stack->edi = &mem_base[addr];
	stack->ecx = ecx;
}

/* ======================================================================= */

asmlinkage void wri_8(unsigned char *paddr, Bit8u value, unsigned char *eip)
{
	unsigned int addr = paddr - mem_base;
	/* check if code has been hit */
	if (e_querymark(addr, 1))
		m_munprotect(addr, 1, eip);
	/* there is a slight chance that this stub hits VGA memory.
	   For that case there is a simple instruction decoder but
	   we must use mov %al,(%edi) (%rdi for x86_64) */
	asm("movb %1,(%2)" : "=m"(*paddr) : "a"(value), "D"(paddr));
}

asmlinkage void wri_16(unsigned char *paddr, Bit16u value, unsigned char *eip)
{
	unsigned int addr = paddr - mem_base;
	if (e_querymark(addr, 2))
		m_munprotect(addr, 2, eip);
	asm("movw %1,(%2)" : "=m"(*paddr) : "a"(value), "D"(paddr));
}

asmlinkage void wri_32(unsigned char *paddr, Bit32u value, unsigned char *eip)
{
	unsigned int addr = paddr - mem_base;
	if (e_querymark(addr, 4))
		m_munprotect(addr, 4, eip);
	asm("movl %1,(%2)" : "=m"(*paddr) : "a"(value), "D"(paddr));
}

#ifdef __i386__

/*
 * stack on entry:
 *	esp+00	return address
 */

#define STUB_STK(cfunc) \
"		leal	(%esi,%ecx,1),%edi\n \
		pushal\n \
		pushl	%eax\n \
		pushl	%edi\n \
		call	"#cfunc"\n \
		addl	$8,%esp\n \
		popal\n \
		ret\n"


#define STUB_WRI(cfunc) \
"		pushl	(%esp)\n"	/* return addr = patch point+3 */ \
"		pushl	%eax\n"		/* value to write */ \
"		pushl	%edi\n"		/* addr where to write */ \
"		call	"#cfunc"\n" \
"		addl	$12,%esp\n"	/* remove parameters */ \
"		ret\n"

#define STUB_MOVS(cfunc,letter) \
"		pushl	(%esp)\n"	/* return addr = patch point+6 */ \
"		lods"#letter"\n"	/* fetch value to write   */ \
"		pushl	%eax\n"		/* value to write         */ \
"		pushl	%edi\n"		/* push fault address     */ \
"		scas"#letter"\n"	/* adjust edi depends:DF  */ \
"		cld\n" \
"		call	"#cfunc"\n" \
"		addl	$12,%esp\n"	/* remove parameters      */ \
"		ret\n"

#define STUB_STOS(cfunc,letter) \
"		pushl	%eax\n"		/* save eax (consecutive stosws) */ \
"		pushl	4(%esp)\n"	/* return addr = patch point+6 */ \
"		pushl	%eax\n"		/* value to write         */ \
"		pushl	%edi\n"		/* push fault address     */ \
"		scas"#letter"\n"	/* adjust edi depends:DF  */ \
"		cld\n" \
"		call	"#cfunc"\n" \
"		addl	$12,%esp\n"	/* remove parameters      */ \
"		popl	%eax\n" 	/* restore eax */ \
"		ret\n"

asm (
".text\n.globl stub_rep__\n"
"stub_rep__:	jecxz	1f\n"		/* zero move, nothing to do */
"		pushl	%eax\n"		/* save regs */
"		pushl	%edx\n"		/* edx used in 16bit overrun emulation, save too */
"		pushfl\n"		/* push flags for DF */
"		pushl	%ecx\n"		/* push count */
"		pushl	%edi\n"		/* push dest address */
"		pushl	%esi\n"		/* push source address */
"		pushl	%esp\n"		/* push stack */
"		cld\n"
"		call	rep_movs_stos\n"
"		addl	$4,%esp\n"	/* remove stack parameter */
"		popl	%esi\n"		/* obtain changed source address */
"		popl	%edi\n"		/* obtain changed dest address */
"		popl	%ecx\n"		/* obtain changed count */
"		popfl\n"		/* real CPU flags back */
"		popl	%edx\n"
"		popl	%eax\n"
"1:		ret\n"
);

/* ======================================================================= */

#else //__x86_64__

#define STUB_STK(cfunc) \
"		leal	(%rsi,%rcx,1),%edi\n" \
"		pushq	%rax\n"		/* save regs */ \
"		pushq	%rcx\n" \
"		pushq	%rdx\n" \
"		pushq	%rdi\n" \
"		pushq	%rsi\n" \
"		movl	%eax,%esi\n" \
					/* pass base address in %rdi */ \
"		call	"#cfunc"\n" \
"		popq	%rsi\n"		/* restore regs */ \
"		popq	%rdi\n" \
"		popq	%rdx\n" \
"		popq	%rcx\n" \
"		popq	%rax\n"	\
"		ret\n"


#define STUB_WRI(cfunc) \
"		movq	(%rsp),%rdx\n"	/* return addr = patch point+3 */ \
"		pushq	%rdi\n"		/* save regs */ \
"		movl	%eax,%esi\n"	/* value to write */ \
					/* pass addr where to write in %rdi */\
"		call	"#cfunc"\n" \
"		popq	%rdi\n"		/* restore regs */ \
"		ret\n"

#define STUB_MOVS(cfunc,letter) \
"		movq	(%rsp),%rdx\n"	/* return addr = patch point+6 */ \
"		movl	%edi,%ecx\n"	/* save edi to pass */ \
"		lods"#letter"\n"	/* fetch value to write   */ \
"		scas"#letter"\n"	/* adjust edi depends:DF  */ \
"		pushq	%rdi\n"		/* save regs */ \
"		pushq	%rsi\n" \
"		pushq	%rax\n"		/* not needed, but stack alignment */ \
"		movl	%eax,%esi\n"	/* value to write         */ \
"		movl	%ecx,%edi\n"	/* pass fault address in %rdi */ \
"		cld\n" \
"		call	"#cfunc"\n" \
"		popq	%rax\n"		/* restore regs */ \
"		popq	%rsi\n" \
"		popq	%rdi\n" \
"		ret\n"

#define STUB_STOS(cfunc,letter) \
"		movq	(%rsp),%rdx\n"	/* return addr = patch point+6 */ \
"		movl	%edi,%ecx\n"	/* save edi to pass */ \
"		scas"#letter"\n"	/* adjust edi depends:DF  */ \
"		pushq	%rdi\n"		/* save regs */ \
"		pushq	%rsi\n"		/* rax is saved for consecutive */ \
"		pushq	%rax\n"		/* merged stosws & stack alignment */ \
"		movl	%eax,%esi\n"	/* value to write         */ \
"		movl	%ecx,%edi\n"	/* pass fault address in %rdi */ \
"		cld\n" \
"		call	"#cfunc"\n" \
"		popq	%rax\n"		/* restore regs */ \
"		popq	%rsi\n" \
"		popq	%rdi\n"	\
"		ret\n"

asm (
".text\n.globl stub_rep__\n"
"stub_rep__:	jrcxz	1f\n"		/* zero move, nothing to do */
"		pushq	%rax\n"		/* save regs */
"		pushq	%rax\n"		/* save rax twice for 16-alignment */
"		pushq	%rdx\n"
"		pushfq\n"		/* push flags for DF */
"		pushq	%rcx\n"
"		pushq	%rdi\n"
"		pushq	%rsi\n"
"		movq	%rsp,%rdi\n"	/* pass stack address in %rdi */
"		cld\n"
"		call	rep_movs_stos\n"
"		popq	%rsi\n"		/* restore regs */
"		popq	%rdi\n"
"		popq	%rcx\n"
"		popfq\n"		/* real CPU flags back */
"		popq	%rdx\n"
"		popq	%rax\n"
"		popq	%rax\n"
"1:		ret\n"
);

#endif

asm (
		".text\n"
"stub_stk_16__:.globl stub_stk_16__\n"STUB_STK(wri_16)
"stub_stk_32__:.globl stub_stk_32__\n"STUB_STK(wri_32)
"stub_wri_8__: .globl stub_wri_8__\n "STUB_WRI(wri_8)
"stub_wri_16__:.globl stub_wri_16__\n"STUB_WRI(wri_16)
"stub_wri_32__:.globl stub_wri_32__\n"STUB_WRI(wri_32)
"stub_movsb__: .globl stub_movsb__\n "STUB_MOVS(wri_8,b)
"stub_movsw__: .globl stub_movsw__\n "STUB_MOVS(wri_16,w)
"stub_movsl__: .globl stub_movsl__\n "STUB_MOVS(wri_32,l)
"stub_stosb__: .globl stub_stosb__\n "STUB_STOS(wri_8,b)
"stub_stosw__: .globl stub_stosw__\n "STUB_STOS(wri_16,w)
"stub_stosl__: .globl stub_stosl__\n "STUB_STOS(wri_32,l)
);

#endif	//HOST_ARCH_X86

/* ======================================================================= */

