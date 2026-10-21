/* Shared file opening in RC's CRT, original 0x01010640..0x01010a10. */
#include "lowio.h"
#include <stdarg.h>
#include <sys/stat.h>

int __cdecl _sopen(const char *path, int flags, int sharing, ...)
{
    SECURITY_ATTRIBUTES security;
    DWORD access, share, disposition, attributes, kind;
    HANDLE handle;
    unsigned char file_flags;
    int descriptor, permissions;
    long last;
    char character;
    va_list arguments;

    security.nLength = sizeof(security);
    security.lpSecurityDescriptor = 0;
    security.bInheritHandle = !(flags & _O_NOINHERIT);
    file_flags = (flags & _O_BINARY) ? 0 :
                 ((flags & _O_TEXT) || _fmode != _O_BINARY ? 0x80 : 0);
    switch (flags & 3) {
    case _O_RDONLY: access = GENERIC_READ; break;
    case _O_WRONLY: access = GENERIC_WRITE; break;
    case _O_RDWR: access = GENERIC_READ | GENERIC_WRITE; break;
    default:
        errno = EINVAL;
        _doserrno = 0;
        return -1;
    }
    switch (sharing) {
    case _SH_DENYRW: share = 0; break;
    case _SH_DENYWR: share = FILE_SHARE_READ; break;
    case _SH_DENYRD: share = FILE_SHARE_WRITE; break;
    case _SH_DENYNO: share = FILE_SHARE_READ | FILE_SHARE_WRITE; break;
    default:
        errno = EINVAL;
        _doserrno = 0;
        return -1;
    }
    switch (flags & (_O_CREAT | _O_TRUNC | _O_EXCL)) {
    case 0: disposition = OPEN_EXISTING; break;
    case _O_CREAT: disposition = OPEN_ALWAYS; break;
    case _O_TRUNC: disposition = TRUNCATE_EXISTING; break;
    case _O_CREAT | _O_TRUNC: disposition = CREATE_ALWAYS; break;
    case _O_CREAT | _O_EXCL: disposition = CREATE_NEW; break;
    default:
        errno = EINVAL;
        _doserrno = 0;
        return -1;
    }
    attributes = FILE_ATTRIBUTE_NORMAL;
    if (flags & _O_CREAT) {
        va_start(arguments, sharing);
        permissions = va_arg(arguments, int);
        va_end(arguments);
        if (!(permissions & ~_umaskval & _S_IWRITE))
            attributes = FILE_ATTRIBUTE_READONLY;
    }
    if (flags & _O_TEMPORARY) {
        attributes |= FILE_FLAG_DELETE_ON_CLOSE;
        access |= DELETE;
    }
    if (flags & 0x1000)
        attributes |= FILE_ATTRIBUTE_TEMPORARY;
    if (flags & _O_SEQUENTIAL)
        attributes |= FILE_FLAG_SEQUENTIAL_SCAN;
    else if (flags & _O_RANDOM)
        attributes |= FILE_FLAG_RANDOM_ACCESS;
    descriptor = _alloc_osfhnd();
    if (descriptor == -1) {
        errno = EMFILE;
        _doserrno = 0;
        return -1;
    }
    handle = CreateFileA(path, access, share, &security, disposition, attributes, 0);
    if (handle == INVALID_HANDLE_VALUE) {
        _dosmaperr(GetLastError());
        return -1;
    }
    kind = GetFileType(handle);
    if (!kind) {
        CloseHandle(handle);
        _dosmaperr(GetLastError());
        return -1;
    }
    if (kind == FILE_TYPE_CHAR)
        file_flags |= 0x40;
    else if (kind == FILE_TYPE_PIPE)
        file_flags |= 0x08;
    _set_osfhnd(descriptor, (long)handle);
    file_flags |= 1;
    _osfile[descriptor] = file_flags;
    if (!(file_flags & 0x48) && (file_flags & 0x80) && (flags & _O_RDWR)) {
        last = _lseek(descriptor, -1, 2);
        if (last == -1) {
            if (_doserrno != ERROR_NEGATIVE_SEEK) {
                _close(descriptor);
                return -1;
            }
        } else {
            if (_read(descriptor, &character, 1) == 1 && character == 26) {
                if (_chsize(descriptor, last) == -1) {
                    _close(descriptor);
                    return -1;
                }
            }
            if (_lseek(descriptor, 0, 0) == -1) {
                _close(descriptor);
                return -1;
            }
        }
    }
    if (!(file_flags & 0x48) && (flags & _O_APPEND))
        _osfile[descriptor] |= 0x20;
    return descriptor;
}
