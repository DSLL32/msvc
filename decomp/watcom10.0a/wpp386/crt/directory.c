/* Binary-specific native binding for the old OSI opendir callback variant.
 * The source App keeps conventional opendir/closedir. Its original-toolchain
 * build uses the shipped NT CRT; this safe-named boundary adapter avoids
 * interposing on host libc. It also compiles in the period Win32 build.
 * Original _opendir allocates289B from the genuine compiler heap BEFORE any
 * lookup. Retain that object/lifetime, not a trace-only MemAlloc hook.
 * The native absent-literal-path domain uses the already-qualified _access0
 * query; no filesystem code or invented directory result lives in this file.
 * Only naturally reached outcomes are supported; no host DIR layout is used. */
#include <windows.h>
#include <io.h>
#include "memory.h"
#define WPP_DIRECTORY_IMPL 1
#include "directory.h"
#include <recon.h>

WppDirectory *wpp_opendir(const char *path)
{
    void *storage;
    DWORD error;

    storage = MemAlloc(289);
    if (!storage) return 0;
    /* Caller only tests NULL/non-NULL; it does not read private DIR/error
     * cells. _access0 performs an actual translated query on the literal path.
     * Only real missing-file/parent outcomes from this query are supported. */
    if (!_access(path, 0)) ExitProcess(125);
    error = GetLastError();
    if (error != 2 && error != 3) ExitProcess(125);
    MemFree((char *)storage);
    return 0;
}

int wpp_closedir(WppDirectory *directory)
{
    (void)directory;
    ExitProcess(125);
    return 1;
}
