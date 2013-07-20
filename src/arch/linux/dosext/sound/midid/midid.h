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

#ifndef _MIDID_H
#define _MIDID_H

typedef unsigned char byte;
typedef enum {FALSE, TRUE} bool;
typedef enum {EMUMODE_MT32, EMUMODE_GM} Emumode;

/* Configuration */
typedef struct Config {
  Emumode mode;		/* Default emulation mode */
  bool resident;	/* TRUE if resident */
  int timeout;          /* timeout to turn off output after */
  int verbosity;        /* bitmask with outputoptions */
  int card;             /* GUS card */
  bool opl3;            /* TRUE if OPL3; FALSE if OPL2 */
  char *inputfile;      /* "" == stdin */
  char *midifile;       /* "" == midid.mid */
  int tempo;		/* "" == 120 beats per minute */
  int ticks_per_quarter_note;	/* "" == 144 */
  char *timid_host;	/* timidity server host name */
  int timid_port;	/* timidity server control port */
  char *timid_bin;	/* timidity binary name */
  char *timid_args;	/* timidity command-line args */
  int timid_mono;
  int timid_8bit;
  int timid_uns;
  int timid_freq;
  int timid_capture;
} Config;

extern Config config;

/* Verbosity level */
extern int debugall; 	/* 1 for all read bytes */
extern int debug;	/* 1 for interpretation of each byte */
extern int ignored;  	/* 1 for ignored but recognised messages */
extern int comments; 	/* 1 for status report */
extern int warning;	/* 1 for warnings during interpretation*/
extern int statistics;	/* 1 for statistics */

void error(char *err);

#endif
