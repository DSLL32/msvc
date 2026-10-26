/* Frontend debug intermediate initialization and CodeView record output. */
#include <stdarg.h>
#include <time.h>
#include "debug-info.h"
#include "debug-types.h"
#include "global-ir.h"
#include "control-ir.h"
#include "parser.h"
#include "segments.h"
#include <recon.h>

unsigned char *debug_record_buffer; /* original 0x8ccb4 */
unsigned debug_stream_offset; /* original 0x9f45c */

/* Original 0x6d660..0x6d6b8. The ordinary CodeView stream begins
   with its record formats and a four-byte version signature. Line-record
   mode retains its separate original initialization entry. */
void begin_debug_info(void)
{
    if (options.configuration.line_records) {
        begin_line_record_debug_info();
        return;
    }
    debug_record_buffer = checked_malloc(4096);
    write_debug_record(12);
    write_debug_record(10);
    *(unsigned *)debug_record_buffer = 1;
    write_debug_record(3, 4, debug_record_buffer);
    debug_stream_offset += 4;
}

/* Original 0x6d6c0..0x6d6da. Line-record mode closes its own
   debug producer; ordinary CodeView output ends with record seven. */
void finish_debug_info(void)
{
    if (options.configuration.line_records) {
        finish_line_record_debug_info();
        return;
    }
    write_debug_record(7);
}

/* These thirteen encountered record schemas belong to the debug stream.
   The second word of each original eight-byte row is not read by this writer. */
char *debug_record_formats[13] = {
    "mdox", "tpzv", "tsfz", "tl", "ti", "tr", "t", "t", "tn", "td",
    "mox", "tb", "tg"
};

/* Original 0x704a0..0x7067e, switch data 0x70680..0x706e4.
   Formats consume their own argument shapes and temporarily select the debug
   FILE for the ordinary shared intermediate writers. Unrecognized format
   letters are annotations and deliberately consume no argument. */
void write_debug_record(unsigned record, ...)
{
    FILE *saved = auxiliary_ir_stream;
    char *format;
    va_list arguments;
    char *block;
    short length;
    int integer;
    unsigned index;
    CppSymbol *symbol;
    auxiliary_ir_stream = debug_file;
    va_start(arguments, record);
    format = debug_record_formats[record];
    if (!format) return;
    for (; *format; ++format) {
        switch (*format) {
        case 'b':
            block = va_arg(arguments, char *);
            write_counted_ir(*(short *)block, block + 2);
            break;
        case 'f':
            integer = 0;
            write_ir_integer(&integer);
            break;
        case 'g':
            putc(1, auxiliary_ir_stream);
            break;
        case 'l':
            length = (short)va_arg(arguments, int);
            block = va_arg(arguments, char *);
            write_counted_ir(length, block);
            break;
        case 'm':
        case 't':
            putc((char)record, auxiliary_ir_stream);
            break;
        case 'o':
            integer = (int)debug_stream_offset;
            write_ir_integer(&integer);
            break;
        case 'r':
            integer = 1;
            write_ir_integer(&integer);
            break;
        case 's':
            symbol = va_arg(arguments, CppSymbol *);
            if (symbol->attributes & 0x40) {
                if ((symbol->kind & 31) == 3) symbol = static_member_alias(symbol);
                else symbol = symbol_alias(symbol);
            }
            index = symbol->index;
            write_ir_index(&index);
            break;
        case 'z':
            putc((char)va_arg(arguments, int), auxiliary_ir_stream);
            break;
        }
    }
    va_end(arguments);
    auxiliary_ir_stream = saved;
}

DebugDatabase *debug_database; /* original0xa269c */
unsigned char *line_record_buffer; /* original0x8cca0 */
unsigned char *line_record_leaf; /* original0x8cc9c */

/* Original0x70800..0x7089c. Open the separate debug database once,
   using the actual reproducible signature option before beginning its stream. */
void begin_line_record_debug_info(void)
{
    if (debug_database) return;
    if (options.configuration.option_Brepro)
        debug_database = open_debug_database(options.configuration.database_path, "wi", 1);
    else debug_database = open_debug_database(options.configuration.database_path,
        "wi", (unsigned)time(0));
    if (!debug_database) {
        report_syntax_error(470, options.configuration.database_path);
        return;
    }
    initialize_line_record_buffer();
    write_debug_record(12);
    write_debug_record(10);
    *(unsigned *)line_record_buffer = 1;
    write_debug_record(3, 4, line_record_buffer);
    emit_line_record_database_reference();
}

/* Original0x70930..0x70948. Keep the two-byte record length and its leaf
   in one allocated owner; the leaf global is the interior pointer. */
void initialize_line_record_buffer(void)
{
    line_record_buffer = checked_malloc(0xff00);
    line_record_leaf = line_record_buffer + 2;
}

/* Original0x708a0..0x7092a. The type-server leaf contains its signature,
   age and counted database filename, then the original descending padding. */
