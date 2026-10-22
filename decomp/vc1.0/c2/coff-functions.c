/* COFF function symbols, source positions and associated debug segments. */
#include "coff-output.h"
#include "coff-writer.h"
#include "output.h"
#include "records.h"
#include "functions.h"
#include "machine.h"
#include "coff-segments.h"
extern int codeview_debug, line_debug;
extern int byte_block_first;
extern char *generated_source;
unsigned next_symbol_identifier(void);
unsigned coff_function_symbol; /* 0007e8bc: current writer symbol handle */
unsigned coff_function_line; /* 0007e8c4 */
unsigned short coff_has_function_lines; /* 0007e8b4 */
unsigned coff_last_function_line; /* 0007e8b8 */
int coff_emitted_function; /* 000850c8 */
static Segment *coff_default_fpo_segment; /* 0007e8c8 */
static char *coff_last_function_filename; /* 0007e8cc */
Segment *coff_function_fpo_segment, *coff_function_debug_segment; /* 00079148,0007914c */
/* Original [0006ccd0,0006cce0): COFF's derived-function type bit. */
unsigned short __fastcall coff_symbol_type(SymbolHeader *symbol)
{
    return symbol->kind == 4 ? 0x20 : 0;
}
/* Original [0006d310,0006d360): line records start with the function handle. */
void __fastcall coff_output_symbol(FunctionSymbol *symbol)
{
    if (coff_function_line && (codeview_debug || line_debug)) {
        coff_begin_lines(coff_function_symbol, (unsigned short)coff_function_line,
            symbol->code_bytes, symbol->symbol.field04);
        coff_has_function_lines = 1;
    } else coff_has_function_lines = 0;
}
/* Original [0006d360,0006d3b0). Close line/debug records after code bytes. */
void __fastcall coff_output_function(FunctionSymbol *symbol)
{
    if (coff_has_function_lines && (codeview_debug || line_debug)) {
        coff_line_marker(coff_has_function_lines);
        coff_end_lines((unsigned short)coff_last_function_line,
            symbol->symbol.field04 + symbol->code_bytes);
    }
    coff_emitted_function = 1;
}
/* Original [0006cac0,0006ccd0). Prepare the function symbol and optional
   per-function debug/FPO sections, then define its COFF entry. */
void __fastcall coff_begin_function(FunctionSymbol *symbol, char *filename, int frame_metadata, unsigned line)
{
    Segment *segment;
    unsigned char *left, *right;
    char *name;
    unsigned storage_class, offset;
    unsigned short section, type;
    unsigned char storage = 0;
    coff_function_line = line;
    coff_mark_symbol(&symbol->symbol);
    if (codeview_debug) {
        if (!(symbol->symbol.field0e & 0x20))
            coff_function_debug_segment = (Segment *)lookup_global_symbol(7);
        else {
            segment = create_segment(".debug$S", "DEBSYM", next_symbol_identifier());
            coff_function_debug_segment = segment;
            segment->alignment = 1;
            segment->word_size = 7;
            segment->format_flags = 1;
            segment->section_number = 4;
            segment->selection = 5;
            segment->identity.owner = &symbol->symbol;
            segment->field45 = 0;
            segment->parent = (Segment *)lookup_global_symbol(symbol->symbol.field18.identifier);
        }
    }
    if (frame_metadata) {
        if (!(symbol->symbol.field0e & 0x20)) {
            if (!coff_default_fpo_segment) {
                segment = create_segment(".debug$F", "DEBFPO", next_symbol_identifier());
                coff_default_fpo_segment = segment;
                coff_function_fpo_segment = segment;
                segment->alignment = 1;
                segment->word_size = 0;
                segment->section_number = 9;
            } else coff_function_fpo_segment = coff_default_fpo_segment;
        } else {
            segment = create_segment(".debug$F", "DEBFPO", next_symbol_identifier());
            coff_function_fpo_segment = segment;
            segment->alignment = 1;
            segment->word_size = 7;
            segment->section_number = 9;
            segment->selection = 5;
            segment->identity.owner = &symbol->symbol;
            segment->field45 = 0;
            segment->parent = (Segment *)lookup_global_symbol(symbol->symbol.field18.identifier);
        }
    }
    if (!coff_last_function_filename) {
        if (!generated_source) coff_last_function_filename = filename;
        else {
            left = (unsigned char *)generated_source;
            right = (unsigned char *)filename;
            while (*left == *right && *left) { ++left; ++right; }
            if (*left == *right) coff_last_function_filename = filename;
        }
    }
    if (filename != coff_last_function_filename && line && (codeview_debug || line_debug)) {
        coff_record_filename(filename);
        coff_last_function_filename = filename;
    }
    storage_class = (symbol->field24 >> 4) & 15;
    if (storage_class == 3) storage = 3;
    else if (storage_class == 5) storage = 2;
    else compiler_internal_error("@(#)coff.c:1.14", 0x118);
    offset = symbol->symbol.field04;
    section = (unsigned short)((Segment *)lookup_global_symbol(symbol->symbol.field18.identifier))->object_section;
    type = coff_symbol_type(&symbol->symbol);
    name = symbol_display_name(&symbol->symbol);
    coff_function_symbol = coff_define_function(symbol->symbol.identifier, name, section,
        storage, offset, type, (short)(line != 0));
    if (!symbol->symbol.field14) {
        symbol->symbol.field14 = coff_function_symbol;
        ((BackendSymbol *)symbol)->output_flags |= 1;
    }
}

