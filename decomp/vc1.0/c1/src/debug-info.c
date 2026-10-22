/* CodeView debug-type and program-database initialization. */
#include <time.h>
#include <stdarg.h>
#include "control-ir.h"
#include "literal-names.h"
#include "debug-info.h"
#include "parser.h"
#include "hash-table.h"
#include <recon.h>
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

/* Original 0x49230..0x4924e, cdecl. Prototype-only operation suppresses
   database type allocation, retaining a zero AX result. */
unsigned short emit_pdb_type(CType *type)
{
    if (options.prototypes_only) return 0;
    return pdb_type_index(type);
}

/* Original 0x49250..0x49290. Interned type descriptors cache their
   sixteen-bit CodeView index while retaining their independent upper word. */
unsigned short pdb_type_index(CType *type)
{
    unsigned short index = (unsigned short)type->field0c;
    if (index) return index;
    if (type->related) return pdb_shape_type_index(type, type->related);
    if (type->attributes & 0x3000) return pdb_qualified_type_index(type);
    index = pdb_base_type_index(type);
    *(unsigned short *)&type->field0c = index;
    return index;
}

/* Original 0x498e0..0x4995b. Record and enum tags have their own
   recursive emitters; unadorned scalar types use CodeView primitive codes. */
unsigned short pdb_base_type_index(CType *type)
{
    unsigned short index, flags = type->flags;
    if (flags & 0x200) {
        index = pdb_enum_type_index((FrontendSymbol *)type->field08);
        *(unsigned short *)&type->field0c = index;
        return index;
    }
    if (flags & 0x400) {
        index = pdb_record_type_index((FrontendSymbol *)type->field08);
        *(unsigned short *)&type->field0c = index;
        return index;
    }
    if (flags & 0x80) return 3;
    if (flags & 0x800) return 2;
    if (!flags) return 0;
    if (flags == 0x802) return 2;
    return pdb_primitive_type_index(flags);
}

/* CodeView primitive codes. The compiler distinguishes int-sized spelling
   from explicit char/short/long and permits eight-byte integer descriptors.
   The signed explicit table ends before the adjacent pointer-model table. */
enum CodeViewPrimitive {
    CV_CHAR = 0x10, CV_SHORT = 0x11, CV_LONG = 0x12, CV_QUAD = 0x13,
    CV_UCHAR = 0x20, CV_USHORT = 0x21, CV_ULONG = 0x22, CV_UQUAD = 0x23,
    CV_REAL32 = 0x40, CV_REAL64 = 0x41, CV_REAL80 = 0x42,
    CV_RCHAR = 0x70, CV_INT2 = 0x72, CV_UINT2 = 0x73,
    CV_INT4 = 0x74, CV_UINT4 = 0x75, CV_INT8 = 0x76, CV_UINT8 = 0x77
};
unsigned short pdb_floating_codes[12] = {
    0, 0, 0, 0, CV_REAL32, 0, 0, 0, CV_REAL64, 0, CV_REAL80, 0
};
unsigned short pdb_natural_unsigned_codes[12] = {
    0, CV_RCHAR, CV_UINT2, 0, CV_UINT4, 0, 0, 0, CV_UINT8, 0, 0, 0
};
unsigned short pdb_natural_signed_codes[12] = {
    0, CV_RCHAR, CV_INT2, 0, CV_INT4, 0, 0, 0, CV_INT8, 0, 0, 0
};
unsigned short pdb_explicit_unsigned_codes[12] = {
    0, CV_UCHAR, CV_USHORT, 0, CV_ULONG, 0, 0, 0, CV_UQUAD, 0, 0, 0
};
/* These tables share an original word-addressed boundary: the pointer
   model bytes immediately follow ten signed-width codes. The union retains
   both established views rather than borrowing unrelated neighboring data. */
typedef struct SignedPointerCodeFields {
    unsigned short signed_codes[10];
    unsigned char models[4];
} SignedPointerCodeFields;
typedef union SignedAndPointerCodes {
    SignedPointerCodeFields fields;
    unsigned short words[12];
} SignedAndPointerCodes;
typedef char SignedAndPointerCodes_layout[(sizeof(SignedAndPointerCodes) == 24 &&
    offsetof(SignedPointerCodeFields, models) == 20) ? 1 : -1];
