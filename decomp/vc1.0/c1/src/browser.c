/* Definitions/references emitted for the browser database builder. */
#include <direct.h>
#include "browser.h"
#include "source-input.h"
#include "global-ir.h"
#include <recon.h>

unsigned char browser_symbol_bits[8192]; /* original0x5b5a0..0x5d59f inclusive */
FILE *browser_output; /* original0x5d5a0 */

/* Original0x271a0..0x2720f. Stage records in a neighboring temporary file
   and use a4096-byte fully buffered stream before writing its header. */
void begin_browser_output(void)
{
    char name[256];
    make_browser_temporary_name(name);
    browser_output = fopen(name,"w+");
    if (!browser_output) report_fatal(83,message_text(340),name);
    setvbuf(browser_output,0,_IOFBF,4096);
    write_browser_header();
}

/* Original0x264d0..0x26537. Preserve the requested basename and replace
   its final character with a neighboring temporary suffix. */
void __stdcall make_browser_temporary_name(char *name)
{
    char *source = options.option_Fr, *destination = name, *last;
    do { *destination++ = *source++; } while (destination[-1]);
    last = destination - 2;
    last[1] = 0;
    *last = *last == 't' ? 'u' : 't';
}
/* Original0x275b0..0x275b9. */
void write_browser_header(void)
{
    write_browser_record(0,0);
}

int browser_suppressed; /* original0x6b58c */

/* Original0x27130..0x2718f. Initialize only the record's meaningful fields;
   retain the original reserved/padding bytes rather than clearing the object. */
void __stdcall write_browser_record(unsigned char kind, void *value)
{
    BrowserRecord record;
    if (!browser_suppressed) {
        record.kind = kind;
        record.line = source_line_number;
        record.name = 0;
        record.source_name = 0;
        record.attributes = 0;
        record.symbol_kind = 0;
        record.symbol_index = 0;
        prepare_browser_record(&record,value);
        append_browser_record(&record);
    }
}

/* Original0x27020..0x270d8. Classify raw browser events while preserving
   their individual symbol/macro/file builders and internal event codes. */
void __stdcall prepare_browser_record(BrowserRecord *record, void *value)
{
    record->line = source_line_number;
    switch (record->kind) {
    case 1: prepare_browser_file_record(record); break;
    case 3: prepare_browser_definition(record,value); break;
    case 4: prepare_browser_reference(record,value); break;
    case 11: prepare_browser_symbol_record(record,value); break;
    case 253: prepare_browser_macro_definition(record,value); break;
    case 254: prepare_browser_macro_reference(record,value); break;
    case 255: prepare_browser_anonymous_record(record); break;
    }
}
BrowserRecord browser_line_template; /* original0x6387c */
FrontendSymbol *browser_last_reference; /* original0x63878 */
unsigned browser_pending_flags; /* original0x63894 */
int browser_last_line = -1; /* original0x6b590 */

/* Original0x26c50..0x26cff. Internal builder events are suppressed; ordinary
   symbol events first insert a line record when their source line changes. */
void __stdcall append_browser_record(BrowserRecord *record)
{
    BrowserRecord line;
    if (!record || browser_suppressed || record->kind == 254 ||
        record->kind == 253 || record->kind == 252) return;
    switch (record->kind) {
    case 0: case 1: case 2: case 8: case 9: case 10: case 14: case 15: case 16:
        break;
    default:
        if (record->line != browser_last_line) {
            line = browser_line_template;
            browser_pending_flags = 0;
            browser_last_reference = 0;
            browser_last_line = record->line;
            line.kind = 2;
            line.line = browser_last_line;
            append_browser_record(&line);
        }
        break;
    }
    auxiliary_ir_stream = browser_output;
    serialize_browser_record(record,!options.z_n);
}

