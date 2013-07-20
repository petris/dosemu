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

/* DANG_BEGIN_MODULE
 *
 * REMARK
 * ser_init.c: Serial ports initialization for DOSEMU
 * Please read the README.serial file in this directory for more info!
 *
 * Lock file stuff was derived from Taylor UUCP with these copyrights:
 * Copyright (C) 1991, 1992 Ian Lance Taylor
 * Uri Blumenthal <uri@watson.ibm.com> (C) 1994
 * Paul Cadach, <paul@paul.east.alma-ata.su> (C) 1994
 *
 * Rest of serial code Copyright (C) 1995 by Mark Rejhon
 *
 * The code in this module is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This module is maintained by Mark Rejhon at these Email addresses:
 *      marky@magmacom.com
 *      ag115@freenet.carleton.ca
 * /REMARK
 *
 * maintainer:
 * Mark Rejhon <marky@ottawa.com>
 *
 * DANG_END_MODULE
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <pwd.h>

#include "config.h"
#include "Linux/serial.h"
#include "emu.h"
#include "port.h"
#include "mouse.h"
#include "bios.h"
#include "pic.h"
#include "serial.h"
#include "ser_defs.h"
#include "utilities.h"	/* due to getpwnam */
#include "iodev.h"
#include "vc.h"

int no_local_video = 0;
com_t com[MAX_SER];
u_char irq_source_num[255];	/* Index to map from IRQ no. to serial port */

/* See README.serial file for more information on the com[] structure
 * The declarations for this is in ../include/serial.h
 */

/*  Determines if the tty is already locked.  Stolen from uri-dip-3.3.7k
 *  Nice work Uri Blumenthal & Ian Lance Taylor!
 *  [nam = complete path to lock file, return = nonzero if locked]
 */
static int tty_already_locked(char *nam)
{
  int  i = 0, pid = 0;
  FILE *fd = (FILE *)0;

  /* Does the lock file on our device exist? */
  if ((fd = fopen(nam, "r")) == (FILE *)0)
    return 0; /* No, return perm to continue */

  /* Yes, the lock is there.  Now let's make sure at least */
  /* there's no active process that owns that lock.        */
  if(config.tty_lockbinary)
    i = read(fileno(fd), &pid, sizeof(pid)) == sizeof(pid);
  else
    i = fscanf(fd, "%d", &pid);

  (void) fclose(fd);

  if (i != 1) /* Lock file format's wrong! Kill't */
    return 0;

  /* We got the pid, check if the process's alive */
  if (kill(pid, 0) == 0)      /* it found process */
    return 1;                 /* Yup, it's running... */

  /* Dead, we can proceed locking this device...  */
  return 0;
}


/*  Locks or unlocks a terminal line Stolen from uri-dip-3.3.7k
 *  Nice work Uri Blumenthal & Ian Lance Taylor!
 *  [path = device name,
 *   mode: 1 = lock, 2 = reaquire lock, anythingelse = unlock,
 *   return = zero if success, greater than zero for failure]
 */
