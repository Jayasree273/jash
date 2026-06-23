#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "jash/token.h"

static void free_tokens(jash_token_t *toks, size_t count) {
    if (!toks) return;
    for (size_t i = 0; i < count; i++) {
        if (toks[i].type == JASH_TOK_WORD && toks[i].lexeme.ptr) {
            free((void *)toks[i].lexeme.ptr);
        }
    }
    free(toks);
}

void jash_token_stream_free(jash_token_stream_t *s) {
    if (!s) return;
    free_tokens(s->tokens, s->count);
    s->tokens = NULL;
    s->count = 0;
    s->original_input = NULL;
    s->complete = true;
}

typedef struct {
    char *buf;
    size_t pos;
    size_t cap;
} strbuf_t;

static void sbuf_init(strbuf_t *sb) {
    sb->buf = NULL;
    sb->pos = 0;
    sb->cap = 0;
}

static void sbuf_append(strbuf_t *sb, char c) {
    if (sb->pos + 1 >= sb->cap) {
        size_t ncap = (sb->cap == 0) ? 32 : (sb->cap * 2);
        char *nb = realloc(sb->buf, ncap);
        if (!nb) return;
        sb->buf = nb;
        sb->cap = ncap;
    }
    if (sb->buf) {
        sb->buf[sb->pos++] = c;
        sb->buf[sb->pos] = '\0';
    }
}

static char *sbuf_finish(strbuf_t *sb) {
    return sb->buf;
}

jash_lex_result_t jash_lex(const char *input_line) {
    jash_lex_result_t r;
    memset(&r, 0, sizeof(r));

    if (!input_line) {
        r.ok = false;
        r.error = (jash_error_t){
            .severity = JASH_ERR_FATAL,
            .domain = JASH_DOM_LEX,
            .code = 1,
            .message = "NULL input_line"
        };
        return r;
    }

    r.ok = true;
    r.stream.original_input = input_line;
    r.stream.complete = true;

    size_t n = strlen(input_line);
    size_t cap = 8;
    size_t count = 0;
    jash_token_t *tokens = calloc(cap, sizeof(*tokens));
    if (!tokens) {
        r.ok = false;
        r.error = (jash_error_t){
            .severity = JASH_ERR_FATAL,
            .domain = JASH_DOM_INTERNAL,
            .code = 2,
            .message = "out of memory"
        };
        return r;
    }

    size_t i = 0;
    while (i < n) {
        while (i < n && isspace((unsigned char)input_line[i])) i++;
        if (i >= n) break;

        size_t lexeme_start = i;
        strbuf_t word_buf;
        sbuf_init(&word_buf);

        bool can_expand = true;
        bool has_quote = false;
        bool unclosed_quote = false;

        while (i < n && !isspace((unsigned char)input_line[i])) {
            char c = input_line[i];

            if (c == '\'') {
                has_quote = true;
                can_expand = false;
                i++;
                while (i < n && input_line[i] != '\'') {
                    sbuf_append(&word_buf, input_line[i]);
                    i++;
                }
                if (i < n && input_line[i] == '\'') {
                    i++;
                } else {
                    unclosed_quote = true;
                    break;
                }
            } else if (c == '"') {
                has_quote = true;
                can_expand = true;
                i++;
                while (i < n && input_line[i] != '"') {
                    if (input_line[i] == '\\' && i + 1 < n) {
                        sbuf_append(&word_buf, input_line[i]);
                        sbuf_append(&word_buf, input_line[i + 1]);
                        i += 2;
                    } else {
                        sbuf_append(&word_buf, input_line[i]);
                        i++;
                    }
                }
                if (i < n && input_line[i] == '"') {
                    i++;
                } else {
                    unclosed_quote = true;
                    break;
                }
            } else if (c == '\\' && i + 1 < n) {
                i++;
                sbuf_append(&word_buf, input_line[i]);
                i++;
            } else {
                sbuf_append(&word_buf, c);
                i++;
            }
        }

        if (unclosed_quote) {
            r.stream.complete = false;
        }

        char *word_content = sbuf_finish(&word_buf);
        if (!word_content) {
            free_tokens(tokens, count);
            r.ok = false;
            r.error = (jash_error_t){
                .severity = JASH_ERR_FATAL,
                .domain = JASH_DOM_INTERNAL,
                .code = 3,
                .message = "out of memory"
            };
            return r;
        }

        if (count + 2 > cap) {
            cap *= 2;
            jash_token_t *nt = realloc(tokens, cap * sizeof(*tokens));
            if (!nt) {
                free(word_content);
                free_tokens(tokens, count);
                r.ok = false;
                r.error = (jash_error_t){
                    .severity = JASH_ERR_FATAL,
                    .domain = JASH_DOM_INTERNAL,
                    .code = 4,
                    .message = "out of memory"
                };
                return r;
            }
            tokens = nt;
        }

        tokens[count++] = (jash_token_t){
            .type = JASH_TOK_WORD,
            .lexeme = (jash_strview_t){ .ptr = word_content, .len = strlen(word_content) },
            .span = jash_span_make(lexeme_start, i),
            .qmode = has_quote ? (can_expand ? JASH_QM_DOUBLE : JASH_QM_SINGLE) : JASH_QM_NONE,
            .can_expand = can_expand
        };
    }

    if (count + 1 > cap) {
        cap++;
        jash_token_t *nt = realloc(tokens, cap * sizeof(*tokens));
        if (!nt) {
            free_tokens(tokens, count);
            r.ok = false;
            r.error = (jash_error_t){
                .severity = JASH_ERR_FATAL,
                .domain = JASH_DOM_INTERNAL,
                .code = 5,
                .message = "out of memory"
            };
            return r;
        }
        tokens = nt;
    }

    tokens[count++] = (jash_token_t){
        .type = JASH_TOK_EOL,
        .lexeme = (jash_strview_t){ .ptr = "", .len = 0 },
        .span = jash_span_make(n, n),
        .qmode = JASH_QM_NONE,
        .can_expand = false
    };

    r.stream.tokens = tokens;
    r.stream.count = count;
    return r;
}
