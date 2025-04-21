#include <ctype.h>
#include <stddef.h>
#include <stdio.h>

#include "allocators/arena.h"
#include "interpreter/scanner.h"

static char     peek(scanner_t *s);
static char     advance(scanner_t *s);
static void     skip_whitespace(scanner_t *s);
static int      is_at_end(scanner_t *s);
static token_t *make_token(scanner_t *s, token_type_t t, size_t row, size_t col);
static token_t *identifier_or_assignment(scanner_t *s, size_t row, size_t col);
static token_t *number_or_word(scanner_t *s, size_t row, size_t col);
static token_t *operator_token(scanner_t *s, size_t row, size_t col);

void scanner_init(scanner_t *s, char *source, arena_t *arena) {
  s->buf     = source;
  s->start   = source;
  s->current = source;
  s->row     = 1;
  s->col     = 1;
  s->arena   = arena;
}

token_t *next_token(scanner_t *s) {
  skip_whitespace(s);
  if (is_at_end(s)) return NULL;

  s->start   = s->current;
  size_t row = s->row;
  size_t col = s->col;

  char c = advance(s);

  if (c == '\n') {
    return make_token(s, TOK_NEWLINE, row, col);
  }

  if (isalpha(c) || c == '_') {
    return identifier_or_assignment(s, row, col);
  }

  if (isdigit(c)) {
    return number_or_word(s, row, col);
  }

  if (c == '|' || c == '&' || c == '<' || c == '>') {
    return operator_token(s, row, col);
  }

  switch (c) {
    case ';': return make_token(s, TOK_SEMI, row, col);
    case '(': return make_token(s, TOK_L_PAREN, row, col);
    case ')': return make_token(s, TOK_R_PAREN, row, col);
    default: return make_token(s, TOK_WORD, row, col);
  }
}

void token_print(const token_t *tok) {
  printf("token {\n");
  printf("  type   = %d,\n", tok->type);
  printf("  lexeme = \"");

  for (char *p = tok->lexeme; *p; ++p) {
    switch (*p) {
      case '\n': printf("\\n"); break;
      case '\t': printf("\\t"); break;
      case '\\': printf("\\\\"); break;
      case '"': printf("\\\""); break;
      default: fputc(*p, stdout);
    }
  }

  printf("\",\n");
  printf("  span   = %zu..%zu,\n", tok->span.start, tok->span.stop);
  printf("  row    = %u,\n", tok->row);
  printf("  column = %u\n", tok->column);
  printf("}\n");
}

static char peek(scanner_t *s) { return *s->current; }

static int is_at_end(scanner_t *s) { return peek(s) == '\0'; }

static char advance(scanner_t *s) {
  char c = *s->current++;
  if (c == '\n') {
    s->row++;
    s->col = 1;
  } else {
    s->col++;
  }
  return c;
}

static void skip_whitespace(scanner_t *s) {
  for (;;) {
    char c = peek(s);
    switch (c) {
      case ' ':
      case '\r':
      case '\t': advance(s); continue;
      default: return;
    }
  }
}

static token_t *make_token(scanner_t *s, token_type_t type, size_t row,
                           size_t col) {
  size_t       len   = s->current - s->start;
  size_t       start = s->start - s->buf;
  size_t       end   = s->current - s->buf;
  token_span_t span  = {start, end};

  char *lexeme = arena_strndup(s->arena, s->start, len);

  token_t *tok = arena_alloc(s->arena, sizeof(token_t));
  if (!tok) return NULL;

  tok->type    = type;
  tok->lexeme  = lexeme;
  tok->literal = NULL;
  tok->span    = span;
  tok->row     = row;
  tok->column  = col;

  return tok;
}

static token_t *identifier_or_assignment(scanner_t *s, size_t row, size_t col) {
  while (isalnum(peek(s)) || peek(s) == '_') {
    advance(s);
  }

  if (peek(s) == '=') {
    return make_token(s, TOK_ASSIGNMENT_WORD, row, col);
  }

  return make_token(s, TOK_WORD, row, col);
}

static token_t *number_or_word(scanner_t *s, size_t row, size_t col) {
  while (isdigit(peek(s))) {
    advance(s);
  }

  if (peek(s) == '<' || peek(s) == '>') {
    return make_token(s, TOK_IO_NUMBER, row, col);
  }

  return make_token(s, TOK_WORD, row, col);
}

static token_t *operator_token(scanner_t *s, size_t row, size_t col) {
  char c = s->start[0];
  char n = peek(s);

  if (c == '&' && n == '&') {
    advance(s);
    return make_token(s, TOK_AND_IF, row, col);
  }
  if (c == '|' && n == '|') {
    advance(s);
    return make_token(s, TOK_OR_IF, row, col);
  }
  if (c == '<' && n == '<') {
    advance(s);
    if (peek(s) == '-') {
      advance(s);
      return make_token(s, TOK_D_LESS_DASH, row, col);
    }
    return make_token(s, TOK_D_LESS, row, col);
  }
  if (c == '>' && n == '>') {
    advance(s);
    return make_token(s, TOK_D_GREAT, row, col);
  }
  if (c == '<' && n == '&') {
    advance(s);
    return make_token(s, TOK_LESS_AND, row, col);
  }
  if (c == '>' && n == '&') {
    advance(s);
    return make_token(s, TOK_GREAT_AND, row, col);
  }
  if (c == '<' && n == '>') {
    advance(s);
    return make_token(s, TOK_LESS_GREAT, row, col);
  }
  if (c == '>' && n == '|') {
    advance(s);
    return make_token(s, TOK_CLOBBER, row, col);
  }

  switch (c) {
    case '|': return make_token(s, TOK_PIPE, row, col);
    case '&': return make_token(s, TOK_AMP, row, col);
    case '<': return make_token(s, TOK_LESS, row, col);
    case '>': return make_token(s, TOK_GREAT, row, col);
    default: return make_token(s, TOK_WORD, row, col);
  }
}
