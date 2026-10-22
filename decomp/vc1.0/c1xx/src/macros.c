/* Predefined macros and interned identifiers with observable name identity. */
#include "macros.h"
#include "keywords.h"
#include "characters.h"
#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include <recon.h>

char *name_return_udt, *name_this, *name_that;
char *name_vftable, *name_vbtable, *name_vfptr, *name_vbptr, *name_vdispmap;
char *name_init_vbases, *name_flags, *name_dtor, *name_vbase_dtor;
char *name_delete_dtor, *name_vector_delete_dtor, *name_assignment;
char *name_most_derived, *name_new, *name_delete, *name_ctor;
char *name_default_ctor_closure, *name_static_destructor;
char *name_vector_ctor, *name_vector_ctor_vb, *name_vector_dtor;
char *name_new_target, *name_atexit, *name_tlregdtor;
char *name_defined, *name_formal, *name_ellipsis, *name_cast;
char *name_main, *name_winmain, *name_dllmain, *name_alignment;
char *name_unnamed, *name_dollar, *name_static_guard, *name_setjmp, *name_catch;

/* Original table 0x9d9e0. Each row assigns one interner result to the shared
   name object used by the compiler's identity comparisons. */
SpecialIdentifier special_identifiers[] = {
    { &name_return_udt, "__$ReturnUdt" },
    { &name_this, "this" },
    { &name_that, "__that" },
    { &name_vftable, "$vftable" },
    { &name_vbtable, "$vbtable" },
    { &name_vfptr, "$vfptr" },
    { &name_vbptr, "$vbptr" },
    { &name_vdispmap, "$vdispmap" },
    { &name_init_vbases, "$initVBases" },
    { &name_flags, "__flags" },
    { &name_dtor, "__dtor" },
    { &name_vbase_dtor, "__vbaseDtor" },
    { &name_delete_dtor, "__delDtor" },
    { &name_vector_delete_dtor, "__vecDelDtor" },
    { &name_assignment, "=" },
    { &name_most_derived, "__most_derived__" },
    { &name_new, "new" },
    { &name_delete, "delete" },
    { &name_ctor, "__ctor" },
    { &name_default_ctor_closure, "__dflt_ctor_closure" },
    { &name_static_destructor, ".local static destructor." },
    { &name_vector_ctor, "__vec_ctor" },
    { &name_vector_ctor_vb, "__vec_ctor_vb" },
    { &name_vector_dtor, "__vec_dtor" },
    { &name_new_target, "Target of operator new()" },
    { &name_atexit, "atexit" },
    { &name_tlregdtor, "__tlregdtor" },
    { &name_defined, "defined" },
    { &name_formal, "formal" },
    { &name_ellipsis, "..." },
    { &name_cast, "cast" },
    { &name_main, "main" },
    { &name_winmain, "WinMain" },
    { &name_dllmain, "DllMain" },
    { &name_alignment, ".alignment member." },
    { &name_unnamed, "__unnamed" },
    { &name_dollar, "$" },
    { &name_static_guard, ".local static guard." },
    { &name_setjmp, "_setjmp" },
    { &name_catch, "Catch" },
    { &anonymous_symbol_name, "__LOCAL_SIZE" },
    { 0, 0 }
};

/* ctime supplies 24 printable bytes, newline, and NUL. The copied newline is
   replaced below, but its terminator must fit during the original strcpy. */
char macro_timestamp[26]; /* original 0x918a8 */
char *macro_date, *macro_time; /* original 0x918a0/0x91898 */

/* Original 0x1efd0..0x1f1d9. */
void __stdcall define_command_line_macros(Vector *definitions)
{
    SpecialIdentifier *identifier;
    long now;
    char *source, *destination;
    short index;
    character_class[1] = CC_MACRO_PARAMETER;
    character_class[2] = CC_MACRO_STRING;
    character_class[3] = CC_MACRO_TOKEN;
    character_class[4] = CC_MACRO_END;
    if (options.configuration.extensions_enabled) {
        character_class['$'] = CC_IDENTIFIER;
        character_flags['$'] = CF_IDENTIFIER;
    }
    for (identifier = special_identifiers; identifier->destination; ++identifier)
        *identifier->destination = intern_identifier(identifier->spelling);
    if (!options.configuration.option_Brepro) {
        frontend_time(&now);
        source = format_timestamp(now);
        destination = macro_timestamp;
        do { *destination++ = *source++; } while (destination[-1]);
        macro_date = macro_timestamp + 4;
        macro_timestamp[15] = 0;
        macro_time = macro_timestamp + 16;
        macro_timestamp[24] = 0;
    } else {
        macro_timestamp[0] = '1';
        macro_timestamp[1] = 0;
        macro_date = macro_timestamp;
        macro_time = macro_timestamp;
    }
    initialize_macro_storage();
    intern_identifier("__LINE__");
    define_macro(0, 0, -3);
    intern_identifier("__FILE__");
    define_macro(0, 0, -4);
    intern_identifier("__DATE__");
    define_macro(0, 0, -5);
    intern_identifier("__TIME__");
    define_macro(0, 0, -6);
    intern_identifier("__TIMESTAMP__");
    define_macro(0, 0, -8);
    intern_identifier("__cplusplus");
    define_macro(0, 0, -7);
    for (index = 0; index < definitions->count; ++index)
        define_command_line_macro(definitions->items[index]);
    if (!options.option_E && !options.prototypes_only)
        write_global_header();
    open_source_file(source_filename, 1);
    forced_source_inputs = options.forced_includes;
    forced_source_index = 0;
    if (options.option_E && forced_source_inputs->count > forced_source_index) {
        current_forced_source = forced_source_inputs->items[forced_source_index++];
        open_forced_include(current_forced_source, 0);
    }
}

