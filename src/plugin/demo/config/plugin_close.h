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
 * This is file plugin_close.h for use within the src/plugin/<name>/
 *
 * It should contain a valid call to the init function of the plug-in such as
 *
 *       my_plugin_close();
 *
 * This routine should do _nothing_, if its counterpart my_plugin_init()
 * did decide to disable the plugin.
 * Don't forget the curly brackets around your statement.
 */

demo_plugin_close();
