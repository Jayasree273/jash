#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "jash/exec.h"
#include "jash/builtins.h"

static jash_exec_result_t res_exited(int code) {
    return (jash_exec_result_t){
        .kind = JASH_RES_EXITED,
        .exit_code = code,
        .signal_number = 0,
        .shell_status = code,
    };
}

static jash_exec_result_t res_signaled(int sig) {
    return (jash_exec_result_t){
        .kind = JASH_RES_SIGNALED,
        .exit_code = 0,
        .signal_number = sig,
        .shell_status = 128 + sig,
    };
}

static jash_exec_result_t res_failed(const char *ctx, const char *msg, int sys_errno) {
    return (jash_exec_result_t){
        .kind = JASH_RES_FAILED_TO_START,
        .error = (jash_error_t){
            .severity = JASH_ERR_RECOVERABLE,
            .domain = JASH_DOM_EXEC,
            .code = 1,
            .sys_errno = sys_errno,
            .context = ctx,
            .message = msg,
            .span = {0, 0},
        },
    };
}

static int wait_to_status(pid_t pid, jash_exec_result_t *out) {
    int st = 0;
    if (waitpid(pid, &st, 0) < 0) {
        *out = (jash_exec_result_t){
            .kind = JASH_RES_FAILED_TO_START,
            .error = (jash_error_t){
                .severity = JASH_ERR_FATAL,
                .domain = JASH_DOM_EXEC,
                .code = 2,
                .sys_errno = errno,
                .context = "waitpid",
                .message = "waitpid failed",
                .span = {0, 0},
            },
        };
        return -1;
    }

    if (WIFEXITED(st)) {
        int ec = WEXITSTATUS(st);
        *out = res_exited(ec);
        return 0;
    }
    if (WIFSIGNALED(st)) {
        int sig = WTERMSIG(st);
        *out = res_signaled(sig);
        return 0;
    }

    *out = res_exited(1);
    return 0;
}

static int apply_redirections(const jash_redir_t *redirs, size_t count) {
    for (size_t i = 0; i < count; i++) {
        int flags = 0;
        int mode = 0644;

        if (redirs[i].type == 1) {
            /* > (JASH_REDIR_OUT) */
            flags = O_WRONLY | O_CREAT | O_TRUNC;
        } else if (redirs[i].type == 2) {
            /* >> (JASH_REDIR_OUT_APPEND) */
            flags = O_WRONLY | O_CREAT | O_APPEND;
        } else if (redirs[i].type == 3) {
            /* < (JASH_REDIR_IN) */
            flags = O_RDONLY;
        } else {
            continue;
        }

        int fd = open(redirs[i].target, flags, mode);
        if (fd < 0) {
            perror("jash: open");
            return -1;
        }

        if (dup2(fd, redirs[i].fd) < 0) {
            perror("jash: dup2");
            close(fd);
            return -1;
        }

        close(fd);
    }
    return 0;
}

static jash_exec_result_t exec_external(char *const argv[], const jash_redir_t *redirs, size_t redir_count) {
    if (!argv || !argv[0]) return res_failed("", "invalid argv", 0);

    pid_t pid = fork();
    if (pid < 0) {
        return res_failed("fork", "fork failed", errno);
    }

    if (pid == 0) {
        /* Child: apply redirections, then exec */
        if (apply_redirections(redirs, redir_count) < 0) {
            _exit(1);
        }

        execvp(argv[0], argv);

        /* If exec returns, it's an error. */
        int e = errno;
        fprintf(stderr, "jash: %s: %s\n", argv[0], strerror(e));
        _exit((e == ENOENT) ? 127 : 126);
    }

    jash_exec_result_t r;
    if (wait_to_status(pid, &r) < 0) return r;
    return r;
}

static jash_exec_result_t exec_builtin_redirected(jash_builtin_id_t id,
                                                   size_t argc,
                                                   char *const argv[],
                                                   jash_shell_state_t *sh,
                                                   const jash_redir_t *redirs,
                                                   size_t redir_count) {
    /* If no redirections, run builtin in-process */
    if (redir_count == 0) {
        jash_builtin_result_t br = jash_builtin_run(id, argc, argv, sh);
        if (!br.ok) {
            return (jash_exec_result_t){
                .kind = JASH_RES_FAILED_TO_START,
                .error = br.error,
            };
        }
        return res_exited(br.status);
    }

    /* With redirections: fork, apply redirs, run builtin in child */
    pid_t pid = fork();
    if (pid < 0) {
        return res_failed("fork", "fork failed", errno);
    }

    if (pid == 0) {
        /* Child: apply redirections, run builtin, exit */
        if (apply_redirections(redirs, redir_count) < 0) {
            _exit(1);
        }

        jash_builtin_result_t br = jash_builtin_run(id, argc, argv, sh);
        _exit(br.ok ? br.status : 1);
    }

    jash_exec_result_t r;
    if (wait_to_status(pid, &r) < 0) return r;
    return r;
}


jash_exec_result_t jash_exec_run(const jash_exec_plan_t *plan, jash_shell_state_t *sh,
                                 const jash_redir_t *redirs, size_t redir_count) {
    if (!plan) {
        return (jash_exec_result_t){
            .kind = JASH_RES_FAILED_TO_START,
            .error = (jash_error_t){
                .severity = JASH_ERR_FATAL,
                .domain = JASH_DOM_EXEC,
                .code = 3,
                .message = "NULL exec plan",
                .span = {0, 0},
            },
        };
    }

    if (plan->kind == JASH_EXEC_BUILTIN) {
        return exec_builtin_redirected(plan->builtin_id, plan->argc, plan->argv, sh, redirs, redir_count);
    }

    /* External: apply redirections in child */
    return exec_external(plan->argv, redirs, redir_count);
}