SignedAndPointerCodes pdb_signed_pointer_codes = {
    { { 0, CV_CHAR, CV_SHORT, 0, CV_LONG, 0, 0, 0, CV_QUAD, 0 }, { 0, 10, 11, 2 } }
};

/* Original 0x493c0..0x49445. The natural-int family also carries a
   plain char when neither signed nor unsigned was spelled explicitly. */
unsigned short pdb_primitive_type_index(unsigned short flags)
{
    unsigned width;
    if (flags & 0x80) return 3;
    if (!(flags & 0x140)) return 0;
    width = flags & 15;
    if (flags & 0x100) return pdb_floating_codes[width];
    if (((flags & 0x20) && (flags & 0x40)) || (width == 1 && !(flags & 0xc000)))
        return (flags & 0x4000) ? pdb_natural_unsigned_codes[width] : pdb_natural_signed_codes[width];
    return (flags & 0x4000) ? pdb_explicit_unsigned_codes[width] : pdb_signed_pointer_codes.words[width];
}

unsigned debug_type_record_count; /* original 0x6e080 */

/* Original 0x47ae0..0x47c61. Flush pending legacy type records in their
   assigned-index order. Emitters can append more entries while walking. */
void flush_debug_types(void)
{
    unsigned position, kind;
    unsigned short nested;
    DebugTypeRecord *record;
    CType description, *type;
    TypeShape *shape;
    FrontendSymbol *symbol;
    if (options.line_records || !debug_type_record_count) return;
    for (position = 0; position < debug_type_record_count; ++position) {
        record = &debug_type_records[position];
        switch (record->kind) {
        case 0:
            type = record->value.type;
            shape = type->related;
            if (!shape) {
                write_legacy_qualified_type(type);
                break;
            }
            description = *type;
            description.related = type->related->related;
            nested = (unsigned short)intern_type(&description)->field0c;
            kind = shape->flags & 7;
            if (kind == 1 || kind == 3) write_legacy_pointer_type(type, nested);
            else if (kind == 2) write_legacy_array_type(type, nested);
            else if (kind == 4)
                write_legacy_function_type(type, nested, debug_type_records[position].index);
            break;
        case 1:
            symbol = record->value.symbol;
            kind = symbol->kind & 0x1f;
            if (kind == 2) write_legacy_enum_type(symbol);
            else if (kind == 4)
                write_legacy_record_type(symbol, record->index, (unsigned short)record->auxiliary);
            break;
        case 2: write_legacy_field_list(record->value.symbol, record->index, 0); break;
        case 3: write_legacy_argument_list((ParameterInfo *)record->value.pointer); break;
        case 7:
            write_legacy_argument_slice((CType **)record->value.pointer, record->auxiliary, record->index);
            break;
        }
    }
    reset_debug_type_records();
}

/* Original 0x49290..0x4931e. A primitive pointer needs no record;
   compound shapes first resolve the nested type's database index. */
unsigned short pdb_shape_type_index(CType *type, TypeShape *shape)
{
    unsigned short index, kind;
    CType *nested;
    index = pdb_primitive_pointer_index(type, shape);
    if (index) {
        *(unsigned short *)&type->field0c = index;
        return index;
    }
    nested = remove_pdb_type_shape(type);
    index = pdb_type_index(nested);
    kind = shape->flags & 7;
    switch (kind) {
    case 0: break;
    case 1: case 3: index = pdb_pointer_type_index(type, index); break;
    case 2: index = pdb_array_type_index(type, index); break;
    case 4: index = pdb_function_type_index(type, index); break;
    default: return 0;
    }
    *(unsigned short *)&type->field0c = index;
    return index;
}

/* Original 0x49320..0x493bf. A single ordinary pointer to a primitive
   can encode both the referent and pointer model in one CodeView index. */
