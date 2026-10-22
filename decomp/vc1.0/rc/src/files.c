#include "driver.h"
#include <recon.h>

/* Checked byte write, original 0x010070d0..0x010070ff. */
void __stdcall write_file(FILE *file, void *bytes, unsigned int count)
{
    if (fwrite(bytes, 1, count, file) != count)
        resource_finish("RC : fatal error RW1022: I/O error writing file.");
}

/* Checked byte read, original 0x01007090..0x010070c1. */
unsigned int __stdcall read_file(FILE *file, void *bytes, unsigned int count)
{
    unsigned int received = fread(bytes, 1, count, file);
    if (ferror(file))
        resource_finish("RC : fatal error RW1021: I/O error reading file.");
    return received;
}

/* Checked position change, original 0x01007100..0x0100714c. */
long __stdcall seek_file(FILE *file, long offset, int origin)
{
    long position;
    if (fseek(file, offset, origin))
        resource_finish("RC : fatal error RW1023: I/O error seeking in file");
    position = ftell(file);
    if (position == -1)
        resource_finish("RC : fatal error RW1023: I/O error seeking in file");
    return position;
}
