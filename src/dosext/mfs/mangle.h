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

/* header file to make mangle.c fit with dosemu

Andrew Tridgell
March 1995

Modified by O.V.Zhirov, July 1998
*/

#if defined(__linux__)
#define DOSEMU 1		/* this is a port to dosemu */
#endif


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>


/* no debugging - the code is perfect! */
#define DEBUG(level,message)
#define PTR_DIFF(p1,p2) ((ptrdiff_t)(((char *)(p1)) - (char *)(p2)))

#define strnorm(s) strlowerDOS(s)
#define strisnormal(s) (!strhasupperDOS(s))

#define safe_memcpy(x,y,s) memmove(x,y,s)

#define lp_strip_dot() 1

#define BOOL int
#ifndef True
#define True 1
#define False 0
#endif


#define CASE_LOWER 0
#define CASE_UPPER 1


typedef char fstring[100];
typedef char pstring[1024];


/* prototypes */
extern unsigned int is_dos_device(const char *fname);
extern void mangle_name_83(char *s, char *MangledMap);
extern BOOL do_fwd_mangled_map(char *s, char *MangledMap);
extern BOOL name_convert(char *Name,BOOL mangle);
extern BOOL is_mangled(const char *s);
extern BOOL check_mangled_stack(char *s, char *MangledMap);

/* prototypes, found in util.c */
extern unsigned char unicode_to_dos_table[0x10000];

BOOL isupperDOS(int c);
BOOL islowerDOS(int c);
BOOL strhasupperDOS(char *s);
BOOL strhaslowerDOS(char *s);
BOOL isalphaDOS(int c);
BOOL isalnumDOS(int c);
BOOL is_valid_DOS_char(int c);
int chrcmpDOS(int c1, int c2);
int strncmpDOS(char *s1, char *s2,int n);
int strcmpDOS(char *s1, char *s2);
int strncasecmpDOS(char *s1, char *s2,int n);
int strcasecmpDOS(char *s1, char *s2);

char *StrnCpy(char *dest,const char *src,int n);
void array_promote(char *array,int elsize,int element);


extern BOOL valid_dos_char[256];

#define VALID_DOS_PCHAR(p) (valid_dos_char[*(unsigned char *)(p)])


#ifndef MANGLE
#define MANGLE 1
#endif

#ifndef MANGLED_STACK
#define MANGLED_STACK 150
#endif

#ifndef CODEPAGE
#define CODEPAGE 866
#endif