unsigned short pdb_primitive_pointer_index(CType *type, TypeShape *shape)
{
    unsigned short index, model;
    if (shape->related || (shape->flags & 7) != 1 || (shape->attributes & 12)) return 0;
    index = pdb_primitive_type_index(type->flags);
    if (!index) return 0;
    model = shape->attributes & 15;
    switch (model) {
    case 1: return (index & 0xfcff) | 0x400;
    case 2: return (index & 0xfdff) | 0x500;
    case 3: return (index & 0xfbff) | 0x300;
    }
    return (index & 0xf8ff) | ((options.option_M & 2) ? 0x500 : 0x400);
}

/* Original 0x4a2e0..0x4a31e. Re-intern the nested type independently
   of the outer descriptor's cached CodeView index. */
CType *remove_pdb_type_shape(CType *type)
{
    CType description = *type;
    *(unsigned short *)&description.field0c = 0;
    description.related = type->related->related;
    return intern_type(&description);
}

/* Original 0x496a0..0x496ee. LF_ARRAY retains the complete byte size,
   short index type and empty counted name after its numeric size field. */
unsigned short pdb_array_type_index(CType *type, unsigned short nested)
{
    unsigned short bytes;
    unsigned short *record = (unsigned short *)pdb_record_cursor;
    record[0] = 3;
    record[1] = nested;
    record[2] = 0x11;
    bytes = write_codeview_number((unsigned char *)(record + 3), type_size(type));
    *((unsigned char *)record + bytes + 6) = 0;
    return intern_pdb_record((unsigned short)(bytes + 7));
}

/* Original 0x4a430..0x4a46f. Small nonnegative numeric leaves occupy
   their value directly; larger values use LF_USHORT or LF_ULONG tags. */
unsigned short write_codeview_number(unsigned char *destination, unsigned number)
{
    if (number < 0x8000) {
        *(unsigned short *)destination = (unsigned short)number;
        return 2;
    }
    if (number < 0x10000) {
        *(unsigned short *)destination = 0x8002;
        *(unsigned short *)(destination + 2) = (unsigned short)number;
        return 4;
    }
    *(unsigned short *)destination = 0x8004;
    *(unsigned *)(destination + 2) = number;
    return 6;
}

/* Original 0x491a0..0x49227. Pad and intern the current record, then
   clear only its occupied bytes so the shared workspace is reusable. */
unsigned short intern_pdb_record(unsigned short bytes)
{
    unsigned short index;
    unsigned position, occupied;
    if (!program_database) return 0;
    bytes += pad_codeview_record(pdb_record_buffer + bytes + 2, (unsigned short)(bytes + 2));
    *(unsigned short *)pdb_record_buffer = bytes;
    occupied = bytes;
    check_pdb_record_size(occupied);
    index = intern_program_database_type(program_database, pdb_record_buffer);
    occupied += 2;
    for (position = 0; position < occupied; ++position) pdb_record_buffer[position] = 0;
    return index;
}

/* Original 0x49180..0x49195. Keep the compiler's lower workspace limit
   distinct from the database manager's own serialized-record bound. */
void check_pdb_record_size(unsigned bytes)
{
    if (bytes >= 0xfafe) fatal_error(67);
}

/* Original 0x496f0..0x49759. Resolve the argument list before touching
   the shared LF_PROCEDURE workspace: argument emission also uses it. */
unsigned short pdb_function_type_index(CType *type, unsigned short result)
{
    typedef struct ProcedureRecord {
        unsigned short leaf, result;
        unsigned char convention, reserved;
        unsigned short count, arguments;
    } ProcedureRecord;
    ProcedureRecord *record;
    unsigned short arguments;
    ParameterInfo *parameters = (ParameterInfo *)type->related->field08;
    arguments = pdb_argument_list_index(parameters);
    record = (ProcedureRecord *)pdb_record_cursor;
    record->leaf = 8;
    record->result = result;
    record->convention = codeview_calling_convention(type->related->attributes);
    record->reserved = 0;
    record->count = (unsigned short)pdb_argument_count((ParameterInfo *)type->related->field08);
    record->arguments = arguments;
    return intern_pdb_record(10);
}

/* Original 0x49760..0x4982d. Resolve nested parameter types into the
   separate index workspace, then assemble LF_ARGLIST in the shared buffer. */
