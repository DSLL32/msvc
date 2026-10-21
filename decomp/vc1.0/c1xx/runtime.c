/* CRT variants predating the shipped LIBC bodies. */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>
#include <errno.h>
#include <io.h>
#include <fcntl.h>

extern unsigned char _osfile[];
extern int _umaskval;
extern int _alloc_osfhnd(void);
extern int _set_osfhnd(int, long);
extern void _dosmaperr(unsigned long);

/* Original [0x80a80,0x80e70). Identical to CL after relocation/call masking. */
int _sopen(const char *name, int mode, int sharing, ...)
{
    SECURITY_ATTRIBUTES sa;
    DWORD access_mode, share_mode, disposition, attributes, file_type;
    HANDLE handle;
    unsigned char flags;
    int fd, permission;
    long end;
    char last;
    va_list ap;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = !(mode & 0x80);
    if (mode & 0x8000) flags = 0;
    else if (mode & 0x4000) flags = 0x80;
    else flags = _fmode == 0x8000 ? 0 : 0x80;
    switch (mode & 3) {
    case 0: access_mode = GENERIC_READ; break;
    case 1: access_mode = GENERIC_WRITE; break;
    case 2: access_mode = GENERIC_READ | GENERIC_WRITE; break;
    default: errno = 22; _doserrno = 0; return -1;
    }
    switch (sharing) {
    case 0x10: share_mode = 0; break;
    case 0x20: share_mode = FILE_SHARE_READ; break;
    case 0x30: share_mode = FILE_SHARE_WRITE; break;
    case 0x40: share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE; break;
    default: errno = 22; _doserrno = 0; return -1;
    }
    switch (mode & 0x700) {
    case 0: case 0x400: disposition = OPEN_EXISTING; break;
    case 0x100: disposition = OPEN_ALWAYS; break;
    case 0x200: case 0x600: disposition = TRUNCATE_EXISTING; break;
    case 0x300: disposition = CREATE_ALWAYS; break;
    case 0x500: case 0x700: disposition = CREATE_NEW; break;
    default: errno = 22; _doserrno = 0; return -1;
    }
    attributes = FILE_ATTRIBUTE_NORMAL;
    if (mode & 0x100) {
        va_start(ap, sharing);
        permission = va_arg(ap, int);
        if (!(permission & ~_umaskval & 0x80)) attributes = FILE_ATTRIBUTE_READONLY;
    }
    if (mode & 0x40) { attributes |= FILE_FLAG_DELETE_ON_CLOSE; access_mode |= DELETE; }
    if (mode & 0x1000) attributes |= FILE_ATTRIBUTE_TEMPORARY;
    if (mode & 0x20) attributes |= FILE_FLAG_SEQUENTIAL_SCAN;
    else if (mode & 0x10) attributes |= FILE_FLAG_RANDOM_ACCESS;
    fd = _alloc_osfhnd();
    if (fd == -1) { errno = 24; _doserrno = 0; return -1; }
    handle = CreateFileA(name, access_mode, share_mode, &sa, disposition, attributes, NULL);
    if (handle == INVALID_HANDLE_VALUE) { _dosmaperr(GetLastError()); return -1; }
    file_type = GetFileType(handle);
    if (!file_type) { CloseHandle(handle); _dosmaperr(GetLastError()); return -1; }
    if (file_type == FILE_TYPE_CHAR) flags |= 0x40;
    else if (file_type == FILE_TYPE_PIPE) flags |= 8;
    _set_osfhnd(fd, (long)handle);
    _osfile[fd] = flags | 1;
    if (!(flags & 0x48) && (flags & 0x80) && (mode & 2)) {
        end = _lseek(fd, -1, SEEK_END);
        if (end == -1) {
            if (_doserrno != 131) { _close(fd); return -1; }
        } else {
            if (_read(fd, &last, 1) == 1 && last == 0x1a && _chsize(fd, end) == -1) {
                _close(fd); return -1;
            }
            if (_lseek(fd, 0, SEEK_SET) == -1) { _close(fd); return -1; }
        }
    }
    if (!(flags & 0x48) && (mode & 8)) _osfile[fd] |= 0x20;
    return fd;
}



extern int _flush(FILE *);
extern void _freebuf(FILE *);
extern int _cflush;

/* Original [0x82580,0x8263f). The executable predates LIBC's 0x400 flag. */
int setvbuf(FILE *stream, char *buffer, int mode, size_t size)
{
    int result = 0;
    if (mode != 4 && (!size || size > 0x7fffffffU || (mode != 0 && mode != 0x40)))
        return -1;
    size &= ~1U;
    _flush(stream);
    _freebuf(stream);
    stream->_flag &= 0xffffc6f3;
    if (mode & 4) {
        stream->_flag |= 4;
        buffer = (char *)&stream->_charbuf;
        size = 2;
    } else if (!buffer) {
        buffer = malloc(size);
        if (!buffer) { ++_cflush; return -1; }
        stream->_flag |= 8;
    } else stream->_flag |= 0x100;
    stream->_bufsiz = size;
    stream->_base = stream->_ptr = buffer;
    stream->_cnt = 0;
    return result;
}

/* Original [0x877c0,0x877f6). */
void _freebuf(FILE *stream)
{
    if ((stream->_flag & 0x83) && (stream->_flag & 8)) {
        free(stream->_base);
        stream->_flag &= ~8;
        stream->_ptr = stream->_base = NULL;
        stream->_cnt = 0;
    }
}
