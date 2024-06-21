# Arena allocator

My personal approach to arena allocation. Read more about the design and
motivation of this allocator here: <https://ziap.github.io/blog/arena>

## How to use

The code is implemented in C as an STB-style single-header library. The header
is portable and can be used on its own. But platform-specific source files are
also available to "configure" the library to use the system page allocator.
This separation breaks the rule of header-only libraries, but it allows for
easier addition of target platforms.

Using the library as a single-header header library is the same as any other
library of the same type: select exactly one C source file that instantiates
the code by defining `ARENA_IMPLEMENTATION`.

```c
#define ARENA_IMPLEMENTATION
#include "arena.h"
```

The platform-specific source files act as the instantiation source files. So if
you use one of those, don't define `ARENA_IMPLEMENTATION` anywhere else.

### Adding a new platform

To add a new platform, redefine these two macros to call your platform's
allocator.

```c
#define ARENA_BACKEND_ALLOC(size) ...
#define ARENA_BACKEND_DEALLOC(ptr, size) ...

#define ARENA_IMPLEMENTATION
#include "arena.h"
```

You can also define `ARENA_BACKEND_RESIZE` if your platform supports efficient
reallocation, but it is not required, as the library can automatically
implement that with `ARENA_BACKEND_ALLOC` and `ARENA_BACKEND_DEALLOC`.

### Configuration

Some macros that you can redefine to configure the library:

| Macro             | Usage                                                      |
| ----------------- | ---------------------------------------------------------- |
| `ARENA_MEMCPY`    | Redefine if your platform can't use libc's `memcpy`        |
| `ARENA_ALIGNMENT` | Customize the memory alignment. Default is `sizeof(void*)` |
| `ARENA_MAX_SIZE`  | The maximum size of each chunk, should be page-aligned     |


## License

This project is licensed under the [MIT License](LICENSE).
