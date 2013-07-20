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

#include "config.h"

#ifdef USE_MHPDBG
#include "mhpdbg.h"
#endif

/* Define if we want graphics in X (of course we want :-) (root@zaphod) */
/* WARNING: This may not work in BSD, because it was written for Linux! */
#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <fenv.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/types.h>
#include <limits.h>
#include <getopt.h>

#ifdef __linux__
#include <sys/vt.h>
#include "Linux/fd.h"
#include "Linux/hdreg.h"
#include <syscall.h>
#endif

#include "emu.h"
#include "vm86plus.h"
#include "bios.h"
#include "mouse.h"
#include "serial.h"
#include "xms.h"
#include "timers.h"
#include "cmos.h"
#include "memory.h"
#include "termio.h"
#include "config.h"
#include "port.h"
#include "int.h"
#include "disks.h"
#include "ipx.h"                /* TRB - add support for ipx */
#include "keymaps.h"
#include "keyb_server.h"
#include "bitops.h"
#include "coopth.h"
#include "utilities.h"
#ifdef X86_EMULATOR
#include "cpu-emu.h"
#endif

#include "video.h"

#include "pic.h"

#include "dpmi.h"
#include "hlt.h"

#ifdef USING_NET
#include "ipx.h"
#endif

/* Needed for DIAMOND define */
#include "vc.h"

/*  */
/* vm86_GP_fault @@@  32768 MOVED_CODE_BEGIN @@@ 01/23/96, ./src/arch/linux/async/sigsegv.c --> src/emu-i386/do_vm86.c  */
/*
 * DANG_BEGIN_FUNCTION vm86_GP_fault
 *
 * description:
 * All from the kernel unhandled general protection faults from V86 mode
 * are handled here. This are mainly port IO and the HLT instruction.
 *
 * DANG_END_FUNCTION
 */

