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

