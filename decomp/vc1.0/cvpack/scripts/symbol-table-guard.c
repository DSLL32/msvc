/* Native-only bounded negative: unknown symbol kinds must stop at the table. */
#include "image.h"
#include "types.h"
#include <recon.h>
extern unsigned short extra_type_count;

int main(void)
{
    unsigned char data[8] = { 1, 0, 0, 0, 2, 0, 255, 255 };
    CvDirectoryEntry entry = { 0x124, 3, { 0 }, 8 };
    ModuleInfo module = { 0 };
    unsigned short padding = 0, skipped = 0;
    extra_type_count = 0;
    pack_new_types(data, &entry, &module, &padding, &skipped);
    return 1;
}
