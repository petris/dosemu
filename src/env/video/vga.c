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

/*
 * video/vga.c - This file contains function for VGA-cards only
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#ifdef __linux__
#include <sys/kd.h>
#include <sys/vt.h>
#endif

#include "bios.h"
#include "emu.h"
#include "int.h"
#include "coopth.h"
#include "port.h"
#include "memory.h"
#include "video.h"
#include "vc.h"
#include "vga.h"
#include "termio.h"

#include "et4000.h"
#include "s3.h"
#include "ati.h"
#include "trident.h"
#include "avance.h"
#include "cirrus.h"
#include "matrox.h"
#include "wdvga.h"
#include "sis.h"
#include "vbe.h"
#include "pci.h"
#include "mapping.h"
#ifdef USE_SVGALIB
#include "svgalib.h"
#endif

#define PLANE_SIZE (64*1024)

static int vga_post_init(void);

/* Here are the REGS values for valid dos int10 call */

static unsigned char vregs[60] =
{
/* BIOS mode 0x12 */
  0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E, 0x00, 0x40, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xEA, 0x8C, 0xDF, 0x28, 0x00, 0xE7, 0x04, 0xE3,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07, 0x38, 0x39, 0x3A, 0x3B,
  0x3C, 0x3D, 0x3E, 0x3F, 0x01, 0x00, 0x0F, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0F, 0xFF,
  0x03, 0x21, 0x0F, 0x00, 0x06,
  0xE3
};

/* These are dummy calls */
static void save_ext_regs_dummy(u_char xregs[], u_short xregs16[])
{
  return;
}

static void restore_ext_regs_dummy(u_char xregs[], u_short xregs16[])
{
  return;
}

static void set_bank_read_dummy(u_char bank)
{
  return;
}

static void set_bank_write_dummy(u_char bank)
{
  return;
}

static u_char dummy_ext_video_port_in(ioport_t port)
{
  v_printf("Bad Read on port 0x%04x\n", port);
  return (0);
}

static void dummy_ext_video_port_out(ioport_t port, u_char value)
{
  v_printf("Bad Write on port 0x%04x with value 0x%02x\n", port, value);
}

static void dosemu_vga_screenoff(void)
{
  v_printf("vga_screenoff\n");
  /* synchronous reset on */
  port_out(0x00, SEQ_I);
  port_out(0x01, SEQ_D);

  /* turn off screen for faster VGA memory acces */
  port_out(0x01, SEQ_I);
  port_out(port_in(SEQ_D) | 0x20, SEQ_D);

  /* synchronous reset off */
  port_out(0x00, SEQ_I);
  port_out(0x03, SEQ_D);
}

static void dosemu_vga_screenon(void)
{
  v_printf("vga_screenon\n");

  /* synchronous reset on */
  port_out(0x00, SEQ_I);
  port_out(0x01, SEQ_D);

  /* turn screen back on */
  port_out(0x01, SEQ_I);
  port_out(port_in(SEQ_D) & 0xDF, SEQ_D);

  /* synchronous reset off */
  port_out(0x00, SEQ_I);
  port_out(0x03, SEQ_D);
}

static int dosemu_vga_setpalvec(int start, int num, u_char * pal)
{
  int i, j;

  /* select palette register */
  port_out(start, PEL_IW);

  for (j = 0; j < num; j++) {
    for (i = 0; i < 10; i++) ;	/* delay (minimum 240ns) */
    port_out(*(pal++), PEL_D);
    for (i = 0; i < 10; i++) ;	/* delay (minimum 240ns) */
    port_out(*(pal++), PEL_D);
    for (i = 0; i < 10; i++) ;	/* delay (minimum 240ns) */
    port_out(*(pal++), PEL_D);
  }

  /* Upon Restore Videos, also restore current Palette REgs */
  /*  port_out(dosemu_regs.regs[PELIW], PEL_IW);
  port_out(dosemu_regs.regs[PELIR], PEL_IR); */

  return j;
}

