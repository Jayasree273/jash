#include <stdio.h>
#include "jash/builtins.h"

jash_builtin_result_t jash_builtin_echo(size_t argc, char *const argv[], jash_shell_state_t *sh) {
    (void)sh;

    for (size_t i = 1; i < argc; i++) {
        fputs(argv[i], stdout);
        if (i + 1 < argc) fputc(' ', stdout);
    }
    fputc('\n', stdout);

    return (jash_builtin_result_t){ .ok = true, .status = 0 };
}
