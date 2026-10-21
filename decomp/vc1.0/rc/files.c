#include "driver.h"

/* Checked byte write, original 0x010070d0..0x010070ff. */
void __stdcall write_file(FILE *file, void *bytes, unsigned int count)
{
    if (fwrite(bytes, 1, count, file) != count)
        resource_finish("RC : fatal error RW1022: I/O error writing file.");
}