void emit_line_record_database_reference(void)
{
    unsigned char *leaf = line_record_leaf;
    unsigned short length, padding;
    if (!debug_database) return;
    *(unsigned short *)leaf = 0x16;
    *(unsigned *)(leaf + 2) = debug_database_signature(debug_database);
    *(unsigned *)(leaf + 6) = debug_database_age(debug_database);
    length = copy_line_record_name(leaf + 10, options.configuration.database_path) + 10;
    padding = pad_line_record(line_record_buffer + length + 2, (unsigned short)(length + 2));
    *(unsigned short *)line_record_buffer = padding + length;
    write_debug_record(11, line_record_buffer);
}
/* Original0x72c20..0x72c39, cdecl2/RET0, defined AX result. */
unsigned short copy_line_record_name(unsigned char *destination, char *name)
{
    return (unsigned short)(copy_debug_name(destination, name) - destination);
}
/* Original0x72650..0x72684, cdecl2/RET0 with a defined AX result.
   Distinct original function from the ordinary CodeView field padder. */
unsigned short pad_line_record(unsigned char *destination, unsigned short length)
{
    unsigned short padding = (unsigned short)(-length) & 3, left;
    for (left = padding; left; --left)
        *destination++ = (unsigned char)(0xf0 + left);
    return padding;
}

int __cdecl debug_database_commit(DebugDatabase *database);
int __cdecl debug_database_close(DebugDatabase *database);

/* Original0x70950..0x7099f, cdecl0. End the debug stream before
   committing and closing the type server; retain failure short-circuiting. */
void finish_line_record_debug_info(void)
{
    write_debug_record(7);
    if (debug_database) {
        if (!debug_database_commit(debug_database) || !debug_database_close(debug_database))
            report_syntax_error(471, options.configuration.database_path);
        debug_database = 0;
    }
}

unsigned char *__stdcall copy_bytes_forward(unsigned char *destination,
    const unsigned char *source, unsigned bytes);

/* Original 0x700f0..0x70189, cdecl. The PCH object contributes a
   real external data symbol identifying the object that owns its debug types. */
void declare_pch_debug_library(void)
{
    char *prefix = "__@@_PchDebugInfoLibrary_";
    unsigned char *cursor;
    unsigned length;
    unsigned char storage;
    CppSymbol *symbol;
    for (length = 0; prefix[length]; ++length) ;
    cursor = copy_bytes_forward(text_work_cursor, (unsigned char *)prefix, length + 1) - 1;
    for (length = 0; options.configuration.pch_object_name[length]; ++length) ;
    copy_bytes_forward(cursor, (unsigned char *)options.configuration.pch_object_name, length + 1);
    symbol = new_symbol(intern_identifier((char *)text_work_cursor), 0);
    storage = symbol->storage[0] & 7;
    symbol->storage[0] = (declaration_storage[0] & ~7) | storage;
    symbol->storage[1] = declaration_storage[1];
    symbol->storage[2] = declaration_storage[2];
    symbol->storage[0] = (symbol->storage[0] & ~5) | 2;
    ++symbol->references;
    symbol->declaration_flags = (symbol->declaration_flags & ~0x100) | 0x200;
}

/* Original0x6d640..0x6d660, cdecl0. Separate-debug replay selects
   database startup (which reuses an open database) or a fresh CodeView buffer. */
void restore_pch_separate_debug(void)
{
    if (options.configuration.line_records) begin_line_record_debug_info();
    else debug_record_buffer = checked_malloc(4096);
}

extern SegmentSymbol *default_data_segment;

/* Original 0x70190..0x702c0, stdcall4. A client forces the PCH debug
   owner into the link through a real .drectve string object, then restores
   the ordinary data segment. The literal's auxiliary WORD is unwritten in
   the original; retain that defect rather than inventing its width flags. */
void __stdcall reference_pch_debug_library(char *name)
{
    char *prefix = "__@@_PchDebugInfoLibrary_";
    char *directive = "-INCLUDE:_";
    unsigned prefix_bytes, directive_bytes, name_bytes;
    unsigned char *cursor;
    SegmentSymbol *saved_segment = default_data_segment;
    CppSymbol *symbol;
    IrToken *literal;
    ArenaPool *pool;
    for (prefix_bytes = 0; prefix[prefix_bytes]; ++prefix_bytes) ;
    for (directive_bytes = 0; directive[directive_bytes]; ++directive_bytes) ;
    cursor = copy_bytes_forward(text_work_cursor, (unsigned char *)directive, directive_bytes);
    cursor = copy_bytes_forward(cursor, (unsigned char *)prefix, prefix_bytes + 1);
    for (name_bytes = 0; name[name_bytes]; ++name_bytes) ;
    cursor = copy_bytes_forward(cursor - 1, (unsigned char *)name, name_bytes + 1);
    if (token_allocation_flags & 0x20000020UL)
        literal = allocate_permanent(sizeof(IrToken));
    else {
        pool = &compiler_arena.pools[2];
        if (pool->remaining < (int)sizeof(IrToken)) refill_arena(&compiler_arena, pool);
        literal = (IrToken *)pool->next;
        pool->remaining -= sizeof(IrToken);
        pool->next += sizeof(IrToken);
    }
    literal->kind = 3;
    literal->flags = 5;
    literal->value.text = (char *)text_work_cursor;
    literal->tail.string.length = (unsigned short)(cursor - text_work_cursor);
    record_string_literal(literal);
    symbol = find_outer_name(intern_identifier((char *)text_work_cursor + directive_bytes));
    if (symbol) symbol->flags |= 8;
    default_data_segment = lookup_segment_symbol(intern_identifier(".drectve"),
        name_unnamed, 1, 2);
    materialize_string_literal(literal);
    global_expression_scope_active = 1;
    default_data_segment = saved_segment;
}