unsigned short pdb_argument_list_index(ParameterInfo *parameters)
{
    unsigned short *saved = get_debug_index_cursor(), *record;
    unsigned short index;
    unsigned count, position;
    index = (unsigned short)parameters->auxiliary;
    if (index) return index;
    count = pdb_argument_count(parameters);
    for (position = 0; position < count; ++position) {
        index = parameters->types[position] ? pdb_type_index(parameters->types[position]) : 0;
        append_debug_index(index);
    }
    check_pdb_record_size(count * 2 + 4);
    record = (unsigned short *)pdb_record_cursor;
    record[0] = 0x201;
    record[1] = (unsigned short)count;
    set_debug_index_cursor(saved);
    for (position = 0; position < count; ++position) record[position + 2] = saved[position];
    set_debug_index_cursor(saved);
    index = intern_pdb_record((unsigned short)(count * 2 + 4));
    *(unsigned short *)&parameters->auxiliary = index;
    return index;
}

/* Original 0x673a8..0x68348: the short-index workspace stops at the
   separately referenced fatal-recursion counter, not at a rounded page. */
unsigned short debug_index_workspace[2000];
unsigned short *debug_index_cursor = debug_index_workspace; /* original 0x6e1b8 */

/* Original 0x4a520..0x4a526. Nested records save this cursor before
   emitting their own indices, then restore it after constructing a record. */
unsigned short *get_debug_index_cursor(void)
{
    return debug_index_cursor;
}

/* Original 0x49830..0x49845. The canonical (void) entry occupies a
   parameter vector slot while encoding an empty CodeView argument list. */
unsigned pdb_argument_count(ParameterInfo *parameters)
{
    if (parameters->types[0] == type_0080) return 0;
    return parameters->count;
}

/* Original 0x4a540..0x4a556. */
void append_debug_index(unsigned short index)
{
    *debug_index_cursor++ = index;
}

/* Original 0x4a530..0x4a53a. */
void set_debug_index_cursor(unsigned short *cursor)
{
    debug_index_cursor = cursor;
}

/* Original 0x48690..0x48716. Near and far forms occupy adjacent
   CodeView calling-convention codes; interrupt has its own fixed code. */
unsigned codeview_calling_convention(unsigned short attributes)
{
    unsigned is_far = (attributes & 7) != 1;
    switch (attributes & 0x70) {
    case 0: case 0x10: return is_far;
    case 0x20: case 0x30: return 2 + is_far;
    case 0x40: return 4 + is_far;
    case 0x50: case 0x60: return 7 + is_far;
    case 0x70: return 11;
    }
    return 15;
}

unsigned short codeview_default_attributes; /* original 0x6e138 */

/* Original 0x49450..0x49698. Emit LF_POINTER, resolving based-symbol
   type references before reusing the common record workspace. */
unsigned short pdb_pointer_type_index(CType *type, unsigned short nested)
{
    TypeShape *shape = type->related;
    FrontendSymbol *base = (FrontendSymbol *)shape->field08;
    unsigned short attributes = shape->attributes, model, extra = 0;
    unsigned char *record = pdb_record_cursor, *payload = record + 6;
    model = attributes & 7;
    if ((attributes & 12) && (attributes & 4)) {
        if (model == 5) {
            if ((base->kind & 0x1f) == 13) {
                if (base->index != 6) {
                    materialize_based_symbol(base);
                    pdb_type_index(base->associated.members->type);
                }
            } else pdb_type_index(base->type);
        } else if (model == 6 || model == 7) pdb_type_index(base->type);
    }
    *(unsigned short *)record = 2;
    *(unsigned short *)(record + 2) = codeview_default_attributes;
    record[2] &= 0x1f;
    *(unsigned short *)(record + 4) = nested;
    if (attributes & 0x1000) record[3] |= 4;
    if (attributes & 0x2000) record[3] |= 2;
    if (!(attributes & 12)) {
        model = shape->attributes & 15;
        record[2] = (record[2] & ~0x1f) | (pdb_signed_pointer_codes.fields.models[model] & 0x1f);
        if (!model && (shape->flags & 7) >= 1 && (shape->flags & 7) <= 4)
            record[2] = (record[2] & 0xea) | 10;
    } else if (!(attributes & 4)) {
        if (attributes & 8) record[2] = (record[2] & 0xe9) | 9;
    } else switch (model) {
    case 4:
        record[2] = (record[2] & 0xe8) | 8;
        *(unsigned short *)payload = 3;
        payload[2] = 0;
        extra = 3;
        break;
    case 5:
        if ((base->kind & 0x1f) == 13 && base->index == 6) {
            /* Fixed register-symbol description for the built-in base. */
            record[2] = (record[2] & 0xe5) | 5;
            *(unsigned short *)payload = 7;
            *(unsigned short *)(payload + 2) = 2;
            *(unsigned short *)(payload + 4) = 0;
            *(unsigned short *)(payload + 6) = 0x1b;
            payload[8] = 0;
            extra = 9;
        } else {
            record[2] = (record[2] & 0xe4) | 4;
            if ((base->kind & 0x1f) == 13) base = base->associated.members;
            extra = write_codeview_symbol(payload, base);
        }
        break;
    case 6:
        record[2] = (record[2] & 0xe7) | 7;
        extra = write_codeview_symbol(payload, base);
        break;
    case 7:
        record[2] = (record[2] & 0xe5) | 5;
        extra = write_codeview_symbol(payload, base);
        break;
    }
    return intern_pdb_record((unsigned short)(extra + 6));
}

