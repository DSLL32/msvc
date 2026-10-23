/* Host-only reader for the raw CL-export argv vector. */
#ifndef C1XX_INVOKE_ARGUMENTS_H
#define C1XX_INVOKE_ARGUMENTS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>

static int read_invoke_arguments(const char *path, char ***arguments)
{
    FILE *file = fopen(path, "rb");
    unsigned address, count, pointer, length, i;
    char **values;
    if (!file || fread(&address, 4, 1, file) != 1 || fread(&count, 4, 1, file) != 1 ||
            !address || !count || count > 512) exit(126);
    values = (char **)malloc((count + 1) * sizeof(char *));
    if (!values) exit(126);
    for (i = 0; i < count; ++i) {
        if (fread(&pointer, 4, 1, file) != 1 || fread(&length, 4, 1, file) != 1 ||
                !pointer || !length || length > 32768) exit(126);
        values[i] = (char *)malloc(length);
        if (!values[i] || fread(values[i], 1, length, file) != length ||
                values[i][length - 1] || memchr(values[i], 0, length - 1)) exit(126);
    }
    values[count] = NULL;
    if (fgetc(file) != EOF || fclose(file)) exit(126);
    *arguments = values;
    return count;
}
#endif
