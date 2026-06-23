#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include "jash/builtins.h"

jash_builtin_result_t jash_builtin_export(size_t argc, char *const argv[], jash_shell_state_t *sh) {
    if (!sh || !sh->env) {
        return (jash_builtin_result_t){
            .ok = false,
            .error = (jash_error_t){
                .severity = JASH_ERR_FATAL,
                .domain = JASH_DOM_EXEC,
                .code = 140,
                .context = "export",
                .message = "no environment table"
            }
        };
    }

    if (argc < 2) {
        /* export with no args: print exported vars (like env) */
        for (size_t i = 0; i < sh->env->count; i++) {
            if (sh->env->entries[i].exported) {
                printf("export %s=%s\n", sh->env->entries[i].name, sh->env->entries[i].value);
            }
        }
        return (jash_builtin_result_t){ .ok = true, .status = 0 };
    }

    /* Process each argument */
    for (size_t i = 1; i < argc; i++) {
        const char *arg = argv[i];
        const char *eq = strchr(arg, '=');

        if (eq) {
            /* export NAME=value */
            size_t name_len = eq - arg;
            char *name = (char *)malloc(name_len + 1);
            if (!name) {
                return (jash_builtin_result_t){
                    .ok = false,
                    .error = (jash_error_t){
                        .severity = JASH_ERR_FATAL,
                        .domain = JASH_DOM_INTERNAL,
                        .code = 141,
                        .message = "out of memory"
                    }
                };
            }
            strncpy(name, arg, name_len);
            name[name_len] = '\0';

            const char *value = eq + 1;
            jash_env_result_t er = jash_env_set(sh->env, name, value, true);
            free(name);

            if (!er.ok) {
                return (jash_builtin_result_t){ .ok = false, .error = er.error };
            }
        } else {
            /* export NAME (mark existing, or create empty if not found) */
            const char *val = jash_env_get(sh->env, arg);
            if (val) {
                /* Already exists: just mark as exported */
                jash_env_result_t er = jash_env_set(sh->env, arg, val, true);
                if (!er.ok) {
                    return (jash_builtin_result_t){ .ok = false, .error = er.error };
                }
            } else {
                /* Does not exist: create with empty value and export (Option 1) */
                jash_env_result_t er = jash_env_set(sh->env, arg, "", true);
                if (!er.ok) {
                    return (jash_builtin_result_t){ .ok = false, .error = er.error };
                }
            }
        }
    }

    return (jash_builtin_result_t){ .ok = true, .status = 0 };
}
