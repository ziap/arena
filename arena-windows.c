#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define ARENA_BACKEND_ALLOC(size) VirtualAllocEx(GetCurrentProcess(), NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)
#define ARENA_BACKEND_DEALLOC(ptr, size) VirtualFreeEx(GetCurrentProcess(), ptr, 0, MEM_RELEASE)
// TODO: Implement reallocation with VirtualAlloc
#define ARENA_IMPLEMENTATION
#include "arena.h"
