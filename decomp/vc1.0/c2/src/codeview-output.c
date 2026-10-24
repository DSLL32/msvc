/* CodeView symbol stream records, introduced as their writers are reached. */
#include "codeview-output.h"
#include "coff-output.h"
#include "options.h"
#include "records.h"
#include "machine.h"
#include "listing.h"
#include "input.h"
#include <string.h>
#include <recon.h>
int codeview_output_started; /* original84fd8 */
extern int optimize_global, coff_object, byte_block_first, cpu_model, floating_frontend;
DebugScope *codeview_scope_cursor; /* original7e8ac */
unsigned codeview_allocation_class; /* original84ff8; alias dispatch override */
FILE *codeview_type_file; /* original7b8ec; db intermediate ownership */
/* Original69ba0..69bd0. The db channel begins with tag12 and one header
   byte; the remaining records use the ordinary initializer grammar. */
void __fastcall read_codeview_initializers(FILE *file)
{
    input_file = file;
    if (input_byte() != 12) compiler_internal_error("@(#)emit.c:1.20", 0x328);
    input_byte();
    read_initializers(file);
}
/* Original69180..6920a. Module-level symbols precede compiler/type records.
   The db channel is the captured fifth frontend input, not a regenerated PDB. */
void finish_debug_module(void)
{
    debug_symbols_segment = (Segment *)lookup_global_symbol(7);
    if (!codeview_output_started) {
        codeview_output_started = 1;
        write_codeview_signature();
        write_codeview_object_name();
    }
    for_each_symbol(0, write_debug_symbol);
    write_codeview_compile_record();
    codeview_type_file = open_intermediate(intermediate_prefix, "db", "r", "db il");
    if (!codeview_type_file) compiler_internal_error("@(#)emit.c:1.20", 0x361);
    read_codeview_initializers(codeview_type_file);
    fclose(codeview_type_file);
}
/* Original69210..69249. This historical placeholder name denotes S_END.
   It has no incoming register/stack arguments; the record is four bytes. */
void begin_debug_function(void)
{
    struct { unsigned short length, kind; } record;
    record.length = 2;
    record.kind = 6;
    output_callbacks.select_segment(debug_symbols_segment);
    output_callbacks.data(&record, sizeof(record));
    debug_symbols_segment->symbol.field04 += sizeof(record);
}

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
/* The44-byte prefix used from a48-byte kind7 alias descriptor. Its final
   input field is not consumed by this dispatcher. */
typedef struct CodeViewAliasSymbol {
    BackendSymbol variable;
    unsigned target_identifier;
} CodeViewAliasSymbol;
typedef char CodeViewAliasPrefixCheck[sizeof(CodeViewAliasSymbol) == 44 ? 1 : -1];
/* Original69930..69b14. Role bits70 and allocation bits780 are distinct
   fields in this symbol-attribute domain; register location is bit8000.
   An alias temporarily borrows its target's name, type and position. */
