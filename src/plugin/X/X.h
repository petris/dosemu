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

#include <X11/X.h>
#include <X11/Xlib.h>

extern Display *display;
extern struct keyboard_client Keyboard_X;

void           get_vga_colors (void);
void           X_handler      (void);
void X_draw_cursor(int x,int y);
void X_restore_cell(int x,int y);
void X_init_videomode(void);

void X_process_key(XKeyEvent *);
void X_process_keys(XKeymapEvent *);

void load_text_font(void);
void X_load_text_font(Display *dpy, int private_dpy,
		      Window, const char *p, int *w, int *h);
int X_handle_text_expose(void);
