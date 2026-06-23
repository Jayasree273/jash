#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "jash/builtins.h"

static const char *get_home(jash_shell_state_t *sh) {
    if (!sh || !sh->env) return NULL;
    return jash_env_get(sh->env, "HOME");
}

jash_builtin_result_t jash_builtin_cd(size_t argc, char *const argv[], jash_shell_state_t *sh) {
    const char *target = NULL;

    if (argc < 2) {
        target = get_home(sh);
        if (!target) {
            return (jash_builtin_result_t){
                .ok = false,
                .error = (jash_error_t){
                    .severity = JASH_ERR_RECOVERABLE,
                    .domain = JASH_DOM_EXEC,
                    .code = 110,
                    .context = "cd",
                    .message = "HOME not set"
                }
            };
        }
    } else {
        target = argv[1];
    }

    if (chdir(target) != 0) {
        return (jash_builtin_result_t){
            .ok = false,
            .error = (jash_error_t){
                .severity = JASH_ERR_RECOVERABLE,
                .domain = JASH_DOM_EXEC,
                .code = 111,
                .sys_errno = errno,
                .context = "cd",
                .message = strerror(errno)
            }
        };
    }

    return (jash_builtin_result_t){ .ok = true, .status = 0 };
}