void __fastcall write_debug_symbol(SymbolHeader *symbol)
{
    BackendSymbol *variable = (BackendSymbol *)symbol, *target;
    CodeViewAliasSymbol *alias;
    char *saved_name;
    unsigned role, saved_offset;
    unsigned short saved_type;
    int kind = symbol->kind;
    if (kind == 1 && !symbol->name) return;
    role = kind == 1 ? variable->attributes & 0x70 : 0;
    if (!(variable->output_flags & 4) &&
        !(kind == 1 && (role == 0x30 ||
            (role == 0x10 && ((symbol->field0e & 0x800) ||
                (variable->attributes & 0x8000))))) &&
        !(kind == 3 && !optimize_global)) return;
    switch (kind) {
    case 1:
        if (!codeview_allocation_class)
            codeview_allocation_class = (variable->attributes & 0x780) >> 7;
        if (!symbol->field0d || symbol->field0d == 0x24) {
            if (role == 0x10 || role == 0x30) {
                if (variable->attributes & 0x8000)
                    write_codeview_register(symbol);
                else write_codeview_frame_variable(symbol);
            } else if (codeview_allocation_class == 4 &&
                (role == 0x60 || role == 0x20 || role == 0x40))
                write_codeview_local_data(symbol);
            else write_codeview_global_data(symbol);
        }
        codeview_allocation_class = 0;
        break;
    case 3:
        if (!symbol->field0d || (symbol->field0e & 0x1000))
            write_codeview_label(symbol);
        break;
    case 7:
        alias = (CodeViewAliasSymbol *)symbol;
        target = (BackendSymbol *)lookup_global_symbol(alias->target_identifier);
        target->output_flags |= 4;
        output_callbacks.mark_symbol(&target->symbol);
        saved_name = target->symbol.name;
        saved_type = target->field20;
        saved_offset = target->symbol.field04;
        target->symbol.name = symbol->name;
        target->field20 = variable->field20;
        target->symbol.field04 += symbol->field04;
        codeview_allocation_class = (variable->attributes & 0x780) >> 7;
        write_debug_symbol(&target->symbol);
        target->symbol.name = saved_name;
        target->output_flags &= ~4;
        target->field20 = saved_type;
        target->symbol.field04 = saved_offset;
        codeview_allocation_class = 0;
        break;
    case 8:
        write_codeview_constant(symbol);
        break;
    case 11:
        write_codeview_typedef(symbol);
        break;
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
typedef struct CodeViewCompileRecord {
    unsigned short length, kind;
    unsigned char machine, language, flags, reserved, name[300];
} CodeViewCompileRecord;
#pragma pack()
typedef char CodeViewCompileSizeCheck[sizeof(CodeViewCompileRecord) == 308 ? 1 : -1];
/* Original69bd0..69c56. S_COMPILE uses the same original852b8 string
   object as the COFF compiler comment; frontend8 sets language byte2. */
void write_codeview_compile_record(void)
{
    CodeViewCompileRecord record;
    char *end = compiler_identification;
    unsigned length;
    while (*end++) ;
    length = (unsigned)(end - compiler_identification) + 6;
    record.length = (unsigned short)length;
    record.kind = 1;
    record.machine = (unsigned char)cpu_model;
    record.language = floating_frontend ? 2 : 0;
    record.flags = record.reserved = 0;
    copy_codeview_string(record.name, compiler_identification);
    output_callbacks.select_segment(debug_symbols_segment);
    output_callbacks.data(&record, length + 2);
    debug_symbols_segment->symbol.field04 += length + 2;
}
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
#pragma pack(2)
typedef struct CodeViewLabelRecord {
    unsigned short length, kind;
    unsigned offset;
    unsigned short section;
    unsigned char flags, name[297];
} CodeViewLabelRecord;
#pragma pack()
typedef char CodeViewLabelSizeCheck[sizeof(CodeViewLabelRecord) == 308 ? 1 : -1];
/* Original69e70..69f2a. User labels use diagnostic spelling, generated
   labels use the backend's display name; the address pair is relocated. */
void __fastcall write_codeview_label(SymbolHeader *symbol)
{
    CodeViewLabelRecord record;
    char *name, *end;
    unsigned length;
    name = symbol->field0d ? symbol_display_name(symbol) : format_function_name(symbol);
    end = name;
    while (*end++) ;
    length = (unsigned)(end - name) + 9;
    record.length = (unsigned short)length;
    record.kind = 0x209;
    record.offset = coff_object && !byte_block_first ? symbol->field04 : 0;
    record.section = 0;
    record.flags = 0;
    copy_codeview_string(record.name, name);
    output_callbacks.select_segment(debug_symbols_segment);
    output_callbacks.data(&record, length + 2);
    output_callbacks.size(2 - length, symbol);
    debug_symbols_segment->symbol.field04 += length + 2;
}
#pragma pack(2)
typedef struct CodeViewFrameVariableRecord {
    unsigned short length, kind;
    unsigned offset;
    unsigned short type;
    unsigned char name[298];
} CodeViewFrameVariableRecord;
#pragma pack()
typedef char CodeViewFrameVariableSizeCheck[sizeof(CodeViewFrameVariableRecord) == 308 ? 1 : -1];
/* Original69730..697bf. S_BPREL32 retains the raw signed stack-offset
   bits; function flag10 adjusts role30 parameters by one DWORD. */
void __fastcall write_codeview_frame_variable(SymbolHeader *symbol)
{
    CodeViewFrameVariableRecord record;
    BackendSymbol *variable = (BackendSymbol *)symbol;
    char *name = format_function_name(symbol), *end;
    unsigned length, offset;
    output_callbacks.select_segment(debug_symbols_segment);
    end = name;
    while (*end++) ;
    length = (unsigned)(end - name) + 8;
    record.length = (unsigned short)length;
    record.kind = 0x200;
    offset = symbol->field04;
    if ((current_function->field28 & 0x10) &&
        (variable->attributes & 0x70) == 0x30) offset -= 4;
    record.offset = offset;
    record.type = variable->field20;
    copy_codeview_string(record.name, name);
    output_callbacks.data(&record, length + 2);
    debug_symbols_segment->symbol.field04 += length + 2;
}
#pragma pack(2)
typedef struct CodeViewTypedefRecord {
    unsigned short length, kind, type;
    unsigned char name[302];
} CodeViewTypedefRecord;
#pragma pack()
typedef char CodeViewTypedefSizeCheck[sizeof(CodeViewTypedefRecord) == 308 ? 1 : -1];
/* Original69d10..69d7e. S_UDT carries only a type index and counted name;
   unused contents of the44-byte kind11 input descriptor are not consulted. */
void __fastcall write_codeview_typedef(SymbolHeader *symbol)
{
    CodeViewTypedefRecord record;
    char *name = format_function_name(symbol), *end = name;
    unsigned length;
    while (*end++) ;
    length = (unsigned)(end - name) + 4;
    record.length = (unsigned short)length;
    record.kind = 4;
    record.type = ((BackendSymbol *)symbol)->field20;
    copy_codeview_string(record.name, name);
    output_callbacks.select_segment(debug_symbols_segment);
    output_callbacks.data(&record, length + 2);
    debug_symbols_segment->symbol.field04 += length + 2;
}
#pragma pack(2)
typedef struct CodeViewDataRecord {
    unsigned short length, kind;
    unsigned offset;
    unsigned short section, type;
    unsigned char name[296];
} CodeViewDataRecord;
#pragma pack()
typedef char CodeViewDataSizeCheck[sizeof(CodeViewDataRecord) == 308 ? 1 : -1];
/* Original697c0..6987f. Local data/TLS records share an address pair
   with a type index; COFF retains the section-relative addend. */
void __fastcall write_codeview_local_data(SymbolHeader *symbol)
{
    CodeViewDataRecord record;
    char *name = format_function_name(symbol), *end;
    unsigned length;
    output_callbacks.select_segment(debug_symbols_segment);
    end = name;
    while (*end++) ;
    length = (unsigned)(end - name) + 10;
    record.length = (unsigned short)length;
    record.kind = symbol->kind == 1 && (symbol->field0e & 0x1000) ? 0x20d : 0x201;
    record.offset = coff_object && !byte_block_first ? symbol->field04 : 0;
    record.section = 0;
    record.type = ((BackendSymbol *)symbol)->field20;
    copy_codeview_string(record.name, name);
    output_callbacks.data(&record, length + 2);
    output_callbacks.size(2 - length, symbol);
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

#pragma pack(2)
typedef struct CodeViewBlockRecord {
    unsigned short length, kind;
    unsigned parent, end, code_bytes, offset;
    unsigned short section;
    unsigned char name;
} CodeViewBlockRecord;
#pragma pack()
typedef char CodeViewBlockRecordSizeCheck[sizeof(CodeViewBlockRecord) == 24 ? 1 : -1];
/* Original [00069b20,00069b97). S_BLOCK32 emits23 initialized bytes;
   the final structure alignment byte is neither initialized nor emitted. */
void __fastcall write_codeview_block(FunctionSymbol *symbol, unsigned offset, unsigned bytes)
{
    CodeViewBlockRecord record;
    record.length = 21;
    record.kind = 0x207;
    record.parent = record.end = 0;
    record.code_bytes = bytes;
    record.offset = offset - symbol->symbol.field04;
    record.section = 0;
    record.name = 0;
    output_callbacks.select_segment(debug_symbols_segment);
    output_callbacks.data(&record, 23);
    output_callbacks.size((unsigned)-7, &symbol->symbol);
    debug_symbols_segment->symbol.field04 += 23;
}