static void dosemu_vga_getpalvec(int start, int num, u_char * pal)
{
  int i, j;

  /* Save Palette Regs */
  /* dosemu_regs.regs[PELIW]=port_in(PEL_IW);
  dosemu_regs.regs[PELIR]=port_in(PEL_IR); */

  /* select palette register */
  port_out(start, PEL_IR);

  for (j = 0; j < num; j++) {
    for (i = 0; i < 10; i++) ;	/* delay (minimum 240ns) */
    *(pal++) = (char) port_in(PEL_D);
    for (i = 0; i < 10; i++) ;	/* delay (minimum 240ns) */
    *(pal++) = (char) port_in(PEL_D);
    for (i = 0; i < 10; i++) ;	/* delay (minimum 240ns) */
    *(pal++) = (char) port_in(PEL_D);
  }

  /* Put Palette regs back */
  /* port_out(dosemu_regs.regs[PELIW], PEL_IW);
  port_out(dosemu_regs.regs[PELIR], PEL_IR); */
}

/* Prepare to do business with the EGA/VGA card */
static inline void disable_vga_card(void)
{
  /* enable video */
  emu_video_retrace_off();
  port_in(IS1_R);
  port_out(0x00, ATT_IW);
}

/* Finish doing business with the EGA/VGA card */
static inline void enable_vga_card(void)
{
  emu_video_retrace_off();
  port_in(IS1_R);
  port_out(0x20, ATT_IW);
  emu_video_retrace_on();
  /* disable video */
}

/* Store current EGA/VGA regs */
static void store_vga_regs(u_char regs[])
{
  int i;

  emu_video_retrace_off();
  /* Start with INDEXS */
  regs[CRTI] = port_in(CRT_I);
  regs[GRAI] = port_in(GRA_I);
  regs[SEQI] = port_in(SEQ_I);
  regs[FCR] = port_in(FCR_R);
  regs[ISR1] = port_in(IS1_R) | 0x09;
  regs[ISR0] = port_in(IS0_R);

  /* save VGA registers */
  for (i = 0; i < CRT_C; i++) {
    port_out(i, CRT_I);
    regs[CRT + i] = port_in(CRT_D);
  }

  for (i = 0; i < ATT_C; i++) {
    port_in(IS1_R);
    port_out(i, ATT_IW);
    regs[ATT + i] = port_in(ATT_R);
  }
  for (i = 0; i < GRA_C; i++) {
    port_out(i, GRA_I);
    regs[GRA + i] = port_in(GRA_D);
  }
  for (i = 1; i < SEQ_C; i++) {
    port_out(i, SEQ_I);
    regs[SEQ + i] = port_in(SEQ_D);
  }
  regs[SEQ + 1] = regs[SEQ + 1] | 0x20;

  regs[MIS] = port_in(MIS_R);

  port_out(regs[CRTI], CRT_I);
  port_out(regs[GRAI], GRA_I);
  port_out(regs[SEQI], SEQ_I);
  v_printf("Store regs complete!\n");
  emu_video_retrace_on();
}

static sem_t cpy_sem;
static pthread_t cpy_thr;
struct vmem_chunk {
  u_char *mem;
  unsigned vmem;
  size_t len;
  int to_vid;
  int ctid;
};
static struct vmem_chunk vmem_chunk_thr;

static void *vmemcpy_thread(void *arg)
{
  struct vmem_chunk *vmc = arg;
  while (1) {
    sem_wait(&cpy_sem);
    if (vmc->to_vid)
      MEMCPY_2DOS(vmc->vmem, vmc->mem, vmc->len);
    else
      MEMCPY_2UNIX(vmc->mem, vmc->vmem, vmc->len);
    coopth_wake_up(vmc->ctid);
  }
  return NULL;
}

static void sleep_cb(void *arg)
{
  sem_t *sem = arg;
  sem_post(sem);
}

