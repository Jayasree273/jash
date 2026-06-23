#ifndef JASH_ERROR_H
#define JASH_ERROR_H

#include <stdbool.h>
#include <stddef.h>

typedef enum jash_err_severity {
    JASH_ERR_RECOVERABLE = 1,
    JASH_ERR_FATAL = 2
} jash_err_severity_t;

typedef enum jash_err_domain {
    JASH_DOM_INPUT = 1,
    JASH_DOM_LEX,
    JASH_DOM_EXPAND,
    JASH_DOM_PARSE,
    JASH_DOM_PLAN,
    JASH_DOM_EXEC,
    JASH_DOM_ENV,
    JASH_DOM_CONFIG,
    JASH_DOM_INTERNAL
} jash_err_domain_t;

typedef struct jash_span {
    size_t start; /* 0-based byte offset */
    size_t end;   /* exclusive */
} jash_span_t;

typedef struct jash_error {
    jash_err_severity_t severity;
    jash_err_domain_t domain;
    int code;              /* project-defined stable code */
    int sys_errno;         /* 0 if not applicable */
    jash_span_t span;      /* {0,0} if unknown */
    const char *context;   /* optional, may be NULL */
    const char *message;   /* required human message */
} jash_error_t;

/* Utilities */
bool jash_span_is_valid(jash_span_t s);
jash_span_t jash_span_make(size_t start, size_t end);

#endif /* JASH_ERROR_H */
