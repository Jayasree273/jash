#include "jash/builtins.h"

jash_builtin_result_t jash_builtin_unset(size_t argc, char *const argv[], jash_shell_state_t *sh) {
    if (!sh || !sh->env) {
        return (jash_builtin_result_t){
            .ok = false,
            .error = (jash_error_t){
                .severity = JASH_ERR_FATAL,
                .domain = JASH_DOM_EXEC,
                .code = 150,
                .context = "unset",
                .message = "no environment table"
            }
        };
    }

    if (argc < 2) {
        return (jash_builtin_result_t){ .ok = true, .status = 0 };
    }

    /* Unset each variable in argv[1..] */
    for (size_t i = 1; i < argc; i++) {
        jash_env_result_t er = jash_env_unset(sh->env, argv[i]);
        if (!er.ok) {
            return (jash_builtin_result_t){ .ok = false, .error = er.error };
        }
    }

    return (jash_builtin_result_t){ .ok = true, .status = 0 };
}
