#ifndef JASH_EXEC_H
#define JASH_EXEC_H

#include <stdbool.h>
#include <stddef.h>
#include "jash/error.h"
#include "jash/command.h"
#include "jash/builtins.h"


typedef enum jash_exec_kind {
    JASH_EXEC_BUILTIN = 1,
    JASH_EXEC_EXTERNAL
} jash_exec_kind_t;

typedef struct jash_exec_plan {
    jash_exec_kind_t kind;
    jash_builtin_id_t builtin_id; /* if builtin */
    char *resolved_path;          /* owned if external, else NULL */
    char **argv;                  /* borrowed from command for now */
    size_t argc;
    jash_span_t span;
} jash_exec_plan_t;

typedef struct jash_plan_result {
    bool ok;
    jash_exec_plan_t plan;
    jash_error_t error;
} jash_plan_result_t;

typedef enum jash_result_kind {
    JASH_RES_EXITED = 1,
    JASH_RES_SIGNALED,
    JASH_RES_FAILED_TO_START
} jash_result_kind_t;

typedef struct jash_exec_result {
    jash_result_kind_t kind;
    int exit_code;       /* if exited */
    int signal_number;   /* if signaled */
    int shell_status;    /* recommended 128+signal */
    jash_error_t error;  /* if failed to start */
} jash_exec_result_t;

/* Planner + executor APIs (stubbed initially) */
jash_plan_result_t jash_plan(const jash_command_t *cmd);
void jash_exec_plan_free(jash_exec_plan_t *p);

jash_exec_result_t jash_exec_run(const jash_exec_plan_t *plan, jash_shell_state_t *sh,
                                 const jash_redir_t *redirs, size_t redir_count);

#endif /* JASH_EXEC_H */
