// malloc(0) shim — linked via -Wl,--wrap=malloc / --wrap=realloc (platformio.ini).
//
// Simon Tatham's smalloc()/srealloc() (upstream/malloc.c) do `p = malloc(size);
// if (!p) fatal("out of memory");` with no size==0 guard. On glibc malloc(0)
// returns a non-NULL pointer, so this is harmless on desktop (and the host test
// passes). On ESP32/newlib malloc(0) returns NULL, so any zero-size allocation
// during generation is misreported as out-of-memory and bricks the game — this
// is what excluded Galaxies (its generator makes a zero-size request at some
// sizes). Mapping 0 -> 1 byte is a universally safe substitution: no caller of
// malloc(0) relies on the size, and it keeps the vendored submodule untouched.
#include <stddef.h>

extern void *__real_malloc(size_t size);
extern void *__real_realloc(void *ptr, size_t size);

void *__wrap_malloc(size_t size) {
  return __real_malloc(size ? size : 1);
}
void *__wrap_realloc(void *ptr, size_t size) {
  return __real_realloc(ptr, size ? size : 1);
}
