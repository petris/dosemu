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
 * NOTE:  don't include this file in DOSEMU code, except for v-net/dosnet.c
 *        (which is a kernel module) and other real kernel code (modules).
 *        User space DOSEMU should compile kernel independently.
 *
 * NOTE2: _If_ you include it, make sure you have a valid symlink
 *        from /usr/include/linux to the real kernel source.
 *        With newer glibc this may be a problem, as the lib headers
 *        may contain only stripped down copies of the kernel headers.
 *        BE WARNED !
 */

#ifndef LX_KERNEL_VERSION
#include <linux/version.h>
#define LX_KERNEL_VERSION  (((LINUX_VERSION_CODE >> 16)*1000000) + \
			(((LINUX_VERSION_CODE >> 8) & 255) *1000) + \
			(LINUX_VERSION_CODE & 255) )
#if (LX_KERNEL_VERSION < 2000028) || ((LX_KERNEL_VERSION >= 2001000) && (LX_KERNEL_VERSION < 2001015))
#error "wrong kernel version: < 2.0.28 or 2.1.x < 2.1.15"
#endif
#endif
