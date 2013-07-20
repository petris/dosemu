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
 * This is file plugin_inte6.h for use within the src/plugin/<name>/
 *
 * It should contain a valid call to a DOSEMU_HELPER service function of
 * the plug-in such as
 *
 *    case (DOS_HELPER_PLUGIN+MYFUNCTION_OFFSET):
 *       if ( ! my_plugin_inte6()) return 0;
 *       break;
 *
 */

case DOS_HELPER_COMMANDS:
	if ( ! commands_plugin_inte6() ) return 0;
	break;
case DOS_HELPER_COMMANDS_DONE:
	if ( ! commands_plugin_inte6_done() ) return 0;
	break;