void vm86_GP_fault(void)
{

  unsigned char *csp, *lina;
  Bit32u org_eip;
  int pref_seg;
  int done,is_rep,prefix66,prefix67;

#if 0
  u_short *ssp;
  static int haltcount = 0;
#endif

#define LWECX	    (prefix66 ^ prefix67 ? REG(ecx) : LWORD(ecx))
#define setLWECX(x) {if (prefix66 ^ prefix67) REG(ecx)=(x); else LWORD(ecx) = (x);}
#define MAX_HALT_COUNT 3

#if 0
    csp = SEG_ADR((unsigned char *), cs, ip);
    ssp = SEG_ADR((us *), ss, sp);
    if ((*csp&0xfd)==0xec) {	/* inb, outb */
    	i_printf("VM86_GP_FAULT at %08lx, cod=%02x %02x*%02x %02x %02x %02x\n"
		 "                 stk=%04x %04x %04x %04x\n",
		 (long)csp,
		 csp[-2], csp[-1], csp[0], csp[1], csp[2], csp[3],
		 ssp[0], ssp[1], ssp[2], ssp[3]);
    }
#endif

  csp = lina = SEG_ADR((unsigned char *), cs, ip);

  /* fprintf(stderr, "CSP in cpu is 0x%04x\n", *csp); */


  /* DANG_BEGIN_REMARK
   * Here we handle all prefixes prior switching to the appropriate routines
   * The exception CS:EIP will point to the first prefix that effects
   * the faulting instruction, hence, 0x65 0x66 is same as 0x66 0x65.
   * So we collect all prefixes and remember them.
   * - Hans Lermen
   * DANG_END_REMARK
   */

  #define __SEG_ADR(type, seg, reg)  type(MK_FP32(seg, LWORD(e##reg)))
  done=0;
  is_rep=0;
  prefix66=prefix67=0;
  pref_seg=-1;

  do {
    switch (*(csp++)) {
       case 0x66:      /* operand prefix */  prefix66=1; break;
       case 0x67:      /* address prefix */  prefix67=1; break;
       case 0x2e:      /* CS */              pref_seg=REG(cs); break;
       case 0x3e:      /* DS */              pref_seg=REG(ds); break;
       case 0x26:      /* ES */              pref_seg=REG(es); break;
       case 0x36:      /* SS */              pref_seg=REG(ss); break;
       case 0x65:      /* GS */              pref_seg=REG(gs); break;
       case 0x64:      /* FS */              pref_seg=REG(fs); break;
       case 0xf2:      /* repnz */
       case 0xf3:      /* rep */             is_rep=1; break;
       default: done=1;
    }
  } while (!done);
  csp--;
  org_eip = REG(eip);
  LWORD(eip) += (csp-lina);

#if defined(X86_EMULATOR) && defined(CPUEMU_DIRECT_IO)
  if (InCompiledCode) {
    prefix66 ^= 1; prefix67 ^= 1;	/* since we come from 32-bit code */
/**/ e_printf("vm86_GP_fault: adjust prefixes to 66=%d,67=%d\n",
	prefix66,prefix67);
  }
#endif
  switch (*csp) {

       /* interrupt calls after prefix: we go back to vm86 */
  case 0xcc:    /* int 3       and let it generate an */
  case 0xcd:    /* int         interrupt (don't advance eip) */
  case 0xce:    /* into */
    break;
  case 0xcf:                   /* iret */
    if (prefix67) goto op0ferr; /* iretd */
    break;
  case 0xf1:                   /* int 1 */
    LWORD(eip)++; /* emulated "undocumented" instruction */
    do_int(1);
    break;

  case 0x6c:                    /* insb */
    /* NOTE: ES can't be overwritten; prefixes 66,67 should use esi,edi,ecx
     * but is anyone using extended regs in real mode? */
    /* WARNING: no test for DI wrapping! */
    LWORD(edi) += port_rep_inb(LWORD(edx), SEG_ADR((Bit8u *),es,di),
	LWORD(eflags)&DF, (is_rep? LWECX:1));
    if (is_rep) setLWECX(0);
    LWORD(eip)++;
    break;

  case 0x6d:			/* (rep) insw / insd */
    /* NOTE: ES can't be overwritten */
    /* WARNING: no test for _DI wrapping! */
    if (prefix66) {
      LWORD(edi) += port_rep_ind(LWORD(edx), SEG_ADR((Bit32u *),es,di),
	LWORD(eflags)&DF, (is_rep? LWECX:1));
    }
    else {
      LWORD(edi) += port_rep_inw(LWORD(edx), SEG_ADR((Bit16u *),es,di),
	LWORD(eflags)&DF, (is_rep? LWECX:1));
    }
    if (is_rep) setLWECX(0);
    LWORD(eip)++;
    break;


  case 0x6e:			/* (rep) outsb */
    if (pref_seg < 0) pref_seg = LWORD(ds);
    /* WARNING: no test for _SI wrapping! */
    LWORD(esi) += port_rep_outb(LWORD(edx), __SEG_ADR((Bit8u *),pref_seg,si),
	LWORD(eflags)&DF, (is_rep? LWECX:1));
    if (is_rep) setLWECX(0);
    LWORD(eip)++;
    break;


  case 0x6f:			/* (rep) outsw / outsd */
    if (pref_seg < 0) pref_seg = LWORD(ds);
    /* WARNING: no test for _SI wrapping! */
    if (prefix66) {
      LWORD(esi) += port_rep_outd(LWORD(edx), __SEG_ADR((Bit32u *),pref_seg,si),
	LWORD(eflags)&DF, (is_rep? LWECX:1));
    }
    else {
      LWORD(esi) += port_rep_outw(LWORD(edx), __SEG_ADR((Bit16u *),pref_seg,si),
	LWORD(eflags)&DF, (is_rep? LWECX:1));
    }
    if (is_rep) setLWECX(0);
    LWORD(eip)++;
    break;

  case 0xe5:			/* inw xx, ind xx */
    if (prefix66) REG(eax) = ind((int) csp[1]);
    else LWORD(eax) = inw((int) csp[1]);
    LWORD(eip) += 2;
    break;
  case 0xe4:			/* inb xx */
    LWORD(eax) &= ~0xff;
    LWORD(eax) |= inb((int) csp[1]);
    LWORD(eip) += 2;
    break;

  case 0xed:			/* inw dx, ind dx */
    if (prefix66) REG(eax) = ind(LWORD(edx));
    else LWORD(eax) = inw(LWORD(edx));
    LWORD(eip) += 1;
    break;
  case 0xec:			/* inb dx */
    LWORD(eax) &= ~0xff;
    LWORD(eax) |= inb(LWORD(edx));
    LWORD(eip) += 1;
    break;

  case 0xe7:			/* outw xx */
    if (prefix66) outd((int)csp[1], REG(eax));
    else outw((int)csp[1], LWORD(eax));
    LWORD(eip) += 2;
    break;
  case 0xe6:			/* outb xx */
    outb((int) csp[1], LO(ax));
    LWORD(eip) += 2;
    break;

  case 0xef:			/* outw dx */
    if (prefix66) outd(LWORD(edx), REG(eax));
    else outw(LWORD(edx), REG(eax));
    LWORD(eip) += 1;
    break;
  case 0xee:			/* outb dx */
    outb(LWORD(edx), LO(ax));
    LWORD(eip) += 1;
    break;

  case 0xf4:			/* hlt...I use it for various things,
		  like trapping direct jumps into the XMS function */
    hlt_handle();
    break;

  case 0x0f: /* was: RDE hack, now handled in cpu.c */
    if (!cpu_trap_0f(csp, NULL)) goto op0ferr;
    break;

  case 0xf0:			/* lock */
  default:
    if (is_rep) fprintf(stderr, "Nope REP F3,CSP = 0x%04x\n", csp[0]);
    /* er, why don't we advance eip here, and
	 why does it work??  */
op0ferr:
    REG(eip) = org_eip;
#ifdef USE_MHPDBG
    mhp_debug(DBG_GPF, 0, 0);
#endif
    set_debug_level('g', 1);
    error("general protection at %p: %x\n", lina,*lina);
    show_regs(__FILE__, __LINE__);
    show_ints(0, 0x33);
    fatalerr = 4;
    leavedos(fatalerr);		/* shouldn't return */
  }				/* end of switch() */

#ifdef TRACE_DPMI
  if (debug_level('t')==0)
#endif
  if (LWORD(eflags) & TF) {
    g_printf("TF: trap done");
    show_regs(__FILE__, __LINE__);
  }

}
/* @@@ MOVE_END @@@ 32768 */



