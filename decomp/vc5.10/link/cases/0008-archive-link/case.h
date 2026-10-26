/* Genuine original-compiled input: distinct /Gy COMDATs with identical bodies,
 * an unused function, and a reference chain spanning three archive members. */
extern int __cdecl archive_mix(int);
int __cdecl folded_first(int value) { return value+11; }
int __cdecl folded_second(int value) { return value+11; }
int __cdecl unused_routine(int value) { return value-17; }
int __stdcall ArchiveEntry(void)
{
    return archive_mix(folded_first(3))+folded_second(3);
}
