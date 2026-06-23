#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "jash/builtins.h"

jash_builtin_result_t jash_builtin_pwd(size_t argc, char *const argv[], jash_shell_state_t *sh) {
    (void)argc;
    (void)argv;
    (void)sh;

    char buf[PATH_MAX];
    if (!getcwd(buf, sizeof(buf))) {
        return (jash_builtin_result_t){
            .ok = false,
            .error = (jash_error_t){
                .severity = JASH_ERR_RECOVERABLE,
                .domain = JASH_DOM_EXEC,
                .code = 100,
                .sys_errno = errno,
                .context = "pwd",
                .message = strerror(errno)
            }
        };
    }

    puts(buf);
    return (jash_builtin_result_t){ .ok = true, .status = 0 };
}