/*
 * DANG_BEGIN_FUNCTION run_vm86
 *
 * description:
 * Here is where DOSEMU runs VM86 mode with the vm86() call
 * which also has the registers that it will be called with. It will stop
 * vm86 mode for many reasons, like trying to execute an interrupt, doing
 * port I/O to ports not opened for I/O, etc ...
 *
 * DANG_END_FUNCTION
 */
void
run_vm86(void)
{
  int retval;

  if (in_dpmi && !in_dpmi_dos_int) {
    run_dpmi();
  } else {
    /*
     * always invoke vm86() with this call.  all the messy stuff will be
     * in here.
     */

    if (
#ifdef X86_EMULATOR
	(debug_level('e')>1)||
#endif
	(debug_level('g')>3)) {
	dbug_printf ("DO_VM86,  cs=%04x:%04x ss=%04x:%04x f=%08x\n",
		_CS, _EIP, _SS, _SP, _EFLAGS);
	if (debug_level('g')>8)
	    dbug_printf ("ax=%04x bx=%04x ss=%04x sp=%04x bp=%04x\n"
	      		 "           cx=%04x dx=%04x ds=%04x cs=%04x ip=%04x\n"
	      		 "           si=%04x di=%04x es=%04x flg=%08x\n",
			_AX, _BX, _SS, _SP, _BP, _CX, _DX, _DS, _CS, _IP,
			_SI, _DI, _ES, _EFLAGS);
    }

    loadfpstate(vm86_fpu_state);
    in_vm86 = 1;
    retval = DO_VM86(&vm86s);
    in_vm86 = 0;
    savefpstate(vm86_fpu_state);
    /* there is no real need to save and restore the FPU state of the
       emulator itself: savefpstate (fnsave) also resets the current FPU
       state using fninit/ldmxcsr which is good enough for calling FPU-using
       routines.
    */
    feenableexcept(FE_DIVBYZERO | FE_OVERFLOW);

#if 0
    /* This will protect us from Mr.Norton's bugs */
    if (_EFLAGS & (AC|ID)) {
      _EFLAGS &= ~(AC|ID);
      if (debug_level('g')>3)
	dbug_printf("BUG: AC,ID set; flags changed to %08x\n",_EFLAGS);
    }
#endif

    if (
#ifdef X86_EMULATOR
	(debug_level('e')>1)||
#endif
	(debug_level('g')>3)) {
	dbug_printf ("RET_VM86, cs=%04x:%04x ss=%04x:%04x f=%08x ret=0x%x\n",
		_CS, _EIP, _SS, _SP, _EFLAGS, retval);
	if (debug_level('g')>8)
	    dbug_printf ("ax=%04x bx=%04x ss=%04x sp=%04x bp=%04x\n"
	      		 "           cx=%04x dx=%04x ds=%04x cs=%04x ip=%04x\n"
	      		 "           si=%04x di=%04x es=%04x flg=%08x\n",
			_AX, _BX, _SS, _SP, _BP, _CX, _DX, _DS, _CS, _IP,
			_SI, _DI, _ES, _EFLAGS);
    }

    switch VM86_TYPE(retval) {
    case VM86_UNKNOWN:
	vm86_GP_fault();
	break;
    case VM86_STI:
	I_printf("Return from vm86() for STI\n");
	break;
    case VM86_INTx:
	do_int(VM86_ARG(retval));
#ifdef USE_MHPDBG
	mhp_debug(DBG_INTx + (VM86_ARG(retval) << 8), 0, 0);
#endif
	break;
#ifdef USE_MHPDBG
    case VM86_TRAP:
	if(!mhp_debug(DBG_TRAP + (VM86_ARG(retval) << 8), 0, 0))
	   do_int(VM86_ARG(retval));
	break;
#endif
    case VM86_PICRETURN:
        I_printf("Return for FORCE_PIC\n");
        break;
    case VM86_SIGNAL:
	I_printf("Return for SIGNAL\n");
	break;
    default:
	error("unknown return value from vm86()=%x,%d-%x\n", VM86_TYPE(retval), VM86_TYPE(retval), VM86_ARG(retval));
	fatalerr = 4;
    }
  }

  do_periodic_stuff();
}