/* Original 0x745b0..0x745c0. */
long __stdcall frontend_time(long *result)
{
    return time(result);
}

/* Original workspace 0x9a160. The formatter copies the first 25 ctime bytes;
   the following byte remains zero in the original BSS, terminating its result. */
static char formatted_timestamp[26];

/* Original 0x74550..0x745ab. */
char *__stdcall format_timestamp(long timestamp)
{
    char *text;
    unsigned i;
    timestamp &= 0x7fffffffL;
    text = ctime(&timestamp);
    for (i = 0; i < 25; ++i)
        formatted_timestamp[i] = text[i];
    if (formatted_timestamp[8] == '0')
        formatted_timestamp[8] = ' ';
    for (i = 0; i < 4; ++i)
        formatted_timestamp[11 + i] = text[20 + i];
    for (i = 0; i < 8; ++i)
        formatted_timestamp[16 + i] = text[11 + i];
    return formatted_timestamp;
}

MacroDefinition **macro_buckets; /* original 0x9f1d4 */
unsigned macro_work_bytes = 0xf000; /* original 0x9fc44 */
char *macro_work_buffer; /* original 0x91888; midpoint aliases parser stack */
char *macro_replacement_start, *macro_expansion_start; /* original 0x9577c/80 */

/* Original 0x1f5a0..0x1f5fe. Replacement text and expansion text grow in
   opposite halves of one work buffer. */
void initialize_macro_storage(void)
{
    macro_buckets = allocate_permanent(2048);
    macro_work_buffer = malloc(macro_work_bytes);
    if (!macro_work_buffer) {
        macro_work_bytes >>= 1;
        macro_work_buffer = retry_malloc(macro_work_bytes);
    }
    macro_work_midpoint = macro_work_buffer + (macro_work_bytes >> 1);
    macro_replacement_start = macro_work_midpoint;
    macro_expansion_start = macro_work_buffer;
}

char *macro_replacement_text; /* original 0x95784 */
short include_depth = -1; /* original 0x9d7f4 */
unsigned browser_symbol_limit = 65535; /* original 0x9f444 */

/* Original 0x1f910..0x1fa8b. The current interned name identifies the macro;
   parameter counts below -2 denote predefined expansions. */
void __stdcall define_macro(char *replacement, unsigned bytes, int parameter_count)
{
    MacroDefinition *definition;
    char *source;
    unsigned i, bucket;
    if (interned_name == name_defined) {
        report_macro_error("#define");
        return;
    }
    definition = lookup_macro();
    if (!include_depth && pch_record_macro(interned_name,
            bytes ? macro_replacement_text : "", parameter_count))
        return;
    if (definition) {
        if (definition->parameter_count < -2) {
            report_macro_error("#define");
            return;
        }
        source = bytes ? replacement : 0;
        if ((parameter_count == -2 || definition->parameter_count == parameter_count) &&
            !macro_replacement_diff(source, definition->replacement, bytes - 1))
            return;
        compiler_warning(1, 5, interned_name);
    } else {
        definition = allocate_permanent(sizeof(MacroDefinition));
        definition->name = interned_name;
        SYMBOL_FROM_NAME(interned_name)->flags |= 8;
        bucket = (unsigned)interned_name & 511;
        definition->next = macro_buckets[bucket];
        macro_buckets[bucket] = definition;
        source = replacement;
    }
    if (bytes) {
        if (bytes >= 0xf94) {
            definition->replacement = allocate_permanent(bytes);
            for (i = 0; i < bytes; ++i)
                definition->replacement[i] = source[i];
        } else
            definition->replacement = copy_arena_bytes(source, 0, bytes);
    } else
        definition->replacement = 0;
    definition->parameter_count = (signed char)parameter_count;
    if (browser_filename && !definition->browser_index) {
        definition->browser_index = browser_symbol_limit--;
        browser_macro(definition);
    }
}

/* Original 0x1fa90..0x1facb. Name identity comes from the shared interner. */
MacroDefinition *lookup_macro(void)
{
    MacroDefinition *definition;
    if (!(SYMBOL_FROM_NAME(interned_name)->flags & 8))
        return 0;
    definition = macro_buckets[(unsigned)interned_name & 511];
    while (definition) {
        if (definition->name == interned_name)
            return definition;
        definition = definition->next;
    }
    return 0;
}

/* Original 0x1f1e0..0x1f2dd. The encoded replacement ends in NUL and D. */
void __stdcall define_command_line_macro(char *definition)
{
    char *input = definition;
    char *output = macro_work_buffer;
    char *equals = 0, *space_end = 0;
    while (*input) {
        if (character_class[(unsigned char)*input] == CC_LEAD_BYTE)
            ++input;
        else if (*input == '=')
            break;
        ++input;
    }
    if (*input == '=') {
        equals = input;
        *input++ = 0;
        while (*input) {
            *output++ = *input;
            if (character_class[(unsigned char)*input] == CC_LEAD_BYTE) {
                *output++ = *++input;
                space_end = 0;
            } else if (isspace((signed char)*input)) {
                if (!space_end)
                    space_end = output;
            } else
                space_end = 0;
            ++input;
        }
        if (space_end)
            output = space_end;
    } else
        *output++ = '1';
    intern_identifier(definition);
    if (output != macro_work_buffer) {
        *output++ = 0;
        *output++ = 'D';
        define_macro(macro_work_buffer, output - macro_work_buffer, 0);
    } else
        define_macro(0, 0, 0);
    if (equals)
        *equals = '=';
}
