#define _GNU_SOURCE
#include <sys/mman.h>

#define ARENA_BACKEND_ALLOC(size) mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
#define ARENA_BACKEND_DEALLOC(ptr, size) munmap(ptr, size)
#define ARENA_BACKEND_RESIZE(ptr, old_size, new_size) mremap(ptr, old_size, new_size, MREMAP_MAYMOVE)
#define ARENA_IMPLEMENTATION
#include "arena.h"