/* Original 0x49cb0..0x49e3e. Named structure/union types initially
   receive forward records. Anonymous tags require their full definition. */
unsigned short pdb_record_type_index(FrontendSymbol *tag)
{
    CType *type = tag->type;
    unsigned short *record = (unsigned short *)pdb_record_cursor;
    unsigned short flags = type->flags, index, properties, bytes;
    unsigned short length; /* Unspecified for an invalid non-tag type. */
    unsigned header;
    index = (unsigned short)type->field0c;
    if (index) return index;
    if (tag->name == name_unnamed) {
        *(unsigned short *)&type->field0c = pdb_complete_record_type_index(tag);
        return (unsigned short)tag->type->field0c;
    }
    tag->flags &= ~2;
    record[1] = 0;
    if (flags == 0x402 || flags == 0x404) {
        record[0] = flags == 0x402 ? 5 : 6;
        properties = codeview_default_attributes | 0x80;
        properties = (properties & ~8) | ((tag->metadata.value & 0x02000000UL) ? 8 : 0);
        record[3] = properties;
        record[2] = 0;
        if (flags == 0x402) {
            record[4] = record[5] = 0;
            header = 12;
        } else header = 8;
        bytes = write_codeview_number((unsigned char *)record + header, 0);
        length = (unsigned short)(header + bytes +
            copy_codeview_name((char *)record + header + bytes, tag->name));
    } else if (flags & 0x200) return pdb_enum_type_index(tag);
    index = intern_pdb_record(length);
    *(unsigned short *)&tag->type->field0c = index;
    return (unsigned short)tag->type->field0c;
}

unsigned short legacy_tag_auxiliary; /* original 0x6e084 */

/* Original 0x478c0..0x479dd. Complete a tag's debug description,
   preserving native PDB emission and the deferred legacy record path. */
unsigned short __stdcall debug_symbol_type_index(FrontendSymbol *symbol)
{
    CType *type;
    FrontendSymbol *member;
    unsigned short flags, index;
    unsigned record, count = 0, kind;
    int include;
    if (options.line_records) {
        pdb_complete_record_type_index(symbol);
        return pdb_record_type_index(symbol);
    }
    if (!symbol) return 0;
    type = symbol->type;
    flags = type->flags;
    index = (unsigned short)type->field0c;
    if (index) return index;
    *(unsigned short *)&type->field0c = next_debug_type_index;
    record = queue_debug_record(1);
    debug_type_records[record].value.symbol = symbol;
    *(unsigned short *)&debug_type_records[record].auxiliary = legacy_tag_auxiliary;
    if (symbol->flags & 1) {
        if (flags & 0x200) count = hash_table_count(symbol->associated.enumerators);
        else for (member = symbol->associated.members; member; member = member->next) {
            kind = member->kind & 0x1f;
            include = kind == 4 && member->name == name_unnamed &&
                      (member->type->flags & 0x200);
            if (!include)
                include = member->name && member->name != name_unnamed &&
                          member->name != name_alignment &&
                          (kind < 1 || kind > 13 || kind == 5 || member->type);
            if (include) {
                if (kind != 2) emit_debug_type(member->type);
                ++count;
            }
        }
    }
    debug_type_records[record].index = (unsigned short)count;
    symbol->flags &= ~2;
    return (unsigned short)symbol->type->field0c;
}

