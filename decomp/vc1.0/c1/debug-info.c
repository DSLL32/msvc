/* CodeView debug-type and program-database initialization. */
#include <time.h>
#include <stdarg.h>
#include "control-ir.h"
#include "literal-names.h"
#include "debug-info.h"
#include "parser.h"
unsigned *debug_record_buffer; /* original 0x5b24c */
unsigned debug_type_bytes; /* original 0x6b810 */

/* Original 0x47450..0x474a8. Program-database output has its own entry;
   the older stream begins with subsection tags and a four-byte signature. */
void begin_debug_info(void)
{
    if (options.line_records) {
        begin_program_database();
        return;
    }
    debug_record_buffer = checked_malloc(4096);
    write_debug_record(12);
    write_debug_record(10);
    *debug_record_buffer = 1;
    write_debug_record(3, 4, debug_record_buffer);
    debug_type_bytes += 4;
}

ProgramDatabase *program_database; /* original 0x6e13c */
unsigned char *pdb_record_buffer; /* original 0x5b240 */
unsigned char *pdb_record_cursor; /* original 0x5b23c */

/* Original 0x48fe0..0x4907c. Open the database once per translation
   unit, using the reproducible signature when that option is selected. */
void begin_program_database(void)
{
    if (program_database) return;
    if (!options.option_Brepro)
        program_database = open_program_database(database_path, "wi", (unsigned)time(0));
    else program_database = open_program_database(database_path, "wi", 1);
    if (!program_database) {
        report_syntax_error(470, database_path);
        return;
    }
    initialize_pdb_record_buffer();
    write_debug_record(12);
    write_debug_record(10);
    *(unsigned *)pdb_record_buffer = 1;
    write_debug_record(3, 4, pdb_record_buffer);
    write_program_database_reference();
}

/* Original 0x49110..0x49128. Leave two bytes for the record length. */
void initialize_pdb_record_buffer(void)
{
    pdb_record_buffer = checked_malloc(0xff00);
    pdb_record_cursor = pdb_record_buffer + 2;
}

/* Each row names the logical fields interpreted by write_debug_record.
   The paired flag is consumed by the buffered CodeView writer. */
DebugRecordFormat debug_record_formats[13] = {
    {"mdox", 0}, {"tpzv", 2}, {"tsfz", 2}, {"tl", 2},
    {"ti", 2}, {"tr", 2}, {"t", 2}, {"t", 2},
    {"tn", 2}, {"td", 2}, {"mox", 0}, {"tb", 2}, {"tg", 0}
};
#define DEBUG_BYTE(value_) do { \
    signed char byte_ = (signed char)(value_); \
    if (--auxiliary_ir_stream->_cnt >= 0) \
        *auxiliary_ir_stream->_ptr++ = (unsigned char)byte_; \
    else _flsbuf(byte_, auxiliary_ir_stream); \
} while (0)

/* Original 0x48d90..0x48fc0, including its 25-entry dispatch table.
   Unrecognized schema letters consume neither an operand nor output. */
void write_debug_record(unsigned record, ...)
{
    FILE *saved;
    char *fields, *text;
    FrontendSymbol *symbol;
    unsigned index;
    int integer, length;
    va_list operands;
    saved = auxiliary_ir_stream;
    auxiliary_ir_stream = debug_file;
    fields = debug_record_formats[record].fields;
    va_start(operands, record);
    if (!fields) return;
    while (*fields) {
        switch (*fields++) {
        case 'b':
            text = va_arg(operands, char *);
            length = *(unsigned short *)text;
            write_counted_ir((short)length, text + 2);
            break;
        case 'f': integer = 0; write_ir_integer(&integer); break;
        case 'g': DEBUG_BYTE(0); break;
        case 'l':
            length = va_arg(operands, int);
            text = va_arg(operands, char *);
            write_counted_ir((short)length, text);
            break;
        case 'm': DEBUG_BYTE(record); break;
        case 'o': integer = debug_type_bytes; write_ir_integer(&integer); break;
        case 'r': integer = 1; write_ir_integer(&integer); break;
        case 's':
            symbol = va_arg(operands, FrontendSymbol *);
            if (symbol->attributes & 0x40) symbol = symbol_alias(symbol);
            index = symbol->index;
            write_ir_index(&index);
            break;
        case 't': DEBUG_BYTE(record); break;
        case 'z': DEBUG_BYTE(va_arg(operands, int)); break;
        }
    }
    va_end(operands);
    auxiliary_ir_stream = saved;
}

