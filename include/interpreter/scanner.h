#ifndef SCANNER_H
#define SCANNER_H

#include <stddef.h>

#include "allocators/arena.h"

typedef enum {
  // Basic tokens
  TOK_WORD,            // default token
  TOK_ASSIGNMENT_WORD, // "COUNT" in `COUNT=3`
  TOK_NEWLINE,         // `\n`
  TOK_IO_NUMBER,       // “2” in `2>err`

  // Operators
  TOK_PIPE,        // |
  TOK_AMP,         // &
  TOK_SEMI,        // ;
  TOK_LESS,        // <
  TOK_GREAT,       // >
  TOK_L_PAREN,     // (
  TOK_R_PAREN,     // )
  TOK_AND_IF,      // &&
  TOK_OR_IF,       // ||
  TOK_D_LESS,      // <<
  TOK_D_GREAT,     // >>
  TOK_LESS_AND,    // <&
  TOK_GREAT_AND,   // >&
  TOK_LESS_GREAT,  // <>
  TOK_D_LESS_DASH, // <<-
  TOK_CLOBBER,     // >|
  // TODO: add scripting tokens
} token_type_t;

typedef struct {
  size_t start;
  size_t stop;
} token_span_t;

typedef struct {
  token_type_t type;
  char       *lexeme;
  char       *literal;
  token_span_t span;
  unsigned int row;
  unsigned int column;
} token_t;

typedef struct {
  char        *buf;
  char        *start;
  char        *current;
  unsigned int row;
  unsigned int col;
  arena_t     *arena;
} scanner_t;

void     scanner_init(scanner_t *s, char *source, arena_t *arena);
token_t *next_token(scanner_t *s);
void     token_print(const token_t *tok);

#endif // SCANNER_H
