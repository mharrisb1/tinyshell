#ifndef AST_H
#define AST_H

#include <stddef.h>
typedef enum {
  AST_SIMPLE,     // a bare command (with assignments, argv[], redirs[])
  AST_PIPELINE,   // cmd1 | cmd2 | ... (vector of AST_SIMPLE or AST_SUBSHELL)
  AST_SEQUENCE,   // left ; right
  AST_AND,        // left && right
  AST_OR,         // left || right
  AST_BACKGROUND, // pipeline & (run in background)
  AST_SUBSHELL    // ( list )
} ast_type_t;

/* var=val */
typedef struct {
  char *name;
  char *value;
} ast_assignment_t;

typedef enum {
  REDIR_IN,         // < file
  REDIR_OUT,        // > file
  REDIR_OUT_APPEND, // >> file
  REDIR_HERE_DOC,   // << word
  REDIR_HERE_STRIP, // <<- word
  REDIR_DUP_IN,     // <&n
  REDIR_DUP_OUT,    // >&n
  REDIR_READWRITE,  // <> file
  REDIR_CLOBBER     // >| file
} ast_redir_type_t;

typedef struct {
  ast_redir_type_t type;
  int              fd;
  char            *target;
} ast_redir_t;

typedef struct ast_node_t ast_node_t;

struct ast_node_t {
  ast_type_t type;
  union {
    // AST_SIMPLE
    struct {
      ast_assignment_t *assigns;
      size_t            n_assigns;
      char            **args;
      size_t            n_args;
      ast_redir_t      *redirs;
      size_t            n_redirs;
    } simple;

    // AST_PIPELINE
    struct {
      ast_node_t **stages;
      size_t       n_stages;
    } pipeline;

    // AST_SEQUENCE, AST_AND, AST_OR
    struct {
      ast_node_t *left;
      ast_node_t *right;
    } binary;

    // AST_BACKGROUND
    struct {
      ast_node_t *child;
    } background;

    // AST_SUBSHELL
    struct {
      ast_node_t *child;
    } subshell;
  } u;
};

void ast_dump(ast_node_t *root);

#endif // AST_H
