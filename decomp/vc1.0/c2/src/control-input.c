#include "control-input.h"
#include "input.h"
#include "records.h"
#include "output.h"
#include <recon.h>
typedef enum ControlField {
    CONTROL_END, CONTROL_INVALID, CONTROL_SOURCE_LINE, CONTROL_SOURCE_FILE,
    CONTROL_DIRECTIVE, CONTROL_STRING, CONTROL_UNSUPPORTED, CONTROL_WORD,
    CONTROL_LONG, CONTROL_SIGNED_WORD, CONTROL_VALUE, CONTROL_WARNING_STATE
} ControlField;
static const ControlField source_line_fields[] = {CONTROL_SOURCE_LINE, CONTROL_END};
static const ControlField source_file_fields[] = {CONTROL_SOURCE_FILE, CONTROL_END};
static const ControlField directive_fields[] = {CONTROL_DIRECTIVE, CONTROL_END};
static const ControlField string_fields[] = {CONTROL_STRING, CONTROL_END};
static const ControlField unsupported_fields[] = {CONTROL_UNSUPPORTED, CONTROL_END};
static const ControlField word_fields[] = {CONTROL_WORD, CONTROL_END};
static const ControlField long_fields[] = {CONTROL_LONG, CONTROL_END};
static const ControlField signed_word_fields[] = {CONTROL_SIGNED_WORD, CONTROL_END};
static const ControlField warning_state_fields[] = {CONTROL_WARNING_STATE, CONTROL_END};
static const ControlField invalid_fields[] = {CONTROL_INVALID, CONTROL_END};
static const ControlField two_invalid_fields[] = {CONTROL_INVALID, CONTROL_INVALID, CONTROL_END};
unsigned pragma_value, saved_pragma_value;
char *current_source_filename;
/* Original [0004ce10,0004ce90). Most directives carry a C string; the
   precompiled-type record instead retains its length byte and binary payload. */
void __fastcall read_module_directive(int kind)
{
    char buffer[256], *text, *destination;
    int length;
    unsigned remaining;
    if (kind == 7) {
        length = (signed char)input_byte();
        text = arena_alloc(1, (unsigned)(length + 1));
        text[0] = (char)length;
        destination = text;
        remaining = (unsigned)length;
        while (remaining--) *++destination = (char)input_byte();
    } else {
        input_string(buffer, sizeof(buffer));
        text = arena_strdup(buffer, 1);
    }
    add_module_directive(kind, text);
}

/* Original [0004cc00,0004ce10), including the control-field jump table. */
void __fastcall read_control_fields(Node *node)
{
    const ControlField *fields;
    char buffer[256], *text;
    unsigned char *pairs, *destination;
    unsigned bytes, i;
    short length, value;
    switch ((signed char)node->detail.control.code) {
    case 1: fields = source_line_fields; break;
    case 2: fields = source_file_fields; break;
    case 3: fields = two_invalid_fields; break;
    case 0x0b: fields = directive_fields; break;
    case 0x16: fields = string_fields; break;
    case 0x17: fields = unsupported_fields; break;
    case 0x18: fields = word_fields; break;
    case 0x1f: fields = long_fields; break;
    case 0x20: fields = signed_word_fields; break;
    case 0x23: fields = warning_state_fields; break;
    case 4: case 5: case 6: case 0x0a: case 0x0c: case 0x0d:
    case 0x0e: case 0x13: case 0x14: case 0x15: case 0x1e: case 0x21:
        fields = invalid_fields; break;
    default: return;
    }
    while (*fields != CONTROL_END) {
        switch (*fields++) {
        case CONTROL_DIRECTIVE:
            read_module_directive((signed char)input_byte());
            break;
        case CONTROL_STRING:
            length = input_counted_string(buffer, 256);
            bytes = length + 1;
            text = arena_alloc(2, bytes);
            node->left.pointer = text;
            buffer[length] = 0;
            for (i = 0; i != bytes; ++i) text[i] = buffer[i];
            break;
        case CONTROL_UNSUPPORTED:
            compiler_internal_error("@(#)p2pragma.c:1.72", 0x9a);
            break;
        case CONTROL_WORD:
            node->left.word = input_compact_short();
            break;
        case CONTROL_LONG:
            node->left.value = input_compact_signed();
            break;
        case CONTROL_SIGNED_WORD:
            node->left.value = input_compact_short();
            break;
        case CONTROL_VALUE:
            pragma_value = saved_pragma_value = input_varuint();
            break;
        case CONTROL_WARNING_STATE:
            pairs = (unsigned char *)buffer;
            bytes = 2;
            for (;;) {
                value = input_compact_short();
                pairs[0] = (unsigned char)value;
                pairs[1] = (unsigned char)((unsigned short)value >> 8);
                pairs += 2;
                if (value == -1) break;
                *pairs++ = input_byte();
                bytes += 3;
            }
            destination = arena_alloc(1, bytes);
            node->right.pointer = destination;
            for (i = 0; i != bytes; ++i) destination[i] = (unsigned char)buffer[i];
            break;
        case CONTROL_SOURCE_LINE:
            current_source_location = pending_source_location = (unsigned short)input_compact_short();
            node->opcode = 1;
            break;
        case CONTROL_SOURCE_FILE:
            current_source_filename = lookup_filename(input_varuint());
            node->right.pointer = current_source_filename;
            node->opcode = 0x56;
            break;
        default:
            compiler_internal_error("@(#)p2pragma.c:1.72", 0x133);
            break;
        }
    }
}
