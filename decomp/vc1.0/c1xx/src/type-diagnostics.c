/* Nested scratch strings used by type and declaration diagnostics. */
#include <stdlib.h>
#include "declarations.h"
#include "cpp-context.h"
#include "type-diagnostics-internal.h"
#include <recon.h>

TypeDiagnosticStorage type_diagnostic_storage; /* original 0x90f54, slots start 0x90f64 */
unsigned next_type_diagnostic_handle = 100; /* original 0xa1f18 */
unsigned type_diagnostic_depth; /* original 0x9fcc0 */

/* Original 0x68a30..0x68a3a. Each nesting level reserves its first
   text handle through the same allocator used for additional strings. */
unsigned short begin_type_diagnostic(void)
{
    ++type_diagnostic_depth;
    return (unsigned short)reserve_type_diagnostic();
}

/* Original 0x68980..0x68a02. Reserve a free centered string buffer.
   On exhaustion retain the original -1 slot, including its forced-input alias.
   No new capacity guard or synthetic fallback handle is introduced. */
short reserve_type_diagnostic(void)
{
    TypeDiagnosticSlot *slot;
    unsigned char *cursor;
    unsigned bytes, words;
    int index = -1, i;
    for (i = 0; i < 32; ++i) {
        if (!type_diagnostic_slots[i].handle) {
            index = i;
            break;
        }
    }
    slot = &type_diagnostic_slots[index];
    if (!slot->buffer) {
        slot->capacity = 128;
        slot->buffer = malloc(128);
    }
    slot->handle = (short)next_type_diagnostic_handle++;
    slot->left = 64;
    slot->right = 65;
    bytes = slot->capacity;
    cursor = (unsigned char *)slot->buffer;
    for (words = bytes / 4; words; --words) {
        *(unsigned *)cursor = 0;
        cursor += 4;
    }
    for (bytes &= 3; bytes; --bytes) *cursor++ = 0;
    return slot->handle;
}

/* Original 0x68260..0x68295. Emit every enclosing class, then its scope
   separator and name. The first component retains its global :: prefix. */
void __stdcall format_diagnostic_symbol(short text, CppSymbol *symbol)
{
    if (symbol) {
        format_diagnostic_symbol(text, next_member_context(symbol));
        append_type_diagnostic(text, "::");
        append_type_diagnostic(text, symbol->name);
    }
}

/* Original 0x67820..0x6785a. Reserve only the string's nonzero bytes,
   leaving the centered diagnostic buffer's existing NUL boundary intact. */
void __stdcall append_type_diagnostic(short text, char *value)
{
    unsigned length = 0, words, bytes;
    char *cursor;
    while (value[length]) ++length;
    cursor = reserve_type_diagnostic_bytes(text, 1, length);
    for (words = length / 4; words; --words) {
        *(unsigned *)cursor = *(unsigned *)value;
        cursor += 4;
        value += 4;
    }
    for (bytes = length & 3; bytes; --bytes) *cursor++ = *value++;
}

/* Original 0x68aa0..0x68c48. Grow a centered text buffer on either side,
   retaining 16-bit capacity/index wrap. Prepend returns the former left
   boundary; its caller writes backward from that byte. Append writes forward.
   The original prepend growth adds bytes+32 to the prior right boundary;
   retain that arithmetic rather than recomputing a compacted string length. */
char *__stdcall reserve_type_diagnostic_bytes(short text, int append, unsigned bytes)
{
    TypeDiagnosticSlot *slot = find_type_diagnostic_slot(text);
    char *buffer, *cursor, *source, *target;
    unsigned capacity, words, count, offset;
    if (!slot) return 0;
    if (append != 0 && append != 1) return 0;
    if ((!append && slot->left < bytes) ||
        (append && (unsigned)slot->right + bytes >= slot->capacity)) {
        capacity = (unsigned short)(slot->capacity + bytes + 32);
        buffer = malloc(capacity);
        cursor = buffer;
        for (words = capacity / 4; words; --words) {
            *(unsigned *)cursor = 0;
            cursor += 4;
        }
        for (count = capacity & 3; count; --count) *cursor++ = 0;
        if (!append) {
            source = slot->buffer + slot->left + 1;
            target = buffer + bytes + 33;
            count = (unsigned)slot->right - slot->left - 1;
        } else {
            source = slot->buffer;
            target = buffer;
            count = slot->right;
        }
        for (words = count / 4; words; --words) {
            *(unsigned *)target = *(unsigned *)source;
            target += 4;
            source += 4;
        }
        for (count &= 3; count; --count) *target++ = *source++;
        free(slot->buffer);
        slot->buffer = buffer;
        if (!append) {
            slot->right = (unsigned short)(slot->right + bytes + 32);
            slot->left = (unsigned short)(bytes + 32);
        }
        slot->capacity = (unsigned short)capacity;
    }
    if (!append) {
        offset = slot->left;
        slot->left = (unsigned short)(offset - bytes);
    } else {
        offset = slot->right;
        slot->right = (unsigned short)(offset + bytes);
    }
    return slot->buffer + offset;
}

/* Original 0x68940..0x6897a. Match all 32 slots by their 16-bit handle,
   including handle zero's first unused slot; the boundary prefix is excluded. */
TypeDiagnosticSlot *__stdcall find_type_diagnostic_slot(short text)
{
    unsigned short index;
    for (index = 0; index < 32; ++index)
        if (type_diagnostic_slots[index].handle == text)
            return &type_diagnostic_slots[index];
    return 0;
}

/* Original 0x68a70..0x68a93. Text starts one byte beyond the current
   left boundary; an unknown handle has no associated string. */
char *__stdcall type_diagnostic_text(short text)
{
    TypeDiagnosticSlot *slot = find_type_diagnostic_slot(text);
    if (!slot) return 0;
    return slot->buffer + slot->left + 1;
}

/* Original 0x68a40..0x68a61. The outermost diagnostic scope releases
   handles without freeing their reusable buffers. An unmatched end retains
   the original unsigned depth wrap and therefore does not clear the pool. */
void end_type_diagnostic(void)
{
    unsigned index;
    if (--type_diagnostic_depth) return;
    for (index = 0; index < 32; ++index)
        type_diagnostic_slots[index].handle = 0;
}
