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

/* definitions for rendering graphics modes -- the middle layer
   between the graphics frontends and the remapper */

struct render_system
{
  /* set the private palette */
  void (*refresh_private_palette)(DAC_entry *col, int num);
  void (*put_image)(int x, int y, unsigned width, unsigned height);
};

extern struct RemapObjectStruct remap_obj;
extern int remap_features;

int register_render_system(struct render_system *render_system);

int remapper_init(unsigned *image_mode, unsigned bits_per_pixel,
		  int have_true_color, int have_shmap);
void remapper_done(void);
void get_mode_parameters(int *wx_res, int *wy_res, int ximage_mode,
			 vga_emu_update_type *veut);
int update_screen(vga_emu_update_type *veut);
Boolean refresh_palette(DAC_entry *col);
