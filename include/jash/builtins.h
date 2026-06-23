#ifndef JASH_BUILTINS_H
#define JASH_BUILTINS_H

#include <stdbool.h>
#include <stddef.h>

#include "jash/error.h"
#include "jash/env.h"

/* Builtin IDs live here to avoid header cycles. */
typedef enum jash_builtin_id {
    JASH_BI_NONE = 0,
    JASH_BI_CD,
    JASH_BI_EXIT,
    JASH_BI_PWD,
    JASH_BI_ECHO,
    JASH_BI_ENV,
    JASH_BI_EXPORT,
    JASH_BI_UNSET
} jash_builtin_id_t;

/* Builtin call result:
   - ok=true => status is valid
   - ok=false => error is valid */
typedef struct jash_builtin_result {
    bool ok;
    int status;
    jash_error_t error;
} jash_builtin_result_t;

/* Shell state accessible to builtins (minimal for v1). */
typedef struct jash_shell_state {
    jash_env_table_t *env;
    int *last_status;     /* may be NULL */
    bool *should_exit;    /* set true by exit builtin */
    int *exit_code;       /* set by exit builtin */
} jash_shell_state_t;

/* Dispatch builtin by id. */
jash_builtin_result_t jash_builtin_run(jash_builtin_id_t id,
                                       size_t argc,
                                       char *const argv[],
                                       jash_shell_state_t *sh);

#endif /* JASH_BUILTINS_H */
