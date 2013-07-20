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

#ifdef USE_RELAYTOOL
extern int SLang_Error;
extern int SLtt_Use_Blink_For_ACS;
extern int libslang_symbol_is_present(char *);
#endif

extern const char *DOSemu_Keyboard_Keymap_Prompt;
extern int DOSemu_Terminal_Scroll;
extern int DOSemu_Slang_Show_Help;
extern struct mouse_client Mouse_xterm;
int term_init(void);
void term_close(void);
int using_xterm(void);
void xtermmouse_get_event (Bit8u **kbp, int *kb_count);
