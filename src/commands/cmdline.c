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

/* a port of CMDLINE.AWK */
#if !defined( __LARGE__) && !defined(__COMPACT__)
#error "Use the LARGE or COMPACT model!"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <dos.h>
#include "detect.h"

int msetenv(char *,char *);

#define CMDBUFFSIZE 10000
/* This program just reads stdin... */
main()
{
	char *buff = malloc(CMDBUFFSIZE), *p, *q, *endb;

  if (!is_dosemu()) {
    printf("This program requires DOSEMU to run, aborting\n");
    exit(1);
  }

	if (!buff)
	    perror("malloc failure"),
	    exit(1);
	*(endb = buff + read(0,buff,CMDBUFFSIZE)) = '\0';
	for (p = buff; p < endb; p = q + strlen(q)+1)
		 if (*p != '-' && ((q = strchr(p,'='))!=0)) {
		     *q++ = '\0';
		     if (msetenv(p,q))
			 exit(1);
		     }
		 else q = p;
}
