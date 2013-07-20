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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "config.h"
#include "emu.h"
#include "memory.h"
#include "doshelpers.h"
#include "builtins.h"
#include "msetenv.h"

#include "cmdline.h"

#define printf	com_printf
#define perror	com_printf
#define read	com_dosread

#define CMDBUFFSIZE 256
/* This program just reads stdin... */
int cmdline_main(int argc, char **argv)
{
	char *buff = malloc(CMDBUFFSIZE), *p, *q, *endb;


	if (!buff) {
	    perror("malloc failure");
	    return(1);
	}
	*(endb = buff + read(0,buff,CMDBUFFSIZE)) = '\0';
	for (p = buff; p < endb; p = q + strlen(q)+1)
		 if (*p != '-' && ((q = strchr(p,'='))!=0)) {
		     *q++ = '\0';
		     if (msetenv(p,q)) {
		         free(buff);
			 return (1);
		     }
		 }
		 else q = p;
	free(buff);
	return 0;
}
