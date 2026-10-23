/* Explicit original/native CRT probe; never a fixture dependency. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
extern char * __cdecl _find(const char *);
#include <recon.h>

static char *first_result;

static void observe(const char *label, const char *pattern)
{
    char *name;
    int count;
    for (count=0; count<32; ++count) {
        errno=123;
        SetLastError(0x1122);
        name=_find(count?NULL:pattern);
        if (name && !first_result) first_result=name;
        printf("%s %d %s errno=%d error=%lu same=%d\n", label, count,
            name?name:"<NULL>", errno, GetLastError(),
            name?name==first_result:-1);
        if (!name) break;
    }
}

int main(void)
{
    observe("before", NULL);
    observe("missing", "absent?.obj");
    observe("one", "unit1.obj");
    observe("wild", "unit?.obj");
    observe("case", "UNIT?.OBJ");
    observe("star", "unit*.obj");
    observe("short", "unit.obj");
    observe("dotstar", "plain.*");
    observe("subdir", "objects\\unit?.obj");
    observe("badpath", "missing\\unit?.obj");
    errno=123;
    SetLastError(0x1122);
    printf("restart %s\n", _find("unit?.obj"));
    printf("replace %s\n", _find("unit1.obj"));
    observe("after-replace", NULL);
    return 0;
}
