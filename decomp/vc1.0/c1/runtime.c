/* Runtime entries whose linked bodies differ from shipped LIBC.LIB. */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>
#include <errno.h>
#include <io.h>
#include <fcntl.h>

extern int _flush(FILE *);
extern void _freebuf(FILE *);
extern int _cflush;

/* Original [0x529b0,0x52a6f). The executable predates LIBC's 0x400 flag. */
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
extern unsigned char _osfile[];
extern int _umaskval;
extern int _alloc_osfhnd(void);
extern int _set_osfhnd(int, long);
extern void _dosmaperr(unsigned long);

/* Original [0x50f50,0x51340). Identical to CL after relocation/call masking. */
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


/* Original [0x572d0,0x57310). */
void _freebuf(FILE *stream)
{
    if ((stream->_flag & 0x83) && (stream->_flag & 8)) {
        free(stream->_base);
        stream->_flag &= ~8;
        stream->_ptr = stream->_base = NULL;
        stream->_cnt = 0;
    }
}

/* Original 0x53f90..0x54026. */
int fseek(FILE *stream, long offset, int origin)
{
    if (!(stream->_flag & 0x83) || (origin != 0 && origin != 1 && origin != 2)) {
        errno = 22;
        return -1;
    }
    stream->_flag &= ~0x10;
    if (origin == 1) {
        offset += ftell(stream);
        origin = 0;
    }
    _flush(stream);
    if (stream->_flag & 0x80)
        stream->_flag &= ~3;
    else if ((stream->_flag & 1) && (stream->_flag & 8))
        stream->_bufsiz = 512;
    return _lseek(stream->_file, offset, origin) == -1 ? -1 : 0;
}

/* Original 0x59e20..0x59f7b. This version leaves the default PST/PDT
   settings when TZ is absent; the shipped library queries Windows instead. */
static char *saved_timezone;
void __tzset(void)
{
    char *zone = getenv("TZ");
    char *left, *right;
    int negative = 0;
    if (!zone || !*zone)
        return;
    if (saved_timezone) {
        left = zone;
        right = saved_timezone;
        while (*left && *left == *right) {
            ++left;
            ++right;
        }
        if (*left == *right)
            return;
    }
    free(saved_timezone);
    saved_timezone = _strdup(zone);
    strncpy(_tzname[0], zone, 3);
    zone += 3;
    if (*zone == '-') {
        negative = 1;
        ++zone;
    }
    _timezone = atol(zone) * 3600;
    while (*zone == '+' || (*zone >= '0' && *zone <= '9'))
        ++zone;
    if (*zone == ':') {
        ++zone;
        _timezone += atol(zone) * 60;
        while (*zone >= '0' && *zone <= '9')
            ++zone;
        if (*zone == ':') {
            ++zone;
            _timezone += atol(zone);
            while (*zone >= '0' && *zone <= '9')
                ++zone;
        }
    }
    if (negative)
        _timezone = -_timezone;
    _daylight = (signed char)*zone;
    if (_daylight)
        strncpy(_tzname[1], zone, 3);
    else
        _tzname[1][0] = 0;
}

extern int _days[];

/* Original 0x59f80..0x5a043. Historical US daylight-saving rules. */
int _isindst(struct tm *time)
{
    int year = time->tm_year;
    int month = time->tm_mon;
    int sunday;
    if (year < 67 || month < 3 || month > 9)
        return 0;
    if (month > 3 && month < 9)
        return 1;
    if (year > 86 && month == 3)
        sunday = _days[month] + 7;
    else
        sunday = _days[month + 1];
    if (!(year & 3))
        ++sunday;
    sunday -= (((year - 1) >> 2) + 365 * year + sunday - 0x63db) % 7;
    if (month == 3)
        return sunday < time->tm_yday ||
            (sunday == time->tm_yday && time->tm_hour >= 2);
    return sunday > time->tm_yday ||
        (sunday == time->tm_yday && time->tm_hour < 1);
}

/* Original 0x517e0..0x51914. This CRT variant counts buffered text-mode
   line endings and uses the original stream flags when seeking read buffers. */
long ftell(FILE *stream)
{
    int descriptor = stream->_file;
    int flags = stream->_flag;
    long position, buffered, used;
    char *cursor, *end;
    if (stream->_cnt < 0) stream->_cnt = 0;
    position = _lseek(descriptor, 0L, SEEK_CUR);
    if (position < 0) return -1L;
    if (!(flags & 0x108)) return position - stream->_cnt;
    used = stream->_ptr - stream->_base;
    buffered = used;
    if (flags & 3) {
        if (_osfile[descriptor] & 0x80)
            for (cursor = stream->_base; cursor < stream->_ptr; ++cursor)
                if (*cursor == '\n') ++buffered;
    } else if (!(flags & 0x80)) {
        errno = EINVAL;
        return -1L;
    }
    if (!position) return buffered;
    if (flags & 1) {
        if (!stream->_cnt) buffered = 0;
        else {
            used += stream->_cnt;
            if (_osfile[descriptor] & 0x80) {
                if (_lseek(descriptor, 0L, SEEK_END) == position) {
                    cursor = stream->_base;
                    end = cursor + used;
                    while (cursor < end) if (*cursor++ == '\n') ++used;
                    if (stream->_flag & 0x2000) ++used;
                } else {
                    _lseek(descriptor, position, SEEK_SET);
                    used = stream->_bufsiz;
                    if (_osfile[descriptor] & 4) ++used;
                }
            }
            position -= used;
        }
    }
    return position + buffered;
}
