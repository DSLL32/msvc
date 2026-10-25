/* Quiet file operations. Original QLSeek424e4c..424e8a. */
#include <errno.h>
#include <io.h>
#include <string.h>
#include "link.h"
#include <recon.h>

long QLSeek( int file, long position, int start, char *name )
{
    long result;

    CheckBreak();
    result = lseek( file, position, start );
    if( result == -1 && name != 0 ) {
        LnkMsg( 0x330a, "12", name, strerror( errno ) );
    }
    return result;
}
