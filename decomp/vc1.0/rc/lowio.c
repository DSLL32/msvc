/* Pending old CRT handle and transfer routines. */
#include "lowio.h"
int __cdecl _alloc_osfhnd(void)
{
    int descriptor;
    for (descriptor = 0; descriptor < _nhandle; ++descriptor) {
        if (!(_osfile[descriptor] & 1)) {
            _osfhnd[descriptor] = -1;
            return descriptor;
        }
    }
    return -1;
}
int __cdecl _set_osfhnd(int descriptor, long handle)
{
    if ((unsigned int)descriptor >= (unsigned int)_nhandle || _osfhnd[descriptor] != -1) {
        errno = EBADF;
        _doserrno = 0;
        return -1;
    }
    switch (descriptor) {
    case 0: SetStdHandle(STD_INPUT_HANDLE, (HANDLE)handle); break;
    case 1: SetStdHandle(STD_OUTPUT_HANDLE, (HANDLE)handle); break;
    case 2: SetStdHandle(STD_ERROR_HANDLE, (HANDLE)handle); break;
    }
    _osfhnd[descriptor] = handle;
    return 0;
}
int __cdecl _free_osfhnd(int descriptor)
{
    __asm int 3
    return -1;
}
long __cdecl _get_osfhandle(int descriptor)
{
    __asm int 3
    return -1;
}
int __cdecl _close(int descriptor)
{
    __asm int 3
    return -1;
}
int __cdecl _read(int descriptor, void *buffer, unsigned int count)
{
    __asm int 3
    return -1;
}
int __cdecl _chsize(int descriptor, long length)
{
    __asm int 3
    return -1;
}

int __cdecl _write(int descriptor, const void *buffer, unsigned int count)
{
    __asm int 3
    return -1;
}
