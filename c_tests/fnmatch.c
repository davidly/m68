// goofy ai-generated implementation because newlib has none

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Helper function to check for leading period (simplified)
static bool is_leading_period(const char *string_ptr, const char *initial_string, int flags) {
    if (!(flags & FNM_PERIOD)) {
        return false; // Not concerned with periods
    }

    if (string_ptr == initial_string) {
        return true; // First character is a leading period
    }

    if ((flags & FNM_PATHNAME) && (*(string_ptr - 1) == '/')) {
        return true; // Period after a slash with FNM_PATHNAME
    }

    return false;
}

// Helper function to match character within a bracket expression (simplified)
static int match_bracket_expression(const char **pattern_ptr, char c, int flags) {
    const char *start_bracket = *pattern_ptr;
    bool negate = false;

    if (**pattern_ptr == '!' || **pattern_ptr == '^') {
        negate = true;
        (*pattern_ptr)++;
    }

    bool matched = false;
    while (**pattern_ptr != '\0' && **pattern_ptr != ']') {
        if (**pattern_ptr == '\\' && !(flags & FNM_NOESCAPE)) {
            (*pattern_ptr)++; // Escape character
            if (**pattern_ptr == '\0') { // Pattern ends with escaped backslash
                return -1; // Indicate error or no match
            }
        }

        char char_to_match = **pattern_ptr;
        (*pattern_ptr)++;

        if (**pattern_ptr == '-' && (*pattern_ptr)[1] != '\0' && (*pattern_ptr)[1] != ']') {
            // Handle range: [a-z]
            (*pattern_ptr)++;
            char range_end = **pattern_ptr;
            (*pattern_ptr)++;
            if (c >= char_to_match && c <= range_end) {
                matched = true;
            }
        } else {
            // Single character match
            if (c == char_to_match) {
                matched = true;
            }
        }
    }

    if (**pattern_ptr == '\0') { // Missing closing bracket
        return -1; // Error or no match
    }

    (*pattern_ptr)++; // Move past ']'

    if (negate) {
        return matched ? FNM_NOMATCH : 0;
    } else {
        return matched ? 0 : FNM_NOMATCH;
    }
}

extern "C" int fnmatch(const char *pattern, const char *string, int flags) {
    const char *p = pattern;
    const char *s = string;
    const char *last_star_p = NULL;
    const char *last_star_s = NULL;
    const char *initial_string = string; // Store for leading period check

    while (*s != '\0') {
//printf( "top of loop, s %lx, *s %c, *p %c\n", s, *s, *p );
        if (*p == '*') {
            // Skip multiple '*'
            while (*p == '*') {
                p++;
            }
            last_star_p = p;
            last_star_s = s;
        } else if (*p == '?') {
            if ((flags & FNM_PATHNAME) && *s == '/') { // '?' cannot match '/' with FNM_PATHNAME
                if (last_star_p) {
                    p = last_star_p;
                    s = last_star_s + 1;
                    continue;
                }
                return FNM_NOMATCH; // FNM_ESLASH would be a better return in a complete implementation
            }
            if (is_leading_period(s, initial_string, flags)) { // '?' cannot match leading '.' with FNM_PERIOD
                 if (last_star_p) {
                    p = last_star_p;
                    s = last_star_s + 1;
                    continue;
                }
                return FNM_NOMATCH; // FNM_EPERIOD would be a better return in a complete implementation
            }
            p++;
            s++;
        } else if (*p == '[') {
            const char *temp_p = p;
            int bracket_result = match_bracket_expression(&temp_p, *s, flags);
            if (bracket_result == 0) {
                p = temp_p;
                s++;
            } else if (bracket_result == FNM_NOMATCH) {
                 if (last_star_p) {
                    p = last_star_p;
                    s = last_star_s + 1;
                    continue;
                }
                return FNM_NOMATCH;
            } else { // Error parsing bracket expression
                 if (last_star_p) {
                    p = last_star_p;
                    s = last_star_s + 1;
                    continue;
                }
                return FNM_NOMATCH; // Simplified error handling
            }
        } else if (*p == '\\' && !(flags & FNM_NOESCAPE)) {
            p++; // Escape the next character
            if (*p == '\0') { // Pattern ends with escaped backslash
                return FNM_NOMATCH; // This is a specific error case
            }
            if (*p != *s) {
                 if (last_star_p) {
                    p = last_star_p;
                    s = last_star_s + 1;
                    continue;
                }
                return FNM_NOMATCH;
            }
            p++;
            s++;
        } else { // Regular character match
            if ((flags & FNM_PATHNAME) && (*p == '/' || *s == '/')) { // '/' must match exactly with FNM_PATHNAME
                if (*p != *s) {
                     if (last_star_p) {
                        p = last_star_p;
                        s = last_star_s + 1;
                        continue;
                    }
                    return FNM_NOMATCH;
                }
            }
            if (is_leading_period(s, initial_string, flags) && *s == '.') { // Leading '.' with FNM_PERIOD must match '.'
                if (*p != *s) { // If pattern has something other than '.'
                     if (last_star_p) {
                        p = last_star_p;
                        s = last_star_s + 1;
//printf( "continue point A\n" );
                        continue;
                    }
                    return FNM_NOMATCH;
                }
            } else if (*p != *s) {
                 if (last_star_p) {
                    p = last_star_p;
                    last_star_s++;
                    s = last_star_s;
//printf( "continue point B\n" );
                    continue;
                }
                return FNM_NOMATCH;
            }
            p++;
            s++;
        }
    }

    // Handle remaining '*' in pattern after string has ended
    while (*p == '*') {
        p++;
    }

    return (*p == '\0' && *s == '\0') ? 0 : FNM_NOMATCH;
}

