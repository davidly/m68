// goofy ai-generated implementation because newlib has none

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h> // For stat, lstat
#include <limits.h>   // For PATH_MAX

extern "C" int lstat (const char *__restrict __path, struct stat *__restrict __buf );

#ifndef PATH_MAX
#define PATH_MAX 2048
#endif

// Helper function to concatenate path components, handling '/'
static void append_path_component(char *resolved_path, const char *component) {
    size_t len = strlen(resolved_path);
    if (len > 0 && resolved_path[len - 1] != '/') {
        strcat(resolved_path, "/");
    }
    strcat(resolved_path, component);
}

char *realpath(const char *path, char *resolved_path_buffer) {
    if (path == NULL) {
        errno = EINVAL;
        return NULL;
    }

    char *result_buf = resolved_path_buffer;
    if (result_buf == NULL) {
        result_buf = (char *)malloc(PATH_MAX); // Allocate buffer if not provided
        if (result_buf == NULL) {
            errno = ENOMEM;
            return NULL;
        }
    }

    result_buf[0] = '\0'; // Initialize resolved path

    // If path is relative, get current working directory
    if (path[0] != '/') {
        if (getcwd(result_buf, PATH_MAX) == NULL) {
            if (resolved_path_buffer == NULL) free(result_buf);
            return NULL;
        }
    } else {
        result_buf[0] = '/';
        result_buf[1] = '\0';
    }

    char temp_path[PATH_MAX];
    strncpy(temp_path, path, PATH_MAX);
    temp_path[PATH_MAX - 1] = '\0';

    char *token;
    char *saveptr;

    token = strtok_r(temp_path, "/", &saveptr);
    while (token != NULL) {
        if (strcmp(token, ".") == 0) {
            // Ignore '.'
        } else if (strcmp(token, "..") == 0) {
            // Go up one directory
            char *last_slash = strrchr(result_buf, '/');
            if (last_slash == result_buf) { // At root
                result_buf[1] = '\0';
            } else if (last_slash != NULL) {
                *last_slash = '\0';
            }
        } else {
            // Append the component
            append_path_component(result_buf, token);

            struct stat st;
            if (lstat(result_buf, &st) == -1) {
                // Handle non-existent component or other lstat error
                if (resolved_path_buffer == NULL) free(result_buf);
                errno = ENOENT; // Example error
                return NULL;
            }
        }
        token = strtok_r(NULL, "/", &saveptr);
    }

    // Handle trailing '/' for directories
    if (path[strlen(path) - 1] == '/' && strcmp(result_buf, "/") != 0) {
        append_path_component(result_buf, ""); // Add trailing slash
    }

    return result_buf;
}

