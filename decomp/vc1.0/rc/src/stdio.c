/* Buffered transfer in the CRT linked into RC. */
#include "lowio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <recon.h>
#pragma intrinsic(memcpy)
int __cdecl _flush(FILE *);

/* Original 0x010123b0..0x01012524. A zero byte product returns count in
 * this old version; retain it rather than applying newer CRT behavior. */
size_t __cdecl fwrite(const void *buffer, size_t size, size_t count, FILE *stream)
{
    const char *next = buffer;
    unsigned int total = size * count, remaining = total, amount, block_size;
    int written;

    block_size = (stream->_flag & 0x10c) ? stream->_bufsiz : 512;
    while (remaining) {
        if ((stream->_flag & 0x108) && stream->_cnt) {
            amount = remaining < (unsigned int)stream->_cnt ? remaining : stream->_cnt;
            memcpy(stream->_ptr, next, amount);
            remaining -= amount;
            stream->_cnt -= amount;
            stream->_ptr += amount;
            next += amount;
        } else if (remaining >= block_size) {
            if ((stream->_flag & 0x108) && _flush(stream))
                return (total - remaining) / size;
            amount = remaining - remaining % block_size;
            written = _write(stream->_file, next, amount);
            if (written == -1) {
                stream->_flag |= _IOERR;
                return (total - remaining) / size;
            }
            remaining -= written;
            next += written;
            if ((unsigned int)written < amount) {
                stream->_flag |= _IOERR;
                return (total - remaining) / size;
            }
        } else {
            if (_flsbuf(*next, stream) == EOF)
                return (total - remaining) / size;
            ++next;
            --remaining;
            block_size = stream->_bufsiz > 0 ? stream->_bufsiz : 1;
        }
    }
    return count;
}

/* Original 0x01010000..0x0101004f. RC uses 512-byte buffers. */
extern int _cflush;
void __cdecl _getbuf(FILE *stream)
{
    ++_cflush;
    stream->_base = malloc(512);
    if (stream->_base) {
        stream->_flag |= _IOMYBUF;
        stream->_bufsiz = 512;
    } else {
        stream->_flag |= _IONBF;
        stream->_base = (char *)&stream->_charbuf;
        stream->_bufsiz = 2;
    }
    stream->_ptr = stream->_base;
    stream->_cnt = 0;
}

