/* Global intermediate record serialization. */
#include "global-ir.h"

/* Shared name-formatting workspace, original 0x65e60. The diagnostic
   name formatter truncates at 247 characters. */
char symbol_name_buffer[248];

unsigned char global_record_kind; /* original 0x6003c */

/* Original 0x36b30..0x36b4f. Rewritten at both startup and completion. */
void write_global_header(void)
{
    rewind(global_file);
    global_record_kind = 0;
    write_global_record(17, 0);
}

FILE *auxiliary_ir_stream; /* original 0x6e434 */
FunctionIrState function_ir; /* original 0x60044..0x60058 */
int function_in_pch; /* original 0x6bf64 */
unsigned pch_symbol_offset_adjustment; /* original 0x6b804 */
int function_call_seen; /* original 0x6bfb0 */
int default_inline_candidate = 1; /* original 0x6bfa8 */
int inline_candidate = 1; /* original 0x6bfac */
int function_not_inlinable; /* original 0x6bfb4 */
int plmf_pragma_seen, plmn_pragma_seen; /* original 0x6bbc4 / 0x6bbc8 */
unsigned pragma_record_number; /* original 0x6b80c */
char *pragma_text; /* original 0x6be6c */
PragmaValue *pragma_values; /* original 0x6b790 */
InlineRegion *inline_regions; /* original 0x5ffd4 */
InlineScope *inline_scopes; /* original 0x5ffdc */
int inline_scope_depth; /* original 0x5ffd8 */
unsigned short inline_base_region; /* original 0x5ffd0 */
unsigned inline_parameter_slots[20]; /* original 0x5ffe4..0x60034 */
unsigned char inline_slot_count; /* original 0x60034 */
unsigned short inline_region_index; /* original 0x5ffe0 */
unsigned *source_file_index; /* original 0x6005c */
unsigned source_file_counter = 10; /* original 0x6bb40 */
char last_record_source[256]; /* original 0x66240..0x66340 */

/* The original writer expands these operations inline. Keeping them as
   macros preserves that function graph and the old FILE buffering ABI. */
#define GLOBAL_STREAM (global_record_kind ? symbol_file : global_file)
#define GLOBAL_BYTE(stream_, value_) do { \
    FILE *out_ = (stream_); int byte_ = (value_); \
    if (--out_->_cnt >= 0) *out_->_ptr++ = (unsigned char)byte_; \
    else _flsbuf(byte_, out_); \
} while (0)
#define GLOBAL_RAW(stream_, type_, value_) do { \
    FILE *out_ = (stream_); type_ value_copy_ = (value_); \
    out_->_cnt -= sizeof(type_); \
    if (out_->_cnt >= 0) { \
        *(type_ *)out_->_ptr = value_copy_; out_->_ptr += sizeof(type_); \
    } else { \
        out_->_cnt += sizeof(type_); fwrite(&value_copy_, sizeof(type_), 1, out_); \
    } \
} while (0)
#define GLOBAL_INTEGER(value_) do { \
    FILE *integer_out_ = GLOBAL_STREAM; int integer_ = (value_); \
    if (integer_ >= -127 && integer_ <= 127) \
        GLOBAL_BYTE(integer_out_, (signed char)integer_); \
    else { GLOBAL_BYTE(integer_out_, 0x80); GLOBAL_RAW(integer_out_, int, integer_); } \
} while (0)
#define GLOBAL_SHORT(value_) do { \
    FILE *short_out_ = GLOBAL_STREAM; short short_ = (short)(value_); \
    if (short_ >= -127 && short_ <= 127) GLOBAL_BYTE(short_out_, (int)short_); \
    else { GLOBAL_BYTE(short_out_, 0x80); GLOBAL_RAW(short_out_, short, short_); } \
} while (0)
#define GLOBAL_INDEX(value_) do { \
    FILE *index_out_ = GLOBAL_STREAM; unsigned index_ = (value_); \
    if (index_ > 0x7fff) { \
        GLOBAL_RAW(index_out_, unsigned short, (unsigned short)(index_ | 0x8000)); \
        index_ >>= 15; \
    } \
    GLOBAL_RAW(index_out_, unsigned short, (unsigned short)index_); \
} while (0)

