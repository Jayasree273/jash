#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "jash/builtins.h"

static int parse_code(const char *s, bool *ok) {
    *ok = false;
    if (!s || !*s) return 0;

    errno = 0;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') return 0;

    /* shell exit codes are typically 0..255 */
    if (v < 0) v = 256 + (v % 256);
    v = v % 256;

    *ok = true;
    return (int)v;
}

jash_builtin_result_t jash_builtin_exit(size_t argc, char *const argv[], jash_shell_state_t *sh) {
    int code = 0;

    if (argc >= 2) {
        bool ok = false;
        code = parse_code(argv[1], &ok);
        if (!ok) {
            return (jash_builtin_result_t){
                .ok = false,
                .error = (jash_error_t){
                    .severity = JASH_ERR_RECOVERABLE,
                    .domain = JASH_DOM_EXEC,
                    .code = 120,
                    .context = "exit",
                    .message = "numeric argument required"
                }
            };
        }
    } else if (sh && sh->last_status) {
        code = (*sh->last_status) & 0xFF;
    }

    if (sh && sh->should_exit) *sh->should_exit = true;
    if (sh && sh->exit_code) *sh->exit_code = code;

    return (jash_builtin_result_t){ .ok = true, .status = code };
}
