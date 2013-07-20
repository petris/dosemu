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
#ifndef DOSEMU_CONFIG_H
#define DOSEMU_CONFIG_H

extern void config_init(int argc, char **argv);
extern void parse_dosemu_users(void);
extern void secure_option_preparse(int *argc, char **argv);
extern void keyb_layout(int value);
extern int cpu_override (int cpu);

typedef void (*config_scrub_t)(void);
int register_config_scrub(config_scrub_t config_scrub);
void unregister_config_scrub(config_scrub_t old_config_scrub);
int define_config_variable(char *name);
char *get_config_variable(char *name);
char *checked_getenv(const char *name);
extern char dosemu_conf[];
extern char global_conf[];
extern char *dosemu_proc_self_exe;
extern int dosemu_argc;
extern char **dosemu_argv;
extern char *commandline_statements;
extern int config_check_only;
extern int kernel_version_code;
extern int dexe_running;

#endif
