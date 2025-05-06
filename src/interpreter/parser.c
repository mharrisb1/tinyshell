#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allocators/arena.h"
#include "collections/vector.h"
#include "interpreter/ast.h"
#include "interpreter/parser.h"
#include "interpreter/scanner.h"

static void             advance(parser_t *parser);
static ast_node_t      *parse_list(parser_t *parser);
static ast_node_t      *parse_and_or(parser_t *parser);
static ast_node_t      *parse_pipeline(parser_t *parser);
static ast_node_t      *parse_command(parser_t *parser);
static ast_node_t      *parse_simple(parser_t *parser);
static bool             is_redir_tok(token_type_t t);
static ast_redir_type_t map_token_to_redir_type(token_type_t op);

void parser_init(parser_t *parser, scanner_t *scanner, arena_t *arena) {
  parser->scanner   = scanner;
  parser->cur       = NULL;
  parser->prev      = NULL;
  parser->arena     = arena;
  parser->had_error = false;

  advance(parser);
}

ast_node_t *parser_parse(parser_t *parser) {
  ast_node_t *root = parse_list(parser);
  if (parser->had_error) return NULL;
  if (parser->cur != NULL) {
    parser_error(parser, "Unexpected input after end of command");
    return NULL;
  }
  return root;
}

void parser_error(parser_t *parser, const char *message) {
  token_t *blame = parser->cur ? parser->cur : parser->prev;
  fprintf(stderr, "tiny: %s\n", message);
  if (blame) {
    fprintf(stderr, "Syntax error at line %u, column %u (near '%s')\n",
            blame->row, blame->column, blame->lexeme);
  }
  parser->had_error = true;
  parser_synchronize(parser);
}

void parser_synchronize(parser_t *parser) {
  advance(parser);
  while (parser->cur) {
    if (parser->prev->type == TOK_SEMI || parser->prev->type == TOK_NEWLINE ||
        parser->prev->type == TOK_AMP) {
      return;
    }

    switch (parser->cur->type) {
      case TOK_ASSIGNMENT_WORD:
      case TOK_NEWLINE:
      case TOK_PIPE:
      case TOK_AMP:
      case TOK_SEMI:
      case TOK_L_PAREN:
      case TOK_AND_IF:
      case TOK_OR_IF: return;
      default: break;
    }

    advance(parser);
  }
}

static void advance(parser_t *parser) {
  parser->prev = parser->cur;
  parser->cur  = next_token(parser->scanner);
}

static bool match(parser_t *parser, token_type_t want) {
  if (parser->cur && parser->cur->type == want) {
    advance(parser);
    return true;
  }
  return false;
}

static token_t *consume(parser_t *parser, token_type_t want,
                        const char *message) {
  if (parser->cur && parser->cur->type == want) {
    advance(parser);
    return parser->prev;
  }
  parser_error(parser, message);
  return NULL;
}

static ast_node_t *parse_list(parser_t *parser) {
  ast_node_t *left = parse_and_or(parser);

  while (match(parser, TOK_SEMI) || match(parser, TOK_NEWLINE) ||
         match(parser, TOK_AMP)) {
    token_type_t sep = parser->prev->type;
    if (sep == TOK_AMP) {
      ast_node_t *bg = arena_alloc(parser->arena, sizeof(ast_node_t));
      if (!bg) {
        parser_error(parser, "Out of memory (parse_list)");
        return NULL;
      }
      bg->type               = AST_BACKGROUND;
      bg->u.background.child = left;
      left                   = bg;
    }

    ast_node_t *right = parse_and_or(parser);

    ast_node_t *seq = arena_alloc(parser->arena, sizeof(ast_node_t));
    if (!seq) {
      parser_error(parser, "Out of memory (parse_list)");
      return NULL;
    }
    seq->type           = AST_SEQUENCE;
    seq->u.binary.left  = left;
    seq->u.binary.right = right;
    left                = seq;
  }

  return left;
}

static ast_node_t *parse_and_or(parser_t *parser) {
  ast_node_t *left = parse_pipeline(parser);

  for (;;) {
    if (match(parser, TOK_AND_IF)) {
      ast_node_t *right = parse_pipeline(parser);

      ast_node_t *node = arena_alloc(parser->arena, sizeof(ast_node_t));
      if (!node) {
        parser_error(parser, "Out of memory (parse_and_or)");
        return NULL;
      }
      node->type           = AST_AND;
      node->u.binary.left  = left;
      node->u.binary.right = right;

      left = node;
      continue;
    } else if (match(parser, TOK_OR_IF)) {
      ast_node_t *right = parse_pipeline(parser);

      ast_node_t *node = arena_alloc(parser->arena, sizeof(ast_node_t));
      if (!node) {
        parser_error(parser, "Out of memory (parser_and_or)");
        return NULL;
      }
      node->type           = AST_OR;
      node->u.binary.left  = left;
      node->u.binary.right = right;

      left = node;
      continue;
    } else {
      break;
    }
  }

  return left;
}

