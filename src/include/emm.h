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

#ifndef __EMS_H
#define __EMS_H

/* increase this when ems.S is changed */
#define DOSEMU_EMS_DRIVER_VERSION 5

#define EMS_ERROR_DISABLED_IN_CONFIG 1
#define EMS_ERROR_VERSION_MISMATCH 2

#define	MAX_HANDLES	255	/* must fit in a byte */
/* this is in EMS pages, which MAX_EMS (defined in Makefile) is in K */
#define MAX_EMM		(config.ems_size >> 4)
#ifndef PAGE_SIZE
#define PAGE_SIZE	4096
#endif
#define	EMM_PAGE_SIZE	(16*1024)
#define EMM_UMA_MAX_PHYS 12
#define EMM_UMA_STD_PHYS 4
#define EMM_CNV_MAX_PHYS 24
#define EMM_MAX_PHYS	(EMM_UMA_MAX_PHYS + EMM_CNV_MAX_PHYS)
#define EMM_MAX_SAVED_PHYS EMM_UMA_STD_PHYS
#define NULL_HANDLE	0xffff
#define	NULL_PAGE	0xffff
#define PAGE_MAP_SIZE(np)	(sizeof(u_short) * 2 * (np))

#ifndef __ASSEMBLER__
/* export a few EMS functions to DPMI so it doesn't have to call interrupt */
int emm_get_partial_map_registers(void *ptr, const u_short *segs);
void emm_set_partial_map_registers(const void *ptr);
int emm_map_unmap_multi(const u_short *array, int handle, int map_len);
int emm_get_size_for_partial_page_map(int pages);
/* end of DPMI/EMS API */

void ems_init(void);
void ems_reset(void);
#endif

#endif
