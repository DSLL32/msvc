/* Native fixture entry only; excluded from the canonical RC link and trace. */
#include "rc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <recon.h>

int __cdecl resource_main(int, char **, char **);
int __stdcall is_space_byte(signed char);
unsigned int __stdcall copy_basename(const char *, char *);

static int direct_helper(const char *mode)
{
    static const char *paths[] = { "", "plain.txt", "one/two\\three.rc",
        "C:leaf", "C:\\dir\\name.bin", "trailing/" };
    FILE *output = fopen("results.bin", "wb");
    unsigned int index, value;
    unsigned char buffer[96];
    if (!output)
        return 90;
    if (!strcmp(mode, "--direct-whitespace")) {
        for (index = 0; index != 256; ++index) {
            value = is_space_byte((signed char)index);
            fwrite(&value, 4, 1, output);
        }
    } else {
        for (index = 0; index != 6; ++index) {
            memset(buffer, 0xa5, sizeof(buffer));
            value = copy_basename(paths[index], (char *)buffer + 8);
            fwrite(&value, 4, 1, output);
            fwrite(buffer, 1, sizeof(buffer), output);
        }
    }
    return fclose(output) ? 91 : 0;
}

int main(int argc, char **argv, char **environment)
{
    if (argc == 2 && (!strcmp(argv[1], "--direct-whitespace") ||
                      !strcmp(argv[1], "--direct-basename")))
        return direct_helper(argv[1]);
    return resource_main(argc, argv, environment);
}
