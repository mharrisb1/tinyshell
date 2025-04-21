#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

typedef struct {
  char  *buf;
  size_t capacity;
  size_t offset;
} arena_t;

int   arena_init(arena_t *a, size_t capacity);
void  arena_free(arena_t *a);
void  arena_reset(arena_t *a);
void *arena_alloc(arena_t *a, size_t n);
char *arena_strndup(arena_t *a, const char *s, size_t n);

#endif // ARENA_H
