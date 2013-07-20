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

typedef struct {
	int x, y, width, height;
	boolean drawn;
	union {
		unsigned short text[2];
		unsigned char graphics[256];
	} backingstore;
} mouse_erase_t;

/* this entire structure gets saved on a driver state save */
void erase_graphics_cursor(mouse_erase_t*);
void draw_graphics_cursor(int,int,int,int,int,int,mouse_erase_t*);
void define_graphics_cursor(short*,short*);
