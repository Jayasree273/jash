#include <stdio.h>
#include "jash/builtins.h"

jash_builtin_result_t jash_builtin_env(size_t argc, char *const argv[], jash_shell_state_t *sh) {
    (void)argc;
    (void)argv;

    if (!sh || !sh->env) {
        return (jash_builtin_result_t){
            .ok = false,
            .error = (jash_error_t){
                .severity = JASH_ERR_FATAL,
                .domain = JASH_DOM_EXEC,
                .code = 130,
                .context = "env",
                .message = "no environment table"
            }
        };
    }

    /* Print exported variables only */
    for (size_t i = 0; i < sh->env->count; i++) {
        if (sh->env->entries[i].exported) {
            printf("%s=%s\n", sh->env->entries[i].name, sh->env->entries[i].value);
        }
    }

    return (jash_builtin_result_t){ .ok = true, .status = 0 };
}