/* Original 0x36e00..0x392dc; field-dispatch table 0x392dc..0x39370. */
void __stdcall write_global_record(unsigned tag, FrontendSymbol *symbol)
{
    CType *type, *inner_type = 0, *aligned_type;
    TypeShape *shape, *base_shape;
    FrontendSymbol *tag_symbol;
    FILE *stream;
    unsigned char *format, code, count;
    unsigned short type_flags, attributes, flags, base_flags, base_attributes;
    unsigned short alignment, alignment_code, encoded_type, machine;
    unsigned kind, storage, index, i, version;
    int array_type = 0, size, distance, no_inline;
    char *left, *right;
    PragmaValue *pragma;

    if (symbol) {
        kind = symbol->kind & 31;
        if (kind >= 1 && kind <= 13 && kind != 5 && (symbol->flags & 0x80))
            return;
        type = symbol->type;
        type_flags = type->flags;
        attributes = type->attributes;
        shape = type->related;
        if (kind == 5)
            goto formats;
        if (symbol->attributes & 2) {
            type = intern_type(drop_first_type_shape(type));
            shape = type->related;
        }
        if ((symbol->storage[0] & 7) == 7) {
            tag = 11;
            goto check_record;
        }
        array_type = shape && (shape->flags & 7) == 2;
        if (array_type && tag == 1)
            tag = 2;
        if (shape && (shape->flags & 7) == 4)
            inner_type = drop_first_type_shape(type);
        if (symbol->attributes & 2) {
            tag_symbol = (FrontendSymbol *)symbol->type->related->field08;
            if ((tag_symbol->kind & 31) == 13 && !(tag_symbol->flags & 2) && !global_record_kind)
                emit_symbol_declaration(tag_symbol);
        }
    } else {
        type = 0;
        type_flags = attributes = 0;
        shape = 0;
    }
check_record:
    if (tag == 11 && !options.debug_info)
        return;
    if (tag == 18) {
        left = last_record_source;
        right = source_filename;
        while (*left && *left == *right) {
            ++left;
            ++right;
        }
        if (*left == *right) {
            *source_file_index = source_file_counter - 1;
            return;
        }
    }
formats:
    format = global_record_formats[tag];
    stream = tag == 18 || !global_record_kind ? global_file : symbol_file;
    while (*format) {
        switch (*format++) {
        case GF_SIZE:
            kind = symbol->kind & 31;
            if (kind == 6 || kind == 7)
                size = type_size(inner_type);
            else {
                size = type_size(type);
                if (!size && (kind == 10 || kind == 11))
                    require_complete_type(type);
            }
            if ((symbol->attributes & 4) && (type_flags & 0x400) &&
                (((FrontendSymbol *)type->field08)->attributes & 0x100) && !shape)
                size += symbol->detail.value;
            GLOBAL_INTEGER(size);
            break;
        case GF_FLAGS:
            kind = symbol->kind & 31;
            flags = (symbol->attributes & 0x8000) && kind != 6 && kind != 7 ? 0x80 : 0;
            if (kind == 6 || kind == 7) {
                if (symbol->attributes & 0x200) flags |= 0x100;
                if (symbol->storage[2] & 1) flags |= 0x400;
            }
            if (symbol->references) {
                if ((kind == 9 || kind == 10) && (symbol->attributes & 0x800))
                    flags |= 0x2000;
                if (kind != 6 && kind != 7) flags |= 0x100;
            }
            if (symbol->attributes & 8) flags |= 0x400;
            if ((attributes & 7) == 3) flags |= 3;
            else if (attributes & 7) flags |= 1;
            if (attributes & 0x100)
                flags |= 0xc;
            else {
                switch (attributes & 0x70) {
                case 0x20: case 0x30: flags |= 8; break;
                case 0x40: flags |= 4; break;
                case 0x50: flags |= 0x18; break;
                case 0x60:
                    if (!shape || (shape->flags & 7) != 4 || !(shape->flags & 0x8000))
                        flags |= 0x10;
                    break;
                case 0x70: flags |= 0x14; break;
                }
            }
            if (kind == 6 || kind == 7) {
                if (symbol->attributes & 0x1000) flags |= 0x8000;
                if (symbol->attributes & 0x100) flags |= 0x1000;
            }
            if (attributes & 0x200) flags |= 0x80;
            if ((symbol->storage[2] & 2) && kind != 7) flags |= 0x1000;
            if (attributes & 0x800) flags |= 0x4000;
            if (kind == 9 && (symbol->storage[1] & 0x80)) flags |= 0x40;
            storage = symbol->storage[0] & 7;
            if (storage >= 5 && storage <= 6 && !(symbol->attributes & 0x400) && options.option_m)
                flags |= 0x4000;
            if (options.option_NoComdat && (symbol->storage[0] & 0x40) &&
                (tag == 14 || (symbol->attributes & 4) || !(symbol->type->attributes & 15)) &&
                (!(symbol->storage[2] & 4) || (symbol->storage[2] & 8)))
                flags |= 0x20;
            if ((symbol->attributes & 2) &&
                (((FrontendSymbol *)symbol->type->related->field08)->kind & 31) == 13) {
                GLOBAL_SHORT(flags | 0x200);
                tag_symbol = (FrontendSymbol *)symbol->type->related->field08;
                GLOBAL_INDEX(tag_symbol->index);
            } else
                GLOBAL_SHORT(flags);
            break;
        case GF_RECORD_TAG:
            GLOBAL_BYTE(stream, (signed char)tag);
            break;
        case GF_ARGUMENT_BYTES:
            if (array_type) GLOBAL_INTEGER(symbol_element_size(symbol));
            else GLOBAL_INTEGER(0);
            break;
        case GF_SYMBOL_INDEX:
            GLOBAL_INDEX(symbol->index);
            break;
        case GF_DEBUG_TYPE:
            if ((symbol->kind & 31) == 4 && (short)symbol->metadata.value)
                GLOBAL_SHORT(symbol->metadata.value);
            else
                GLOBAL_SHORT(type->field0c);
            break;
        case GF_CONTEXT:
            GLOBAL_BYTE(stream, tag == 3 ? (unsigned char)symbol->metadata.value : global_record_kind);
            break;
        case GF_REGISTER:
            kind = symbol->kind & 31;
            if (kind == 8)
                code = (unsigned char)symbol->detail.value;
            else if (symbol->attributes & 8)
                code = (symbol->storage[0] & 7) == 2 ? 0 : 0x25;
            else if (kind == 9 && (symbol->storage[0] & 7) == 3)
                code = 0x24;
            else code = 0;
            GLOBAL_BYTE(stream, (signed char)code);
            break;
        case GF_NAME:
            if (symbol->name && symbol->name != name_unnamed) {
                auxiliary_ir_stream = stream;
                write_decorated_name(stream, symbol);
            } else GLOBAL_BYTE(stream, 0);
            break;
        case GF_COMMON_KIND:
            GLOBAL_BYTE(stream, (symbol->attributes & 0x400) ? 2 : 1);
            break;
        case GF_ZERO:
            GLOBAL_BYTE(stream, 0);
            break;
        case GF_DISTANCE:
            kind = symbol->kind & 31;
            storage = symbol->storage[0] & 7;
            if (kind == 10 || kind == 11)
                distance = 3;
            else if (storage >= 1 && storage <= 3) {
                if ((attributes & 0x1000) && (!shape || (symbol->flags & 0x10)) &&
                    (!(symbol->attributes & 2) ||
                     (((FrontendSymbol *)symbol->type->related->field08)->kind & 31) != 13 ||
                     ((FrontendSymbol *)symbol->type->related->field08)->index == 3))
                    distance = 4;
                else
                    distance = ((shape ? shape->attributes : attributes) & 0x1000) ? 4 : 2;
            } else distance = 1;
            GLOBAL_BYTE(stream, distance);
            break;
        case GF_TYPE:
            aligned_type = type;
            if (tag == 4 || tag == 16 || tag == 14) {
                aligned_type = inner_type;
                base_shape = inner_type->related;
                base_flags = base_shape ? base_shape->flags : 0;
                base_attributes = base_shape ? base_shape->attributes : 0;
                code = 0;
            } else {
                base_flags = shape ? shape->flags : 0;
                base_attributes = shape ? shape->attributes : 0;
                code = (type_flags & 0x400) &&
                    (type_flags == 0x414 || (((FrontendSymbol *)symbol->type->field08)->metadata.value & 0x1000000)) ? 0x10 : 0;
            }
            alignment = type_alignment(aligned_type) >> 1;
            alignment_code = 0;
            while (alignment) { ++alignment_code; alignment >>= 1; }
            if (++alignment_code > 5) alignment_code = 0;
            alignment_code <<= 9;
            if ((base_flags & 7) == 2) code |= 6;
            else if (base_attributes) code |= (base_attributes & 7) == 3 ? 11 : 3;
            else if (type_flags & 0x4000) code |= 2;
            else {
                switch (type_flags) {
                case 0x41:
                    code |= !options.extensions_enabled && options.option_J && !(type_flags & 0x8000) ? 2 : 1;
                    break;
                case 0x42: case 0x44: case 0x64: case 0x214: case 0x8041: code |= 1; break;
                case 0x80: code |= 7; break;
                case 0x104: case 0x108: case 0x128: code |= 5; break;
                case 0x412: case 0x414: case 0x418: code |= 6; break;
                case 0x802: code |= 2; break;
                }
            }
            encoded_type = (short)(signed char)code | alignment_code;
            if (encoded_type > 0x7f)
                GLOBAL_BYTE(stream, (unsigned char)((encoded_type >> 8) | 0x80));
            GLOBAL_BYTE(stream, (unsigned char)encoded_type);
            break;
        case GF_REFERENCES:
            GLOBAL_SHORT(symbol->references);
            break;
        case GF_ENUM_VALUE:
            if (symbol->detail.text && symbol->detail.text != name_unnamed) {
                auxiliary_ir_stream = stream;
                write_ir_string(symbol->detail.text);
            } else GLOBAL_BYTE(stream, 0);
            break;
        case GF_STORAGE:
            storage = symbol->storage[0] & 7;
            if (attributes & 0x400)
                code = (symbol->flags & 1) ? 6 : 2;
            else if ((symbol->storage[2] & 12) == 12)
                code = (symbol->flags & 1) ? 7 : 2;
            else if ((symbol->storage[2] & 12) == 4)
                code = 8;
            else if (symbol->storage[0] & 0x40)
                code = (symbol->attributes & 4) ? (options.option_NoComdat ? 1 : 4) : 3;
            else if (storage >= 3 && storage <= 6)
                code = 4;
            else if (symbol->attributes & 4)
                code = 1;
            else if (storage == 1 || (array_type && !shape->field08))
                code = 2;
            else code = 3;
            GLOBAL_BYTE(stream, code);
            break;
        case GF_FUNCTION_STORAGE:
            storage = symbol->storage[0] & 7;
            kind = symbol->kind & 31;
            if ((kind == 6 || kind == 7) && (symbol->flags & 1)) {
                if (attributes & 0x400) code = storage == 3 ? 8 : 7;
                else if ((symbol->storage[2] & 12) == 12) code = 9;
                else if ((symbol->storage[2] & 12) == 4) code = 10;
                else if (symbol->storage[0] & 0x40) code = options.option_NoComdat ? 5 : 3;
                else code = storage == 3 ? 3 : 5;
            } else if (storage == 3) {
                report_syntax_error(129, symbol->name);
                code = 2;
            } else {
                if ((symbol->storage[1] & 1) && storage != 1)
                    compiler_warning(1, 506, symbol->name);
                code = (symbol->storage[2] & 12) == 4 ? 10 : 4;
            }
            GLOBAL_BYTE(stream, code);
            break;
        case GF_ZERO_19:
            GLOBAL_INTEGER(0);
            break;
        case GF_FUNCTION_END:
            GLOBAL_SHORT(2);
            GLOBAL_BYTE(stream, (signed char)(symbol->references & 255));
            GLOBAL_BYTE(stream, (signed char)(symbol->references >> 8));
            break;
        case GF_ALIAS_INDEX:
            GLOBAL_INDEX(symbol_alias(symbol)->index);
            break;
        case GF_TAG_INDEX:
            tag_symbol = (FrontendSymbol *)symbol->type->related->field08;
            GLOBAL_INDEX(tag_symbol->index);
            break;
        case GF_TAG_KIND:
            code = (unsigned char)symbol->metadata.value;
            GLOBAL_BYTE(stream, code == 0 || code == 2 ? code : 1);
            break;
        case GF_STREAM_POSITIONS:
            GLOBAL_INTEGER(function_ir.expression_offset);
            GLOBAL_INTEGER(function_ir.symbol_offset + (function_in_pch ? pch_symbol_offset_adjustment : 0));
            break;
        case GF_COMPLEXITY:
            GLOBAL_SHORT(function_ir.complexity > 65535 ? -1 : (short)function_ir.complexity);
            break;
        case GF_OPTIMIZATION:
            code = ((symbol->attributes & 0x8000) ? 4 : 0) |
                ((symbol->storage[1] & 1) ? 0x80 : 0) |
                (function_call_seen ? 8 : 0) | (function_in_pch ? 1 : 0) |
                (function_ir.force_output ? 0x20 : 0);
            no_inline = attributes & 0x800;
            function_not_inlinable = no_inline || (options.option_GA &&
                ((attributes & 0x400) || ((attributes & 7) == 2 && options.option_GEf)));
            if (symbol->storage[2] & 1) {
                function_not_inlinable = 1;
                code &= 0x7f;
            }
            switch (options.option_Ob) {
            case 0:
                if (!(symbol->storage[1] & 1)) code |= 0x20;
                break;
            case 2:
                if (inline_candidate) {
                    if (function_not_inlinable) goto cannot_inline;
                    if (!plmn_pragma_seen && !normalized_PLMN && !plmf_pragma_seen && !normalized_PLMF &&
                        (((symbol->storage[0] & 7) == 3 && symbol->references == 1) ||
                         (unsigned)options.option_Bu > function_ir.complexity))
                        code |= 0x40;
                }
                /* The general inline eligibility checks also apply to /Ob2. */
            case 1:
                if (function_not_inlinable || (symbol->type->related->flags & 0x8000) ||
                    (no_inline && !option_Au)) {
                cannot_inline:
                    code &= 0xbf;
                    if (symbol->storage[1] & 1)
                        compiler_warning(4, 710, symbol->name);
                } else if (symbol->storage[1] & 1)
                    code |= 0x40;
                else if (!(code & 0x40)) code |= 0x20;
                break;
            }
            GLOBAL_BYTE(stream, (signed char)code);
            break;
        case GF_INLINE_REGIONS:
            GLOBAL_SHORT((symbol->attributes & 0x1000) >> 12);
            GLOBAL_SHORT((unsigned short)((inline_region_index + 1) << 4));
            fwrite(inline_regions, 16, (unsigned)inline_region_index + 1, stream);
            break;
        case GF_MACHINE_LEGACY:
        case GF_MACHINE:
            machine = 0x386;
            fwrite(&machine, 2, 1, stream);
            break;
        case GF_COMPILER_VERSION:
            version = 0x1301d2e;
            fwrite(&version, 4, 1, stream);
            break;
        case GF_SYMBOL_COUNT16:
            fwrite(&next_symbol_index, 2, 1, stream);
            break;
        case GF_RESERVED_BYTE:
            GLOBAL_BYTE(stream, 0);
            break;
        case GF_PRAGMA_VALUES:
            GLOBAL_INTEGER(pragma_record_number);
            auxiliary_ir_stream = stream;
            if (pragma_text) write_ir_filename(pragma_text);
            else GLOBAL_BYTE(stream, 0);
            count = 0;
            for (pragma = pragma_values; pragma; pragma = pragma->next) ++count;
            GLOBAL_BYTE(stream, (signed char)count);
            for (pragma = pragma_values; pragma; pragma = pragma->next) {
                GLOBAL_BYTE(stream, pragma->kind);
                if (pragma->kind == 7) {
                    count = (unsigned char)(pragma->value[0] + 1);
                    for (i = 0; i < count; ++i) GLOBAL_BYTE(stream, pragma->value[i]);
                } else write_ir_string((char *)pragma->value);
            }
            break;
        case GF_SOURCE_FILE:
            auxiliary_ir_stream = global_file;
            *source_file_index = source_file_counter++;
            write_ir_index(source_file_index);
            write_ir_filename(source_filename);
            left = last_record_source;
            right = source_filename;
            do { *left++ = *right++; } while (left[-1]);
            break;
        case GF_PARAMETER_COUNT:
            GLOBAL_SHORT(function_ir.parameter_count);
            break;
        case GF_SYMBOL_COUNT32:
            fwrite(&next_symbol_index, 4, 1, stream);
            break;
        }
    }
}

