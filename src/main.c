#include <stdio.h>
#include <stdlib.h>

#include "allocators/arena.h"
#include "repl.h"

#define CAPACITY 4096

int main(int argc, char **argv) {
  // TODO: add CLI:
  //       - run command `-c`
  //       - ability to debug (showing tokens and AST)
  //       - ability to show system orchestration like process IDs, etc
  (void)argc;
  (void)argv;

  arena_t arena;
  if (!arena_init(&arena, CAPACITY)) {
    fprintf(stderr, "Error: failed to initialize arena\n");
    return EXIT_FAILURE;
  }

  repl_run(&arena);

  arena_free(&arena);
  return EXIT_SUCCESS;
}