static int tty_lock(char *path, int mode)
{
  char saved_path[strlen(config.tty_lockdir) + 1 +
                  strlen(config.tty_lockfile) +
                  strlen(path) + 1];
  struct passwd *pw;
  pid_t ime;
  char *slash;

  if (path == NULL) return(0);        /* standard input */
  slash = strrchr(path, '/');
  if (slash == NULL)
    slash = path;
  else
    slash++;

  sprintf(saved_path, "%s/%s%s", config.tty_lockdir, config.tty_lockfile,
	  slash);

  if (mode == 1) {      /* lock */
    {
      FILE *fd;
      if (tty_already_locked(saved_path) == 1) {
        error("attempt to use already locked tty %s\n", saved_path);
        return (-1);
      }
      unlink(saved_path);	/* kill stale lockfiles, if any */
      fd = fopen(saved_path, "w");
      if (fd == (FILE *)0) {
        error("tty: lock: (%s): %s\n", saved_path, strerror(errno));
        return(-1);
      }

      ime = getpid();
      if(config.tty_lockbinary)
	write (fileno(fd), &ime, sizeof(ime));
      else
	fprintf(fd, "%10d\n", (int)ime);

      (void)fclose(fd);
    }

    /* Make sure UUCP owns the lockfile.  Required by some packages. */
    if ((pw = getpwnam(OWNER_LOCKS)) == NULL) {
      error("tty: lock: UUCP user %s unknown!\n", OWNER_LOCKS);
      return(0);        /* keep the lock anyway */
    }

    (void) chown(saved_path, pw->pw_uid, pw->pw_gid);
    (void) chmod(saved_path, 0644);
  }
  else if (mode == 2) { /* re-acquire a lock after a fork() */
    FILE *fd;

     fd = fopen(saved_path,"w");
     if (fd == (FILE *)0) {
      error("tty_lock: reacquire (%s): %s\n",
              saved_path, strerror(errno));
      return(-1);
    }
    ime = getpid();

    if(config.tty_lockbinary)
      write (fileno(fd), &ime, sizeof(ime));
    else
      fprintf(fd, "%10d\n", (int)ime);

    (void) fclose(fd);
    (void) chmod(saved_path, 0444);
    return(0);
  }
  else {    /* unlock */
    FILE *fd;
    int retval;

    fd = fopen(saved_path,"w");
    if (fd == (FILE *)0) {
      error("tty_lock: can't reopen %s to delete: %s\n",
             saved_path, strerror(errno));
      return (-1);
    }

    retval = unlink(saved_path);
    if (retval < 0) {
      error("tty: unlock: (%s): %s\n", saved_path,
             strerror(errno));
      return(-1);
    }
  }
  return(0);
}

static void async_serial_run(void *arg)
{
  int num = (long)arg;
  s_printf("SER%d: Async notification received\n", num);
  serial_update(num);
}

/* This function opens ONE serial port for DOSEMU.  Normally called only
 * by do_ser_init below.   [num = port, return = file descriptor]
 */
int ser_open(int num)
{
  s_printf("SER%d: Running ser_open, fd=%d\n",num, com[num].fd);

  if (com_cfg[num].mouse && !on_console()) {
    s_printf("SER%d: Not touching mouse outside of the console!\n",num);
    return(-1);
  }

  if (com[num].fd != -1) return (com[num].fd);

  if ( com_cfg[num].virtual )
  {
    s_printf("SER: Running ser_open, %s\n", com_cfg[num].dev);
    /* don't try to remove any lock: they don't make sense for ttyname(0) */
    s_printf("Opening Virtual Port\n");
    com[num].dev_locked = FALSE;
  } else if (config.tty_lockdir[0]) {
    if (tty_lock(com_cfg[num].dev, 1) >= 0) {		/* Lock port */
      /* We know that we have access to the serial port */
      com[num].dev_locked = TRUE;

      /* If the port is used for a mouse, then remove lockfile, because
       * the use of the mouse serial port can be switched between processes,
       * such as on Linux virtual consoles.
       */
      if (com_cfg[num].mouse)
        if (tty_lock(com_cfg[num].dev, 0) >= 0)   		/* Unlock port */
          com[num].dev_locked = FALSE;
    } else {
      /* The port is in use by another process!  Don't touch the port! */
      com[num].dev_locked = FALSE;
      com[num].fd = -2;
      return(-1);
    }
  } else {
    s_printf("Warning: Port locking disabled in the config.\n");
    com[num].dev_locked = FALSE;
  }

  if (!com_cfg[num].dev || !com_cfg[num].dev[0]) {
    s_printf("SER%d: Device file not yet defined!\n",num);
    return (-1);
  }

  com[num].fd = RPT_SYSCALL(open(com_cfg[num].dev, O_RDWR | O_NONBLOCK));
  if (com[num].fd < 0) {
    error("SERIAL: Unable to open device %s: %s\n",
      com_cfg[num].dev, strerror(errno));
    goto fail_unlock;
  }
  if (!isatty(com[num].fd)) {
    error("SERIAL: Serial port device %s is not a tty, closing\n",
      com_cfg[num].dev);
    goto fail_close;
  }
  RPT_SYSCALL(tcgetattr(com[num].fd, &com[num].oldset));

  if (com_cfg[num].low_latency) {
    struct serial_struct ser_info;
    int err = ioctl(com[num].fd, TIOCGSERIAL, &ser_info);
    if (err) {
      error("SER%d: failure getting serial port settings, %s\n",
          num, strerror(errno));
    } else {
      ser_info.flags |= ASYNC_LOW_LATENCY;
      err = ioctl(com[num].fd, TIOCSSERIAL, &ser_info);
      if (err)
        error("SER%d: failure setting low_latency flag, %s\n",
            num, strerror(errno));
      else
        s_printf("SER%d: low_latency flag set\n", num);
    }
  }

  add_to_io_select(com[num].fd, async_serial_run, (void *)(long)num);
  return com[num].fd;

fail_close:
  close(com[num].fd);
  /* fall through */
fail_unlock:
  if (com[num].dev_locked && tty_lock(com_cfg[num].dev, 0) >= 0)   		/* Unlock port */
    com[num].dev_locked = FALSE;

  com[num].fd = -2; // disable permanently
  return -1;
}

