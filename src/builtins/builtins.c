#include <string.h>
#include "jash/builtins.h"

/* forward declarations */
jash_builtin_result_t jash_builtin_pwd(size_t argc, char *const argv[], jash_shell_state_t *sh);
jash_builtin_result_t jash_builtin_cd(size_t argc, char *const argv[], jash_shell_state_t *sh);
jash_builtin_result_t jash_builtin_exit(size_t argc, char *const argv[], jash_shell_state_t *sh);
jash_builtin_result_t jash_builtin_env(size_t argc, char *const argv[], jash_shell_state_t *sh);
jash_builtin_result_t jash_builtin_export(size_t argc, char *const argv[], jash_shell_state_t *sh);
jash_builtin_result_t jash_builtin_unset(size_t argc, char *const argv[], jash_shell_state_t *sh);
jash_builtin_result_t jash_builtin_echo(size_t argc, char *const argv[], jash_shell_state_t *sh);

static jash_builtin_result_t ok_status(int st) {
    return (jash_builtin_result_t){ .ok = true, .status = st };
}


jash_builtin_result_t jash_builtin_run(jash_builtin_id_t id,
                                       size_t argc,
                                       char *const argv[],
                                       jash_shell_state_t *sh) {
    switch (id) {
        case JASH_BI_PWD:    return jash_builtin_pwd(argc, argv, sh);
        case JASH_BI_CD:     return jash_builtin_cd(argc, argv, sh);
        case JASH_BI_EXIT:   return jash_builtin_exit(argc, argv, sh);
        case JASH_BI_ENV:    return jash_builtin_env(argc, argv, sh);
        case JASH_BI_EXPORT: return jash_builtin_export(argc, argv, sh);
        case JASH_BI_UNSET:  return jash_builtin_unset(argc, argv, sh);
        case JASH_BI_ECHO:   return jash_builtin_echo(argc, argv, sh);

        case JASH_BI_NONE:
        default:
            return ok_status(2);
    }
}