/* Store EGA/VGA display planes (4) */
static void store_vga_mem(u_char * mem, int banks)
{
  int cbank, plane, planar;
  unsigned vmem = GRAPH_BASE;

  if (config.chipset == VESA && banks > 1)
    vmem = vesa_get_lfb();
  else if (config.chipset == ET4000) {
/*
 * The following is from the X files
 * we need this here , cause we MUST disable the ROM SYNC feature
*/
    u_char temp1;

    port_out(0x34, CRT_I);
    temp1 = port_in(CRT_D);
    port_out(temp1 & 0x0F, CRT_D);
    port_in(0x3cd);
    port_out(0x00, 0x3cd);
  }
  planar = 1;
  if (vmem != GRAPH_BASE) {
    planar = 0;
    vmem -= PLANE_SIZE;
  } else if (banks > 1) {
    port_out(0x4, SEQ_I);
    /* check whether we are using packed or planar mode;
       for standard VGA modes we set 640x480x16 colors  */
    if (port_in(SEQ_D) & 8) planar = 0;
  } else {
    set_regs((u_char *) vregs, 1);
  }
  for (cbank = 0; cbank < banks; cbank++) {
    if (planar && banks > 1) set_bank_read(cbank);
    for (plane = 0; plane < 4; plane++) {
      if (planar) {
        /* Store planes */
	port_out(0x04, GRA_I);
        port_out(plane, GRA_D);
      } else if (vmem == GRAPH_BASE)
	set_bank_read(cbank * 4 + plane);
      else
	vmem += PLANE_SIZE;

      /* reading video memory can be very slow: 16384MB takes
	 1.5 seconds here using a linear frame buffer. So we'll
	 have lots of SIGALRMs coming by. Another solution to
	 this problem would be to use a thread --Bart */
      /* SOLVED: with coopthreads_v2 we can do that in a separate
       * pthread's thread, here's how: --stsp */
      vmem_chunk_thr.mem = mem;
      vmem_chunk_thr.vmem = vmem;
      vmem_chunk_thr.len = PLANE_SIZE;
      vmem_chunk_thr.to_vid = 0;
      vmem_chunk_thr.ctid = coopth_get_tid();
      coopth_set_sleep_handler(sleep_cb, &cpy_sem);
      coopth_sleep();
      /* end of magic: chunk copied */

      v_printf("BANK READ Bank=%d, plane=0x%02x, mem=%08x\n", cbank, plane, READ_DWORD(vmem));
      mem += PLANE_SIZE;
    }
  }
  v_printf("GRAPH_BASE to mem complete!\n");
  if (config.chipset == ET4000) {
    port_out(0x00, 0x3cd);
  }
}

/* Restore EGA/VGA display planes (4) */
static void restore_vga_mem(u_char * mem, int banks)
{
  int plane, cbank, planar;
  unsigned vmem = GRAPH_BASE;

  if (config.chipset == VESA && banks > 1)
    vmem = vesa_get_lfb();
  else if (config.chipset == ET4000)
    port_out(0x00, 0x3cd);
  planar = 1;
  if (vmem != GRAPH_BASE) {
    planar = 0;
    vmem -= PLANE_SIZE;
  } else if (banks > 1) {
    port_out(0x4, SEQ_I);
    /* check whether we are using packed or planar mode;
       for standard VGA modes we set 640x480x16 colors  */
    if (port_in(SEQ_D) & 8) planar = 0;
  } else {
    set_regs((u_char *) vregs, 1);
  }
  if (planar) {
      /* disable Set/Reset Register */
      port_out(0x01, GRA_I);
      port_out(0x00, GRA_D);
  }
  for (cbank = 0; cbank < banks; cbank++) {
    if (planar && banks > 1) set_bank_write(cbank);
    for (plane = 0; plane < 4; plane++) {
      if (planar) {
        /* Store planes */
        port_out(0x02, SEQ_I);
        port_out(1 << plane, SEQ_D);
      } else if (vmem == GRAPH_BASE)
	set_bank_write(cbank * 4 + plane);
      else
	vmem += PLANE_SIZE;

      vmem_chunk_thr.mem = mem;
      vmem_chunk_thr.vmem = vmem;
      vmem_chunk_thr.len = PLANE_SIZE;
      vmem_chunk_thr.to_vid = 1;
      vmem_chunk_thr.ctid = coopth_get_tid();
      coopth_set_sleep_handler(sleep_cb, &cpy_sem);
      coopth_sleep();
      /* end of magic: chunk copied */

      v_printf("BANK WRITE Bank=%d, plane=0x%02x, mem=%08x\n", cbank, plane, *(int *)mem);
      mem += PLANE_SIZE;
    }
  }
  v_printf("mem to GRAPH_BASE complete!\n");
}

/* Restore EGA/VGA regs */
static void restore_vga_regs(u_char regs[], u_char xregs[], u_short xregs16[])
{
  restore_ext_regs(xregs, xregs16);
  set_regs(regs, 0);
  v_printf("Restore_vga_regs completed!\n");
}

