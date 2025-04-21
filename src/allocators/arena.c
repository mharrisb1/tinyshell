#include "allocators/arena.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

int arena_init(arena_t *a, size_t capacity) {
  a->buf = malloc(capacity);
  if (!a->buf) return 0;
  a->capacity = capacity;
  a->offset   = 0;
  return 1;
}

void arena_free(arena_t *a) {
  free(a->buf);
  a->buf = NULL;
}

void arena_reset(arena_t *a) { a->offset = 0; }

void *arena_alloc(arena_t *a, size_t n) {
  size_t aligned = (n + 7) & ~7;
  if (a->offset + aligned > a->capacity) return NULL;
  void *ptr = a->buf + a->offset;
  a->offset += aligned;
  return ptr;
}

char *arena_strndup(arena_t *a, const char *s, size_t n) {
  char *dst = arena_alloc(a, n + 1);
  if (!dst) return NULL;
  memcpy(dst, s, n);
  dst[n] = '\0';
  return dst;
}
