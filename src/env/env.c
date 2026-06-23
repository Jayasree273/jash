#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include "jash/env.h"

static char *xstrdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s);
    char *p = (char *)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n + 1);
    return p;
}

bool jash_env_is_valid_name(const char *name) {
    if (!name || !name[0]) return false;
    if (!((name[0] >= 'A' && name[0] <= 'Z') ||
          (name[0] >= 'a' && name[0] <= 'z') ||
          name[0] == '_')) return false;
    for (size_t i = 1; name[i]; i++) {
        char c = name[i];
        if (!((c >= 'A' && c <= 'Z') ||
              (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') ||
              c == '_')) return false;
    }
    return true;
}

void jash_env_init(jash_env_table_t *t) {
    t->entries = NULL;
    t->count = 0;
    t->cap = 0;
}

void jash_env_free(jash_env_table_t *t) {
    if (!t) return;
    for (size_t i = 0; i < t->count; i++) {
        free(t->entries[i].name);
        free(t->entries[i].value);
    }
    free(t->entries);
    t->entries = NULL;
    t->count = 0;
    t->cap = 0;
}

static ssize_t find_idx(const jash_env_table_t *t, const char *name) {
    for (size_t i = 0; i < t->count; i++) {
        if (strcmp(t->entries[i].name, name) == 0) return (ssize_t)i;
    }
    return -1;
}

const char *jash_env_get(const jash_env_table_t *t, const char *name) {
    if (!t || !name) return NULL;
    ssize_t idx = find_idx(t, name);
    if (idx < 0) return NULL;
    return t->entries[idx].value;
}

jash_env_result_t jash_env_set(jash_env_table_t *t, const char *name, const char *value, bool exported) {
    jash_env_result_t r = { .ok = false };

    if (!t || !name || !value) {
        r.error = (jash_error_t){
            .severity = JASH_ERR_FATAL,
            .domain = JASH_DOM_ENV,
            .code = 1,
            .message = "invalid env_set arguments"
        };
        return r;
    }

    if (!jash_env_is_valid_name(name)) {
        r.error = (jash_error_t){
            .severity = JASH_ERR_RECOVERABLE,
            .domain = JASH_DOM_ENV,
            .code = 2,
            .context = name,
            .message = "invalid variable name"
        };
        return r;
    }

    ssize_t idx = find_idx(t, name);
    if (idx >= 0) {
        char *nv = xstrdup(value);
        if (!nv) {
            r.error = (jash_error_t){
                .severity = JASH_ERR_FATAL,
                .domain = JASH_DOM_INTERNAL,
                .code = 3,
                .message = "out of memory"
            };
            return r;
        }
        free(t->entries[idx].value);
        t->entries[idx].value = nv;
        if (exported) t->entries[idx].exported = true;
        r.ok = true;
        return r;
    }

    if (t->count == t->cap) {
        size_t ncap = (t->cap == 0) ? 8 : (t->cap * 2);
        void *p = realloc(t->entries, ncap * sizeof(t->entries[0]));
        if (!p) {
            r.error = (jash_error_t){
                .severity = JASH_ERR_FATAL,
                .domain = JASH_DOM_INTERNAL,
                .code = 4,
                .message = "out of memory"
            };
            return r;
        }
        t->entries = (jash_env_entry_t *)p;
        t->cap = ncap;
    }

    t->entries[t->count].name = xstrdup(name);
    t->entries[t->count].value = xstrdup(value);
    t->entries[t->count].exported = exported;

    if (!t->entries[t->count].name || !t->entries[t->count].value) {
        free(t->entries[t->count].name);
        free(t->entries[t->count].value);
        r.error = (jash_error_t){
            .severity = JASH_ERR_FATAL,
            .domain = JASH_DOM_INTERNAL,
            .code = 5,
            .message = "out of memory"
        };
        return r;
    }

    t->count++;
    r.ok = true;
    return r;
}

jash_env_result_t jash_env_unset(jash_env_table_t *t, const char *name) {
    jash_env_result_t r = { .ok = false };

    if (!t || !name) {
        r.error = (jash_error_t){
            .severity = JASH_ERR_FATAL,
            .domain = JASH_DOM_ENV,
            .code = 6,
            .message = "invalid env_unset arguments"
        };
        return r;
    }

    ssize_t idx = find_idx(t, name);
    if (idx < 0) {
        r.ok = true; /* unset of missing variable is not an error */
        return r;
    }

    free(t->entries[idx].name);
    free(t->entries[idx].value);

    /* swap with last */
    t->entries[idx] = t->entries[t->count - 1];
    t->count--;

    r.ok = true;
    return r;
}
