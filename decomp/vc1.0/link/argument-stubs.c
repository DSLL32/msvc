/* Pending argument expansion/file-classification vertices. */
#include "arguments.h"
void expand_wildcards(const char *pattern)
{
    __asm { int 3 }
}
unsigned char is_library_file(const char *path,int file)
{
    __asm { int 3 }
    return 0;
}
