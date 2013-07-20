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

/* definitions for updating text modes */

#include "translate.h"
#define CONFIG_SELECTION 1

/********************************************/

#define ATTR_FG(attrib) (attrib & 0x0F & vga.attr.data[0x12])
#define ATTR_BG(attrib) (attrib >> 4)

extern Boolean have_focus;
extern int use_bitmap_font;

struct text_system
{
   /* function to draw a string in text mode using attribute attr */
   void (*Draw_string)(int x, int y , unsigned char *s, int len, Bit8u attr);
   void (*Draw_line)(int x, int y , int len);
   void (*Draw_cursor)(int x, int y, Bit8u attr, int first, int last, Boolean focus);
   void (*SetPalette) (DAC_entry color);
};

struct RemapObjectStruct;
struct RectArea;

int register_text_system(struct text_system *text_system);
struct RectArea draw_bitmap_cursor(int x, int y, Bit8u attr, int start, int end, Boolean focus);
struct RectArea draw_bitmap_line(int x, int y, int len);
void blink_cursor(void);
void reset_redraw_text_screen(void);
void update_cursor(void);
void resize_text_mapper(int image_mode);
struct RectArea convert_bitmap_string(int x, int y, unsigned char *text,
				      int len, Bit8u attr);
int update_text_screen(void);
void redraw_text_screen(void);
void text_gain_focus(void);
void text_lose_focus(void);

#ifdef CONFIG_SELECTION
/* for selections */
t_unicode* end_selection(void);
void clear_if_in_selection(void);
void start_selection(int col, int row);
void start_extend_selection(int col, int row);
void clear_selection_data(void);
void extend_selection(int col, int row);

int x_to_col(int x, int w_x_res);
int y_to_row(int y, int w_y_res);

#endif
