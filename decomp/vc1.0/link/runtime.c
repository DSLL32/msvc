/* Runtime differences from the shipped LIBC.LIB, reconstructed from LINK.EXE. */
#include "link.h"
extern int _flush(FILE *);
extern void _freebuf(FILE *);
extern int _cflush;
/* Original [00052330,000523f0). The executable predates LIBC's 0x400 flag. */
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
/* [00055690,000556d0) */
void _freebuf(FILE *stream)
{
    if ((stream->_flag & 0x83) && (stream->_flag & 8)) {
        free(stream->_base);
        stream->_flag &= ~8;
        stream->_ptr = stream->_base = NULL;
        stream->_cnt = 0;
    }
}

/* Original [000556d0,00055820): the older mode alphabet and stream flags. */
extern int _commode;
extern int _sopen(const char *, int, int, ...);
FILE *_openfile(const char *name, const char *mode, int sharing, FILE *stream)
{
    int flags = _commode, oflag, commit_seen = 0, valid = 1, handle;
    switch (*mode) {
    case 'a': oflag = 0x109; flags |= 2; break;
    case 'r': oflag = 0; flags |= 1; break;
    case 'w': oflag = 0x301; flags |= 2; break;
    default: return NULL;
    }
    ++mode;
    while (*mode && valid) {
        switch (*mode) {
        case '+':
            if (oflag & 2) valid = 0;
            else { oflag |= 2; oflag &= ~1; flags |= 0x80; flags &= ~3; }
            break;
        case 'b':
            if (oflag & 0xc000) valid = 0;
            else oflag |= 0x8000;
            break;
        case 't':
            if (oflag & 0xc000) valid = 0;
            else oflag |= 0x4000;
            break;
        case 'c':
            if (commit_seen) valid = 0;
            else { commit_seen = 1; flags |= 0x4000; }
            break;
        case 'n':
            if (commit_seen) valid = 0;
            else { commit_seen = 1; flags &= ~0x4000; }
            break;
        default: valid = 0; break;
        }
        ++mode;
    }
    handle = _sopen(name, oflag, sharing, 0x1a4);
    if (handle < 0) return NULL;
    ++_cflush;
    stream->_flag = flags;
    stream->_cnt = 0;
    stream->_ptr = stream->_base = NULL;
    stream->_tmpfname = NULL;
    stream->_file = handle;
    return stream;
}

#include <errno.h>
extern unsigned char _osfile[];
extern int _umaskval;
extern int _alloc_osfhnd(void);
extern int _set_osfhnd(int, long);
extern void _dosmaperr(unsigned long);

/* [00058120,000584d0). This older CRT does not support the later hint flags. */
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
/* [00053b30,00053c10). Older stream buffering flags. */
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

/* [00052970,00052ae0). Both locale handle and code page select the fast path. */
#undef toupper
extern unsigned long _lc_handle[];
extern unsigned int _lc_codepage;
int toupper(int c)
{
    char bytes[3];
    WCHAR wide[2], mapped[2];
    int count, lower;
    if (!_lc_handle[2] && !_lc_codepage) {
        lower = __mb_cur_max > 1 ? _isctype(c,2) : (_pctype[c] & 2);
        return lower ? c-0x20 : c;
    }
    if (c < 0x100) {
        lower = __mb_cur_max > 1 ? _isctype(c,2) : (_pctype[c] & 2);
        if (!lower) return c;
    }
    if (_pctype[(c & 0xff00) >> 8] & 0x8000) {
        bytes[0]=(char)(c >> 8); bytes[1]=(char)c; bytes[2]=0;
    } else { bytes[0]=(char)c; bytes[1]=0; }
    count=MultiByteToWideChar(_lc_codepage,1,bytes,-1,wide,2);
    if (!count) return c;
    count=LCMapStringW(_lc_handle[2],0x200,wide,count,mapped,2);
    if (!count) return c;
    count=WideCharToMultiByte(_lc_codepage,0x210,mapped,count,bytes,2,NULL,NULL);
    if (!count) return c;
    if (count==1) return (signed char)bytes[0];
    return ((signed char)bytes[1] << 8) | (signed char)bytes[0];
}

/* [00059510,00059680). The older locale/code-page fast-path test. */
#undef tolower
int tolower(int c)
{
    char bytes[3];
    WCHAR wide[2],mapped[2];
    int count,upper;
    if (!_lc_handle[2] && !_lc_codepage) {
        upper=__mb_cur_max>1?_isctype(c,1):(_pctype[c]&1);
        return upper?c+0x20:c;
    }
    if (c<0x100) {
        upper=__mb_cur_max>1?_isctype(c,1):(_pctype[c]&1);
        if (!upper) return c;
    }
    if (_pctype[(c&0xff00)>>8]&0x8000) {
        bytes[0]=(char)(c>>8); bytes[1]=(char)c; bytes[2]=0;
    } else { bytes[0]=(char)c; bytes[1]=0; }
    count=MultiByteToWideChar(_lc_codepage,1,bytes,-1,wide,2);
    if (!count) return c;
    count=LCMapStringW(_lc_handle[2],0x100,wide,count,mapped,2);
    if (!count) return c;
    count=WideCharToMultiByte(_lc_codepage,0x210,mapped,count,bytes,2,NULL,NULL);
    if (!count) return c;
    if (count==1) return (signed char)bytes[0];
    return ((signed char)bytes[1]<<8)|(signed char)bytes[0];
}

/* The executable predates the library parser's Win32 timezone fallback. */
#include <time.h>
char *timezone_previous;
void __tzset(void)
{
    char *zone=getenv("TZ"),*left,*right;
    int negative=0,difference;
    if (!zone || !*zone) return;
    if (timezone_previous) {
        left=zone; right=timezone_previous;
        do {
            difference=(unsigned char)*left-(unsigned char)*right;
            if (difference || !*left) break;
            ++left; ++right;
        } while (1);
        if (!difference) return;
    }
    free(timezone_previous);
    timezone_previous=_strdup(zone);
    strncpy(_tzname[0],zone,3);
    zone+=3;
    if (*zone=='-') { negative=1; ++zone; }
    _timezone=atol(zone)*3600;
    while (*zone=='+' || (*zone>='0' && *zone<='9')) ++zone;
    if (*zone==':') {
        ++zone;
        _timezone+=atol(zone)*60;
        while (*zone>='0' && *zone<='9') ++zone;
        if (*zone==':') {
            ++zone;
            _timezone+=atol(zone);
            while (*zone>='0' && *zone<='9') ++zone;
        }
    }
    if (negative) _timezone=-_timezone;
    _daylight=*zone;
    if (_daylight) strncpy(_tzname[1],zone,3);
    else _tzname[1][0]=0;
}

