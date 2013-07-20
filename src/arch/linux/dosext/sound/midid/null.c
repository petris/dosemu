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

/***********************************************************************
  null (flush) device -- mainly for testing
 ***********************************************************************/

#include "device.h"
#include <stdlib.h>

static bool null_detect(void)
{
	return(TRUE);
}

static bool null_init(void)
{
	return(TRUE);
}

static void null_doall(void)
{
}

static void null_doall2(int a,int b)
{
}

static void null_doall3(int a,int b,int c)
{
}

static bool null_setmode(Emumode new_mode)
{
  /* Our NULL driver has all emulations in the world :) */
  return TRUE;
}

void register_null(Device * dev)
{
	dev->name = "null";
	dev->version = 100;
	dev->detect = null_detect;
	dev->init = null_init;
	dev->done = null_doall;
	dev->pause = NULL;
	dev->resume = NULL;
	dev->flush = null_doall;
	dev->noteon = null_doall3;
	dev->noteoff = null_doall3;
	dev->control = null_doall3;
	dev->notepressure = null_doall3;
	dev->channelpressure = null_doall2;
	dev->bender = null_doall2;
	dev->program = null_doall2;
	dev->setmode = null_setmode;
}
