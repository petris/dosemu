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

#ifndef _EVENTS_H
#define _EVENTS_H

/* Interface to device driver */

void do_noteoff(int chn);
void do_noteon(int chn);
void do_notepressure(int chn);
void do_program(int chn);
void do_channelpressure(int chn);
void do_bender(int chn);
void do_allnotesoff(void);
void do_modemessage(int chn,int control);
void do_controlchange(int chn);

void do_sysex(void);
void do_quarter_frame(void);
void do_song_position(void);
void do_song_select(void);
void do_tune_request(void);
void do_midi_clock(void);
void do_midi_tick(void);
void do_midi_start(void);
void do_midi_continue(void);
void do_midi_stop(void);
void do_active_sensing(void);
void do_reset(void);

#endif