/* Original 0x4b680..0x4b7f5. Two bytes hold indices through 0x7fff;
   larger indices set bit 15 and shift their upper bits around that marker. */
void write_ir_index(unsigned *index)
{
    unsigned value = *index;
    putc((signed char)value, auxiliary_ir_stream);
    if (value <= 0x7fff) putc((signed char)(value >> 8), auxiliary_ir_stream);
    else {
        putc((short)(signed char)(value >> 8) | 0x80, auxiliary_ir_stream);
        value <<= 1;
        putc((signed char)(value >> 16), auxiliary_ir_stream);
        putc((signed char)(value >> 24), auxiliary_ir_stream);
    }
}

/* Original 0x4b9a0..0x4b9e5. The terminating NUL is part of the IR name. */
void write_ir_filename(char *text)
{
    do { putc((signed char)*text, auxiliary_ir_stream); } while (*text++);
}

/* Original 0x2e160..0x2e264. C names are already prefixed by the
   declarator; stdcall and fastcall functions append their parameter bytes. */
void __stdcall write_decorated_name(FILE *stream, FrontendSymbol *symbol)
{
    char *text = symbol->name;
    unsigned kind = symbol->kind & 31;
    unsigned convention;
    int length;

    while (*text) { GLOBAL_BYTE(stream, (signed char)*text); ++text; }
    if (kind == 6 || kind == 7) {
        convention = symbol->type->attributes & 0x70;
        if (convention == 0x60 || convention == 0x40) {
            if (!(symbol->attributes & 0x4000)) cache_parameter_bytes(symbol);
            GLOBAL_BYTE(stream, '@');
            length = format_integer(symbol->detail.value, symbol_name_buffer, 10);
            symbol_name_buffer[length] = '\0';
            text = symbol_name_buffer;
            while (*text) { GLOBAL_BYTE(stream, (signed char)*text); ++text; }
        }
    }
    GLOBAL_BYTE(stream, 0);
}

