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

#ifndef DMEMORY_H
#define DMEMORY_H

typedef struct dpmi_pm_block_stuct {
  struct   dpmi_pm_block_stuct *next;
  unsigned int handle;
  unsigned int size;
  unsigned int base;
  u_short  *attrs;
  int linear;
} dpmi_pm_block;

typedef struct dpmi_pm_block_root_struc {
  dpmi_pm_block *first_pm_block;
} dpmi_pm_block_root;

dpmi_pm_block *lookup_pm_block(dpmi_pm_block_root *root, unsigned long h);
void dpmi_alloc_pool(void);
void dpmi_free_pool(void);
dpmi_pm_block *DPMI_malloc(dpmi_pm_block_root *root, unsigned int size);
dpmi_pm_block *DPMI_mallocLinear(dpmi_pm_block_root *root, unsigned int base, unsigned int size, int committed);
int DPMI_free(dpmi_pm_block_root *root, unsigned int handle);
dpmi_pm_block *DPMI_realloc(dpmi_pm_block_root *root, unsigned int handle, unsigned int newsize);
dpmi_pm_block *DPMI_reallocLinear(dpmi_pm_block_root *root, unsigned long handle, unsigned long newsize, int committed);
void DPMI_freeAll(dpmi_pm_block_root *root);
int DPMI_MapConventionalMemory(dpmi_pm_block_root *root, unsigned long handle,
  unsigned long offset, unsigned long low_addr, unsigned long cnt);
int DPMI_SetPageAttributes(dpmi_pm_block_root *root, unsigned long handle, int offs, us attrs[], int count);
int DPMI_GetPageAttributes(dpmi_pm_block_root *root, unsigned long handle, int offs, us attrs[], int count);

#define PAGE_MASK	(~(PAGE_SIZE-1))
/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr)	(((addr)+PAGE_SIZE-1)&PAGE_MASK)

#endif
