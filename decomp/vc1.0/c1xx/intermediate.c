/* Frontend intermediate file creation and output. */
#include "initialization.h"

/* Original 0x74250..0x7430d. */
FILE *open_intermediate(char *prefix, char *extension, char *mode)
{
    char filename[260];
    char *output = filename;
    FILE *file;
    while ((*output++ = *prefix++) != 0)
        ;
    if (extension) {
        --output;
        while ((*output++ = *extension++) != 0)
            ;
    }
    file = fopen(filename, mode);
    if (!file)
        report_fatal(83, message_text(339), filename);
    setvbuf(file, 0, 0, 4096);
    return file;
}