/* Original 0x36500..0x3653d. This global-record entry remains distinct
   from expression_type_alignment despite their identical original bodies. */
unsigned short __stdcall type_alignment(CType *type)
{
    if (type->related && (type->related->flags & 7) != 2) return 4;
    if (type->flags & 0x400)
        return (unsigned short)((((FrontendSymbol *)type->field08)->metadata.value >> 16) & 0xff);
    return type->flags & 15;
}

/* Original 0x4b800..0x4b95b. The reserved byte 0x80 introduces the
   full little-endian integer; every other small value fits in one byte. */
void write_ir_integer(int *value)
{
    signed char *byte = (signed char *)value;
    unsigned count;
    if (*value >= -127 && *value <= 127) {
        putc(*byte, auxiliary_ir_stream);
        return;
    }
    putc(0x80, auxiliary_ir_stream);
    for (count = 0; count < 4; ++count) putc(byte[count], auxiliary_ir_stream);
}

/* Original 0x4ba40..0x4baa0. Retain the signed sixteen-bit loop count
   used by the frontend's counted text records. */
void write_counted_ir(short length, char *text)
{
    write_ir_short(&length);
    while (--length >= 0) putc((signed char)*text++, auxiliary_ir_stream);
}

/* Original 0x4b580..0x4b675. A reserved byte distinguishes a complete
   little-endian short from the single-byte signed representation. */
void write_ir_short(short *value)
{
    signed char *byte = (signed char *)value;
    if (*value >= -127 && *value <= 127) {
        putc(*byte, auxiliary_ir_stream);
        return;
    }
    putc(0x80, auxiliary_ir_stream);
    putc(byte[0], auxiliary_ir_stream);
    putc(byte[1], auxiliary_ir_stream);
}
