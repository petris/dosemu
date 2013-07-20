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

#ifndef __SMALLOC_H
#define __SMALLOC_H

#define SM_COMMIT_SUPPORT 1

struct memnode {
  struct memnode *next;
  size_t size;
  int used;
  unsigned char *mem_area;
};

typedef struct mempool {
  struct memnode mn;
#if SM_COMMIT_SUPPORT
  int (*commit)(void *area, size_t size);
  int (*uncommit)(void *area, size_t size);
#endif
} smpool;

#define SM_EMPTY_NODE { \
  NULL,		 /* *next */ \
  0, 		 /* size */ \
  0,		 /* used */ \
  NULL,		 /* *mem_area */ \
}

#if SM_COMMIT_SUPPORT
#define SM_EMPTY_POOL { \
  SM_EMPTY_NODE, /* mn */ \
  NULL,		 /* commit */ \
  NULL,		 /* uncommit */ \
}
#else
#define SM_EMPTY_POOL { \
  SM_EMPTY_NODE, /* mn */ \
}
#endif

extern void *smalloc(struct mempool *mp, size_t size);
extern void smfree(struct mempool *mp, void *ptr);
extern void *smrealloc(struct mempool *mp, void *ptr, size_t size);
extern int sminit(struct mempool *mp, void *start, size_t size);
#if SM_COMMIT_SUPPORT
extern int sminit_com(struct mempool *mp, void *start, size_t size,
    int (*commit)(void *area, size_t size),
    int (*uncommit)(void *area, size_t size));
#endif
extern int smdestroy(struct mempool *mp);
extern size_t smget_free_space(struct mempool *mp);
extern size_t smget_largest_free_area(struct mempool *mp);
extern int smget_area_size(struct mempool *mp, void *ptr);
extern void smregister_error_notifier(void (*func)(char *fmt, ...)
  FORMAT(printf, 1, 2));

#endif