/* Save all necessary info to allow switching vt's */
void save_vga_state(struct video_save_struct *save_regs)
{

  v_printf("Saving data for %s\n", save_regs->video_name);
  port_enter_critical_section(__func__);
  dosemu_vga_screenoff();
  disable_vga_card();
  store_vga_regs(save_regs->regs);
  save_ext_regs(save_regs->xregs, save_regs->xregs16);
  v_printf("ALPHA mode save being attempted\n");
  save_regs->banks = 1;
  port_out(0x06, GRA_I);
  if (!(port_in(GRA_D) & 0x01)) {
    /* text mode */
    v_printf("ALPHA mode save being performed\n");
  }
  else if (save_regs->video_mode > 0x13 && config.chipset && save_regs != &linux_regs)
        /*not standard VGA modes*/      /* not plainvga */
    save_regs->banks = (config.gfxmemsize + (4 * PLANE_SIZE / 1024) - 1) /
      (4 * PLANE_SIZE / 1024);
  v_printf("Mode  == %d\n", save_regs->video_mode);
  v_printf("Banks == %d\n", save_regs->banks);
  if (save_regs->banks) {
    if (!save_regs->mem) {
      save_regs->mem = malloc(save_regs->banks * 4 * PLANE_SIZE);
    }

    store_vga_mem(save_regs->mem, save_regs->banks);
  }
  dosemu_vga_getpalvec(0, 256, save_regs->pal);
  restore_vga_regs(save_regs->regs, save_regs->xregs, save_regs->xregs16);
  restore_ext_regs(save_regs->xregs, save_regs->xregs16);
  enable_vga_card();
  port_leave_critical_section();

  v_printf("Store_vga_state complete\n");
}

/* Restore all necessary info to allow switching back to vt */
void restore_vga_state(struct video_save_struct *save_regs)
{

  v_printf("Restoring data for %s\n", save_regs->video_name);
  port_enter_critical_section(__func__);
  /* my Matrox G550 seem to completely ignore the bit15, so
   * lets disable the below trick. Are there any cards that
   * really need this? */
#if 0
  /* do a BIOS setmode to the original mode before restoring registers.
     This helps if we don't restore all registers ourselves or if the
     VESA BIOS is buggy */
  if (config.chipset == PLAINVGA || config.chipset == VESA) {
    char bios_data_area[0x100];
    int mode = save_regs->video_mode;
    if (config.chipset == VESA && !config.gfxmemsize)
      mode |= 0x8000;		/* preserve video memory */
    memcpy(bios_data_area, (void *)BIOS_DATA_SEG, 0x100);
    do_int10_setmode(mode);
    memcpy((void *)BIOS_DATA_SEG, bios_data_area, 0x100);
  }
#endif
  dosemu_vga_screenoff();
  disable_vga_card();
  restore_vga_regs(save_regs->regs, save_regs->xregs, save_regs->xregs16);
  restore_ext_regs(save_regs->xregs, save_regs->xregs16);
  if (save_regs->banks)
    restore_vga_mem(save_regs->mem, save_regs->banks);
  if (save_regs->release_video) {
    v_printf("Releasing video memory\n");
    free(save_regs->mem);
    save_regs->mem = NULL;
  }
  dosemu_vga_setpalvec(0, 256, save_regs->pal);
  restore_vga_regs(save_regs->regs, save_regs->xregs, save_regs->xregs16);
  restore_ext_regs(save_regs->xregs, save_regs->xregs16);
  v_printf("Permissions=%d\n", permissions);
  enable_vga_card();
  dosemu_vga_screenon();
  port_leave_critical_section();

  v_printf("Restore_vga_state complete\n");
}

