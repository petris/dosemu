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

#ifndef UTILITIES_H
#define UTILITIES_H

#include "dosemu_debug.h"

struct cmd_db {
	char cmdname[12];
	void (*cmdproc)(int, char *[]);
};

extern char *logptr, *logbuf;
extern int logbuf_size, logfile_limit;

int argparse(char *s, char *argvx[], int maxarg);
typedef void cmdprintf_func(const char *fmt, ...);
void call_cmd(const char *cmd, int maxargs, const struct cmd_db *cmdtab,
	 cmdprintf_func *printf);
void sigalarm_onoff(int on);
void sigalarm_block(int block);

char *strprintable(char *s);
char *chrprintable(char c);
void open_proc_scan(char *name);
void close_proc_scan(void);
char *get_proc_string_by_key(char *key);
void advance_proc_bufferptr(void);
void reset_proc_bufferptr(void);
int get_proc_intvalue_by_key(char *key);
int integer_sqrt(int x);
int exists_dir(char *name);
int exists_file(char *name);
void subst_file_ext(char *ptr);
char *strcatdup(char *s1, char *s2);
char *assemble_path(char *dir, char *file, int append_pid);
char *mkdir_under(char *basedir, char *dir, int append_pid);
char *get_path_in_HOME(char *path);
char *get_dosemu_local_home(void);
char *readlink_malloc (const char *filename);
void dosemu_error(char *fmt, ...) FORMAT(printf, 1, 2);
void *load_plugin(const char *plugin_name);

/* returns y = sqrt(x), for y*y beeing a power of 2 below x
 */
static __inline__ int power_of_2_sqrt(int val)
{
	register int res;
	__asm__ __volatile__(" \
		bsrl	%2,%0\n \
	" : "=r" (res) : "0" ((int)-1), "r" (val) );
	if (res <0) return 0;
	return 1 << (res >> 1);
}

/*
 * from the Linux kernel:
 * min()/max() macros that also do
 * strict type-checking.. See the
 * "unnecessary" pointer comparison.
 */
#define min(x,y) ({ \
	typeof(x) _x = (x);	\
	typeof(y) _y = (y);	\
	(void) (&_x == &_y);		\
	_x < _y ? _x : _y; })

#define max(x,y) ({ \
	typeof(x) _x = (x);	\
	typeof(y) _y = (y);	\
	(void) (&_x == &_y);		\
	_x > _y ? _x : _y; })

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define ALIGN(x,a) (((x)+(a)-1)&~((a)-1))

#endif /* UTILITIES_H */