void ser_set_params(int num)
{
  int data = 0;
  /* The following adjust raw line settings needed for DOSEMU serial     */
  /* These defines are based on the Minicom 1.70 communications terminal */
#if 1
  com[num].newset.c_cflag |= (CLOCAL | CREAD);
  com[num].newset.c_cflag &= ~(HUPCL | CRTSCTS);
  com[num].newset.c_iflag |= (IGNBRK | IGNPAR);
  com[num].newset.c_iflag &= ~(BRKINT | PARMRK | INPCK | ISTRIP |
                               INLCR | IGNCR | INLCR | ICRNL | IXON |
                               IXOFF | IUCLC | IXANY | IMAXBEL);
  com[num].newset.c_oflag &= ~(OPOST | OLCUC | ONLCR | OCRNL | ONOCR |
                               ONLRET | OFILL | OFDEL);
  com[num].newset.c_lflag &= ~(XCASE | ISIG | ICANON | IEXTEN | ECHO |
                               ECHONL | ECHOE | ECHOK | ECHOPRT | ECHOCTL |
                               ECHOKE | NOFLSH | TOSTOP);
#else
  /* These values should only be used as a last resort, or for testing */
  com[num].newset.c_iflag = IGNBRK | IGNPAR;
  com[num].newset.c_lflag = 0;
  com[num].newset.c_oflag = 0;
  com[num].newset.c_cflag |= CLOCAL | CREAD;
  com[num].newset.c_cflag &= ~(HUPCL | CRTSCTS);
#endif

#ifdef __linux__
  com[num].newset.c_line = 0;
#endif
  com[num].newset.c_cc[VMIN] = 1;
  com[num].newset.c_cc[VTIME] = 0;
  if (com_cfg[num].system_rtscts)
    com[num].newset.c_cflag |= CRTSCTS;
  tcsetattr(com[num].fd, TCSANOW, &com[num].newset);

  com[num].dll = 0x30;			/* Baudrate divisor LSB: 2400bps */
  com[num].dlm = 0;			/* Baudrate divisor MSB: 2400bps */
  com[num].IER = 0;			/* Interrupt Enable Register */
  com[num].IIR.mask = 0;		/* Interrupt I.D. Register */
  com[num].LCR = UART_LCR_WLEN8;	/* Line Control Register: 5N1 */
  com[num].DLAB = 0;			/* DLAB for baudrate change */
  com[num].FCReg = 0; 			/* FIFO Control Register */
  com[num].rx_fifo_trigger = 1;		/* Receive FIFO trigger level */
  com[num].MCR = 0;			/* Modem Control Register */
  com[num].LSR = UART_LSR_TEMT | UART_LSR_THRE;   /* Txmit Hold Reg Empty */
  com[num].MSR = 0;			/* Modem Status Register */
  com[num].SCR = 0; 			/* Scratch Register */
  com[num].int_condition = TX_INTR;	/* FLAG: Pending xmit intr */
  com[num].IIR.flags = 0;		/* FLAG: FIFO disabled */
  com[num].ms_timer = 0;		/* Modem Status check timer */
  com[num].rx_timer = 0;		/* Receive read() polling timer */
  com[num].rx_timeout = 0;		/* FLAG: No Receive timeout */
  com[num].rx_fifo_size = 16;		/* Size of receive FIFO to emulate */
  com[num].tx_cnt = 0;
  uart_clear_fifo(num,UART_FCR_CLEAR_CMD);	/* Initialize FIFOs */
  fossil_setup(num);

  if(s2_printf) s_printf("SER%d: do_ser_init: running ser_termios\n",num);
  ser_termios(num);			/* Set line settings now */
  modstat_engine(num);

  /* Pull down DTR and RTS.  This is the most natural for most comm */
  /* devices including mice so that DTR rises during mouse init.    */
  if (!com_cfg[num].pseudo) {
    data = TIOCM_DTR | TIOCM_RTS;
    if (ioctl(com[num].fd, TIOCMBIC, &data) && errno == EINVAL) {
      s_printf("SER%d: TIOCMBIC unsupported, setting pseudo flag\n", num);
      com_cfg[num].pseudo = 1;
    }
  }
}