static void pcivga_init(void)
{
  unsigned long base, size;
  int i;
  pciRec *pcirec;

  if (!config.pci)
    /* set up emulated r/o PCI config space, enough
       for VGA BIOSes to use */
    pcirec = pciemu_setup(PCI_CLASS_DISPLAY_VGA << 8);
  else
    pcirec = pcibios_find_class(PCI_CLASS_DISPLAY_VGA << 8, 0);
  if (!pcirec) {
    /* only set pci_video=0 if no PCI is available. Otherwise
       it's set by default */
    config.pci_video = 0;
    return;
  }
  v_printf("PCIVGA: found PCI device, bdf=%#x\n", pcirec->bdf);
  for (i = 0; i < 6; i++) {
    base = pcirec->region[i].base;
    if (!base)
      continue;
    size = pcirec->region[i].size;
    if (pcirec->region[i].type == PCI_BASE_ADDRESS_SPACE_IO) {
      emu_iodev_t io_device;
      v_printf("PCIVGA: found IO region at %#lx [%#lx]\n", base, size);

      /* register PCI VGA ports */
      io_device.irq = EMU_NO_IRQ;
      io_device.fd = -1;
      io_device.handler_name = "std port io";
      io_device.start_addr = base;
      io_device.end_addr = base + size;
      port_register_handler(io_device, PORT_FAST);
    } else if (base >= LOWMEM_SIZE + HMASIZE) {
      v_printf("PCIVGA: found MEM region at %#lx [%#lx]\n", base, size);
      register_hardware_ram('v', base, size);
    }
  }
}

static void set_console_video(void)
{
  /* warning! this must come first! the VT_ACTIVATES which some below
     * cause set_dos_video() and set_linux_video() to use the modecr
     * settings.  We have to first find them here.
     */
  int permtest;

  permtest = set_ioperm(0x3d4, 2, 1);	/* get 0x3d4 and 0x3d5 */
  permtest |= set_ioperm(0x3da, 1, 1);
  permtest |= set_ioperm(0x3c0, 2, 1);	/* get 0x3c0 and 0x3c1 */
  if ((config.chipset == S3) ||
      (config.chipset == CIRRUS) ||
      (config.chipset == WDVGA) ||
      (config.chipset == MATROX)) {
    permtest |= set_ioperm(0x102, 2, 1);
    permtest |= set_ioperm(0x2ea, 4, 1);
  }
  if (config.chipset == ATI) {
    permtest |= set_ioperm(0x102, 1, 1);
    permtest |= set_ioperm(0x1ce, 2, 1);
    permtest |= set_ioperm(0x2ec, 4, 1);
  }
  if ((config.chipset == MATROX) ||
      (config.chipset == WDVGA)) {
    permtest |= set_ioperm(0x3de, 2, 1);
  }
}

static int vga_initialize(void)
{
  set_console_video();

  linux_regs.mem = NULL;
  dosemu_regs.mem = NULL;
  get_perm();

  /* defaults to override */
  save_ext_regs = save_ext_regs_dummy;
  restore_ext_regs = restore_ext_regs_dummy;
  set_bank_read = set_bank_read_dummy;
  set_bank_write = set_bank_write_dummy;
  ext_video_port_in = dummy_ext_video_port_in;
  ext_video_port_out = dummy_ext_video_port_out;

  if (config.pci_video)
    pcivga_init();

  switch (config.chipset) {
  case PLAINVGA:
    v_printf("Plain VGA in use\n");
    /* no special init here */
    break;
  case TRIDENT:
    vga_init_trident();
    v_printf("Trident CARD in use\n");
    break;
  case ET4000:
    vga_init_et4000();
    v_printf("ET4000 CARD in use\n");
    break;
  case S3:
    vga_init_s3();
    v_printf("S3 CARD in use\n");
    break;
  case AVANCE:
    vga_init_avance();
    v_printf("Avance Logic CARD in use\n");
    break;
  case ATI:
    vga_init_ati();
    v_printf("ATI CARD in use\n");
    break;
  case CIRRUS:
    vga_init_cirrus();
    v_printf("Cirrus CARD in use\n");
    break;
  case MATROX:
    vga_init_matrox();
    v_printf("Matrox CARD in use\n");
    break;
  case WDVGA:
    vga_init_wd();
    v_printf("Paradise CARD in use\n");
    break;
  case SIS:
    vga_init_sis();
    v_printf("SIS CARD in use\n");
    break;
  case SVGALIB:
#ifdef USE_SVGALIB
    vga_init_svgalib();
    v_printf("svgalib handles the graphics\n");
#else
    error("svgalib support is not compiled in, \"plainvga\" will be used.\n");
#endif
    break;
  case VESA:
    v_printf("Using the VESA BIOS for save/restore\n");
    /* init done later */
    break;

  default:
    v_printf("Unspecific VIDEO selected = 0x%04x\n", config.chipset);
  }

  linux_regs.video_name = "Linux Regs";
  /* the mode at /dev/mem:0x449 will be correct for both vgacon and vesafb.
     Other fbs and X can often restore themselves */
  load_file("/dev/mem", 0x449, &linux_regs.video_mode, 1);
  linux_regs.release_video = 0;

  dosemu_regs.video_name = "Dosemu Regs";
  dosemu_regs.video_mode = 3;
  dosemu_regs.regs[CRTI] = 0;
  dosemu_regs.regs[SEQI] = 0;
  dosemu_regs.regs[GRAI] = 0;

  /* don't release it; we're trying text mode restoration */
  dosemu_regs.release_video = 1;

  return 0;
}