/* Preserve the inlined CRT word-buffer operation without adding a callee. */
#define BROWSER_WORD(value_) do { \
    unsigned short word_ = (unsigned short)(value_); \
    FILE *stream_ = auxiliary_ir_stream; \
    if ((stream_->_cnt -= 2) >= 0) { \
        *(unsigned short *)stream_->_ptr = word_; stream_->_ptr += 2; \
    } else { stream_->_cnt += 2; fwrite(&word_,2,1,stream_); } \
} while (0)
#define BROWSER_MARK(index_) \
    (browser_symbol_bits[(index_) >> 3] |= (unsigned char)(1U << ((index_) & 7)))

/* Original0x26540..0x2684b; dispatch0x2684c..0x2688b. Browser indices
   are serialized as fixed sixteen-bit values; tracking marks referenced or
   scope-sensitive definitions for the later filtering pass. */
void __stdcall serialize_browser_record(BrowserRecord *record, int track_symbols)
{
    unsigned index;
    putc((signed char)record->kind,auxiliary_ir_stream);
    switch (record->kind) {
    case 0:
        putc(2,auxiliary_ir_stream);
        putc(0,auxiliary_ir_stream);
        putc(2,auxiliary_ir_stream);
        putc(0,auxiliary_ir_stream);
        write_ir_string(browser_current_directory());
        return;
    case 1: case 15:
        write_ir_string(record->name);
        return;
    case 2:
        BROWSER_WORD(record->line);
        return;
    case 3:
        if (track_symbols) {
            if ((record->attributes & 1) ||
                ((record->symbol_kind == 1 || record->symbol_kind == 4) ?
                    !(record->attributes & 0x20) :
                    (record->symbol_kind >= 16 && record->symbol_kind <= 17)))
                BROWSER_MARK(record->symbol_index);
        }
        putc((signed char)record->symbol_kind,auxiliary_ir_stream);
        BROWSER_WORD(record->attributes);
        index = record->symbol_index;
        putc(index & 255,auxiliary_ir_stream);
        putc((index >> 8) & 255,auxiliary_ir_stream);
        write_ir_string(record->name ? record->name : "");
        return;
    case 4: case 5: case 11:
        if (track_symbols) BROWSER_MARK(record->symbol_index);
        index = record->symbol_index;
        putc(index & 255,auxiliary_ir_stream);
        putc((index >> 8) & 255,auxiliary_ir_stream);
        return;
    }
}

/* Original0x43cc0..0x43d17. Ordinary records retain the actual directory;
   reproducible mode substitutes a dot, using the same temporary arena buffer. */
char *browser_current_directory(void)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    char *name;
    if (pool->remaining < 256) refill_arena(&compiler_arena,pool);
    name = (char *)pool->next;
    pool->remaining -= 256;
    pool->next += 256;
    if (!options.option_Brepro) return _getcwd(name,256);
    name[0] = '.';
    name[1] = 0;
    return name;
}

/* Original0x275d0..0x275de. */
void __stdcall browser_symbol_definition(FrontendSymbol *symbol)
{
    write_browser_record(3,symbol);
}

/* Browser categories for the frontend's twelve symbol variants. Original
   0x6b5a0: tags share category17; labels and ordinary/function names have
   their own categories. Type aliases are refined from their base type below. */
unsigned char browser_symbol_categories[12] = {
    0, 0, 17, 17, 0, 10, 1, 1, 2, 4, 3, 0
};

/* Original0x26e10..0x26f60. Exclude internal symbols, then distinguish
   storage, nesting and the base type for a browser definition record. */
void __stdcall prepare_browser_definition(BrowserRecord *record, FrontendSymbol *symbol)
{
    unsigned kind = symbol->kind & 31;
    unsigned storage, type;
    int tag;
    if (kind == 11 || (symbol->flags & 0x20)) {
        record->kind = 252;
        return;
    }
    prepare_browser_symbol_record(record,symbol);
    storage = symbol->storage[0] & 7;
    if (storage == 7) {
        record->attributes = 0;
        record->symbol_kind = 7;
        return;
    }
    tag = kind == 2 || kind == 3;
    if (storage == 1 || storage == 2 || storage == 3) {
        record->attributes = storage == 3 ? 2 : 0x40;
        if ((kind == 7 && !(symbol->flags & 1)) ||
            (kind != 7 && storage == 1 && !(symbol->attributes & 4)))
            record->attributes |= 0x20;
    } else if (parameter_nesting >= 2) record->attributes |= 1;
    if (tag) record->attributes |= 0x40;
    if (kind != 4) {
        record->symbol_kind = browser_symbol_categories[kind];
        return;
    }
    type = symbol->type->flags;
    if ((type & 0x408) == 0x408) record->symbol_kind = 15;
    else if ((type & 0x402) == 0x402) record->symbol_kind = 8;
    else if ((type & 0x404) == 0x404) record->symbol_kind = 11;
    else if (type & 0x200) record->symbol_kind = 9;
}

