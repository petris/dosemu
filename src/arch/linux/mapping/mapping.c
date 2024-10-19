/*
 * (C) Copyright 1992, ..., 2007 the "DOSEMU-Development-Team".
 *
 * for details see file COPYING.DOSEMU in the DOSEMU distribution
 */

/* file mapping.c
 *
 * generic mapping driver interface
 * (C) Copyright 2000, Hans Lermen, lermen@fgan.de
 *
 * NOTE: We handle _all_ memory mappings within the mapping drivers,
 *       mmap-type as well as IPC shm, except for X-mitshm (in X.c),
 *       which is a special case and doesn't involve DOS space atall.
 *
 *       If you ever need to do mapping within DOSEMU (except X-mitshm),
 *       _always_ use the interface supplied by the mapping drivers!
 *       ^^^^^^^^         ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 */

#include "emu.h"
#include "hma.h"
#include "utilities.h"
#include "Linux/mman.h"
#include "mapping.h"
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>

#ifndef __x86_64__
#undef MAP_32BIT
#define MAP_32BIT 0
#endif

struct mem_map_struct {
  off_t src;
  void *base;
  void *dst;
  int len;
  int mapped;
};

#define MAX_KMEM_MAPPINGS 4096
static int kmem_mappings = 0;
static struct mem_map_struct kmem_map[MAX_KMEM_MAPPINGS];

static int init_done = 0;
unsigned char *mem_base;
char *lowmem_base;

static struct mappingdrivers *mappingdrv[] = {
#ifdef HAVE_SHM_OPEN
  &mappingdriver_shm, /* first try shm_open */
#endif
  &mappingdriver_ashm,  /* then anon-shared-mmap */
  &mappingdriver_file, /* and then a temp file */
};

/* The alias map is used to track alias mappings from the first 1MB + HMA
   to the corresponding addresses in Linux address space (either lowmem
   or EMS). The DOS address (&mem_base[address]) may be r/w
   protected by cpuemu, but the alias is never protected,
   so it can be used to write without needing to unprotect and reprotect
   afterwards.
   If the alias is not used (hardware RAM from /dev/mem, or DPMI memory
   (aliasing using fn 0x509 is safely ignored here)),
   the address is identity-mapped to &mem_base[address].

   The alias is also not used for vgaemu memory to allow special traps to occur
   in the C patches for cpuemu (LINEAR2UNIX in cpatch.c).
   This should be cleaned up.
*/
static unsigned char *aliasmap[(LOWMEM_SIZE+HMASIZE)/PAGE_SIZE];

static void update_aliasmap(unsigned char *dosaddr, size_t mapsize,
			    unsigned char *unixaddr)
{
  unsigned int dospage, i;

  if (dosaddr >= &mem_base[LOWMEM_SIZE+HMASIZE])
    return;
  dospage = (dosaddr - mem_base) >> PAGE_SHIFT;
  for (i = 0; i < mapsize >> PAGE_SHIFT; i++)
    aliasmap[dospage + i] = unixaddr + (i << PAGE_SHIFT);
}

void *dosaddr_to_unixaddr(unsigned int addr)
{
  if (addr < LOWMEM_SIZE + HMASIZE)
    return aliasmap[addr >> PAGE_SHIFT] + (addr & (PAGE_SIZE - 1));
  return &mem_base[addr];
}

void *physaddr_to_unixaddr(unsigned int addr)
{
  if (addr < LOWMEM_SIZE + HMASIZE)
    return dosaddr_to_unixaddr(addr);
  /* XXX something other than XMS? */
  return &ext_mem_base[addr - (LOWMEM_SIZE + HMASIZE)];
}

static int map_find_idx(struct mem_map_struct *map, int max, off_t addr)
{
  int i;
  for (i = 0; i < max; i++) {
    if (map[i].src == addr)
      return i;
  }
  return -1;
}

