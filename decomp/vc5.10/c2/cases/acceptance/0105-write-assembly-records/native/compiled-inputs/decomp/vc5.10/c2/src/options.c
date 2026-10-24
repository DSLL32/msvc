#include "c2.h"
#include <recon.h>

char *c2_debug_file, *c2_offset_file, *c2_unexpected_argument;
char *c2_output_name, *c2_assembly_flags, *c2_listing_file, *c2_assembly_file;
char *c2_alignment_options;
int c2_identifier_limit = 2058;
int c2_object_output, c2_assembly_enabled, c2_mixed_assembly;
int c2_assembly_source, c2_assembly_bytes, c2_extra_listing;
int c2_assembly_redirected, c2_internal_listing;

void c2_initialize_options(void)
{
    char *old_offset = c2_offset_file;
    char *old_debug = c2_debug_file;
    char *(__cdecl *find_flags)(const char *, const char *);
    int result;

    result = c2_match_option(c2_options, c2_next_argument(), c2_next_argument, 0);
    while (result) {
        if (result == 6)
            c2_fatal(7, c2_unexpected_argument, "p2");
        if (c2_debug_file != old_debug)
            old_debug = c2_debug_file;
        if (c2_offset_file != old_offset)
            old_offset = c2_offset_file;
        result = c2_match_option(c2_options, c2_next_argument(), c2_next_argument, 0);
    }
    c2_validate_alignment(c2_alignment_options);
    if (c2_output_name)
        c2_object_output = 1;
    if (c2_identifier_limit > 2058)
        c2_identifier_limit = 2058;
    if (c2_assembly_flags) {
        find_flags = strpbrk;
        c2_assembly_enabled = 1;
        if (find_flags(c2_assembly_flags, "cC"))
            c2_mixed_assembly = 1;
        if (find_flags(c2_assembly_flags, "sS"))
            c2_assembly_source = 1;
        if (find_flags(c2_assembly_flags, "bB"))
            c2_assembly_bytes = 1;
    }
    if (c2_listing_file)
        c2_extra_listing = 1;
    if (c2_assembly_file) {
        c2_assembly_enabled = 1;
        if (!freopen(c2_assembly_file, "wt", stdout))
            c2_system_fatal(83, c2_get_diagnostic_string(310), c2_assembly_file);
        c2_assembly_redirected = 1;
    }
    if (c2_assembly_enabled || c2_mixed_assembly || c2_extra_listing)
        c2_internal_listing = 0;
}

char *c2_next_argument(void)
{
    if (--c2_argc > 0)
        return *++c2_argv;
    return NULL;
}

void __fastcall c2_validate_alignment(char *alignment)
{
    if (alignment)
        while (*alignment)
            ++alignment;
}