/* This function closes ONE serial port for DOSEMU.  Normally called
 * only by do_ser_init below.   [num = port, return = file error code]
 */
static int ser_close(int num)
{
  int i;
  s_printf("SER%d: Running ser_close\n",num);
  remove_from_io_select(com[num].fd);
  uart_clear_fifo(num,UART_FCR_CLEAR_CMD);

  /* save current dosemu settings of the file and restore the old settings
   * before closing the file down.
   */
  (void)RPT_SYSCALL(tcgetattr(com[num].fd, &com[num].newset));
  (void)RPT_SYSCALL(tcsetattr(com[num].fd, TCSADRAIN, &com[num].oldset));
  i = RPT_SYSCALL(close(com[num].fd));
  com[num].fd = -1;

  /* Clear the lockfile from DOSEMU */
  if (com[num].dev_locked) {
    if (tty_lock(com_cfg[num].dev, 0) >= 0)
      com[num].dev_locked = FALSE;
  }
  return (i);
}


static Bit8u com_readb(ioport_t port) {
  int tmp;
  for (tmp = 0; tmp < config.num_ser; tmp++) {
    if (((u_short)(port & ~7)) == com_cfg[tmp].base_port) {
      return(do_serial_in(tmp, (int)port));
    }
  }
  return 0;
}

static void com_writeb(ioport_t port, Bit8u value) {
  int tmp;
  for (tmp = 0; tmp < config.num_ser; tmp++) {
    if (((u_short)(port & ~7)) == com_cfg[tmp].base_port) {
      do_serial_out(tmp, (int)port, (int)value);
    }
  }
}

/* The following function is the main initialization routine that
 * initializes the UART for ONE serial port.  This includes setting up
 * the environment, define default variables, the emulated UART's init
 * stat, and open/initialize the serial line.   [num = port]
 */