static ast_node_t *parse_pipeline(parser_t *parser) {
  ast_node_t *first = parse_command(parser);

  if (!match(parser, TOK_PIPE)) return first;

  vector_t stages;
  vector_init(&stages, sizeof(ast_node_t *), parser->arena);
  vector_push(&stages, &first);

  do {
    ast_node_t *next_stage = parse_command(parser);
    vector_push(&stages, &next_stage);
  } while (match(parser, TOK_PIPE));

  ast_node_t *pipe_node = arena_alloc(parser->arena, sizeof(ast_node_t));
  if (!pipe_node) {
    parser_error(parser, "Out of memory (parse_pipeline)");
    return NULL;
  }
  pipe_node->type              = AST_PIPELINE;
  pipe_node->u.pipeline.stages = stages;

  return pipe_node;
}

static ast_node_t *parse_command(parser_t *parser) {
  if (match(parser, TOK_L_PAREN)) {
    ast_node_t *child = parse_list(parser);
    consume(parser, TOK_R_PAREN, "Expect ')' after subshell");
    ast_node_t *subshell = arena_alloc(parser->arena, sizeof(ast_node_t));
    if (!subshell) {
      parser_error(parser, "Out of memory (parse_command)");
      return NULL;
    }
    subshell->type             = AST_SUBSHELL;
    subshell->u.subshell.child = child;
    return subshell;
  } else {
    return parse_simple(parser);
  }
}

static ast_node_t *parse_simple(parser_t *parser) {
  vector_t assigns;
  vector_t args;
  vector_t redirs;

  vector_init(&assigns, sizeof(ast_assignment_t), parser->arena);
  vector_init(&args, sizeof(char *), parser->arena);
  vector_init(&redirs, sizeof(ast_redir_t), parser->arena);

  ast_node_t *node = arena_alloc(parser->arena, sizeof(ast_node_t));
  if (!node) {
    parser_error(parser, "Out of memory (parse_simple)");
    return NULL;
  }
  node->type             = AST_SIMPLE;
  node->u.simple.assigns = assigns;
  node->u.simple.args    = args;
  node->u.simple.redirs  = redirs;

  while (match(parser, TOK_ASSIGNMENT_WORD)) {
    token_t *tok = parser->prev;

    char *eq = strchr(tok->lexeme, '=');
    if (eq == NULL) continue;
    *eq         = '\0';
    char *name  = tok->lexeme;
    char *value = eq + 1;

    ast_assignment_t assign;
    assign.name  = name;
    assign.value = value;
    vector_push(&node->u.simple.assigns, &assign);
  }

  while (match(parser, TOK_WORD))
    vector_push(&node->u.simple.args, &parser->prev->lexeme);

  if (node->u.simple.assigns.length == 0 && node->u.simple.args.length == 0) {
    parser_error(parser, "Expected command name or assignment");
    return node;
  }

  while (parser->cur && is_redir_tok(parser->cur->type)) {
    token_type_t op = parser->cur->type;
    advance(parser);

    int          fd;
    token_type_t redir_op;
    if (op == TOK_IO_NUMBER) {
      fd       = atoi(parser->prev->lexeme);
      redir_op = parser->cur->type;
      advance(parser);
    } else {
      switch (op) {
        case TOK_LESS:
        case TOK_D_LESS:
        case TOK_LESS_AND:
        case TOK_LESS_GREAT:
        case TOK_D_LESS_DASH: fd = 1; break;
        default: fd = 0; break;
      }
      redir_op = op;
    }

    token_t *target_tok =
        consume(parser, TOK_WORD, "Expected file name after redirection");

    if (!target_tok) break;

    ast_redir_t redir = {
        .type   = map_token_to_redir_type(redir_op),
        .fd     = fd,
        .target = target_tok->lexeme,
    };
    vector_push(&node->u.simple.redirs, &redir);
  }

  return node;
}

static bool is_redir_tok(token_type_t t) {
  switch (t) {
    case TOK_LESS:
    case TOK_GREAT:
    case TOK_D_LESS:
    case TOK_D_GREAT:
    case TOK_LESS_AND:
    case TOK_GREAT_AND:
    case TOK_LESS_GREAT:
    case TOK_D_LESS_DASH:
    case TOK_IO_NUMBER:
    case TOK_CLOBBER: return true;
    default: return false;
  }
}

static ast_redir_type_t map_token_to_redir_type(token_type_t op) {
  switch (op) {
    case TOK_LESS: return REDIR_IN;
    case TOK_GREAT: return REDIR_OUT;
    case TOK_D_LESS: return REDIR_HERE_DOC;
    case TOK_D_GREAT: return REDIR_OUT_APPEND;
    case TOK_LESS_AND: return REDIR_DUP_IN;
    case TOK_GREAT_AND: return REDIR_DUP_OUT;
    case TOK_LESS_GREAT: return REDIR_READWRITE;
    case TOK_D_LESS_DASH: return REDIR_HERE_STRIP;
    case TOK_CLOBBER: return REDIR_CLOBBER;
    default: return REDIR_IN;
  }
}
