#include <stdlib.h>
#include <string.h>
#include "jash/exec.h"

static jash_builtin_id_t builtin_from_name(const char *s) {
    if (!s) return JASH_BI_NONE;
    if (strcmp(s, "cd") == 0) return JASH_BI_CD;
    if (strcmp(s, "exit") == 0) return JASH_BI_EXIT;
    if (strcmp(s, "pwd") == 0) return JASH_BI_PWD;
    if (strcmp(s, "echo") == 0) return JASH_BI_ECHO;
    if (strcmp(s, "env") == 0) return JASH_BI_ENV;
    if (strcmp(s, "export") == 0) return JASH_BI_EXPORT;
    if (strcmp(s, "unset") == 0) return JASH_BI_UNSET;
    return JASH_BI_NONE;
}

void jash_exec_plan_free(jash_exec_plan_t *p) {
    if (!p) return;
    free(p->resolved_path);
    p->resolved_path = NULL;
}

jash_plan_result_t jash_plan(const jash_command_t *cmd) {
    jash_plan_result_t r;
    memset(&r, 0, sizeof(r));

    if (!cmd || cmd->argc == 0 || !cmd->argv || !cmd->argv[0]) {
        r.ok = false;
        r.error = (jash_error_t){
            .severity = JASH_ERR_FATAL,
            .domain = JASH_DOM_PLAN,
            .code = 1,
            .message = "invalid command"
        };
        return r;
    }

    r.ok = true;
    r.plan.argv = cmd->argv;
    r.plan.argc = cmd->argc;
    r.plan.span = cmd->span;

    jash_builtin_id_t bi = builtin_from_name(cmd->argv[0]);
    if (bi != JASH_BI_NONE) {
        r.plan.kind = JASH_EXEC_BUILTIN;
        r.plan.builtin_id = bi;
        r.plan.resolved_path = NULL;
        return r;
    }

    /* External resolution is milestone later */
    r.plan.kind = JASH_EXEC_EXTERNAL;
    r.plan.builtin_id = JASH_BI_NONE;
    r.plan.resolved_path = NULL;
    return r;
}
