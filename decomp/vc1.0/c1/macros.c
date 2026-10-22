/* Command-line and predefined macro setup. */
#include <time.h>
#include <ctype.h>
#include <stdlib.h>
#include "macros.h"
#include "keywords.h"
#include "characters.h"
#include "lexer.h"
#include "source-input.h"

char *name_defined, *name_formal, *name_ellipsis, *name_cast;
char *name_main, *name_winmain, *name_dllmain, *name_alignment;
char *name_unnamed, *name_dollar, *name_static_guard, *name_setjmp, *name_catch;
static struct SpecialIdentifier {
    char **destination;
    char *spelling;
} special_identifiers[] = {
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
/* The original 25 copied bytes are followed by a zeroed BSS terminator. */
char macro_timestamp[26]; /* original 0x607f8 */
char *macro_date, *macro_time; /* original 0x607f4 / 0x607f0 */

/* Original 0x1bf70..0x1c185. */
void __stdcall define_command_line_macros(Vector *definitions)
{
    struct SpecialIdentifier *identifier;
    long now;
    char *source, *destination;
    short index;
    character_class[1] = CC_MACRO_PARAMETER;
    character_class[2] = CC_MACRO_STRING;
    character_class[3] = CC_MACRO_TOKEN;
    character_class[4] = CC_MACRO_END;
    if (options.extensions_enabled) {
        character_class['$'] = CC_IDENTIFIER;
        character_flags['$'] = CF_IDENTIFIER;
    }
    for (identifier = special_identifiers; identifier->destination; ++identifier)
        *identifier->destination = intern_identifier(identifier->spelling);
    if (!options.option_Brepro) {
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
    if (!options.extensions_enabled) {
        intern_identifier("__STDC__");
        define_macro(0, 0, -7);
    }
    for (index = 0; index < definitions->count; ++index)
        define_command_line_macro(definitions->items[index]);
    if (!options.option_E && !options.prototypes_only)
        write_global_header();
    open_source_file(source_filename, 1);
    forced_source_inputs = options.forced_includes;
    forced_source_index = 0;
    if (options.option_E && forced_source_inputs->count > forced_source_index) {
        current_forced_source = forced_source_inputs->items[forced_source_index++];
        open_include_file(current_forced_source, 0);
    }
}

/* Original 0x4be00..0x4be10. */
long __stdcall frontend_time(long *result)
{
    return time(result);
}

/* Original timestamp workspace at 0x68350. */
/* Include the zeroed BSS byte read by the caller's string-copy loop. */
static char formatted_timestamp[26];

/* Original 0x4bda0..0x4bdfb. */
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

MacroDefinition **macro_buckets; /* original 0x6b644 */
unsigned macro_work_bytes = 0xf000; /* original 0x6bff4 */
char *macro_work_buffer, *macro_expansion_next; /* 0x6078c / 0x63898 */
char *macro_replacement_start; /* original 0x63934 */
char *macro_expansion_start; /* original 0x63938 */
char *macro_replacement_text; /* original 0x6393c */
short include_depth = -1; /* original 0x6a7d4 */

/* Original 0x1c550..0x1c5ae. */
void initialize_macro_storage(void)
{
    macro_buckets = allocate_permanent(2048);
    macro_work_buffer = malloc(macro_work_bytes);
    if (!macro_work_buffer) {
        macro_work_bytes >>= 1;
        macro_work_buffer = retry_malloc(macro_work_bytes);
    }
    macro_expansion_next = macro_work_buffer + (macro_work_bytes >> 1);
    macro_replacement_start = macro_expansion_next;
    macro_expansion_start = macro_work_buffer;
}

/* Original 0x1c8c0..0x1ca3b. The identifier being defined is the
   interner's current name; parameter counts below -2 denote predefined macros. */
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

/* Original 0x1ca40..0x1ca7b. */
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

/* Original 0x1c190..0x1c28d. */
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

int macro_space_seen; /* original 0x63928 */

/* Original 0x1dff0..0x1e0fb. Preserve a single separating space in the
   macro output cursor while inspecting input without consuming its token. */
int skip_macro_space(void)
{
    unsigned flags;
    unsigned char character, class;
    int result = 0;
    scanner_flags &= ~0x800000UL;
    macro_space_seen = 0;
    for (;;) {
        character = (unsigned char)*source_text_cursor++;
        class = character_class[character];
        if (class <= CC_CARRIAGE_RETURN) macro_space_seen = 1;
        else if (class == CC_ESCAPE_OR_END) {
            flags = recover_special_character();
            if (flags & 0xa) break;
            if (flags & ~0x50U) continue;
            --source_text_cursor;
            result = 1;
            break;
        } else if (class == CC_NEWLINE) {
            if (scanner_flags & 0x10000) { --source_text_cursor; break; }
            ++source_line_number;
            if (options.option_E) fputc((unsigned char)source_text_cursor[-1], preprocessor_stream);
            scanner_flags |= 0x800000;
            macro_space_seen = 1;
        } else {
            --source_text_cursor;
            result = 1;
            break;
        }
    }
    if (macro_space_seen) {
        if (macro_work_buffer + macro_work_bytes - 5 <= macro_expansion_next)
            macro_scan_error(56);
        if (macro_expansion_next[-1] != ' ') *macro_expansion_next++ = ' ';
    }
    return result;
}

/* Original 0x1ca80..0x1cad3. Empty definitions compare by nullness;
   nonempty replacements compare the caller-supplied encoded byte count. */
int __stdcall macro_replacement_diff(char *left, char *right, unsigned bytes)
{
    unsigned index;
    if (!left || !right) return left != right;
    for (index = 0; index < bytes; ++index)
        if (left[index] != right[index]) return 1;
    return 0;
}
