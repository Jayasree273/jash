#include <stdlib.h>
#include <string.h>
#include "jash/command.h"

static char *dup_strview(jash_strview_t sv) {
    char *s = (char *)malloc(sv.len + 1);
    if (!s) return NULL;
    memcpy(s, sv.ptr, sv.len);
    s[sv.len] = '\0';
    return s;
}

void jash_command_free(jash_command_t *cmd) {
    if (!cmd) return;
    for (size_t i = 0; i < cmd->argc; i++) free(cmd->argv[i]);
    free(cmd->argv);
    cmd->argv = NULL;
    cmd->argc = 0;

    for (size_t i = 0; i < cmd->asgn_count; i++) {
        free(cmd->asgn[i].name);
        free(cmd->asgn[i].value);
    }
    free(cmd->asgn);
    cmd->asgn = NULL;
    cmd->asgn_count = 0;

    for (size_t i = 0; i < cmd->redir_count; i++) {
        free(cmd->redirs[i].target);
    }
    free(cmd->redirs);
    cmd->redirs = NULL;
    cmd->redir_count = 0;
}

jash_parse_result_t jash_parse(const jash_token_stream_t *ts) {
    jash_parse_result_t r;
    memset(&r, 0, sizeof(r));

    if (!ts) {
        r.kind = JASH_PARSE_ERROR;
        r.error = (jash_error_t){
            .severity = JASH_ERR_FATAL,
            .domain = JASH_DOM_PARSE,
            .code = 1,
            .message = "NULL token stream"
        };
        return r;
    }

    /* First pass: count argv and redirections (skip redir tokens) */
    size_t words = 0;
    size_t redir_count = 0;
    for (size_t i = 0; i < ts->count; i++) {
        if (ts->tokens[i].type != JASH_TOK_WORD) continue;

        const char *lex = ts->tokens[i].lexeme.ptr;

        if ((strcmp(lex, ">") == 0 || strcmp(lex, ">>") == 0 || strcmp(lex, "<") == 0) &&
            i + 1 < ts->count && ts->tokens[i + 1].type == JASH_TOK_WORD) {
            redir_count++;
            i++; /* skip target */
        } else if (strcmp(lex, ">") != 0 && strcmp(lex, ">>") != 0 && strcmp(lex, "<") != 0) {
            words++;
        }
    }

    if (words == 0) {
        r.kind = JASH_PARSE_EMPTY;
        return r;
    }

    r.kind = JASH_PARSE_OK;
    r.cmd.argv = (char **)calloc(words + 1, sizeof(char *));
    r.cmd.redirs = (jash_redir_t *)calloc(redir_count, sizeof(jash_redir_t));

    if (!r.cmd.argv || (redir_count > 0 && !r.cmd.redirs)) {
        jash_command_free(&r.cmd);
        r.kind = JASH_PARSE_ERROR;
        r.error = (jash_error_t){
            .severity = JASH_ERR_FATAL,
            .domain = JASH_DOM_INTERNAL,
            .code = 2,
            .message = "out of memory"
        };
        return r;
    }

    /* Second pass: populate argv and redirs */
    size_t ai = 0;
    size_t ri = 0;
    for (size_t i = 0; i < ts->count; i++) {
        if (ts->tokens[i].type != JASH_TOK_WORD) continue;

        const char *lex = ts->tokens[i].lexeme.ptr;

        if (strcmp(lex, ">") == 0 && i + 1 < ts->count && ts->tokens[i + 1].type == JASH_TOK_WORD) {
            r.cmd.redirs[ri].type = 1; /* JASH_REDIR_OUT */
            r.cmd.redirs[ri].fd = 1;
            r.cmd.redirs[ri].target = dup_strview(ts->tokens[i + 1].lexeme);
            if (!r.cmd.redirs[ri].target) {
                jash_command_free(&r.cmd);
                r.kind = JASH_PARSE_ERROR;
                r.error = (jash_error_t){
                    .severity = JASH_ERR_FATAL,
                    .domain = JASH_DOM_INTERNAL,
                    .code = 3,
                    .message = "out of memory"
                };
                return r;
            }
            ri++;
            i++;
        } else if (strcmp(lex, ">>") == 0 && i + 1 < ts->count && ts->tokens[i + 1].type == JASH_TOK_WORD) {
            r.cmd.redirs[ri].type = 2; /* JASH_REDIR_OUT_APPEND */
            r.cmd.redirs[ri].fd = 1;
            r.cmd.redirs[ri].target = dup_strview(ts->tokens[i + 1].lexeme);
            if (!r.cmd.redirs[ri].target) {
                jash_command_free(&r.cmd);
                r.kind = JASH_PARSE_ERROR;
                r.error = (jash_error_t){
                    .severity = JASH_ERR_FATAL,
                    .domain = JASH_DOM_INTERNAL,
                    .code = 4,
                    .message = "out of memory"
                };
                return r;
            }
            ri++;
            i++;
        } else if (strcmp(lex, "<") == 0 && i + 1 < ts->count && ts->tokens[i + 1].type == JASH_TOK_WORD) {
            r.cmd.redirs[ri].type = 3; /* JASH_REDIR_IN */
            r.cmd.redirs[ri].fd = 0;
            r.cmd.redirs[ri].target = dup_strview(ts->tokens[i + 1].lexeme);
            if (!r.cmd.redirs[ri].target) {
                jash_command_free(&r.cmd);
                r.kind = JASH_PARSE_ERROR;
                r.error = (jash_error_t){
                    .severity = JASH_ERR_FATAL,
                    .domain = JASH_DOM_INTERNAL,
                    .code = 5,
                    .message = "out of memory"
                };
                return r;
            }
            ri++;
            i++;
        } else {
            /* Regular argument (not a redir operator) */
            char *arg = dup_strview(ts->tokens[i].lexeme);
            if (!arg) {
                jash_command_free(&r.cmd);
                r.kind = JASH_PARSE_ERROR;
                r.error = (jash_error_t){
                    .severity = JASH_ERR_FATAL,
                    .domain = JASH_DOM_INTERNAL,
                    .code = 6,
                    .message = "out of memory"
                };
                return r;
            }
            r.cmd.argv[ai++] = arg;
        }
    }

    r.cmd.argv[ai] = NULL;
    r.cmd.argc = ai;
    r.cmd.asgn = NULL;
    r.cmd.asgn_count = 0;
    r.cmd.redir_count = ri;
    r.cmd.span = jash_span_make(0, 0);
    return r;
}