/* Original 0x49e40..0x4a02b. The tag caches a complete definition's
   index separately from its interned type's forward-reference index. */
unsigned short pdb_complete_record_type_index(FrontendSymbol *tag)
{
    unsigned short flags = tag->type->flags;
    unsigned short properties = codeview_default_attributes, fields, bytes;
    unsigned short length; /* Unspecified for an invalid non-record type. */
    unsigned short *record = (unsigned short *)pdb_record_cursor;
    unsigned count = 0, header;
    if (tag->metadata.tag.debug_index) return tag->metadata.tag.debug_index;
    if (flags & 0x200) return pdb_complete_enum_type_index(tag);
    if (!(unsigned short)tag->type->field0c && tag->name != name_unnamed)
        pdb_record_type_index(tag);
    if (!(tag->flags & 1)) return 0;
    tag->flags &= ~2;
    fields = pdb_field_list_index(tag, &count, &properties);
    record[1] = (unsigned short)count;
    if (flags == 0x402 || flags == 0x404) {
        record[0] = flags == 0x402 ? 5 : 6;
        record[3] = properties;
        record[2] = fields;
        if (flags == 0x402) {
            record[4] = record[5] = 0;
            header = 12;
        } else {
            properties = (properties & ~1) | (top_packing(packing_stack)->alignment == 1);
            record[3] = properties;
            header = 8;
        }
        record[3] = (record[3] & ~8) | ((tag->metadata.tag.flags & 2) ? 8 : 0);
        bytes = write_codeview_number((unsigned char *)record + header, tag->detail.value);
        length = (unsigned short)(header + bytes +
            copy_codeview_name((char *)record + header + bytes, tag->name));
    }
    tag->metadata.tag.debug_index = intern_pdb_record(length);
    if (tag->name == name_unnamed)
        *(unsigned short *)&tag->type->field0c = tag->metadata.tag.debug_index;
    return tag->metadata.tag.debug_index;
}

/* Original 0x4a030..0x4a07e. The first pass resolves member references;
   the second serializes them. Keep the fourth builder operand even though
   this C frontend's implementation currently leaves the property word alone. */
unsigned short pdb_field_list_index(FrontendSymbol *tag, unsigned *count, unsigned short *properties)
{
    unsigned short *saved = get_debug_index_cursor();
    unsigned short index;
    build_pdb_field_list(1, tag, count, properties);
    set_debug_index_cursor(saved);
    index = build_pdb_field_list(2, tag, count, properties);
    set_debug_index_cursor(saved);
    return index;
}

/* Original 0x4a080..0x4a180. Retain the unused fourth property operand:
   both original wrapper calls pass four stack slots and clean sixteen bytes. */
unsigned short build_pdb_field_list(unsigned pass, FrontendSymbol *tag,
    unsigned *count, unsigned short *properties)
{
    unsigned char *record = pdb_record_cursor;
    FrontendSymbol *member;
    unsigned short bytes = 0;
    unsigned kind;
    int include;
    *count = 0;
    if (pass == 2) *(unsigned short *)record = 0x204;
    for (member = tag->associated.members; member; member = member->next) {
        kind = member->kind & 0x1f;
        include = kind == 4 && member->name == name_unnamed && (member->type->flags & 0x200);
        if (!include)
            include = member->name && member->name != name_unnamed &&
                      member->name != name_alignment &&
                      (kind < 1 || kind > 13 || kind == 5 || member->type);
        if (include) {
            bytes += write_pdb_member(pass, member, pdb_record_cursor + bytes + 2);
            bytes += pad_pdb_field_record(pass, record + bytes + 2, bytes);
            ++*count;
            check_pdb_record_size((unsigned)bytes + 2);
        }
    }
    if (pass == 2) return intern_pdb_record((unsigned short)(bytes + 2));
    return 0;
}

/* Original 0x4a1e0..0x4a292. Bitfield indices are retained in the
   separate workspace between passes; ordinary types cache their own index. */
