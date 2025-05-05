#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

#include "allocators/arena.h"
#include "interpreter/ast.h"
#include "interpreter/scanner.h"

typedef struct {
  scanner_t *scanner;
  token_t   *cur;
  token_t   *prev;
  arena_t   *arena;
  bool       had_error;
} parser_t;

void        parser_init(parser_t *parser, scanner_t *scanner, arena_t *arena);
ast_node_t *parser_parse(parser_t *parser);
void        parser_error(parser_t *parser, const char *message);
void        parser_synchronize(parser_t *parser);

#endif // PARSER_H
