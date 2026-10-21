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
    if ((unsigned int)descriptor >= (unsigned int)_nhandle ||
            !(_osfile[descriptor] & 1) || _osfhnd[descriptor] == -1) {
        errno = EBADF;
        _doserrno = 0;
        return -1;
    }
    switch (descriptor) {
    case 0: SetStdHandle(STD_INPUT_HANDLE, 0); break;
    case 1: SetStdHandle(STD_OUTPUT_HANDLE, 0); break;
    case 2: SetStdHandle(STD_ERROR_HANDLE, 0); break;
    }
    _osfhnd[descriptor] = -1;
    return 0;
}
long __cdecl _get_osfhandle(int descriptor)
{
    if ((unsigned int)descriptor < (unsigned int)_nhandle && (_osfile[descriptor] & 1))
        return _osfhnd[descriptor];
    errno = EBADF;
    _doserrno = 0;
    return -1;
}
int __cdecl _close(int descriptor)
{
    DWORD error = 0;
    long other = -1, standard = -2;
    if ((unsigned int)descriptor >= (unsigned int)_nhandle) {
        errno = EBADF;
        _doserrno = 0;
        return -1;
    }
    if (descriptor == 1 || descriptor == 2) {
        other = _get_osfhandle(2);
        standard = _get_osfhandle(1);
    }
    if (other != standard && !CloseHandle((HANDLE)_get_osfhandle(descriptor)))
        error = GetLastError();
    _free_osfhnd(descriptor);
    if (error) {
        _dosmaperr(error);
        return -1;
    }
    _osfile[descriptor] = 0;
    return 0;
}
int __cdecl _read(int descriptor, void *buffer, unsigned int count)
{
    unsigned char *next = buffer, *output, *end;
    unsigned int done = 0;
    unsigned char lookahead;
    DWORD received, error;
    extern unsigned char _pipech[];

    if ((unsigned int)descriptor >= (unsigned int)_nhandle) {
        errno = EBADF;
        _doserrno = 0;
        return -1;
    }
    if (!count || (_osfile[descriptor] & 2))
        return 0;
    if ((_osfile[descriptor] & 0x48) && _pipech[descriptor] != '\n') {
        *next++ = _pipech[descriptor];
        --count;
        done = 1;
        _pipech[descriptor] = '\n';
    }
    if (!ReadFile((HANDLE)_osfhnd[descriptor], next, count, &received, 0)) {
        error = GetLastError();
        if (error == ERROR_ACCESS_DENIED) {
            errno = EBADF;
            _doserrno = error;
        } else if (error == ERROR_BROKEN_PIPE)
            return 0;
        else
            _dosmaperr(error);
        return -1;
    }
    done += received;
    if (!(_osfile[descriptor] & 0x80))
        return done;
    next = buffer;
    if (*next == '\n')
        _osfile[descriptor] |= 4;
    else
        _osfile[descriptor] &= ~4;
    output = next;
    end = next + done;
    while (next < end) {
        if (*next == 26) {
            if (!(_osfile[descriptor] & 0x40))
                _osfile[descriptor] |= 2;
            break;
        }
        if (*next != '\r')
            *output++ = *next++;
        else if (next < end - 1) {
            if (next[1] == '\n') {
                next += 2;
                *output++ = '\n';
            } else
                *output++ = *next++;
        } else {
            error = 0;
            ++next;
            if (!ReadFile((HANDLE)_osfhnd[descriptor], &lookahead, 1, &received, 0))
                error = GetLastError();
            if (error || !received)
                *output++ = '\r';
            else if (_osfile[descriptor] & 0x48) {
                if (lookahead == '\n')
                    *output++ = '\n';
                else {
                    *output++ = '\r';
                    _pipech[descriptor] = lookahead;
                }
            } else if (output == buffer && lookahead == '\n')
                *output++ = '\n';
            else {
                _lseek(descriptor, -1, 1);
                if (lookahead != '\n')
                    *output++ = '\r';
            }
        }
    }
    return output - (unsigned char *)buffer;
}
int __cdecl _chsize(int descriptor, long length)
{
    __asm int 3
    return -1;
}

int __cdecl _write(int descriptor, const void *buffer, unsigned int count)
{
    const unsigned char *next = buffer;
    unsigned char translated[1025], *output;
    unsigned int newlines = 0, done = 0, amount;
    DWORD written, error = 0;

    if ((unsigned int)descriptor >= (unsigned int)_nhandle) {
        errno = EBADF;
        _doserrno = 0;
        return -1;
    }
    if (!count)
        return 0;
    if (_osfile[descriptor] & 0x20)
        _lseek(descriptor, 0, 2);
    if (_osfile[descriptor] & 0x80) {
        while ((unsigned int)(next - (const unsigned char *)buffer) < count) {
            output = translated;
            do {
                if (*next == '\n') {
                    ++newlines;
                    *output++ = '\r';
                }
                *output++ = *next++;
            } while ((unsigned int)(next - (const unsigned char *)buffer) < count &&
                     output - translated < 1024);
            amount = output - translated;
            if (!WriteFile((HANDLE)_osfhnd[descriptor], translated, amount, &written, 0)) {
                error = GetLastError();
                break;
            }
            done += written;
            if (written < amount)
                break;
        }
    } else if (WriteFile((HANDLE)_osfhnd[descriptor], buffer, count, &written, 0))
        done = written;
    else
        error = GetLastError();
    if (done)
        return done - newlines;
    if (error) {
        if (error == ERROR_ACCESS_DENIED) {
            errno = EBADF;
            _doserrno = error;
        } else
            _dosmaperr(error);
        return -1;
    }
    if ((_osfile[descriptor] & 0x40) && *(const unsigned char *)buffer == 26)
        return 0;
    errno = ENOSPC;
    _doserrno = 0;
    return -1;
}
