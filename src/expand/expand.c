#include <stdio.h>
#include <ctype.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "jash/token.h"
#include "jash/env.h"

static void free_tokens(jash_token_t *toks) {
    free(toks);
}

static char *xstrdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s);
    char *p = (char *)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n + 1);
    return p;
}

static void sbuf_append(char **buf, size_t *pos, size_t *cap, const char *s, size_t slen) {
    if (*pos + slen + 1 > *cap) {
        size_t ncap = (*cap == 0) ? 64 : (*cap * 2);
        while (ncap < *pos + slen + 1) ncap *= 2;
        char *nb = realloc(*buf, ncap);
        if (!nb) return; /* silent fail for now; caller checks */
        *buf = nb;
        *cap = ncap;
    }
    if (*buf) {
        memcpy(*buf + *pos, s, slen);
        *pos += slen;
        (*buf)[*pos] = '\0';
    }
}

typedef struct {
    bool ok;
    char *expanded;
} expand_word_result_t;

static expand_word_result_t expand_word(const jash_token_t *tok,
                                        const jash_env_table_t *env,
                                        int last_status) {
    expand_word_result_t res = { .ok = false, .expanded = NULL };

    if (!tok || tok->lexeme.ptr == NULL) {
        return res;
    }

    /* If not expandable, return a copy of the original */
    if (!tok->can_expand) {
        res.expanded = xstrdup(tok->lexeme.ptr);
        res.ok = (res.expanded != NULL);
        return res;
    }

    char *buf = NULL;
    size_t pos = 0, cap = 0;
    const char *src = tok->lexeme.ptr;
    size_t slen = tok->lexeme.len;

    for (size_t i = 0; i < slen; i++) {
        if (src[i] == '$' && i + 1 < slen) {
            const char *val = NULL;
            size_t consumed = 0;

            if (src[i + 1] == '?') {
                /* $? -> last_status */
                char status_str[16];
                snprintf(status_str, sizeof(status_str), "%d", last_status);
                val = status_str;
                consumed = 2;
            } else if (src[i + 1] == '{') {
                /* ${NAME} -> look for closing } */
                size_t j = i + 2;
                while (j < slen && src[j] != '}') j++;
                if (j < slen) {
                    size_t name_len = j - (i + 2);
                    char *name = (char *)malloc(name_len + 1);
                    if (name) {
                        memcpy(name, src + i + 2, name_len);
                        name[name_len] = '\0';
                        val = jash_env_get(env, name);
                        free(name);
                        consumed = j - i + 1; /* including } */
                    }
                }
            } else if (isalnum((unsigned char)src[i + 1]) || src[i + 1] == '_') {
                /* $NAME -> scan alphanumeric + _ */
                size_t j = i + 1;
                while (j < slen && (isalnum((unsigned char)src[j]) || src[j] == '_')) j++;
                size_t name_len = j - (i + 1);
                char *name = (char *)malloc(name_len + 1);
                if (name) {
                    memcpy(name, src + i + 1, name_len);
                    name[name_len] = '\0';
                    val = jash_env_get(env, name);
                    free(name);
                    consumed = j - i;
                }
            }

            if (val) {
                sbuf_append(&buf, &pos, &cap, val, strlen(val));
                i += consumed - 1;
            } else {
                sbuf_append(&buf, &pos, &cap, "$", 1);
            }
        } else if (src[i] == '~' && (i == 0 || src[i - 1] == '/')) {
            /* ~ at word start or after / -> home directory */
            const char *home = jash_env_get(env, "HOME");
            if (!home) {
                struct passwd *pw = getpwuid(getuid());
                if (pw) home = pw->pw_dir;
            }
            if (home) {
                sbuf_append(&buf, &pos, &cap, home, strlen(home));
            } else {
                sbuf_append(&buf, &pos, &cap, "~", 1);
            }
        } else if (src[i] == '\\' && i + 1 < slen) {
            /* Backslash escape (in double quotes) */
            i++;
            sbuf_append(&buf, &pos, &cap, src + i, 1);
        } else {
            sbuf_append(&buf, &pos, &cap, src + i, 1);
        }
    }

    if (!buf) {
        res.expanded = xstrdup("");
    } else {
        res.expanded = buf;
    }
    res.ok = (res.expanded != NULL);
    return res;
}

typedef struct jash_expand_result {
    bool ok;
    jash_token_stream_t stream;
    jash_error_t error;
} jash_expand_result_t;

jash_expand_result_t jash_expand(const jash_token_stream_t *in,
                                 const jash_env_table_t *env,
                                 int last_status) {
    jash_expand_result_t r;
    memset(&r, 0, sizeof(r));

    if (!in) {
        r.ok = false;
        r.error = (jash_error_t){
            .severity = JASH_ERR_FATAL,
            .domain = JASH_DOM_EXPAND,
            .code = 1,
            .message = "NULL token stream"
        };
        return r;
    }

    r.ok = true;
    r.stream.original_input = in->original_input;
    r.stream.complete = in->complete;
    r.stream.count = in->count;
    r.stream.tokens = NULL;

    if (in->count == 0) {
        return r;
    }

    r.stream.tokens = (jash_token_t *)calloc(in->count, sizeof(jash_token_t));
    if (!r.stream.tokens) {
        r.ok = false;
        r.error = (jash_error_t){
            .severity = JASH_ERR_FATAL,
            .domain = JASH_DOM_INTERNAL,
            .code = 2,
            .message = "out of memory"
        };
        return r;
    }

    for (size_t i = 0; i < in->count; i++) {
        const jash_token_t *in_tok = &in->tokens[i];

        if (in_tok->type != JASH_TOK_WORD) {
            r.stream.tokens[i] = *in_tok;
            continue;
        }

        expand_word_result_t ew = expand_word(in_tok, env, last_status);
        if (!ew.ok) {
            r.ok = false;
            r.error = (jash_error_t){
                .severity = JASH_ERR_FATAL,
                .domain = JASH_DOM_INTERNAL,
                .code = 3,
                .message = "expansion failed"
            };
            free_tokens(r.stream.tokens);
            r.stream.tokens = NULL;
            return r;
        }

        /* Create a new token with expanded content */
        r.stream.tokens[i] = (jash_token_t){
            .type = JASH_TOK_WORD,
            .lexeme = (jash_strview_t){ .ptr = ew.expanded, .len = strlen(ew.expanded) },
            .span = in_tok->span,
            .qmode = JASH_QM_NONE,
            .can_expand = false
        };
    }

    return r;
}
