#ifndef JASH_ENV_H
#define JASH_ENV_H

#include <stdbool.h>
#include <stddef.h>
#include "jash/error.h"

typedef struct jash_env_entry {
    char *name;      /* owned */
    char *value;     /* owned */
    bool exported;
} jash_env_entry_t;

typedef struct jash_env_table {
    jash_env_entry_t *entries; /* owned */
    size_t count;
    size_t cap;
} jash_env_table_t;

typedef struct jash_env_result {
    bool ok;
    jash_error_t error;
} jash_env_result_t;

void jash_env_init(jash_env_table_t *t);
void jash_env_free(jash_env_table_t *t);

const char *jash_env_get(const jash_env_table_t *t, const char *name);
jash_env_result_t jash_env_set(jash_env_table_t *t, const char *name, const char *value, bool exported);
jash_env_result_t jash_env_unset(jash_env_table_t *t, const char *name);
bool jash_env_is_valid_name(const char *name);

#endif /* JASH_ENV_H */
