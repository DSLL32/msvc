/* CRT differences in the backend executable. */
#include "c2.h"
#include <stdarg.h>
#include <io.h>
#include <errno.h>
#include <recon.h>
extern unsigned char _osfile[];
extern int _umaskval;
extern int _alloc_osfhnd(void);
extern int _set_osfhnd(int, long);
extern void _dosmaperr(unsigned long);

/* Original [00071d50,00071e90). This earlier CRT accounts for translated
   CR/LF bytes in buffered text streams, including a partially filled buffer
   at EOF. Its only callee is the low-level seek routine. */
long ftell(FILE *stream)
{
    int fd = stream->_file, flags;
    long position, offset, buffered;
    char *p, *end;
    if (stream->_cnt < 0) stream->_cnt = 0;
    position = _lseek(fd, 0, SEEK_CUR);
    if (position < 0) return -1;
    flags = stream->_flag;
    if (!(flags & 0x108)) return position - stream->_cnt;
    offset = stream->_ptr - stream->_base;
    if (flags & 3) {
        if (_osfile[fd] & 0x80) {
            for (p = stream->_base; p < stream->_ptr; ++p)
                if (*p == '\n') ++offset;
        }
    } else if (!(flags & 0x80)) {
        errno = 22;
        return -1;
    }
    if (!position) return offset;
    if (flags & 1) {
        if (!stream->_cnt) offset = 0;
        else {
            buffered = stream->_cnt + (stream->_ptr - stream->_base);
            if (_osfile[fd] & 0x80) {
                if (_lseek(fd, 0, SEEK_END) == position) {
                    end = stream->_base + buffered;
                    for (p = stream->_base; p < end; ++p)
                        if (*p == '\n') ++buffered;
                    if (stream->_flag & 0x2000) ++buffered;
                } else {
                    _lseek(fd, position, SEEK_SET);
                    buffered = stream->_bufsiz;
                    if (_osfile[fd] & 4) ++buffered;
                }
            }
            position -= buffered;
        }
    }
    return offset + position;
}

/* Original [00075ff0,000763e0). Same earlier CRT variant found in CL. */
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

extern void _getbuf(FILE *);
/* Original [00072ec0,00072fa0), same earlier CRT body as CL. */
int _filbuf(FILE *stream)
{
    int count;
    if (!(stream->_flag & 0x83) || (stream->_flag & 0x40)) return EOF;
    if (stream->_flag & 2) { stream->_flag |= 0x20; return EOF; }
    stream->_flag |= 1;
    if (!(stream->_flag & 0x10c)) _getbuf(stream);
    else stream->_ptr = stream->_base;
    count = _read(stream->_file, stream->_base, stream->_bufsiz);
    stream->_cnt = count;
    if (!count || count == -1) {
        stream->_flag |= count == 0 ? 0x10 : 0x20;
        stream->_cnt = 0;
        return EOF;
    }
    if (!(stream->_flag & 0x82) && (_osfile[stream->_file] & 0x82) == 0x82)
        stream->_flag |= 0x2000;
    if (stream->_bufsiz == 512 && (stream->_flag & 8)) stream->_bufsiz = 4096;
    --stream->_cnt;
    return (unsigned char)*stream->_ptr++;
}

/* Original [00074320,00074360), the earlier CRT buffer flags used by CL. */
void _freebuf(FILE *stream)
{
    if ((stream->_flag & 0x83) && (stream->_flag & 8)) {
        free(stream->_base);
        stream->_flag &= ~8;
        stream->_ptr = stream->_base = NULL;
        stream->_cnt = 0;
    }
}

extern int _flush(FILE *stream);
/* Original [00071f00,00071fa0), before the later CRT seek fast paths. */
int fseek(FILE *stream, long offset, int origin)
{
    if (!(stream->_flag & 0x83) ||
        (origin != SEEK_SET && origin != SEEK_CUR && origin != SEEK_END)) {
        errno = 22;
        return -1;
    }
    stream->_flag &= ~0x10;
    if (origin == SEEK_CUR) {
        offset += ftell(stream);
        origin = SEEK_SET;
    }
    _flush(stream);
    if (stream->_flag & 0x80) stream->_flag &= ~3;
    else if ((stream->_flag & 9) == 9) stream->_bufsiz = 512;
    return _lseek(stream->_file, offset, origin) == -1 ? -1 : 0;
}
