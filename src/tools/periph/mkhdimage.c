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

/* mkhdimage.c, for the Linux DOS emulator
 *
 * cheesy program to create an hdimage header
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#ifdef __linux__
#include <getopt.h>
#endif
#include <errno.h>
#include <string.h>

int sectors = 17, heads = 4, cylinders = 40, header_size = 128;

static void usage(void)
{
  fprintf(stderr, "mkhdimage [-h <heads>] [-s <sectors>] [-c|-t <cylinders>]\n");
}

int
main(int argc, char **argv)
{
  int c;
  int pos = 0;

  while ((c = getopt(argc, argv, "h:s:t:c:s:")) != EOF) {
    switch (c) {
    case 'h':
      heads = atoi(optarg);
      break;
    case 's':
      sectors = atoi(optarg);
      break;
    case 'c':			/* cylinders */
    case 't':			/* tracks */
      cylinders = atoi(optarg);
      break;
    default:
      fprintf(stderr, "Unknown option '%c'\n", c);
      usage();
      exit(1);
    }
  }

#define WRITE(fd, ptr, size) ({ int w_tmp=0; \
do { w_tmp=write(fd,ptr,size); } while ((w_tmp == -1) && (errno == EINTR)); \
if (w_tmp == -1) \
  fprintf(stderr, "WRITE ERROR: %d, *%s* in file %s, line %d\n", \
	  errno, strerror(errno), __FILE__, __LINE__); \
w_tmp; })

  pos += WRITE(STDOUT_FILENO, "DOSEMU", 7);
  pos += WRITE(STDOUT_FILENO, &heads, 4);
  pos += WRITE(STDOUT_FILENO, &sectors, 4);
  pos += WRITE(STDOUT_FILENO, &cylinders, 4);
  pos += WRITE(STDOUT_FILENO, &header_size, 4);

  {
    char tmp[256];

    memset(tmp, 0, 256);

    pos += WRITE(STDOUT_FILENO, tmp, header_size - pos);
    fprintf(stderr, "Pos now is %d\n", pos);
  }

  return 0;
}
