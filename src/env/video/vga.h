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
 * video/vga.h - prototypes for VGA-card specific functions
 */

#ifndef DOSEMU_VGA_H
#define DOSEMU_VGA_H

EXTERN void (*save_ext_regs) (u_char xregs[], u_short xregs16[]);
EXTERN void (*restore_ext_regs) (u_char xregs[], u_short xregs16[]);
EXTERN void (*set_bank_read) (unsigned char bank);
EXTERN void (*set_bank_write) (unsigned char bank);
EXTERN void (*ext_video_port_out) (ioport_t port, unsigned char value);

EXTERN u_char(*ext_video_port_in) (ioport_t port);

void save_vga_state(struct video_save_struct *save_regs);
void restore_vga_state(struct video_save_struct *save_regs);

#endif
/* End of video/vga.h */