static void vga_close(void)
{
  clear_console_video();
  /* if the Linux console uses fbcon we can force
     a complete text redraw by doing round-trip
     vc switches; otherwise (vgacon) it doesn't hurt */
  if(!config.detach) {
    int arg;
    ioctl(console_fd, VT_OPENQRY, &arg);
    vt_activate(arg);
    vt_activate(scr_state.console_no);
    ioctl(console_fd, VT_DISALLOCATE, arg);
  }
  ioctl(console_fd, KIOCSOUND, 0);	/* turn off any sound */

  pthread_cancel(cpy_thr);
  pthread_join(cpy_thr, NULL);
  sem_destroy(&cpy_sem);
}

struct video_system Video_graphics = {
   vga_initialize,
   vga_post_init,
   vga_close,
   NULL,
   NULL,             /* update_screen */
   NULL,
   NULL,
   NULL
};

#if 0
/* Store current actuall EGA/VGA regs */
static void dump_video_regs(void)
{
  int i;

  emu_video_retrace_off();
  /* save VGA registers */
  v_printf("CRT=");
  for (i = 0; i < CRT_C; i++) {
    port_out(i, CRT_I);
    v_printf("%02x ", (u_char) port_in(CRT_D));
  }
  v_printf("\n");
  v_printf("ATT=");
  for (i = 0; i < ATT_C; i++) {
    port_in(IS1_R);
    port_out(i, ATT_IW);
    v_printf("%02x ", (u_char) port_in(ATT_R));
  }
  v_printf("\n");
  v_printf("GRA=");
  for (i = 0; i < GRA_C; i++) {
    port_out(i, GRA_I);
    v_printf("%02x ", (u_char) port_in(GRA_D));
  }
  v_printf("\n");
  v_printf("SEQ=");
  for (i = 0; i < SEQ_C; i++) {
    port_out(i, SEQ_I);
    v_printf("%02x ", (u_char) port_in(SEQ_D));
  }
  v_printf("\n");
  v_printf("MIS=0x%02x\n", (u_char) port_in(MIS_R));

  /* et4000 regs should be added here .... :-) */
  if (config.chipset == TRIDENT) {
    trident_set_old_regs();
    port_out(0x0c, SEQ_I);
    v_printf("0C=0x%02x ", (u_char) port_in(SEQ_D));
    port_out(0x0d, SEQ_I);
    v_printf("0D=0x%02x ", (u_char) port_in(SEQ_D));
    port_out(0x0e, SEQ_I);
    v_printf("0E=0x%02x ", (u_char) port_in(SEQ_D));
    trident_set_new_regs();
    port_out(0x0d, SEQ_I);
    v_printf("0D=0x%02x ", (u_char) port_in(SEQ_D));
    port_out(0x0e, SEQ_I);
    v_printf("0E=0x%02x ", (u_char) port_in(SEQ_D));
    port_out(0x0f, SEQ_I);
    v_printf("0F=0x%02x ", (u_char) port_in(SEQ_D));
    port_out(0x1e, CRT_I);
    v_printf("CRT 1E=0x%02x ", (u_char) port_in(CRT_D));
    port_out(0x1f, CRT_I);
    v_printf("CRT 1F=0x%02x ", (u_char) port_in(CRT_D));
    port_out(0x0f, GRA_I);
    v_printf("GRA 0F=0x%02x\n", (u_char) port_in(GRA_D));
  }
  else if (config.chipset == ET4000) {
    v_printf("ET4000 port 0x3c3=0x%02x\n", (u_char) port_in(0x3c3));
    v_printf("ET4000 port 0x3cd=0x%02x\n", (u_char) port_in(0x3cd));
    port_out(0x24, CRT_I);
    v_printf("ET4000 CRT 0x24  =0x%02x\n", (u_char) port_in(CRT_D));
    port_out(0x32, CRT_I);
    v_printf("ET4000 CRT 0x32  =0x%02x\n", (u_char) port_in(CRT_D));
    port_out(0x33, CRT_I);
    v_printf("ET4000 CRT 0x33  =0x%02x\n", (u_char) port_in(CRT_D));
    port_out(0x34, CRT_I);
    v_printf("ET4000 CRT 0x34  =0x%02x\n", (u_char) port_in(CRT_D));
    port_out(0x35, CRT_I);
    v_printf("ET4000 CRT 0x35  =0x%02x\n", (u_char) port_in(CRT_D));
    port_out(0x36, CRT_I);
    v_printf("ET4000 CRT 0x36  =0x%02x\n", (u_char) port_in(CRT_D));
    port_out(0x37, CRT_I);
    v_printf("ET4000 CRT 0x37  =0x%02x\n", (u_char) port_in(CRT_D));
    port_out(0x3f, CRT_I);
    v_printf("ET4000 CRT 0x3f  =0x%02x\n", (u_char) port_in(CRT_D));
    port_out(0x6, SEQ_I);
    v_printf("ET4000 SEQ 0x06  =0x%02x\n", (u_char) port_in(SEQ_D));
    port_out(0x7, SEQ_I);
    v_printf("ET4000 SEQ 0x07  =0x%02x\n", (u_char) port_in(SEQ_D));
    port_in(IS1_R);
    port_out(0x16, ATT_IW);
    v_printf("ET4000 ATT 0x16  =0x%02x\n", (u_char) port_in(ATT_R));
    v_printf("ET4000 port 0x3cb=0x%02x\n", (u_char) port_in(0x3cb));
    v_printf("ET4000 port 0x3d8=0x%02x\n", (u_char) port_in(0x3d8));
    v_printf("ET4000 port 0x3bf=0x%02x\n", (u_char) port_in(0x3bf));
    port_out(0xd, GRA_I);
    v_printf("ET4000 GRA 0x0d  =0x%02x\n", (u_char) port_in(GRA_D));
    port_out(0xe, GRA_I);
    v_printf("ET4000 GRA 0x0e  =0x%02x\n", (u_char) port_in(GRA_D));
  }
  emu_video_retrace_on();
}
#endif