long __cdecl ftell(FILE *stream)
{
    int descriptor = stream->_file, flags = stream->_flag;
    long position, offset, buffered;
    char *cursor, *end;

    if (stream->_cnt < 0)
        stream->_cnt = 0;
    position = _lseek(descriptor, 0, SEEK_CUR);
    if (position < 0)
        return -1;
    if (!(flags & 0x108))
        return position - stream->_cnt;
    offset = stream->_ptr - stream->_base;
    if (flags & (_IOREAD | _IOWRT)) {
        if (_osfile[descriptor] & 0x80)
            for (cursor = stream->_base; cursor < stream->_ptr; ++cursor)
                if (*cursor == '\n')
                    ++offset;
    } else if (!(flags & _IORW)) {
        errno = EINVAL;
        return -1;
    }
    if (!position)
        return offset;
    if (flags & _IOREAD) {
        if (!stream->_cnt)
            offset = 0;
        else {
            buffered = stream->_cnt + (stream->_ptr - stream->_base);
            if (_osfile[descriptor] & 0x80) {
                if (_lseek(descriptor, 0, SEEK_END) == position) {
                    end = stream->_base + buffered;
                    for (cursor = stream->_base; cursor < end; ++cursor)
                        if (*cursor == '\n')
                            ++buffered;
                    if (flags & 0x2000)
                        ++buffered;
                } else {
                    _lseek(descriptor, position, SEEK_SET);
                    buffered = stream->_bufsiz;
                    if (_osfile[descriptor] & 4)
                        ++buffered;
                }
            }
            position -= buffered;
        }
    }
    return offset + position;
}
int __cdecl fseek(FILE *stream, long offset, int origin)
{
    if (!(stream->_flag & (_IOREAD | _IOWRT | _IORW)) ||
            (origin != SEEK_SET && origin != SEEK_CUR && origin != SEEK_END)) {
        errno = EINVAL;
        return -1;
    }
    stream->_flag &= ~_IOEOF;
    if (origin == SEEK_CUR) {
        offset += ftell(stream);
        origin = SEEK_SET;
    }
    _flush(stream);
    if (stream->_flag & _IORW)
        stream->_flag &= ~(_IOREAD | _IOWRT);
    return _lseek(stream->_file, offset, origin) == -1 ? -1 : 0;
}
size_t __cdecl fread(void *buffer, size_t size, size_t count, FILE *stream)
{
    char *next = buffer;
    unsigned int total = size * count, remaining = total, amount, block_size;
    int received;

    if (!total)
        return 0;
    block_size = (stream->_flag & 0x10c) ? stream->_bufsiz : 512;
    while (remaining) {
        if ((stream->_flag & 0x10c) && stream->_cnt) {
            amount = remaining < (unsigned int)stream->_cnt ? remaining : stream->_cnt;
            memcpy(next, stream->_ptr, amount);
            remaining -= amount;
            stream->_cnt -= amount;
            stream->_ptr += amount;
            next += amount;
        } else if (remaining >= block_size) {
            amount = remaining - remaining % block_size;
            received = _read(stream->_file, next, amount);
            if (!received || received == -1) {
                stream->_flag |= received ? _IOERR : _IOEOF;
                return (total - remaining) / size;
            }
            remaining -= received;
            next += received;
        } else {
            received = _filbuf(stream);
            if (received == EOF)
                return (total - remaining) / size;
            *next++ = (char)received;
            --remaining;
            block_size = stream->_bufsiz;
        }
    }
    return count;
}
int __cdecl _filbuf(FILE *stream)
{
    if (!(stream->_flag & (_IOREAD | _IOWRT | _IORW)) || (stream->_flag & _IOSTRG))
        return EOF;
    if (stream->_flag & _IOWRT) {
        stream->_flag |= _IOERR;
        return EOF;
    }
    stream->_flag |= _IOREAD;
    if (!(stream->_flag & 0x10c))
        _getbuf(stream);
    else
        stream->_ptr = stream->_base;
    stream->_cnt = _read(stream->_file, stream->_base, stream->_bufsiz);
    if (!stream->_cnt || stream->_cnt == -1) {
        stream->_flag |= stream->_cnt ? _IOERR : _IOEOF;
        stream->_cnt = 0;
        return EOF;
    }
    if (!(stream->_flag & (_IOWRT | _IORW)) && (_osfile[stream->_file] & 0x82) == 0x82)
        stream->_flag |= 0x2000;
    --stream->_cnt;
    return (unsigned char)*stream->_ptr++;
}

/* Original 0x0100fba0..0x0100fbd6. */
void __cdecl _freebuf(FILE *stream)
{
    if ((stream->_flag & (_IOREAD | _IOWRT | _IORW)) && (stream->_flag & _IOMYBUF)) {
        free(stream->_base);
        stream->_flag &= ~_IOMYBUF;
        stream->_ptr = stream->_base = 0;
        stream->_cnt = 0;
    }
}

/* Original 0x01013be0..0x01013c6b. String streams permit pushback only
 * when the byte already present matches; ordinary streams store the byte. */
int __cdecl ungetc(int character, FILE *stream)
{
    if (character == EOF)
        return EOF;
    if (!(stream->_flag & _IOREAD) &&
        (!(stream->_flag & _IORW) || (stream->_flag & _IOWRT)))
        return EOF;
    if (!stream->_base)
        _getbuf(stream);
    if (stream->_ptr == stream->_base) {
        if (stream->_cnt)
            return EOF;
        ++stream->_ptr;
    }
    if (stream->_flag & _IOSTRG) {
        if ((unsigned char)*--stream->_ptr != (unsigned char)character) {
            ++stream->_ptr;
            return EOF;
        }
    } else
        *--stream->_ptr = (char)character;
    ++stream->_cnt;
    stream->_flag &= ~_IOEOF;
    stream->_flag |= _IOREAD;
    return (unsigned char)character;
}
