/*
 * written March/April 2000 (original code)
 * Bart Oldeman <bart.oldeman@bristol.ac.uk>
 * with help from Matan Ziv-Av <matan@svgalib.org>
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

/* Interface for svgalib.
 *
 * Notes:
 *  - This should work with any card directly supported by svgalib,
 *    NOT its vesa option.
 *  - Be aware that DOS programs which change extended VGA registers
 *    while in the background are not in a proper state after a
 *    return to the DOS running VC.
 *  - Please don't use svgalib's ati driver, as it takes to many
 *    I/O permissions, which may result in a changing display even
 *    when on another VC than dosemu.
 *
 */

#include <vga.h>

#include "emu.h"
#include "svgalib.h"
#include "priv.h"
#include "vc.h"
#include "vga.h"

/*
 * svgalibSave --
 *      save the current video mode
 */
/* Read and save chipset-specific registers */

void svgalib_save_ext_regs(u_char xregs[], u_short xregs16[])
{
    vga_chipset_saveregs(xregs);
}

/*
 * svgalibRestore --
 *      restore a video mode
 */
void svgalib_restore_ext_regs(u_char xregs[], u_short xregs16[])
{
    vga_chipset_setregs(xregs);
}

void svgalib_setbank(unsigned char bank)
{
    vga_setpage(bank);
}

static int svgalib_init(void)
{
       int r, chipset;

       PRIV_SAVE_AREA
       vga_norevokeprivs();
       vga_disablechipset(VESA);
       enter_priv_on();
       r=vga_simple_init();
       leave_priv_setting();
       chipset = vga_getcurrentchipset();
       v_printf("SVGALIB: selected chipset %i\n", chipset);
       return r;
}

void vga_init_svgalib(void)
{
       if (!svgalib_init()) {
               save_ext_regs = svgalib_save_ext_regs;
               restore_ext_regs = svgalib_restore_ext_regs;
               set_bank_read = svgalib_setbank;
               set_bank_write = svgalib_setbank;
       }
}