/* init_vga_card - Initialize a VGA-card */

static int vga_post_init(void)
{
  sem_init(&cpy_sem, 0, 0);
  pthread_create(&cpy_thr, NULL, vmemcpy_thread, &vmem_chunk_thr);

  /* this function initialises vc switch routines */
  Video_console.init();

  if (!config.mapped_bios) {
    error("CAN'T DO VIDEO INIT, BIOS NOT MAPPED!\n");
    leavedos(23);
  }

  g_printf("INITIALIZING VGA CARD BIOS!\n");

  /* If there's a DOS TSR in real memory (say, univbe followed by loadlin)
     then don't call int10 here yet */
  if (!config.vbios_post) {
    unsigned int addr = SEGOFF2LINEAR(FP_SEG16(int_bios_area[0x10]),
				      FP_OFF16(int_bios_area[0x10]));
    if (addr < VBIOS_START || addr >= VBIOS_START + VBIOS_SIZE) {
      error("VGA: int10 is not in the BIOS (loadlin used?)\n"
	    "Try the vga_reset utility of svgalib or set $_vbios_post=(1) "
	    " in dosemu.conf\n");
      leavedos(23);
    }
  }

  if (config.chipset == VESA) {
    port_enter_critical_section(__func__);
    vesa_init();
    port_leave_critical_section();
  }

  /* fall back to 256K if not autodetected at this stage */
  if (config.gfxmemsize < 0) config.gfxmemsize = 256;
  v_printf("VGA: mem size %ld\n", config.gfxmemsize);

  save_vga_state(&linux_regs);

  dosemu_vga_screenon();
  config.vga = 1;
  set_vc_screen_page();
  video_initialized = 1;
  return 0;
}

/* End of video/vga.c */
