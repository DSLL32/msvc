/* Static initializer records from the frontend. */
#include "initializers.h"
#include "input.h"
#include "backend.h"
Initializer *global_initializers; /* 0007c14c */
static Initializer *free_initializers; /* 0007c144 */
typedef char InitializerSizeCheck[sizeof(Initializer) == 28 ? 1 : -1];
/* Original [0004eca0,0004ed40). Global links group records by symbol;
   within a symbol, kind-zero block markers stay sorted by their offset. */
void __fastcall insert_initializer(Initializer *initializer)
{
    Initializer **slot = &global_initializers;
    Initializer *current, *following_symbol, *tail;
    long offset;
    int replace_head;
    while (*slot && (*slot)->content.reference.symbol_id != initializer->content.reference.symbol_id)
        slot = &(*slot)->content.reference.link;
    current = *slot;
    if (current) {
        following_symbol = current->content.reference.link;
        offset = initializer->content.reference.value;
        replace_head = current->content.reference.value > offset;
        while (current && (*slot)->content.reference.value <= offset) {
            slot = &(*slot)->next;
            while (*slot && (*slot)->kind != 0) slot = &(*slot)->next;
            current = *slot;
        }
        current = *slot;
        if (current) {
            tail = initializer->next;
            while (tail->next) tail = tail->next;
            tail->next = current;
        }
        if (replace_head) {
            initializer->content.reference.link = following_symbol;
            current->content.reference.link = NULL;
        }
    }
    *slot = initializer;
}
/* Original [0004ec60,0004eca0). Free-list links use +8; reused records
   have all seven words cleared, matching fresh arena-zero allocations. */
Initializer *__fastcall new_initializer(int kind)
{
    Initializer *initializer = free_initializers;
    unsigned *word, i;
    if (initializer) {
        free_initializers = initializer->content.reference.link;
        word = (unsigned *)initializer;
        for (i = 0; i < 7; ++i) word[i] = 0;
    } else initializer = arena_alloc(0, sizeof(Initializer));
    initializer->kind = kind;
    return initializer;
}
/* Original [0004e830,0004e840). */
void initialize_globals(void)
{
    read_initializers(initializer_file);
}

/* Original [0004e840,0004e8e0). */
void __fastcall read_initializers(FILE *file)
{
    int tag;
    Initializer *initializer;
    input_file = file;
    for (;;) {
        tag = (signed char)input_byte();
        switch (tag) {
        case -1: return;
        case 0:
            initializer = new_initializer(tag);
            initializer->content.reference.symbol_id = input_varuint();
            initializer->content.reference.value = input_compact_signed();
            read_initializer_body(initializer);
            insert_initializer(initializer);
            break;
        case 9:
            input_varuint();
            break;
        case 10:
            initializer = new_initializer(tag);
            initializer->content.reference.symbol_id = 0;
            initializer->content.reference.value = input_compact_signed();
            read_initializer_body(initializer);
            emit_initializer(initializer);
            release_initializer(initializer);
            break;
        default:
            compiler_internal_error("@(#)p2symtab.c:1.89", 0x5d2);
            break;
        }
    }
}

/* Original [0004e900,0004ec60), including the eleven-entry tag table.
   Tags 5/6 delimit a repeated pattern. Its scalar/byte pieces are combined
   into one arena-zero byte string before it joins the ordinary record chain. */
void __fastcall read_initializer_body(Initializer *initializer)
{
    Initializer *tail = initializer, *pending = NULL, *record;
    int tag, subtag, repeated = 0, left_bytes, right_bytes;
    short count;
    unsigned i;
    unsigned char *left, *right, *bytes;
    for (;;) {
        tag = (signed char)input_byte();
        if (tag == 7) return;
        if (tag == 6) {
            repeated = 0;
            tail->next = pending;
            tail = pending;
            pending = NULL;
            continue;
        }
        record = new_initializer(tag);
        switch (tag) {
        case 1:
            record->repeat = 1;
            goto scalar_value;
        case 2:
            record->content.relocation.symbol_id = input_varuint();
            record->content.relocation.offset = input_compact_signed();
            record->content.relocation.flags = input_byte();
            break;
        case 3:
            record->repeat = 1;
            goto byte_string;
        case 4:
            record->content.byte_argument = input_byte();
            break;
        case 5:
            record->repeat = input_compact_signed();
            repeated = 1;
            subtag = (signed char)input_byte();
            if (subtag != 1 && subtag != 3)
                compiler_internal_error("@(#)p2symtab.c:1.89", 0x5f0);
            record->kind = 1;
            if (subtag == 1) goto scalar_value;
            goto byte_string;
        case 8:
            record->content.argument = input_compact_signed();
            break;
        case 11:
            record->kind = 1;
            record->repeat = 1;
            record->content.datum.type = 6;
            count = input_compact_short();
            record->content.datum.bytes = (short)(count + 2);
            bytes = arena_alloc(4, (unsigned)(int)record->content.datum.bytes);
            record->content.datum.value.bytes = bytes;
            bytes[0] = (unsigned char)count;
            bytes[1] = (unsigned char)((unsigned short)count >> 8);
            input_bytes(count, bytes + 2);
            break;
        default:
            compiler_internal_error("@(#)p2symtab.c:1.89", 0x641);
            break;
        }
finish_record:
        if (!repeated) {
            tail->next = record;
            tail = record;
            if (pending) compiler_internal_error("@(#)p2symtab.c:1.89", 0x646);
            continue;
        }
        if (!pending) {
            pending = record;
            continue;
        }
        if (pending->kind != 1) compiler_internal_error("@(#)p2symtab.c:1.89", 0x64e);
        left = pending->content.datum.type == 6 ? pending->content.datum.value.bytes :
            (unsigned char *)&pending->content.datum.value;
        right = record->content.datum.type == 6 ? record->content.datum.value.bytes :
            (unsigned char *)&record->content.datum.value;
        left_bytes = pending->content.datum.bytes;
        right_bytes = record->content.datum.bytes;
        bytes = arena_alloc(0, (unsigned)(left_bytes + right_bytes));
        for (i = 0; i < (unsigned)left_bytes; ++i) bytes[i] = left[i];
        for (i = 0; i < (unsigned)right_bytes; ++i) bytes[left_bytes + i] = right[i];
        pending->content.datum.bytes = (short)(pending->content.datum.bytes + record->content.datum.bytes);
        pending->content.datum.type = 6;
        pending->content.datum.value.bytes = bytes;
        release_initializer(record);
        continue;
scalar_value:
        record->content.datum.type = (signed char)input_byte();
        record->content.datum.bytes = (signed char)input_byte();
        switch (record->content.datum.bytes) {
        case 1: record->content.datum.value.byte = input_byte(); break;
        case 2: record->content.datum.value.word = input_compact_short(); break;
        case 4:
            if (record->content.datum.type == 5) record->content.datum.value.float_bits = input_float_bits();
            else record->content.datum.value.integer = input_compact_signed();
            break;
        case 8: record->content.datum.value.double_bits = input_double_bits(); break;
        case 10: record->content.datum.value.extended = input_extended(); break;
        }
        goto finish_record;
byte_string:
        record->kind = 1;
        record->content.datum.bytes = input_compact_short();
        record->content.datum.value.bytes = arena_alloc(0, (unsigned)(int)record->content.datum.bytes);
        input_bytes(record->content.datum.bytes, record->content.datum.value.bytes);
        record->content.datum.type = 6;
        goto finish_record;
    }
}
