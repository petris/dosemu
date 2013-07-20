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

#ifndef COOPTH_H
#define COOPTH_H

#define COOPTH_TID_INVALID (-1)

typedef void (*coopth_func_t)(void *arg);
typedef void (*coopth_hndl_t)(int tid);

void coopth_init(void);
int coopth_create(char *name);
int coopth_create_multi(char *name, int len);
int coopth_start(int tid, coopth_func_t func, void *arg);
int coopth_set_permanent_post_handler(int tid, coopth_hndl_t func);
int coopth_set_ctx_handlers(int tid, coopth_hndl_t pre, coopth_hndl_t post);
int coopth_set_sleep_handlers(int tid, coopth_hndl_t pre, coopth_hndl_t post);
int coopth_set_post_handler(coopth_func_t func, void *arg);
void coopth_join(int tid, void (*helper)(void));
int coopth_flush(void (*helper)(void));
int coopth_set_detached(int tid);
int coopth_init_sleeping(int tid);
int coopth_set_sleep_handler(coopth_func_t func, void *arg);
int coopth_set_cleanup_handler(coopth_func_t func, void *arg);
void coopth_push_user_data(int tid, void *udata);
void coopth_push_user_data_cur(void *udata);
void *coopth_pop_user_data(int tid);
void *coopth_pop_user_data_cur(void);
int coopth_get_tid(void);
int coopth_get_scheduled(void);
void coopth_ensure_attached(void);
void coopth_yield(void);
void coopth_wait(void);
void coopth_sleep(void);
void coopth_detach(void);
void coopth_attach(void);
void coopth_leave(void);
void coopth_exit(void);
void coopth_wake_up(int tid);
void coopth_asleep(int tid);
void coopth_cancel(int tid);
void coopth_done(void);
void coopth_run(void);

#endif