/*
 * DANG_BEGIN_FUNCTION loopstep_run_vm86
 *
 * description:
 * Here we collect all stuff, that has to be executed within
 * _one_ pass (step) of a loop containing run_vm86().
 * DANG_END_FUNCTION
 */
void loopstep_run_vm86(void)
{
	run_vm86();
	hardware_run();
	pic_run();		/* trigger any hardware interrupts requested */
}


static int callback_level;
#define MAX_CBKS 256
static far_t callback_rets[MAX_CBKS];
Bit16u CBACK_OFF;

static void callback_return(Bit32u off2, void *arg)
{
    far_t ret;
    assert(callback_level > 0);
    ret = callback_rets[callback_level - 1];
    REG(cs) = ret.segment;
    LWORD(eip) = ret.offset;
}

/*
 * do_call_back() calls a 16-bit DOS routine with a far call.
 * NOTE: It does _not_ save any of the vm86 registers except old cs:ip !!
 *       The _caller_ has to do this.
 */
static void __do_call_back(Bit16u cs, Bit16u ip, int intr)
{
	far_t *ret;

	if (fault_cnt && !in_leavedos) {
		error("do_call_back() executed within the signal context!\n");
		leavedos(25);
	}

	/* save return address - dont use DOS stack for that :( */
	assert(callback_level < MAX_CBKS);
	ret = &callback_rets[callback_level];
	ret->segment = REG(cs);
	ret->offset = LWORD(eip);
	REG(cs) = CBACK_SEG;
	LWORD(eip) = CBACK_OFF;

	if (intr)
		fake_int_to(cs, ip); /* far jump to the vm86(DOS) routine */
	else
		fake_call_to(cs, ip); /* far jump to the vm86(DOS) routine */

	callback_level++;
	/* no need to even put the thread to sleep:
	 * since the code flow was changed, coopth_yield()
	 * will magically not return before callback is finished. */
	/* Unfortunately this trick doesn't work for detached threads */
	coopth_ensure_attached();
	coopth_yield();
	callback_level--;
}

void do_call_back(Bit16u cs, Bit16u ip)
{
    __do_call_back(cs, ip, 0);
}

void do_int_call_back(int intno)
{
    __do_call_back(ISEG(intno), IOFF(intno), 1);
}

int vm86_init(void)
{
    emu_hlt_t hlt_hdlr;
    hlt_hdlr.name = "do_call_back";
    hlt_hdlr.start_addr = -1;
    hlt_hdlr.len = 1;
    hlt_hdlr.func = callback_return;
    CBACK_OFF = hlt_register_handler(hlt_hdlr);
    return 0;
}
