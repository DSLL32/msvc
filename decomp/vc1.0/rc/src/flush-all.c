/* Older fixed-stream-table flushing. The public wrappers and private walker
 * stay together so calls do not use the newer private FFLUSH.OBJ neighbor. */
#include "lowio.h"
#include <stdio.h>
extern FILE *_lastiob;
int __cdecl _flush(FILE *);
int __cdecl flsall(int);
int __cdecl _commit(int);

/* Original 0x0100f760..0x0100f7a7. */
int __cdecl fflush(FILE *stream)
{
    if (!stream)
        return flsall(0);
    if (_flush(stream))
        return EOF;
    if (stream->_flag & 0x4000)
        return _commit(stream->_file) == 0 ? 0 : EOF;
    return 0;
}
/* Original 0x0100f820..0x0100f82b. */
int __cdecl _flushall(void)
{
    return flsall(1);
}
/* Original 0x0100f830..0x0100f8a7. The last-stream boundary is inclusive. */
int __cdecl flsall(int all)
{
    FILE *stream;
    int count = 0, status = 0;
    for (stream = _iob; stream <= _lastiob; ++stream) {
        if (!(stream->_flag & 0x83))
            continue;
        if (all == 1) {
            if (fflush(stream) != EOF)
                ++count;
        } else if (!all && (stream->_flag & _IOWRT)) {
            if (fflush(stream) == EOF)
                status = EOF;
        }
    }
    return all == 1 ? count : status;
}

/* Named pending graph vertex, original 0x0100f8e0. */
int __cdecl _commit(int descriptor)
{
    __asm int 3
    return -1;
}
