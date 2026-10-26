/* Primitive encodings on the selected intermediate stream. */
#include "global-ir.h"
#include <recon.h>

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

/* Original 0x73fb0..0x7410b. Signed values -127..127 occupy one
   byte; the reserved 0x80 prefix introduces all four little-endian bytes. */
void write_ir_integer(int *value)
{
    char *bytes = (char *)value;
    unsigned index;
    if (*value >= -127 && *value <= 127) putc(*bytes, auxiliary_ir_stream);
    else {
        putc(0x80, auxiliary_ir_stream);
        for (index = 0; index < sizeof(int); ++index)
            putc(bytes[index], auxiliary_ir_stream);
    }
}

/* Original 0x741f0..0x74250. Write the short count before copying
   its payload; retain the original decrement of the sixteen-bit formal. */
void write_counted_ir(short length, char *text)
{
    write_ir_short(&length);
    while (--length >= 0) putc(*text++, auxiliary_ir_stream);
}

/* Original 0x73d30..0x73e25. The short variant shares the reserved
   marker convention while retaining its two-byte source width. */
void write_ir_short(short *value)
{
    char *bytes = (char *)value;
    if (*value >= -127 && *value <= 127) putc(bytes[0], auxiliary_ir_stream);
    else {
        putc(0x80, auxiliary_ir_stream);
        putc(bytes[0], auxiliary_ir_stream);
        putc(bytes[1], auxiliary_ir_stream);
    }
}

/* Original 0x741a0..0x741e4. This is the distinct browse/global string
   writer, retaining its own entry despite sharing the filename encoding. */
void write_ir_string(char *text)
{
    do {
        putc((signed char)*text, auxiliary_ir_stream);
    } while (*text++);
}

/* Original 0x74110..0x74128 and0x74130..0x74148, cdecl4. Preserve
   raw target representations, not host text formatting. These auxiliary
   initializer paths are qualified by static ABI/graph proof and native units;
   no current CLI first-entry observation is claimed. */
void write_ir_float(float *value)
{
    fwrite(value, 4, 1, auxiliary_ir_stream);
}

void write_ir_double(double *value)
{
    fwrite(value, 8, 1, auxiliary_ir_stream);
}
