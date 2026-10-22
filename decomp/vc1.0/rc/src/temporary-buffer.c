/* Temporary standard-stream buffers in RC's older CRT. */
#include "driver.h"
#include <stdlib.h>
#include <io.h>
extern int _cflush;
void *_stdbuf[2];

/* Original 0x01010ca0..0x01010d3e; the original buffer is 512 bytes. */
int __cdecl _stbuf(FILE *stream)
{
    int index;
    if (!_isatty(stream->_file))
        return 0;
    if (stream == stdout)
        index = 0;
    else if (stream == stderr)
        index = 1;
    else
        return 0;
    ++_cflush;
    if (stream->_flag & 0x10c)
        return 0;
    if (!_stdbuf[index]) {
        _stdbuf[index] = malloc(512);
        if (!_stdbuf[index])
            return 0;
    }
    stream->_base = stream->_ptr = _stdbuf[index];
    stream->_bufsiz = stream->_cnt = 512;
    stream->_flag |= 0x1102;
    return 1;
}

int __cdecl _flush(FILE *);
/* Original 0x01010d40..0x01010d8d. */
void __cdecl _ftbuf(int used, FILE *stream)
{
    if (used) {
        if (stream->_flag & 0x1000) {
            _flush(stream);
            stream->_flag &= ~0x1100;
            stream->_bufsiz = 0;
            stream->_ptr = stream->_base = 0;
        }
    } else if (stream->_flag & 0x1000)
        _flush(stream);
}
