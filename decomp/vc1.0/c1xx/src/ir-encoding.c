/* Primitive encodings on the selected intermediate stream. */
#include "global-ir.h"

/* Original 0x73e30..0x73fa4. Indices use a little-endian 15-bit short
   form, or two words with the high-bit continuation marker in the first.
   Signed-byte promotion to _flsbuf matches the original putc expansions. */
void write_ir_index(unsigned *index)
{
    unsigned value = *index;
    putc((signed char)value, auxiliary_ir_stream);
    if (value <= 0x7fff) {
        putc((signed char)(value >> 8), auxiliary_ir_stream);
        return;
    }
    putc((signed char)(value >> 8) | 0x80, auxiliary_ir_stream);
    value <<= 1;
    putc((signed char)(value >> 16), auxiliary_ir_stream);
    putc((signed char)(value >> 24), auxiliary_ir_stream);
}

/* Original 0x74150..0x74194. Filenames include their terminating NUL. */
void write_ir_filename(char *text)
{
    do {
        putc((signed char)*text, auxiliary_ir_stream);
    } while (*text++);
}