unsigned short write_pdb_member(unsigned pass, FrontendSymbol *member, unsigned char *destination)
{
    unsigned short index, bytes;
    if (pass == 1) {
        if ((member->kind & 0x1f) == 2) append_debug_index(pdb_bitfield_type_index(member));
        else pdb_type_index(member->type);
        return 0;
    }
    *(unsigned short *)destination = 0x406;
    *(unsigned short *)(destination + 4) = codeview_default_attributes | 3;
    if ((member->kind & 0x1f) == 2) index = read_debug_index();
    else index = (unsigned short)member->type->field0c;
    *(unsigned short *)(destination + 2) = index;
    bytes = write_codeview_signed_number(destination + 6, (int)member->detail.value);
    return (unsigned short)(bytes + 6 +
        copy_codeview_name((char *)destination + bytes + 6, member->name));
}

/* Original 0x4a2a0..0x4a2da. LF_BITFIELD includes the declared storage
   type plus its independently packed bit width and bit position. */
unsigned short pdb_bitfield_type_index(FrontendSymbol *member)
{
    unsigned char *record = pdb_record_cursor;
    unsigned short index = pdb_type_index(member->type);
    *(unsigned short *)record = 0x206;
    record[2] = member->metadata.bitfield.width;
    record[3] = member->metadata.bitfield.offset;
    *(unsigned short *)(record + 4) = index;
    return intern_pdb_record(6);
}

/* Original 0x4a180..0x4a19f. Resolution pass one consumes no record
   bytes; only serialization pass two emits member-alignment leaves. */
unsigned short pad_pdb_field_record(unsigned pass, unsigned char *destination, unsigned short bytes)
{
    if (pass == 2) return pad_codeview_record(destination, bytes);
    return 0;
}

/* Original 0x4a3b0..0x4a3d0. Nonnegative offsets use the compact
   unsigned representation; negative offsets require a signed numeric leaf. */
unsigned short write_codeview_signed_number(unsigned char *destination, int number)
{
    if (number >= 0) return write_codeview_number(destination, (unsigned)number);
    return write_codeview_signed_leaf(destination, number);
}

/* Original 0x4a560..0x4a570. Serialization reads forward through the
   previously resolved index sequence after restoring its starting cursor. */
unsigned short read_debug_index(void)
{
    return *debug_index_cursor++;
}

/* Original 0x49850..0x498d2. Resolve the unqualified type before
   assembling its LF_MODIFIER record and caching the resulting index. */
unsigned short pdb_qualified_type_index(CType *type)
{
    unsigned short *record = (unsigned short *)pdb_record_cursor;
    unsigned short index, attributes;
    CType *unqualified = remove_pdb_type_qualifiers(type);
    index = pdb_type_index(unqualified);
    record[0] = 1;
    attributes = codeview_default_attributes;
    record[1] = attributes;
    attributes = (attributes & ~1) | ((type->attributes & 0x1000) ? 1 : 0);
    record[1] = attributes;
    record[1] = (attributes & ~2) | ((type->attributes & 0x2000) ? 2 : 0);
    record[2] = index;
    index = intern_pdb_record(6);
    *(unsigned short *)&type->field0c = index;
    return index;
}

/* Original 0x4a320..0x4a35b. The unqualified descriptor is interned
   with a cleared debug-index word, preserving every other descriptor field. */
CType *remove_pdb_type_qualifiers(CType *type)
{
    CType description = *type;
    description.attributes &= ~0x3000;
    *(unsigned short *)&description.field0c = 0;
    return intern_type(&description);
}

/* Original 0x474b0..0x474cb. Keep native database finalization separate
   from the legacy stream's terminating debug subsection record. */
void finish_debug_info(void)
{
    if (options.line_records) finish_program_database();
    else write_debug_record(7);
}

/* Original 0x49130..0x4917f. Terminate the debug stream, commit native
   records, then close the manager. A failed commit skips the close call. */
void finish_program_database(void)
{
    write_debug_record(7);
    if (program_database) {
        if (!commit_program_database(program_database) || !close_program_database(program_database))
            report_syntax_error(471, database_path);
        program_database = 0;
    }
}
