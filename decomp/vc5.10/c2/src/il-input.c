#include "c2.h"
#include <recon.h>

int c2_read_il_byte(void)
{
    /* The target expands getc instead of calling the locked CRT entry. */
    if (--c2_current_il->_cnt >= 0)
        return (unsigned char)*c2_current_il->_ptr++;
    return _filbuf(c2_current_il);
}
