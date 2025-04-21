#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "allocators/arena.h"
#include "interpreter/scanner.h"
#include "repl.h"

#define TEXT_GREEN(text) "\033[32m" text "\033[0m"

static const char GREETING[] = TEXT_GREEN("  _____     ____     ______\n"
                                          " /      \\  |  o |   | sup. |\n"
                                          "|        |/ ___\\|  /_______|\n"
                                          "|_________/\n"
                                          "|_|_| |_|_|\n\n");

void repl_run(arena_t *arena) {
  char  *line = NULL;
  size_t len  = 0;

  printf("%s", GREETING);
  while (printf(TEXT_GREEN("> ")), fflush(stdout),
         getline(&line, &len, stdin) != -1) {
    arena_reset(arena);

    scanner_t scanner;
    scanner_init(&scanner, line, arena);

    token_t *tok;
    while ((tok = next_token(&scanner)) != NULL) {
      token_print(tok);
    }
  }

  free(line);
}
