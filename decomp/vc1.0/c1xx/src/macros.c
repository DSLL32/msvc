/* Predefined macros and interned identifiers with observable name identity. */
#include <sys/stat.h>
#include "macros.h"
#include "keywords.h"
#include "characters.h"
#include "directives-internal.h"
#include "macro-expansion.h"
#include "browser.h"
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
    define_macro(0, 0, -9);
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

/* Original0x745c0..0x745f8, stdcall8. A failed descriptor query leaves
   the caller's timestamp untouched; the 36-byte CRT structure owns st_mtime. */
int __stdcall get_file_timestamp(int handle, long *timestamp)
{
    struct _stat information;
    if (_fstat(handle, &information)) return 0;
    *timestamp = information.st_mtime;
    return 1;
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

int macro_formal_count; /* original 0x95788 */

/* Original 0x1f6a0..0x1f7c5, cdecl. Parse the macro's source name,
   distinguish an adjacent parameter list, then collect its replacement. The
   strict dialect retains the parameter-prefix bytes in the stored record. */
void define_source_macro(void)
{
    unsigned char character, classification;
    unsigned bytes;
    char *replacement;
    character = next_directive_character();
    classification = character_class[character];
    if (classification != CC_IDENTIFIER && classification != CC_WIDE_PREFIX) {
        report_syntax_error(7);
        skip_source_line();
        return;
    }
    scan_source_identifier();
    macro_formal_count = 0;
    macro_replacement_text = macro_work_buffer;
    for (;;) {
        character = (unsigned char)*source_text_cursor++;
        classification = character_class[character];
        if (classification == CC_NEWLINE) {
            --source_text_cursor;
            define_macro(0, 0, 0);
            return;
        }
        if (classification == CC_LEFT_PAREN) {
            read_macro_parameters();
            if (!macro_formal_count) macro_formal_count = -1;
            break;
        }
        if (classification == CC_CARRIAGE_RETURN) continue;
        if (classification == CC_SPACE) break;
        if (classification == CC_ESCAPE_OR_END &&
            (recover_special_character() & ~0x50U)) continue;
        if (classification != CC_SLASH || !skip_source_comment())
            report_syntax_error(8, (unsigned)character);
        break;
    }
    bytes = read_macro_replacement();
    if (!options.configuration.extensions_enabled) {
        bytes += (unsigned)(macro_replacement_text - macro_work_buffer);
        replacement = macro_work_buffer;
    } else replacement = macro_replacement_text;
    define_macro(replacement, bytes, macro_formal_count);
}

/* Original 0x1fad0..0x1fb23. Missing replacement text is equal only to
   another missing replacement; otherwise compare the requested byte span. */
int __stdcall macro_replacement_diff(char *left, char *right, unsigned bytes)
{
    unsigned i;
    if (!left || !right)
        return left != right;
    for (i = 0; i < bytes; ++i)
        if (left[i] != right[i])
            return 1;
    return 0;
}

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

/* Original 0x1fb30..0x1ff28. Compress replacement whitespace, retain
   literals, and encode references to formal macro parameters. */
unsigned read_macro_replacement(void)
{
    char *cursor = macro_replacement_text, *identifier;
    unsigned char character, classification;
    unsigned recovery;
    int parameter, stringify = 0, characterize = 0;
read_nonspace:
    character = next_directive_character();
scan_character:
    if ((unsigned)(macro_work_buffer + macro_work_bytes - 5) <= (unsigned)cursor)
        fatal_error(11, token_text);
    classification = character_class[character];
    switch (classification) {
    case CC_SLASH:
        if (!skip_source_comment()) goto copy_character;
        /* Fall through: a comment is replaced by ordinary whitespace. */
    case CC_SPACE: case CC_CARRIAGE_RETURN:
        character = next_directive_character();
        if (character_class[character] != CC_NEWLINE) *cursor++ = ' ';
        goto scan_character;
    case CC_ESCAPE_OR_END:
        recovery = recover_special_character();
        if (recovery & 0x50) {
            if (!(recovery & 0x10)) goto copy_character;
            *cursor++ = character;
            character = next_source_character();
            classification = character_class[character];
            if (classification == CC_APOSTROPHE || classification == CC_QUOTE) {
                --cursor;
                report_syntax_error(295, (unsigned)character);
                goto read_nonspace;
            }
            goto copy_character;
        }
        character = (unsigned char)*source_text_cursor++;
        goto scan_character;
    case CC_APOSTROPHE: case CC_QUOTE:
        cursor = copy_macro_literal(cursor, macro_work_buffer + macro_work_bytes - 5, character);
        character = (unsigned char)*source_text_cursor++;
        goto scan_character;
    case CC_INVALID:
        report_syntax_error(18, (unsigned)character);
        character = (unsigned char)*source_text_cursor++;
        goto scan_character;
    case CC_HASH:
read_hash_suffix:
        classification = character_class[(unsigned char)*source_text_cursor++];
        if (classification == CC_ESCAPE_OR_END &&
            (recover_special_character() & ~0x50U)) goto read_hash_suffix;
        if (classification == CC_HASH) {
            if (cursor == macro_replacement_text) {
                report_syntax_error(160);
                goto scan_character;
            }
            if (cursor[-1] == ' ') --cursor;
            character = next_directive_character();
            if (character_class[character] == CC_NEWLINE) {
                --source_text_cursor;
                report_syntax_error(161);
            }
            goto scan_character;
        }
        if (classification == CC_AT && macro_formal_count &&
            options.configuration.extensions_enabled) {
            characterize = 1;
        } else if (!macro_formal_count || classification == CC_AT) {
            *cursor++ = '#';
            character = (unsigned char)source_text_cursor[-1];
            goto scan_character;
        } else {
            --source_text_cursor;
            stringify = 1;
        }
        character = next_directive_character();
        classification = character_class[character];
        if (classification != CC_IDENTIFIER && classification != CC_WIDE_PREFIX) {
            stringify = characterize = 0;
            report_syntax_error(162);
            goto scan_character;
        }
        /* Fall through to the parameter-name scan. */
    case CC_IDENTIFIER: case CC_WIDE_PREFIX:
        identifier = cursor;
write_identifier:
        if ((unsigned)(macro_work_buffer + macro_work_bytes - 5) <= (unsigned)cursor)
            fatal_error(11, token_text);
        *cursor++ = character;
read_identifier_character:
        character = (unsigned char)*source_text_cursor++;
        if (character_flags[character] & CF_IDENTIFIER) goto write_identifier;
        if (character_class[character] == CC_ESCAPE_OR_END &&
            (recover_special_character() & ~0x50U)) goto read_identifier_character;
        *cursor = 0;
        parameter = find_macro_parameter(identifier);
        if (parameter >= 1) {
            cursor = identifier;
            *cursor++ = stringify ? 2 : (characterize ? 3 : 1);
            *cursor++ = (unsigned char)parameter;
        } else if (stringify || characterize) report_syntax_error(162);
        stringify = characterize = 0;
        goto scan_character;
    case CC_NEWLINE:
        --source_text_cursor;
        if (cursor == (options.configuration.extensions_enabled ?
                       macro_replacement_text : macro_work_buffer)) return 0;
        if ((unsigned)(macro_work_buffer + macro_work_bytes - 5) <= (unsigned)cursor)
            fatal_error(11, token_text);
        *cursor++ = 0;
        *cursor = 0x44;
        return cursor + 1 - macro_replacement_text;
    }
copy_character:
    *cursor++ = character;
    character = (unsigned char)*source_text_cursor++;
    goto scan_character;
}

/* Original 0x1f7d0..0x1f904. Formal names are stored as a byte-sized
   record length followed by the spelling and its terminating NUL. */
void read_macro_parameters(void)
{
    char *name, *cursor;
    unsigned char character, classification;
new_parameter:
    cursor = macro_replacement_text + 1;
    name = cursor;
next_character:
    character = next_directive_character();
    classification = character_class[character];
    if (classification == CC_COMMA || classification == CC_RIGHT_PAREN) {
        if ((unsigned)name < (unsigned)cursor) {
            if (find_macro_parameter(name) >= 1) report_syntax_error(9, name);
            else {
                *macro_replacement_text = (unsigned char)(cursor - macro_replacement_text);
                macro_replacement_text = cursor;
                ++macro_formal_count;
            }
        } else if (classification == CC_COMMA || macro_formal_count > 0)
            report_syntax_error(10, (unsigned)character);
        if (character_class[character] == CC_RIGHT_PAREN) return;
        goto new_parameter;
    }
    if (classification != CC_IDENTIFIER && classification != CC_WIDE_PREFIX) {
        report_syntax_error(10, (unsigned)character);
        goto next_character;
    }
    if (name != cursor) report_syntax_error(10, (unsigned)character);
    *cursor++ = character;
read_identifier:
    character = (unsigned char)*source_text_cursor++;
    while (character_flags[character] & CF_IDENTIFIER) {
        *cursor++ = character;
        character = (unsigned char)*source_text_cursor++;
    }
    if (!character) {
        if (fill_source_buffer()) macro_expansion_error(4);
        goto read_identifier;
    }
    if (character == '\\' && consume_line_escape()) goto read_identifier;
    --source_text_cursor;
    *cursor++ = 0;
    goto next_character;
}

/* Original 0x1ff30..0x1ff92. Formal records retain their byte-sized
   stride; the comparison is inline in the original, without a CRT call. */
int __stdcall find_macro_parameter(char *name)
{
    char *record = macro_work_buffer;
    char *left, *right;
    int index;
    for (index = 1; index <= macro_formal_count; ++index) {
        left = name;
        right = record + 1;
        while (*left == *right) {
            if (!*left) return index;
            ++left;
            ++right;
        }
        record += (unsigned char)*record;
    }
    return -1;
}

/* Original 0x1f600..0x1f697. Remove only this interned definition; predefined
   names are protected, and browser references follow the actual removed owner. */
void undefine_source_macro(void)
{
    MacroDefinition *definition, *previous;
    unsigned bucket;
    if (!include_depth && match_pch_string_event(7, 0, interned_name)) return;
    if (interned_name == name_defined) {
        report_macro_error("#undef");
        return;
    }
    bucket = (unsigned)interned_name & 511;
    previous = 0;
    definition = macro_buckets[bucket];
    while (definition && definition->name != interned_name) {
        previous = definition;
        definition = definition->next;
    }
    if (!definition) return;
    if (definition->parameter_count < -2) {
        report_macro_error("#undef");
        return;
    }
    if (previous) previous->next = definition->next;
    else macro_buckets[bucket] = definition->next;
    if (browser_filename) reference_browser_macro(definition);
}

/* Original0x3c930..0x3c94a, stdcall4. Protected macro names keep
   their actual interned spelling and the rejected directive in warning117. */
void __stdcall report_macro_error(char *directive)
{
    compiler_warning(1, 117, interned_name, directive);
}

/* Original0x72e60..0x72e8f, stdcall4. Preserve the second real allocation
   attempt before the original fatal diagnostic; no invented reserve release. */
void *__stdcall retry_malloc(unsigned bytes)
{
    void *memory = malloc(bytes);
    if (!memory) memory = malloc(bytes);
    if (!memory) { fatal_error(60); return 0; }
    return memory;
}