/* Original 0x49080..0x4910a. LF_TYPESERVER carries the exact database
   signature, age and counted absolute path, padded with LF_PAD bytes. */
void write_program_database_reference(void)
{
    unsigned char *record;
    unsigned short length, padding;
    record = pdb_record_cursor;
    if (!program_database) return;
    *(unsigned short *)record = 0x16;
    *(unsigned *)(record + 2) = program_database_signature(program_database);
    *(unsigned *)(record + 6) = program_database_age(program_database);
    length = copy_codeview_name((char *)record + 10, database_path) + 10;
    padding = pad_codeview_record(pdb_record_buffer + length + 2, (unsigned short)(length + 2));
    *(unsigned short *)pdb_record_buffer = length + padding;
    write_debug_record(11, pdb_record_buffer);
}

/* Original 0x4a470..0x4a489. */
unsigned short copy_codeview_name(char *destination, char *source)
{
    return (unsigned short)(append_codeview_name(destination, source) - destination);
}

/* Original 0x4bd50..0x4bd93. CodeView names have one length byte and
   at most 255 characters; the returned cursor follows the copied name. */
char *__stdcall append_codeview_name(char *destination, char *source)
{
    char *end;
    unsigned length, i;
    end = source;
    while (*end) ++end;
    length = end - source;
    if (length > 255) length = 255;
    *destination++ = (unsigned char)length;
    for (i = 0; i < length; ++i) *destination++ = *source++;
    return destination;
}

/* Original 0x4a1a0..0x4a1d4. Emit LF_PADn ... LF_PAD1 to reach
   the next four-byte boundary, returning the number of pad bytes. */
unsigned short pad_codeview_record(unsigned char *destination, unsigned short bytes)
{
    unsigned short padding, remaining;
    unsigned char value;
    padding = (unsigned short)(-bytes) & 3;
    remaining = padding;
    value = (unsigned char)(padding - 16);
    while (remaining--) *destination++ = value--;
    return padding;
}

/* Original 0x36af0..0x36b2f. Named declarations force their type graph
   before a subsequent symbol record needs its debug index. */
void __stdcall prepare_symbol_debug_type(FrontendSymbol *symbol)
{
    unsigned kind = symbol->kind & 0x1f;
    if ((kind == 6 || kind == 7) && !(symbol->flags & 1)) return;
    if (symbol->name != name_unnamed && symbol->name && !(symbol->flags & 0x20)) {
        emit_debug_type(symbol->type);
        flush_debug_types();
    }
}

DebugTypeRecord *debug_type_records; /* original 0x5b248 */
unsigned short next_debug_type_index = 0x1000; /* original 0x6b808 */

/* Original 0x474d0..0x47623. Walk the interned type graph before queuing
   compound legacy records. /Zi uses the separate PDB type emitter. The
   descriptor's low field0c word is its debug index, independently of flags. */
void __stdcall emit_debug_type(CType *type)
{
    TypeShape *shape;
    CType description, *nested;
    unsigned short index;
    unsigned record, kind;
    if (options.line_records) {
        emit_pdb_type(type);
        return;
    }
    shape = type->related;
    if (options.prototypes_only) return;
    if (!type) {
        *(unsigned short *)&type->field0c = 0;
        return;
    }
    if ((unsigned short)type->field0c) return;
    if (shape) {
        index = primitive_pointer_debug_index(type);
        *(unsigned short *)&type->field0c = index;
        if (index) return;
        description = *type;
        description.related = type->related->related;
        nested = intern_type(&description);
        emit_debug_type(nested);
        kind = shape->flags & 7;
        if (!kind) {
            *(unsigned short *)&type->field0c = (unsigned short)nested->field0c;
            return;
        }
        if (kind >= 1 && kind <= 3) {
            *(unsigned short *)&type->field0c = next_debug_type_index;
            record = queue_debug_record(0);
            debug_type_records[record].value.type = type;
        } else if (kind == 4) emit_function_debug_type(type);
    } else if (type->attributes & 0x3000) {
        *(unsigned short *)&type->field0c = next_debug_type_index;
        record = queue_debug_record(0);
        debug_type_records[record].value.type = type;
    } else *(unsigned short *)&type->field0c = primitive_debug_index(type);
}
