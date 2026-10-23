/* CodeView symbol stream records, introduced as their writers are reached. */
#include "codeview-output.h"
#include "coff-output.h"
#include "options.h"
#include "records.h"
#include <string.h>
#include <recon.h>
int codeview_output_started; /* original84fd8 */
extern int optimize_global;
DebugScope *codeview_scope_cursor; /* original7e8ac */

/* Original69250..692a3. Emit the procedure and lexical scopes, then the
   S_END record (the existing begin_debug_function placeholder at69210). */
void __fastcall end_debug_function(FunctionSymbol *symbol)
{
    unsigned storage;
    if (!symbol || symbol->symbol.kind != 4 ||
        ((storage = symbol->field24 & 0xf0) != 0x50 && storage != 0x30))
        compiler_internal_error("@(#)emit.c:1.20", 0x238);
    write_codeview_function(symbol);
    codeview_scope_cursor = current_function->debug_scope;
    write_debug_scopes(symbol);
    begin_debug_function();
}
/* Original69580..6964f. The signed scope line field is lexical depth.
   The shared cursor stops at the first scope outside this recursive walk. */
void __fastcall write_debug_scopes(FunctionSymbol *symbol)
{
    DebugScope *scope;
    DebugLink *record;
    int depth, block;
    if (!codeview_scope_cursor) return;
    depth = codeview_scope_cursor->line;
    while ((scope = codeview_scope_cursor) != NULL && scope->line >= depth) {
        block = scope->records && scope->line > 2 && !optimize_global;
        if (block)
            write_codeview_block(symbol, scope->start_offset,
                scope->end_offset - scope->start_offset);
        for (record = codeview_scope_cursor->records; record; record = record->next)
            write_debug_symbol(record->symbol);
        scope = codeview_scope_cursor->next;
        if (scope && scope->line > depth) {
            codeview_scope_cursor = scope;
            write_debug_scopes(symbol);
            scope = codeview_scope_cursor;
        }
        codeview_scope_cursor = scope;
        if (block) begin_debug_function();
    }
}

/* Original69100..69114; final call is a tail jump in the original. */
void initialize_codeview_output(void)
{
    codeview_output_started = 1;
    write_codeview_signature();
    write_codeview_object_name();
}

/* Original69cd0..69d03. The initial CodeView symbol-stream signature. */
void write_codeview_signature(void)
{
    unsigned signature = 1;
    output_callbacks.select_segment(debug_symbols_segment);
    output_callbacks.data(&signature, sizeof(signature));
    debug_symbols_segment->symbol.field04 += sizeof(signature);
}

/* The kind10 symbol input at4d8a5 owns this same original83278 word. */
extern long il_option_flags;
unsigned __fastcall copy_codeview_string(unsigned char *destination, char *source);
#pragma pack(2)
typedef struct CodeViewProcedureRecord {
    unsigned short length, kind;
    unsigned parent, end, next;
    unsigned code_bytes, debug_start, debug_end, offset;
    unsigned short section, type;
    unsigned char flags, name[270];
} CodeViewProcedureRecord;
#pragma pack()
typedef char CodeViewProcedureSizeCheck[sizeof(CodeViewProcedureRecord) == 308 ? 1 : -1];
/* Original69650..69728. A32-bit procedure record plus its counted name;
   the two relocation words are filled by the selected object backend. The
   original computes length before copy_codeview_string's255-byte limit. */
void __fastcall write_codeview_function(FunctionSymbol *symbol)
{
    CodeViewProcedureRecord record;
    char *name = format_function_name(&symbol->symbol), *end;
    unsigned length;
    output_callbacks.select_segment(debug_symbols_segment);
    end = name;
    while (*end++) ;
    length = (unsigned)(end - name) + 35;
    record.length = (unsigned short)length;
    record.kind = (symbol->field24 & 0xf0) == 0x50 ? 0x205 : 0x204;
    record.parent = record.end = record.next = 0;
    record.code_bytes = symbol->code_bytes;
    record.debug_start = (long)(short)symbol->prologue_bytes;
    record.debug_end = (long)(short)symbol->epilogue_offset;
    record.offset = 0;
    record.section = 0;
    record.type = (unsigned short)symbol->field20;
    record.flags = 0;
    copy_codeview_string(record.name, name);
    output_callbacks.data(&record, length + 2);
    output_callbacks.size(26 - length, &symbol->symbol);
    debug_symbols_segment->symbol.field04 += length + 2;
}
/* Original69c60..69cc9. S_OBJNAME consists of the fixed header and a
   one-byte counted object name; the terminating source NUL is not emitted. */
void write_codeview_object_name(void)
{
    struct {
        unsigned short length, kind;
        unsigned signature;
        unsigned char name[300];
    } record;
    char *end = object_filename;
    unsigned length;
    while (*end++) ;
    length = (unsigned)(end - object_filename) + 6;
    record.length = (unsigned short)length;
    record.kind = 9;
    record.signature = (unsigned)il_option_flags;
    copy_codeview_string(record.name, object_filename);
    output_callbacks.select_segment(debug_symbols_segment);
    output_callbacks.data(&record, length + 2);
    debug_symbols_segment->symbol.field04 += length + 2;
}

/* Original69520..69580, ECX destination/EDX source. Preserve the original
   two-byte spare tail convention at length+2 (outside the emitted count). */
unsigned __fastcall copy_codeview_string(unsigned char *destination, char *source)
{
    char *end;
    int length = 0;
    if (source) {
        end = source;
        while (*end++) ;
        length = (int)(end - source) - 1;
    }
    if (length > 255) {
        compiler_warning(1, 0x312, 255);
        length = 255;
    }
    destination[0] = (unsigned char)length;
    strncpy((char *)destination + 1, source, length);
    destination[length + 2] = 0;
    return length + 1;
}
