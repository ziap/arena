/*
arena.h - Zap's personal approach to arena allocation
USAGE
  This is an STB-style single-header library.
  #define ARENA_IMPLEMENTATION // (in *one* C file)
  #include "arena.h"
  
  #define ARENA_ALLOC and ARENA_DEALLOC to avoid using malloc/free and create a
  custom backend for the allocator.

TODO:
  - Accept alignment as an argument not as a configuration

LICENSE
  See end of file for license information.
*/

#ifndef ARENA_H
#define ARENA_H

#ifndef ARENA_MAX_SIZE
#define ARENA_MAX_SIZE 16384
#endif

#include <stddef.h>

typedef struct ArenaNode ArenaNode;

typedef struct {
  ArenaNode *free;
  ArenaNode *current;
  ArenaNode *last;
  void *last_allocation;
} Arena;

extern Arena Arena_create(void);
extern void *Arena_alloc(Arena *arena, size_t size);
extern void *Arena_resize(Arena *arena, void *ptr, size_t old_size, size_t new_size);
extern void Arena_reset(Arena *arena);
extern void Arena_destroy(Arena arena);

#endif

#ifdef ARENA_IMPLEMENTATION

#include <stdint.h>

#if !defined(ARENA_BACKEND_ALLOC) || !defined(ARENA_BACKEND_DEALLOC)
#include <stdlib.h>
#define ARENA_BACKEND_ALLOC(size) malloc(size)
#define ARENA_BACKEND_DEALLOC(ptr, size) free(ptr)
#define ARENA_BACKEND_RESIZE(ptr, old_size, new_size) realloc(ptr, new_size)
#endif

#ifndef ARENA_MEMCPY
#include <string.h>
#define ARENA_MEMCPY memcpy
#endif

#ifndef ARENA_BACKEND_RESIZE
static void *Arena_backend_resize(void *ptr, size_t old_size, size_t new_size) {
  void *new_ptr = ARENA_BACKEND_ALLOC(new_size);
  ARENA_MEMCPY(new_ptr, ptr, old_size);
  ARENA_BACKEND_DEALLOC(ptr, old_size);
  return new_ptr;
}
#define ARENA_BACKEND_RESIZE Arena_backend_resize
#endif

#ifndef ARENA_ALIGNMENT
#define ARENA_ALIGNMENT (sizeof(void*))
#endif

#define ARENA_ALIGN(p) (-(uintptr_t)((void*)p) & (ARENA_ALIGNMENT - 1))
#define ARENA_NODE_SIZE (sizeof(ArenaNode) + ARENA_ALIGN(sizeof(ArenaNode)))

struct ArenaNode {
  ArenaNode *prev;
  ArenaNode *next;
  char *pos;
  char *end;
  char data[];
};

static ArenaNode *Arena_create_node(Arena *arena, size_t size) {
  if (arena->free) {
    ArenaNode *node = arena->free;
    arena->free = node->prev;
    if (node->end - node->data >= size) {
      return node;
    }
    ARENA_BACKEND_DEALLOC(node, ARENA_NODE_SIZE + node->end - node->data);
  }
  ArenaNode *node = ARENA_BACKEND_ALLOC(ARENA_NODE_SIZE + size);
  node->end = node->data + size;
  return node;
}

Arena Arena_create(void) {
  ArenaNode *node = ARENA_BACKEND_ALLOC(ARENA_NODE_SIZE + ARENA_MAX_SIZE);
  node->prev = NULL;
  node->next = NULL;
  node->pos = node->data;
  node->end   = node->data + ARENA_MAX_SIZE;
  return (Arena) {
    .free = NULL,
    .current = node,
    .last = node,
    .last_allocation = NULL
  };
}

void *Arena_alloc(Arena *arena, size_t size) {
  if (size >= ARENA_MAX_SIZE) {
    ArenaNode *node = Arena_create_node(arena, size);
    node->pos = node->data + size;
    ArenaNode *prev = arena->current->prev;
    arena->current->prev = node;
    node->next = arena->current;
    node->prev = prev;
    if (prev) prev->next = node;
    else arena->last = node;
    arena->last_allocation = node->data;
    return node->data;
  }
  arena->current->pos += ARENA_ALIGN(arena->current->pos);
  if (arena->current->pos + size > arena->current->end) {
    ArenaNode *node = Arena_create_node(arena, ARENA_MAX_SIZE);
    node->pos = node->data;
    node->next = NULL;
    node->prev = arena->current;
    arena->current->next = node;
    arena->current = node;
  }
  void *data = arena->current->pos;
  arena->current->pos += size;
  arena->last_allocation = data;
  return data;
}

void *Arena_resize(Arena *arena, void *ptr, size_t old_size, size_t new_size) {
  if (old_size >= ARENA_MAX_SIZE && new_size > ARENA_MAX_SIZE) {
    ArenaNode *old_node = (ArenaNode*)((char*)ptr - ARENA_NODE_SIZE);
    size_t old_node_size = ARENA_NODE_SIZE + old_node->end - old_node->data;
    size_t new_node_size = new_size + ARENA_NODE_SIZE;
    ArenaNode *new_node = ARENA_BACKEND_RESIZE(old_node, old_node_size, new_node_size);
    new_node->end = new_node->data + new_size;

    if (new_node->prev) new_node->prev->next = new_node;
    else arena->last = new_node;

    if (new_node->next) new_node->next->prev = new_node;
    else arena->current = new_node;

    new_node->pos = new_node->data + new_size;
    return new_node->data;
  }

  if (ptr && ptr == arena->last_allocation) {
    arena->current->pos -= old_size;
    if (arena->current->pos + new_size <= arena->current->end) {
      arena->current->pos += new_size;
      return ptr;
    }
  }
  void *new_ptr = Arena_alloc(arena, new_size);
  ARENA_MEMCPY(new_ptr, ptr, old_size);
  return new_ptr;
}

void Arena_reset(Arena *arena) {
  if (arena->current != arena->last) {
    arena->last->prev = arena->free;
    arena->free = arena->current->prev;
    arena->current->prev = NULL;
    arena->current->next = NULL;
    arena->last = arena->current;
  }
  arena->last_allocation = NULL;
  arena->current->pos = arena->current->data;
}

void Arena_destroy(Arena arena) {
  if (arena.last) arena.last->prev = arena.free;
  ArenaNode *node = arena.current;
  while (node) {
    ArenaNode *prev = node->prev;
    ARENA_BACKEND_DEALLOC(node, ARENA_NODE_SIZE + node->end - node->data);
    node = prev;
  }
}

#endif

/*
------------------------------------------------------------------------------
LICENSE
------------------------------------------------------------------------------
MIT License
Copyright (c) 2022 Zap
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