/* Original0x26df0..0x26e0f. */
void __stdcall prepare_browser_symbol_record(BrowserRecord *record, FrontendSymbol *symbol)
{
    record->symbol_index = symbol->index;
    if (!record->name) record->name = record->source_name = symbol->name;
}

/* Original0x27600..0x27611. */
void __stdcall browser_macro(MacroDefinition *definition)
{
    write_browser_record(253,definition);
}

/* Original0x26d50..0x26d7e. Object and parameterized macro definitions
   become ordinary definition records with distinct browser categories. */
void __stdcall prepare_browser_macro_definition(BrowserRecord *record, MacroDefinition *macro)
{
    record->name = record->source_name = macro->name;
    record->attributes = 0;
    record->symbol_kind = macro->parameter_count == 0 ? 5 : 6;
    record->symbol_index = macro->browser_index;
    record->kind = 3;
}

/* Original0x275a0..0x275a9. */
void browser_open_file(void)
{
    write_browser_record(1,0);
}

/* Original0x26dd0..0x26dec. File names own a temporary heap copy; source
   association is intentionally absent for these records. */
void prepare_browser_file_record(BrowserRecord *record)
{
    record->name = copy_browser_name(source_filename);
    record->source_name = 0;
}

/* Original0x26d80..0x26dc3. The caller retains the heap name until the
   record-filtering pass; copying uses compiler-inlined string operations. */
char *copy_browser_name(char *name)
{
    char *source = name, *copy, *destination;
    unsigned bytes = 1;
    while (*source++) ++bytes;
    destination = copy = malloc(bytes);
    source = name;
    do { *destination++ = *source++; } while (destination[-1]);
    return copy;
}

int browser_scope_depth; /* original0x6b5b4 */
/* Original0x27520..0x2752f. */
void browser_begin_function_scope(void)
{
    ++browser_scope_depth;
    write_browser_record(8,0);
}

/* Original0x27630..0x2763e. */
void __stdcall browser_function_parameters(FrontendSymbol *symbol)
{
    write_browser_record(11,symbol);
}

/* Original0x274f0..0x27513. Unbalanced close requests clamp the state;
   only a matching open scope emits an end record. */
void browser_finish_scope(void)
{
    if (browser_scope_depth < 1) browser_scope_depth = 0;
    else {
        --browser_scope_depth;
        write_browser_record(9,0);
    }
}

/* Original0x27590..0x27599. */
void browser_close_file(void)
{
    write_browser_record(10,0);
}

/* Original0x275e0..0x275f1. */
void __stdcall browser_macro_reference(MacroDefinition *definition)
{
    write_browser_record(254,definition);
}

/* Original0x26f70..0x26f83. */
void __stdcall prepare_browser_macro_reference(BrowserRecord *record, MacroDefinition *macro)
{
    record->kind = 4;
    record->symbol_index = macro->browser_index;
}

/* Original0x275c0..0x275ce. */
void __stdcall browser_symbol_reference(FrontendSymbol *symbol)
{
    write_browser_record(4,symbol);
}

/* Original0x26f90..0x26fb7. Internal symbols and a retained immediately
   preceding reference are suppressed before selecting the symbol index. */
void __stdcall prepare_browser_reference(BrowserRecord *record, FrontendSymbol *symbol)
{
    record->kind = 252;
    if (browser_last_reference != symbol && !(symbol->flags & 0x20)) {
        prepare_browser_symbol_record(record,symbol);
        record->kind = 4;
    }
}
