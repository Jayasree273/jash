#ifndef JASH_COMMAND_H
#define JASH_COMMAND_H

#include <stdbool.h>
#include <stddef.h>
#include "jash/error.h"
#include "jash/token.h"

typedef struct jash_assignment {
    char *name;   /* owned */
    char *value;  /* owned */
    bool exported;
} jash_assignment_t;

typedef struct jash_redir {
    int type;           /* 1=>, 2=>>, 3=< */
    char *target;       /* owned; filename */
    int fd;             /* 0=stdin, 1=stdout, 2=stderr */
} jash_redir_t;

typedef struct jash_command {
    char **argv;              /* owned strings */
    size_t argc;
    jash_assignment_t *asgn;  /* owned */
    size_t asgn_count;
    jash_redir_t *redirs;     /* owned */
    size_t redir_count;
    jash_span_t span;
} jash_command_t;

typedef enum jash_parse_kind {
    JASH_PARSE_OK = 1,
    JASH_PARSE_EMPTY,
    JASH_PARSE_ERROR
} jash_parse_kind_t;

typedef struct jash_parse_result {
    jash_parse_kind_t kind;
    jash_command_t cmd;   /* valid if OK */
    jash_error_t error;   /* valid if ERROR */
} jash_parse_result_t;

/* Parser API */
jash_parse_result_t jash_parse(const jash_token_stream_t *expanded_tokens);
void jash_command_free(jash_command_t *cmd);

#endif /* JASH_COMMAND_H */
