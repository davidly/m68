/*
 * dump_auxv_stack.c
 *
 * Build:
 *   cc -Wall -Wextra -O2 dump_auxv_stack.c -o dump_auxv_stack
 *
 * Linux initial stack layout:
 *
 *   argc
 *   argv[0]
 *   ...
 *   argv[argc - 1]
 *   NULL
 *   envp[0]
 *   ...
 *   NULL
 *   auxv[0].a_type
 *   auxv[0].a_val
 *   ...
 *   AT_NULL
 *   0
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <elf.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef AT_EXECFN
#define AT_EXECFN 31
#endif

#ifndef AT_SYSINFO_EHDR
#define AT_SYSINFO_EHDR 33
#endif

#ifndef AT_MINSIGSTKSZ
#define AT_MINSIGSTKSZ 51
#endif

#ifndef AT_SYSINFO
#define AT_SYSINFO 32
#endif

#ifndef AT_L1I_CACHESIZE
#define AT_L1I_CACHESIZE 27
#endif

#ifndef AT_L1I_CACHEGEOMETRY
#define AT_L1I_CACHEGEOMETRY 28
#endif

extern char **environ;

struct aux_name {
    uintptr_t id;
    const char *name;
    const char *datatype;
};

static const struct aux_name aux_names[] = {
    { AT_NULL,          "AT_NULL",          "terminator" },
    { AT_IGNORE,        "AT_IGNORE",        "ignored" },
    { AT_EXECFD,        "AT_EXECFD",        "fd" },
    { AT_PHDR,          "AT_PHDR",          "address" },
    { AT_PHENT,         "AT_PHENT",         "integer" },
    { AT_PHNUM,         "AT_PHNUM",         "integer" },
    { AT_PAGESZ,        "AT_PAGESZ",        "integer" },
    { AT_BASE,          "AT_BASE",          "address" },
    { AT_FLAGS,         "AT_FLAGS",         "flags" },
    { AT_ENTRY,         "AT_ENTRY",         "address" },
    { AT_NOTELF,        "AT_NOTELF",        "boolean" },
    { AT_UID,           "AT_UID",           "uid_t" },
    { AT_EUID,          "AT_EUID",          "uid_t" },
    { AT_GID,           "AT_GID",           "gid_t" },
    { AT_EGID,          "AT_EGID",          "gid_t" },
    { AT_PLATFORM,      "AT_PLATFORM",      "string address" },
    { AT_HWCAP,         "AT_HWCAP",         "bitmask" },
    { AT_CLKTCK,        "AT_CLKTCK",        "integer" },
    { AT_SECURE,        "AT_SECURE",        "boolean" },
    { AT_BASE_PLATFORM, "AT_BASE_PLATFORM", "string address" },
    { AT_RANDOM,        "AT_RANDOM",        "16-byte address" },
    { AT_HWCAP2,        "AT_HWCAP2",        "bitmask" },
    { AT_EXECFN,        "AT_EXECFN",        "string address" },
    { AT_SYSINFO_EHDR,  "AT_SYSINFO_EHDR",  "address" },
    { AT_MINSIGSTKSZ,   "AT_MINSIGSTKSZ",   "integer" },
    { AT_SYSINFO,       "AT_SYSINFO",       "address" },
    { AT_L1I_CACHESIZE, "AT_L1I_CACHESIZE", "integer" },
    { AT_L1I_CACHEGEOMETRY, "AT_L1I_CACHEGEOMETRY", "cache geometry" },
};

static const struct aux_name *find_aux_name(uintptr_t id)
{
    size_t i;

    for (i = 0; i < sizeof(aux_names) / sizeof(aux_names[0]); i++) {
        if (aux_names[i].id == id)
            return &aux_names[i];
    }

    return 0;
}

static int is_string_aux(uintptr_t id)
{
    return id == AT_PLATFORM ||
           id == AT_BASE_PLATFORM ||
           id == AT_EXECFN;
}

static void print_random(uintptr_t addr)
{
    const unsigned char *p;
    int i;

    p = (const unsigned char *)addr;

    for (i = 0; i < 16; i++)
        printf("%02x", p[i]);
}

static void print_aux(uintptr_t id, uintptr_t val)
{
    const struct aux_name *n;

    n = find_aux_name(id);

    printf("%-3" PRIuPTR " %-18s %-16s 0x%" PRIxPTR,
           id,
           n ? n->name : "UNKNOWN",
           n ? n->datatype : "unknown",
           val);

    if (is_string_aux(id) && val != 0) {
        printf("  \"%s\"", (const char *)val);
    } else if (id == AT_RANDOM && val != 0) {
        printf("  ");
        print_random(val);
    } else if (n != 0 &&
               (strcmp(n->datatype, "integer") == 0 ||
                strcmp(n->datatype, "boolean") == 0 ||
                strcmp(n->datatype, "uid_t") == 0 ||
                strcmp(n->datatype, "gid_t") == 0 ||
                strcmp(n->datatype, "fd") == 0)) {
        printf("  (%" PRIuPTR ")", val);
    }

    putchar('\n');
}

int main(int argc, char **argv, char **envp)
{
    uintptr_t *p;

    (void)argc;
    (void)argv;

    /*
     * Prefer envp from main.  It points into the original stack area on
     * normal Linux C runtimes.  environ may later be changed by setenv().
     */
    if (envp == 0)
        envp = environ;

    while (*envp != 0)
        envp++;

    /*
     * envp now points at the NULL after the environment vector.
     * The aux vector starts at the next machine word.
     */
    p = (uintptr_t *)(envp + 1);

    printf("%-3s %-18s %-16s %s\n", "ID", "NAME", "DATATYPE", "VALUE");
    printf("%-3s %-18s %-16s %s\n", "--", "----", "--------", "-----");

    for (;;) {
        uintptr_t id;
        uintptr_t val;

        id = p[0];
        val = p[1];

        print_aux(id, val);

        p += 2;

        if (id == AT_NULL)
            break;
    }

    return 0;
}