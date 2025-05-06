#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>
#include <string.h>

#include "allocators/arena.h"

typedef struct {
  void    *data;
  size_t   elem_size;
  size_t   capacity;
  size_t   length;
  arena_t *arena;
} vector_t;

static inline void vector_init(vector_t *vec, size_t elem_size, arena_t *arena) {
  vec->data      = NULL;
  vec->elem_size = elem_size;
  vec->capacity  = 0;
  vec->length    = 0;
  vec->arena     = arena;
}

static inline void vector_reserve(vector_t *vec, size_t capacity) {
  if (capacity > vec->capacity) {
    void *data = arena_alloc(vec->arena, capacity * vec->elem_size);
    if (vec->length) memcpy(data, vec->data, vec->length * vec->elem_size);
    vec->data     = data;
    vec->capacity = capacity;
  }
}

static inline void vector_push(vector_t *vec, const void *elem) {
  if (vec->length == vec->capacity) {
    size_t capacity = vec->capacity ? vec->capacity * 2 : 4;
    vector_reserve(vec, capacity);
  }
  memcpy((char *)vec->data + vec->length * vec->elem_size, elem, vec->elem_size);
  vec->length++;
}
#endif // VECTOR_H
