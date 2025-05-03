#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <partyline/partyline.h>

#include "allocators/arena.h"
#include "interpreter/ast.h"
#include "interpreter/parser.h"
#include "interpreter/scanner.h"
#include "repl.h"

#define TEXT_GREEN(text) "\033[32m" text "\033[0m"

static const char GREETING[] = TEXT_GREEN("  _____     ____     ______\n"
                                          " /      \\  |  o |   | sup. |\n"
                                          "|        |/ ___\\|  /_______|\n"
                                          "|_________/\n"
                                          "|_|_| |_|_|\n\n");

void repl_run(arena_t *arena) {
  char *line;

  printf("%s", GREETING);
  while ((line = partyline(TEXT_GREEN("> "))) != NULL) {
    arena_reset(arena);

    scanner_t scanner;
    scanner_init(&scanner, line, arena);

    parser_t parser;
    parser_init(&parser, scanner, arena);

    ast_node_t *root = parser_parse(&parser);
    if (!parser.had_error) ast_dump(root);
    free(line);
  }
}
