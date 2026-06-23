#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "jash/token.h"
#include "jash/command.h"
#include "jash/exec.h"
#include "jash/env.h"
#include "jash/builtins.h"

/* Temporary prototype until we add include/jash/expand.h */
typedef struct jash_expand_result {
    bool ok;
    jash_token_stream_t stream;
    jash_error_t error;
} jash_expand_result_t;

jash_expand_result_t jash_expand(const jash_token_stream_t *in,
                                 const jash_env_table_t *env,
                                 int last_status);

static void print_error(const jash_error_t *e) {
    const char *ctx = (e->context) ? e->context : "";
    if (ctx[0]) fprintf(stderr, "jash: %s: %s\n", ctx, e->message);
    else fprintf(stderr, "jash: %s\n", e->message);
}

int main(void) {
    jash_env_table_t env;
    jash_env_init(&env);

    int last_status = 0;
    bool should_exit = false;
    int exit_code = 0;

    char *line = NULL;
    size_t cap = 0;

    while (1) {
        fputs("jash$ ", stdout);
        fflush(stdout);

        ssize_t nread = getline(&line, &cap, stdin);
        if (nread < 0) {
            fputs("\n", stdout);
            break;
        }

        /* trim newline */
        if (nread > 0 && line[nread - 1] == '\n') line[nread - 1] = '\0';

        /* Lex */
        jash_lex_result_t lx = jash_lex(line);
        if (!lx.ok) {
            print_error(&lx.error);
            if (lx.error.severity == JASH_ERR_FATAL) break;
            continue;
        }

        /* Expand (no-op for now) */
        jash_expand_result_t ex = jash_expand(&lx.stream, &env, last_status);
        if (!ex.ok) {
            print_error(&ex.error);
            jash_token_stream_free(&lx.stream);
            if (ex.error.severity == JASH_ERR_FATAL) break;
            continue;
        }

        /* Parse */
        jash_parse_result_t pr = jash_parse(&ex.stream);
        if (pr.kind == JASH_PARSE_ERROR) {
            print_error(&pr.error);
            jash_token_stream_free(&ex.stream);
            jash_token_stream_free(&lx.stream);
            if (pr.error.severity == JASH_ERR_FATAL) break;
            continue;
        }
        if (pr.kind == JASH_PARSE_EMPTY) {
            jash_token_stream_free(&ex.stream);
            jash_token_stream_free(&lx.stream);
            continue;
        }

        /* Plan */
        jash_plan_result_t pl = jash_plan(&pr.cmd);
        if (!pl.ok) {
            print_error(&pl.error);
            jash_command_free(&pr.cmd);
            jash_token_stream_free(&ex.stream);
            jash_token_stream_free(&lx.stream);
            if (pl.error.severity == JASH_ERR_FATAL) break;
            continue;
        }

        /* Execute */
        jash_shell_state_t sh = {
            .env = &env,
            .last_status = &last_status,
            .should_exit = &should_exit,
            .exit_code = &exit_code
        };

        jash_exec_result_t rr = jash_exec_run(&pl.plan, &sh, pr.cmd.redirs, pr.cmd.redir_count);
        if (rr.kind == JASH_RES_FAILED_TO_START) {
            print_error(&rr.error);
            last_status = 127;
        } else if (rr.kind == JASH_RES_SIGNALED) {
            last_status = rr.shell_status;
        } else {
            last_status = rr.exit_code;
        }

        jash_exec_plan_free(&pl.plan);
        jash_command_free(&pr.cmd);
        jash_token_stream_free(&ex.stream);
        jash_token_stream_free(&lx.stream);

        if (should_exit) break;
    }

    free(line);
    jash_env_free(&env);
    return should_exit ? exit_code : 0;
}
