#include "jash/error.h"

bool jash_span_is_valid(jash_span_t s) {
    return s.end >= s.start;
}

jash_span_t jash_span_make(size_t start, size_t end) {
    jash_span_t s;
    s.start = start;
    s.end = end;
    return s;
}
