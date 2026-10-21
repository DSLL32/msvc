/* Preprocessor lookup and macro text storage. */
#include "preprocessor.h"

PreprocessorSymbol **preprocessor_buckets; /* original 0x6b654 */
unsigned char *macro_text_begin, *macro_text_next; /* original 0x6b678/0x6b67c */

/* Original 0x1adb0..0x1adcd, ending with a tail call to initialize_macro_text. */
void initialize_preprocessor(void)
{
    if (!preprocessor_buckets)
        preprocessor_buckets = allocate_permanent(8192);
    initialize_macro_text();
}

/* Original 0x1b590..0x1b5a5. */
void initialize_macro_text(void)
{
    macro_text_begin = allocate_permanent(8192);
    macro_text_next = macro_text_begin;
}

char *interned_name; /* original 0x6b66c */

/* Original 0x1b460..0x1b587. Keyword names reserve one prefix byte. */
void __stdcall intern_keyword(char *name)
{
    char *input;
    unsigned hash = 0, length, i, remaining;
    PreprocessorSymbol **bucket, **previous, *symbol;
    for (input = name; *input; ++input)
        hash = hash * 4 + (unsigned char)*input + (hash >> 7);
    length = input - name;
    bucket = &preprocessor_buckets[hash & 0x7ff];
    previous = bucket;
    for (symbol = *bucket; symbol; symbol = symbol->next) {
        for (i = 0; i < length && symbol->name[i] == name[i]; ++i)
            ;
        if (i == length && !symbol->name[length]) {
            *previous = symbol->next;
            symbol->next = *bucket;
            *bucket = symbol;
            interned_name = symbol->name;
            return;
        }
        previous = &symbol->next;
    }
    remaining = macro_text_begin - macro_text_next + 8192;
    if (remaining < length + 12)
        initialize_macro_text();
    ++macro_text_next;
    symbol = (PreprocessorSymbol *)macro_text_next;
    symbol->next = *bucket;
    *bucket = symbol;
    for (i = 0; i < length; ++i)
        symbol->name[i] = name[i];
    symbol->name[length] = 0;
    interned_name = symbol->name;
    macro_text_next += length + 6;
}

/* Original 0x1b330..0x1b451. Ordinary identifier interning. */
char *__stdcall intern_identifier(char *name)
{
    char *input;
    unsigned hash = 0, length, i, remaining;
    PreprocessorSymbol **bucket, **previous, *symbol;
    for (input = name; *input; ++input)
        hash = hash * 4 + (unsigned char)*input + (hash >> 7);
    length = input - name;
    bucket = &preprocessor_buckets[hash & 0x7ff];
    previous = bucket;
    for (symbol = *bucket; symbol; symbol = symbol->next) {
        for (i = 0; i < length && symbol->name[i] == name[i]; ++i)
            ;
        if (i == length && !symbol->name[length]) {
            *previous = symbol->next;
            symbol->next = *bucket;
            *bucket = symbol;
            interned_name = symbol->name;
            return interned_name;
        }
        previous = &symbol->next;
    }
    remaining = macro_text_begin - macro_text_next + 8192;
    if (remaining < length + 12)
        initialize_macro_text();
    symbol = (PreprocessorSymbol *)macro_text_next;
    symbol->next = *bucket;
    *bucket = symbol;
    for (i = 0; i < length; ++i)
        symbol->name[i] = name[i];
    symbol->name[length] = 0;
    interned_name = symbol->name;
    macro_text_next += length + 6;
    return interned_name;
}
