/* Driver-only options selected by the D pass marker. */
#include "cl.h"
#include <recon.h>

typedef struct DriverOption {
    char *name;
    unsigned action;
    void *target;
} DriverOption;

char *object_name, *image_name, *map_name, *pdb_name, *browser_name;
int map_requested, bk_requested, reproducible, timing_requested, bz_requested;
int minimal_rebuild, allow_pch, is_dll;
int first_option_pass = 1;
int cleanup_temporaries = 1;
int keep_preprocessor_output = 1;
int use_standard_include = 1;

/* Actions: 1 set, 2 clear, 3 filename, 4 callback; bit8 limits first pass. */
DriverOption driver_options[] = {
    { "Fo", 3, &object_name }, { "Fe", 3, &image_name },
    { "bA", 4, (void *)configure_assembly },
    { "ba", 4, (void *)enable_pass },
    { "bc", 4, (void *)toggle_pass },
    { "bd", 4, (void *)disable_pass },
    { "bE", 9, &default_c_language }, { "bh", 9, &help_requested },
    { "bil", 3, &driver_storage.temporary }, { "bm", 9, &map_requested },
    { "bo", 12, (void *)set_output_option },
    { "bx", 12, (void *)set_pass_suffix },
    { "bp", 12, (void *)set_pass_name },
    { "br", 10, &keep_preprocessor_output }, { "bt", 11, &source_language },
    { "bU", 4, (void *)undefine_macro },
    { "bu", 4, (void *)undefine_defaults },
    { "bv", 9, &debug_exceptions }, { "Bk", 9, &bk_requested },
    { "Brepro", 9, &reproducible }, { "Bt", 9, &timing_requested },
    { "Bz", 9, &bz_requested }, { "Fm", 3, &map_name },
    { "LD", 12, (void *)enable_dll }, { "LDd", 12, (void *)enable_dll },
    { "X", 10, &use_standard_include }, { "Fd", 3, &pdb_name },
    { "Gm", 9, &minimal_rebuild },
    { "FR", 3, &browser_name }, { "Fr", 3, &browser_name },
    { "ZM", 9, &allow_pch }, { NULL, 0, NULL }
};

/* 00402849..00402933: apply driver settings once or on every pass as tagged. */
void prepare_inputs(Driver *state)
{
    Option *option;
    DriverOption *setting;
    char expanded[512], **target;
    typedef void (*OptionHandler)(Option *);
    for (option = state->options; option; option = option->next) {
        if (!option_for_pass(option, &compiler_passes[0]))
            continue;
        for (setting = driver_options; setting->name; ++setting) {
            if (option->name[0] != setting->name[0] || strcmp(option->name, setting->name))
                continue;
            if (!(setting->action & 8) || first_option_pass) {
                switch (setting->action & 7) {
                case 1: *(int *)setting->target = 1; break;
                case 2: *(int *)setting->target = 0; break;
                case 3:
                    if (option->value) {
                        expand_filename(expanded, option->value, state);
                        target = (char **)setting->target;
                        if (*target)
                            release(*target);
                        *target = duplicate(expanded);
                    }
                    break;
                case 4:
                    ((OptionHandler)setting->target)(option);
                    break;
                }
            }
            break;
        }
    }
    first_option_pass = 0;
}

/* 004010d8..004010f7 */
int option_for_pass(Option *option, CompilerPass *pass)
{
    return strchr(option->matching, (unsigned char)pass->option_prefix[0]) != NULL;
}

/* 00402594..00402633: validate per-source output naming, then consume it. */
void set_output_option(Option *option)
{
    char *name, *filename, *last, *next;
    if (!*option->value)
        return;
    if (source_count >= 2) {
        name = (char *)_mbschr((unsigned char *)option->value, ' ') + 1;
        filename = (char *)_mbschr((unsigned char *)name, ' ');
        *filename++ = 0;
        last = filename;
        while ((next = (char *)_mbsinc((unsigned char *)last)) != NULL && *next)
            last = next;
        if (!_mbsstr((unsigned char *)filename, (unsigned char *)"%b") &&
            !_mbschr((unsigned char *)"\\/", (unsigned char)*last))
            fatal(2036, name, filename);
    }
    *option->value = 0;
}

/* 00402319..004023bb */
void prepare_environment(Driver *state)
{
    char directory[1024], *temporary;
    unsigned length;
    if (!state->temporary) {
        temporary = getenv("TMP");
        if (temporary) {
            strcpy(directory, temporary);
            length = strlen(directory);
            if (directory[length - 1] != '\\' && directory[length - 1] != ':' &&
                directory[length - 1] != '/') {
                directory[length] = '\\';
                directory[length + 1] = 0;
            }
            temporary = join_text(directory, "XXXXXX");
        } else {
            temporary = duplicate("XXXXXX");
        }
        state->temporary = temporary;
        state->temporary = _mktemp(temporary);
    } else {
        cleanup_temporaries = 0;
    }
}
