#ifndef JASH_TOKEN_H
#define JASH_TOKEN_H

#include <stdbool.h>
#include <stddef.h>
#include "jash/error.h"

typedef enum jash_quote_mode {
    JASH_QM_NONE = 0,
    JASH_QM_SINGLE,
    JASH_QM_DOUBLE
} jash_quote_mode_t;

typedef enum jash_token_type {
    JASH_TOK_WORD = 1,
    JASH_TOK_WS,
    JASH_TOK_EOL
} jash_token_type_t;

typedef struct jash_strview {
    const char *ptr;  /* may be owned by token; freed by jash_token_stream_free */
    size_t len;
} jash_strview_t;

typedef struct jash_token {
    jash_token_type_t type;
    jash_strview_t lexeme;     /* content (quotes stripped); may be malloc'd */
    jash_span_t span;          /* span in original input */
    jash_quote_mode_t qmode;   /* how it was formed */
    bool can_expand;           /* false for single-quoted */
} jash_token_t;

typedef struct jash_token_stream {
    const char *original_input; /* borrowed from input layer */
    jash_token_t *tokens;       /* owned by stream; lexemes are malloc'd */
    size_t count;
    bool complete;              /* false if input incomplete (open quote) */
} jash_token_stream_t;

typedef struct jash_lex_result {
    bool ok;
    jash_token_stream_t stream;
    jash_error_t error;
} jash_lex_result_t;

jash_lex_result_t jash_lex(const char *input_line);
void jash_token_stream_free(jash_token_stream_t *s);

#endif /* JASH_TOKEN_H */
