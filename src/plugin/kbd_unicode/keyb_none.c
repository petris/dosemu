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

/* This file contains the dummy keyboard client.
 * It does nothing.
 */

#include "keyboard.h"
#include "keyb_clients.h"
#include "utilities.h"

/* DANG_BEGIN_FUNCTION none_probe
 *
 * Succeed if we can run the dummy keyboard client, (we always can).
 * but first try the other fall-back (slang keyboard)
 *
 * DANG_END_FUNCTION
 */

static int none_probe(void)
{
	return !load_plugin("term");
}

struct keyboard_client Keyboard_none =
{
	"No keyboard",	/* name */
	none_probe,	/* probe */
	NULL,		/* init */
	NULL,		/* reset */
	NULL,		/* close */
	NULL,		/* run */
	NULL,		/* set_leds */
};

