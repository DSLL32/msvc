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

/* 00402458..00402499: disable selected compiler stages, or all for '.'. */
void disable_pass(Option *option)
{
    int i, all = option->value[0] == '.';
    for (i = 0; i < 5; ++i)
        if (all || _mbschr((unsigned char *)option->value,
                          (unsigned char)compiler_passes[i].option_prefix[0]))
            compiler_passes[i].flags &= ~1;
}

/* 00402499..0040250a: copy an enabled source pass's low bit to a target. */
void toggle_pass(Option *option)
{
    unsigned enabled = (unsigned)option;
    int index;
    /* If the source selector is unknown, the original retains the Option
     * address in its result register. Preserve its low bit, including the
     * zero bit of an ordinary aligned allocation, rather than invent a match. */
    for (index = 0; index < 5; ++index) {
        if (compiler_passes[index].option_prefix[0] == option->value[0]) {
            enabled = compiler_passes[index].flags & 1;
            break;
        }
    }
    if (!enabled)
        return;
    for (index = 0; index < 5; ++index) {
        if (compiler_passes[index].option_prefix[0] == option->value[1]) {
            compiler_passes[index].flags =
                ((enabled ^ compiler_passes[index].flags) & 1) ^
                compiler_passes[index].flags;
            break;
        }
    }
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

/* 00402633..004027cf: consume a driver assembly-output option, preserving
 * explicit per-pass FA/Fa settings and their detached node identities. */
static char assembly_name[1024];
void configure_assembly(Option *option)
{
    char *passes = option->value;
    char *format, *cursor;
    Option *format_pattern, *name_pattern, *format_option, *name_option;
    int invalid = 0, code = 0;
    if (!*passes)
        return;
    format = (char *)_mbschr((unsigned char *)passes, ' ');
    if (format)
        *format++ = 0;
    format_pattern = new_option("FA", "?", passes, 4);
    name_pattern = new_option("Fa", "?", passes, 0);
    format_option = detach_options(&driver->options, format_pattern, NULL);
    name_option = detach_options(&driver->options, name_pattern, NULL);
    if (!format_option) {
        release(format_pattern->value);
        format_pattern->value = duplicate(format);
        format_option = format_pattern;
    } else {
        format = format_option->value;
        delete_option(format_pattern);
    }
    if (!name_option) {
        name_option = name_pattern;
        release(name_option->value);
        name_option->value = duplicate("%b");
    } else
        delete_option(name_pattern);
    strcpy(assembly_name, name_option->value);
    if (format) {
        for (cursor = format; *cursor; ++cursor) {
            if (*cursor == 'c')
                code = 1;
            else if (*cursor != 'a' && *cursor != 'b' && *cursor != 's')
                invalid = 1;
        }
        if (invalid)
            warning(4015, "/FA", format + 1);
    }
    apply_extension(code ? "cod" : "asm", assembly_name, NULL);
    release(name_option->value);
    name_option->value = duplicate(assembly_name);
    append_option(&driver->options, format_option);
    append_option(&driver->options, name_option);
    *option->value = 0;
}

/* 004023c4..004023fb: replace names for a selected pass or all five passes. */
void set_pass_name(Option *option)
{
    int index;
    char pass = option->value[0];
    for (index = 0; index < 5; ++index) {
        if (pass == '.' || pass == compiler_passes[index].option_prefix[0])
            compiler_passes[index].name = duplicate(option->value + 1);
    }
}

/* 00402432..00402458: enable a named pass, or all passes for '.'. */
void enable_pass(Option *option)
{
    int index;
    char pass = option->value[0];
    for (index = 0; index < 5; ++index) {
        if (pass == '.' || pass == compiler_passes[index].option_prefix[0])
            compiler_passes[index].flags |= 1;
    }
}

/* 004023fb..00402432: replace the optional leading pass argument filename. */
void set_pass_suffix(Option *option)
{
    int index;
    char pass = option->value[0];
    for (index = 0; index < 5; ++index) {
        if (pass == '.' || pass == compiler_passes[index].option_prefix[0])
            compiler_passes[index].options = duplicate(option->value + 1);
    }
}

/* 0040253e..00402594: remove only protected default macro definitions once.
 * The encountered guardDWORD00409028 is1 both on disk and at this frontier. */
int default_macros = 1;
void undefine_defaults(Option *unused)
{
    Option *option, *next;
    if (!default_macros)
        return;
    default_macros = 0;
    for (option = driver->options; option; option = next) {
        next = option->next;
        if ((option->flags & 1) && !strcmp(option->name, "D"))
            delete_option(unlink_option(&driver->options, option));
    }
}

/* 004027cf..00402849: enable DLL output; an explicit .exp input suppresses
 * the implicit import-library option. The encountered DWORD0040902c is1. */
int dll_options_enabled = 1;
void enable_dll(Option *unused)
{
    Input *input;
    Option *pattern, *patterns;
    if (!dll_options_enabled)
        return;
    dll_options_enabled = 1;
    is_dll = 1;
    for (input = driver->inputs; input; input = input->next) {
        if (input->kind == 7) {
            pattern = new_option("implib\\:", NULL, NULL, 0);
            patterns = expand_options("=implib\\::?", pattern);
            delete_options(detach_options(&driver->options, patterns, NULL));
            delete_option(pattern);
            delete_options(patterns);
            break;
        }
    }
}