static void do_ser_init(int num)
{
  emu_iodev_t io_device;

  /* The following section sets up default com port, interrupt, base
  ** port address, and device path if they are undefined. The defaults are:
  **
  **   COM1:   irq = 4    base_port = 0x3F8    device = /dev/ttyS0
  **   COM2:   irq = 3    base_port = 0x2F8    device = /dev/ttyS1
  **   COM3:   irq = 4    base_port = 0x3E8    device = /dev/ttyS2
  **   COM4:   irq = 3    base_port = 0x2E8    device = /dev/ttyS3
  */

  static struct {
    int interrupt;
    ioport_t base_port;
    char *dev;
    char *handler_name;
  } default_com[MAX_SER] = {
    { 4, 0x3F8, "/dev/ttyS0", "COM1" },
    { 3, 0x2F8, "/dev/ttyS1", "COM2" },
    { 4, 0x3E8, "/dev/ttyS2", "COM3" },
    { 3, 0x2E8, "/dev/ttyS3", "COM4" },

    { 3, 0x4220, "/dev/ttyS4", "COM5" },
    { 3, 0x4228, "/dev/ttyS5", "COM6" },
    { 3, 0x5220, "/dev/ttyS6", "COM7" },
    { 3, 0x5228, "/dev/ttyS7", "COM8" },

    { 4, 0x6220, "/dev/ttyS8", "COM9" },
    { 4, 0x6228, "/dev/ttyS9", "COM10" },
    { 4, 0x7220, "/dev/ttyS10", "COM11" },
    { 4, 0x7228, "/dev/ttyS11", "COM12" },

    { 4, 0x8220, "/dev/ttyS12", "COM13" },
    { 4, 0x8228, "/dev/ttyS13", "COM14" },
    { 4, 0x9220, "/dev/ttyS14", "COM15" },
    { 4, 0x9228, "/dev/ttyS15", "COM16" },
  };

  if (com_cfg[num].real_comport == 0) {		/* Is comport number undef? */
    error("SER%d: No COMx port number given\n", num);
    config.exitearly = 1;
    return;
  }

  if (com_cfg[num].interrupt <= 0) {		/* Is interrupt undefined? */
    /* Define it depending on using standard irqs */
    com_cfg[num].interrupt = pic_irq_list[default_com[com_cfg[num].real_comport-1].interrupt];
  }

  if (com_cfg[num].base_port <= 0) {		/* Is base port undefined? */
    /* Define it depending on using standard addrs */
    com_cfg[num].base_port = default_com[com_cfg[num].real_comport-1].base_port;
  }

  if (!com_cfg[num].dev || !com_cfg[num].dev[0]) {	/* Is the device file undef? */
    /* Define it using std devs */
    com_cfg[num].dev = default_com[com_cfg[num].real_comport-1].dev;
  }
  iodev_add_device(com_cfg[num].dev);

  /* FOSSIL emulation is inactive at startup. */
  com[num].fossil_active = FALSE;

  /* convert irq number to pic_ilevel number and set up interrupt
   * if irq is invalid, no interrupt will be assigned
   */
  if(!irq_source_num[com_cfg[num].interrupt]) {
    s_printf("SER%d: enabling interrupt %d\n", num, com_cfg[num].interrupt);
    pic_seti(com_cfg[num].interrupt, pic_serial_run, 0, NULL);
  }
  irq_source_num[com_cfg[num].interrupt]++;

  /*** The following is where the real initialization begins ***/

  /* Tell the port manager that we exist and that we're alive */
  io_device.read_portb  = com_readb;
  io_device.write_portb = com_writeb;
  io_device.read_portw  = NULL;
  io_device.write_portw = NULL;
  io_device.read_portd  = NULL;
  io_device.write_portd = NULL;
  io_device.start_addr  = com_cfg[num].base_port;
  io_device.end_addr    = com_cfg[num].base_port+7;
  io_device.irq         = (irq_source_num[com_cfg[num].interrupt] == 1 ?
                           com_cfg[num].interrupt : EMU_NO_IRQ);
  io_device.fd		= -1;
  io_device.handler_name = default_com[num].handler_name;
  port_register_handler(io_device, 0);

  /* Information about serial port added to debug file */
  s_printf("SER%d: COM%d, intlevel=%d, base=0x%x, device=%s\n",
        num, com_cfg[num].real_comport, com_cfg[num].interrupt,
        com_cfg[num].base_port, com_cfg[num].dev);
#if 0
  /* first call to serial timer update func to initialize the timer */
  /* value, before the com[num] structure is initialized */
  serial_timer_update();
#endif
  /* Set file descriptor as unused, then attempt to open serial port */
  com[num].fd = -1;
}

void serial_reset(void)
{
  int num;
  /* Clean the BIOS data area at 0040:0000 for serial ports */
  WRITE_WORD(0x400, 0);
  WRITE_WORD(0x402, 0);
  WRITE_WORD(0x404, 0);
  WRITE_WORD(0x406, 0);
  /* Write serial port information into BIOS data area 0040:0000
   * This is for DOS and many programs to recognize ports automatically
   */
  for (num = 0; num < config.num_ser; num++) {
    if ((com_cfg[num].real_comport >= 1) && (com_cfg[num].real_comport <= 4)) {
      WRITE_WORD(0x400 + (com_cfg[num].real_comport-1)*2, com_cfg[num].base_port);

      /* Debugging to determine whether memory location was written properly */
      s_printf("SER%d: BIOS memory location %p has value of %#x\n", num,
	       ((u_short *) (0x400) + (com_cfg[num].real_comport-1))
	       ,READ_WORD(0x400 + 2*(com_cfg[num].real_comport-1)));
    }
  }
}

/* DANG_BEGIN_FUNCTION serial_init
 *
 * This is the master serial initialization function that is called
 * upon startup of DOSEMU to initialize ALL the emulated UARTs for
 * all configured serial ports.  The UART is initialized via the
 * initialize_uart function, which opens the serial ports and defines
 * variables for the specific UART.
 *
 * If the port is a mouse, the port is only initialized when i
 *
 * DANG_END_FUNCTION
 */