/* Original [0006cce0,0006d010). A reference makes a symbol eligible for
   output; definitions may also require their logical section to be opened.
   The shared +24 attributes have different storage fields for objects and
   functions. Storage values outside those assigned by the front end have
   indeterminate writer arguments in the original. */
void __fastcall coff_mark_symbol(SymbolHeader *symbol)
{
    BackendSymbol *backend = (BackendSymbol *)symbol;
    FunctionSymbol *function, *fallback;
    unsigned attributes, storage_class, value = 0, handle;
    unsigned short section = 0, type;
    unsigned char storage = 0;
    char *name;
    if (backend->output_flags & 1) return;
    if (!(backend->output_flags & 4))
        compiler_internal_error("@(#)coff.c:1.14", 0x191);
    backend->output_flags |= 1;
    if (symbol->kind == 1) {
        attributes = backend->attributes;
        if (attributes & 0x800)
            coff_write_directive("-export:", symbol_display_name(symbol), 0);
        if ((attributes & 0x70) == 0x10 || (attributes & 0x70) == 0x30) return;
        if ((symbol->field0e & 0x20) && (attributes & 0x780) != 0x200) {
            storage = 2;
            section = coff_no_section;
            value = 0;
            type = coff_symbol_type(symbol);
            name = symbol_display_name(symbol);
            symbol->field14 = coff_define_symbol(name, section, storage, value, type);
        }
        switch ((attributes & 0x780) >> 7) {
        case 1: storage = 2; break;
        case 2:
            storage = 2; value = 0; section = coff_no_section;
            break;
        case 3:
            storage = 2; value = symbol->value; section = coff_no_section;
            break;
        case 4:
            if (!byte_block_first && !(symbol->field0e & 0x20)) {
                coff_declare_segment((Segment *)lookup_global_symbol(symbol->field18.identifier));
                return;
            }
            storage = 3;
            break;
        }
        if (((attributes & 0x780) >> 7) == 1 || ((attributes & 0x780) >> 7) == 4) {
            value = symbol->field04;
            section = coff_declare_segment((Segment *)lookup_global_symbol(symbol->field18.identifier));
        }
    } else if (symbol->kind == 3) {
        if (!byte_block_first) return;
        value = symbol->field04;
        section = coff_declare_segment((Segment *)lookup_global_symbol(symbol->field18.identifier));
        type = coff_symbol_type(symbol);
        name = symbol_display_name(symbol);
        symbol->field14 = coff_define_symbol(name, section, 6, value, type);
        return;
    } else if (symbol->kind == 4) {
        function = (FunctionSymbol *)symbol;
        attributes = function->field24;
        if (attributes & 0x800)
            coff_write_directive("-export:", symbol_display_name(symbol), 0);
        if (attributes & 0x8000) {
            fallback = (FunctionSymbol *)lookup_global_symbol(function->field40);
            fallback->flags52 |= 0x20;
            ((BackendSymbol *)fallback)->output_flags |= 4;
            coff_mark_symbol(&fallback->symbol);
            type = coff_symbol_type(symbol);
            name = symbol_display_name(symbol);
            symbol->field14 = coff_define_weak_symbol(name, coff_no_section, 105,
                symbol->field04, type, fallback->symbol.field14, 2);
            return;
        }
        if ((symbol->field0e & 0x20) && (attributes & 0xf0) != 0x30 && !(attributes & 0x1000)) {
            storage = 2; section = coff_no_section; value = 0;
            type = coff_symbol_type(symbol);
            name = symbol_display_name(symbol);
            symbol->field14 = coff_define_symbol(name, section, storage, value, type);
        }
        storage_class = (function->field24 >> 4) & 15;
        if (storage_class == 3 || storage_class == 5) {
            coff_declare_segment((Segment *)lookup_global_symbol(symbol->field18.identifier));
            return;
        }
        if (storage_class == 4) {
            storage = 2; section = coff_no_section; value = 0;
        } else compiler_internal_error("@(#)coff.c:1.14", 0x21a);
    } else return;
    type = coff_symbol_type(symbol);
    name = symbol_display_name(symbol);
    handle = coff_define_symbol(name, section, storage, value, type);
    if (!symbol->field14) symbol->field14 = handle;
}