static int map_find(struct mem_map_struct *map, int max,
  unsigned char *addr, int size, int mapped)
{
  int i;
  unsigned char *dst, *dst1;
  unsigned char *min = (void *)-1;
  int idx = -1;
  unsigned char *max_addr = addr + size;
  for (i = 0; i < max; i++) {
    if (!map[i].dst || !map[i].len || map[i].mapped != mapped)
      continue;
    dst = map[i].dst;
    dst1 = dst + map[i].len;
    if (dst >= addr && dst < max_addr) {
      if (min == (void *)-1 || dst < min) {
        min = dst;
        idx = i;
      }
    }
    if (dst1 > addr && dst < max_addr) {
      if (min == (void *)-1 || dst1 < min) {
        min = addr;
        idx = i;
      }
    }
  }
  return idx;
}

static void kmem_unmap_single(int cap, int idx)
{
  kmem_map[idx].base = mmap(0, kmem_map[idx].len, PROT_NONE,
	       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  mremap_mapping(cap, kmem_map[idx].dst, kmem_map[idx].len,
      kmem_map[idx].len, MREMAP_MAYMOVE | MREMAP_FIXED, kmem_map[idx].base);
  kmem_map[idx].mapped = 0;
}

static void kmem_unmap_mapping(int cap, void *addr, int mapsize)
{
  int i;
  if (addr == (void*)-1)
    return;
  while ((i = map_find(kmem_map, kmem_mappings, addr, mapsize, 1)) != -1) {
    kmem_unmap_single(cap, i);
  }
}

static void kmem_map_single(int cap, int idx)
{
  mremap_mapping(cap, kmem_map[idx].base, kmem_map[idx].len, kmem_map[idx].len,
      MREMAP_MAYMOVE | MREMAP_FIXED, kmem_map[idx].dst);
  update_aliasmap(kmem_map[idx].dst, kmem_map[idx].len, kmem_map[idx].dst);
  kmem_map[idx].mapped = 1;
}

#if 0
static void kmem_map_mapping(int cap, void *addr, int mapsize)
{
  int i;
  if (addr == (void*)-1)
    return;
  while ((i = map_find(kmem_map, kmem_mappings, addr, mapsize, 0)) != -1) {
    kmem_map_single(cap, i);
  }
}
#endif

void *extended_mremap(void *addr, size_t old_len, size_t new_len,
	int flags, void * new_addr)
{
	return (void *)syscall(SYS_mremap, addr, old_len, new_len, flags, new_addr);
}

void *alias_mapping(int cap, unsigned targ, size_t mapsize, int protect, void *source)
{
  void *target = &mem_base[targ], *addr;
  Q__printf("MAPPING: alias, cap=%s, targ=%#x, size=%zx, protect=%x, source=%p\n",
	cap, targ, mapsize, protect, source);
  /* for non-zero INIT_LOWRAM the target is a hint */
  if (!((cap & MAPPING_INIT_LOWRAM) && target))
    cap |= MAPPING_FIXED;
  if (cap & MAPPING_COPYBACK) {
    if (cap & (MAPPING_LOWMEM | MAPPING_HMA)) {
      memcpy(source, target, mapsize);
    } else {
      error("COPYBACK is not supported for mapping type %#x\n", cap);
      return MAP_FAILED;
    }
  }
  kmem_unmap_mapping(MAPPING_OTHER, target, mapsize);
  addr = mappingdriver.alias(cap, target, mapsize, protect, source);
  update_aliasmap(target, mapsize, (cap & MAPPING_VGAEMU) ? target : source);
  if (cap & MAPPING_INIT_LOWRAM) {
    *(unsigned char **)&mem_base = addr;
  }
  return addr;
}

void *mmap_mapping(int cap, void *target, size_t mapsize, int protect, off_t source)
{
  int fixed = target == (void *)-1 ? 0 : MAP_FIXED;
  void *addr;
  Q__printf("MAPPING: map, cap=%s, target=%p, size=%zx, protect=%x, source=%#llx\n",
	cap, target, mapsize, protect, (long long)source);
  if (cap & MAPPING_KMEM) {
    int i;
    {
      i = map_find_idx(kmem_map, kmem_mappings, source);
      if (i == -1) {
	error("KMEM mapping for %#llx was not allocated!\n", (long long)source);
	return MAP_FAILED;
      }
      if (kmem_map[i].len != mapsize) {
	error("KMEM mapping for %#llx allocated for size %#x, but %#zx requested\n",
	      (long long)source, kmem_map[i].len, mapsize);
	return MAP_FAILED;
      }
      if (target != (void*)-1) {
	kmem_map[i].dst = target;
	if (cap & MAPPING_COPYBACK) {
	  memcpy(kmem_map[i].base, target, mapsize);
	}
	kmem_map_single(cap, i);
      } else {
	target = kmem_map[i].base;
      }
    }
    mprotect_mapping(cap, target, mapsize, protect);
    return target;
  }

  if (cap & MAPPING_COPYBACK) {
    error("COPYBACK is not supported for mapping type %#x\n", cap);
    return MAP_FAILED;
  }

  kmem_unmap_mapping(MAPPING_OTHER, target, mapsize);

  if (cap & MAPPING_SCRATCH) {
    fixed = (cap & MAPPING_FIXED) ? MAP_FIXED : 0;
    if (!fixed && target == (void *)-1) target = NULL;
#ifdef __x86_64__
    if (fixed == 0 && (cap & (MAPPING_DPMI|MAPPING_VGAEMU)))
      fixed = MAP_32BIT;
#endif
    addr = mmap(target, mapsize, protect,
		MAP_PRIVATE | fixed | MAP_ANONYMOUS, -1, 0);
    update_aliasmap(addr, mapsize, addr);
  } else {
    dosemu_error("Wrong mapping type %#x\n", cap);
    config.exitearly = 1;
    return MAP_FAILED;
  }
  Q__printf("MAPPING: map success, cap=%s, addr=%p\n", cap, addr);
  return addr;
}

void *mremap_mapping(int cap, void *source, size_t old_size, size_t new_size,
  unsigned long flags, void *target)
{
  Q__printf("MAPPING: remap, cap=%s, source=%p, old_size=%zx, new_size=%zx, target=%p\n",
	cap, source, old_size, new_size, target);
  if (target != (void *)-1) {
    return extended_mremap(source, old_size, new_size, flags, target);
  }
  return mremap(source, old_size, new_size, flags);
}

int mprotect_mapping(int cap, void *addr, size_t mapsize, int protect)
{
  Q__printf("MAPPING: mprotect, cap=%s, addr=%p, size=%zx, protect=%x\n",
	cap, addr, mapsize, protect);
  return mprotect(addr, mapsize, protect);
}

/*
 * This gets called on DOSEMU startup to determine the kind of mapping
 * and setup the appropriate function pointers
 */
void mapping_init(void)
{
  int i, found = -1;
  int numdrivers = sizeof(mappingdrv) / sizeof(mappingdrv[0]);

  if (init_done) return;

  memset(&mappingdriver, 0, sizeof(struct mappingdrivers));
  if (config.mappingdriver && strcmp(config.mappingdriver, "auto")) {
    /* first try the mapping driver the user wants */
    for (i=0; i < numdrivers; i++) {
      if (!strcmp(mappingdrv[i]->key, config.mappingdriver)) {
        found = i;
        break;
      }
    }
    if (found < 0) {
      error("Wrong mapping driver specified: %s\n", config.mappingdriver);
      leavedos(2);
    }
  }
  if (found < 0) found = 0;
  for (i=found; i < numdrivers; i++) {
    if (mappingdrv[i] && (*mappingdrv[i]->open)(MAPPING_PROBE)) {
      memcpy(&mappingdriver, mappingdrv[i], sizeof(struct mappingdrivers));
      Q_printf("MAPPING: using the %s driver\n", mappingdriver.name);
      init_done = 1;
      return;
    }
    if (found > 0) {
      found = -1;
      /* As we want to restart the loop, because of 'i++' at end of loop,
       * we need to set 'i = -1'
       */
      i = -1;
    }
  }
  error("MAPPING: cannot allocate an appropriate mapping driver\n");
  leavedos(2);
}

/* this gets called on DOSEMU termination cleanup all mapping stuff */
void mapping_close(void)
{
  if (init_done && mappingdriver.close) close_mapping(MAPPING_ALL);
}

static char dbuf[256];
char *decode_mapping_cap(int cap)
{
  char *p = dbuf;
  p[0] = 0;
  if (!cap) return dbuf;
  if ((cap & MAPPING_ALL) == MAPPING_ALL) {
    p += sprintf(p, " ALL");
  }
  else {
    if (cap & MAPPING_OTHER) p += sprintf(p, " OTHER");
    if (cap & MAPPING_EMS) p += sprintf(p, " EMS");
    if (cap & MAPPING_DPMI) p += sprintf(p, " DPMI");
    if (cap & MAPPING_VGAEMU) p += sprintf(p, " VGAEMU");
    if (cap & MAPPING_VIDEO) p += sprintf(p, " VIDEO");
    if (cap & MAPPING_VC) p += sprintf(p, " VC");
    if (cap & MAPPING_HGC) p += sprintf(p, " HGC");
    if (cap & MAPPING_HMA) p += sprintf(p, " HMA");
    if (cap & MAPPING_SHARED) p += sprintf(p, " SHARED");
    if (cap & MAPPING_INIT_HWRAM) p += sprintf(p, " INIT_HWRAM");
    if (cap & MAPPING_INIT_LOWRAM) p += sprintf(p, " INIT_LOWRAM");
    if (cap & MAPPING_EXTMEM) p += sprintf(p, " EXTMEM");
  }
  if (cap & MAPPING_KMEM) p += sprintf(p, " KMEM");
  if (cap & MAPPING_LOWMEM) p += sprintf(p, " LOWMEM");
  if (cap & MAPPING_SCRATCH) p += sprintf(p, " SCRATCH");
  if (cap & MAPPING_SINGLE) p += sprintf(p, " SINGLE");
  if (cap & MAPPING_MAYSHARE) p += sprintf(p, " MAYSHARE");
  if (cap & MAPPING_COPYBACK) p += sprintf(p, " COPYBACK");
  return dbuf;
}

int open_mapping(int cap)
{
  return mappingdriver.open(cap);
}

void close_mapping(int cap)
{
  if (mappingdriver.close) mappingdriver.close(cap);
}

void *alloc_mapping(int cap, size_t mapsize, off_t target)
{
  void *addr;

  Q__printf("MAPPING: alloc, cap=%s, source=%#llx\n", cap, (long long)target);
  if (cap & MAPPING_KMEM) {
    if (target == -1) {
      error("KMEM mapping without target\n");
      leavedos(64);
    }
    if (map_find_idx(kmem_map, kmem_mappings, target) != -1) {
      error("KMEM mapping for %#llx allocated twice!\n", (long long)target);
      return MAP_FAILED;
    }
    open_kmem();
    addr = mmap(0, mapsize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_32BIT,
		mem_fd,	target);
    close_kmem();
    if (addr == MAP_FAILED)
      return addr;

    kmem_map[kmem_mappings].src = target; /* target is actually source */
    kmem_map[kmem_mappings].base = addr;
    kmem_map[kmem_mappings].dst = NULL;
    kmem_map[kmem_mappings].len = mapsize;
    kmem_map[kmem_mappings].mapped = 0;
    kmem_mappings++;
    Q__printf("MAPPING: %s region allocated at %p\n", cap, addr);
    return addr;
  }

  addr = mappingdriver.alloc(cap, mapsize);
  mprotect_mapping(cap, addr, mapsize, PROT_READ | PROT_WRITE);

  if (cap & MAPPING_INIT_LOWRAM) {
    Q__printf("MAPPING: LOWRAM_INIT, cap=%s, base=%p\n", cap, addr);
    *(char **)(&lowmem_base) = addr;
  }
  return addr;
}

void free_mapping(int cap, void *addr, size_t mapsize)
{
  if (cap & MAPPING_KMEM) {
    return;
  }
  mprotect_mapping(cap, addr, mapsize, PROT_READ | PROT_WRITE);
  mappingdriver.free(cap, addr, mapsize);
}

void *realloc_mapping(int cap, void *addr, size_t oldsize, size_t newsize)
{
  if (!addr) {
    if (oldsize)  // no-no, realloc of the lowmem is not good too
      dosemu_error("realloc_mapping() called with addr=NULL, oldsize=%#zx\n", oldsize);
    Q_printf("MAPPING: realloc from NULL changed to malloc\n");
    return alloc_mapping(cap, newsize, -1);
  }
  if (!oldsize)
    dosemu_error("realloc_mapping() addr=%p, oldsize=0\n", addr);
  return mappingdriver.realloc(cap, addr, oldsize, newsize);
}

int munmap_mapping(int cap, void *addr, size_t mapsize)
{
  /* First of all remap the kmem mappings */
  kmem_unmap_mapping(MAPPING_OTHER, addr, mapsize);

    if (cap & MAPPING_KMEM) {
      /* Already done */
      return 0;
    }

  return mappingdriver.munmap(cap, addr, mapsize);
}

struct hardware_ram {
  size_t base;
  unsigned vbase;
  size_t size;
  int type;
  struct hardware_ram *next;
};

static struct hardware_ram *hardware_ram;

/*
 * DANG_BEGIN_FUNCTION map_hardware_ram
 *
 * description:
 *  Initialize the hardware direct-mapped pages
 *
 * DANG_END_FUNCTION
 */
void map_hardware_ram(void)
{
  struct hardware_ram *hw;
  int cap;
  unsigned char *p;

  for (hw = hardware_ram; hw != NULL; hw = hw->next) {
    if (!hw->type || hw->type == 'e') { /* virtual hardware ram, base==vbase */
      hw->vbase = hw->base;
      continue;
    }
    cap = (hw->type == 'v' ? MAPPING_VC : MAPPING_INIT_HWRAM) | MAPPING_KMEM;
    if (hw->base < LOWMEM_SIZE)
      hw->vbase = hw->base;
    alloc_mapping(cap, hw->size, hw->base);
    p = hw->vbase == -1 ? (void *)-1 : &mem_base[hw->vbase];
    p = mmap_mapping(cap, p, hw->size, PROT_READ | PROT_WRITE,
		     hw->base);
    if (p == MAP_FAILED) {
      error("mmap error in map_hardware_ram %s\n", strerror (errno));
      return;
    }
    hw->vbase = p - mem_base;
    g_printf("mapped hardware ram at 0x%08zx .. 0x%08zx at %#x\n",
	     hw->base, hw->base+hw->size-1, hw->vbase);
  }
}

int register_hardware_ram(int type, unsigned int base, unsigned int size)
{
  struct hardware_ram *hw;

  if (!can_do_root_stuff && type != 'e') {
    dosemu_error("can't use hardware ram in low feature (non-suid root) DOSEMU\n");
    return 0;
  }
  c_printf("Registering HWRAM, type=%c base=%#x size=%#x\n", type, base, size);
  hw = malloc(sizeof(*hw));
  hw->base = base;
  if (type == 'e')
    hw->vbase = base;
  else
    hw->vbase = -1;
  hw->size = size;
  hw->type = type;
  hw->next = hardware_ram;
  hardware_ram = hw;
  if (base >= LOWMEM_SIZE || type == 'h')
    memcheck_reserve(type, base, size);
  return 1;
}

/* given physical address addr, gives the corresponding vbase or -1 */
unsigned get_hardware_ram(unsigned addr)
{
  struct hardware_ram *hw;

  for (hw = hardware_ram; hw != NULL; hw = hw->next)
    if (hw->vbase != -1 &&
	hw->base <= addr && addr < hw->base + hw->size)
      return hw->vbase + addr - hw->base;
  return -1;
}

void list_hardware_ram(void (*print)(const char *, ...))
{
  struct hardware_ram *hw;

  (*print)("hardware_ram: %s\n", hardware_ram ? "" : "no");
  if (!hardware_ram) return;
  (*print)("hardware_pages:\n");
  for (hw = hardware_ram; hw != NULL; hw = hw->next)
    (*print)("%08x-%08x\n", hw->base, hw->base + hw->size - 1);
}