void serial_init(void)
{
  int i;
  warn("SERIAL $Id$\n");
  s_printf("SER: Running serial_init, %d serial ports\n", config.num_ser);

  /* Do UART init here - Need to set up registers and init the lines. */
  for (i = 0; i < config.num_ser; i++) {
    com[i].fd = -1;
    com[i].dev_locked = FALSE;

    /* Serial port init is skipped if the port is used for a mouse, and
     * dosemu is running in Xwindows, or not at the console.  This is due
     * to the fact the mouse is in use by Xwindows (internal driver is used)
     * Direct access to the mouse by dosemu is useful mainly at the console.
     */
    if (com_cfg[i].mouse && !on_console())
      s_printf("SER%d: Not touching mouse outside of the console!\n",i);
#ifdef USE_GPM
    else if (com_cfg[i].mouse && strcmp(Mouse->name, "gpm") == 0)
      s_printf("SER%d: GPM competing with direct access is racy: "
               "only using GPM\n",i);
#endif
    else
      do_ser_init(i);
  }
}

/* Like serial_init, this is the master function that is called externally,
 * but at the end, when the user quits DOSEMU.  It deinitializes all the
 * configured serial ports.
 */
void serial_close(void)
{
  int i;
  s_printf("SER: Running serial_close\n");
  for (i = 0; i < config.num_ser; i++) {
    if (com[i].fd < 0)
      continue;
    if (com_cfg[i].mouse && !on_console())
      s_printf("SER%d: Not touching mouse outside of the console!\n",i);
#ifdef USE_GPM
    else if (com_cfg[i].mouse && strcmp(Mouse->name, "gpm") == 0)
      s_printf("SER%d: GPM competing with direct access is racy: "
               "only using GPM\n",i);
#endif
    else {
      (void)RPT_SYSCALL(tcsetattr(com[i].fd, TCSADRAIN, &com[i].oldset));
      ser_close(i);
    }
  }
}

/* The following de-initializes the mouse on the serial port that the mouse
 * has been enabled on.  For mouse sharing purposes, this is the function
 * that is called when the user switches out of the VC running DOSEMU.
 * (Also, this silly function name needs to be changed soon.)
 */
void child_close_mouse(void)
{
  u_char i, rtrn;
  if (on_console()) {
    s_printf("MOUSE: CLOSE function starting. num_ser=%d\n", config.num_ser);
    for (i = 0; i < config.num_ser; i++) {
      s_printf("MOUSE: CLOSE port=%d, dev=%s, fd=%d, valid=%d\n",
                i, com_cfg[i].dev, com[i].fd, com_cfg[i].mouse);
      if ((com_cfg[i].mouse == TRUE) && (com[i].fd > 0)) {
        s_printf("MOUSE: CLOSE port=%d: Running ser_close.\n", i);
        rtrn = ser_close(i);
        if (rtrn) s_printf("MOUSE SERIAL ERROR - %s\n", strerror(errno));
      }
      else {
        s_printf("MOUSE: CLOSE port=%d: Not running ser_close.\n", i);
      }
    }
    s_printf("MOUSE: CLOSE function ended.\n");
  }
}

/* The following initializes the mouse on the serial port that the mouse
 * has been enabled on.  For mouse sharing purposes, this is the function
 * that is called when the user switches back into the VC running DOSEMU.
 * (Also, this silly function name needs to be changed soon.)
 */
void child_open_mouse(void)
{
  u_char i;
  if (on_console()) {
    s_printf("MOUSE: OPEN function starting.\n");
    for (i = 0; i < config.num_ser; i++) {
      s_printf("MOUSE: OPEN port=%d, type=%d, dev=%s, valid=%d\n",
                i, config.mouse.type, com_cfg[i].dev, com_cfg[i].mouse);
      if (com_cfg[i].mouse == TRUE) {
        s_printf("MOUSE: OPEN port=%d: Running ser-open.\n", i);
        com[i].fd = -1;
        ser_open(i);
        tcgetattr(com[i].fd, &com[i].newset);
      }
    }
  }
}
