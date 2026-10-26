/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  DWARF line-program opcode generation used by the linker.
*
****************************************************************************/

#include "cmdline.h"
#include <recon.h>

#define DWLINE_BASE (-1)
#define DWLINE_RANGE 4
#define DWLINE_OPCODE_BASE 10
#define MAX_ADDR_INCR ((255 - DWLINE_OPCODE_BASE) / DWLINE_RANGE)

unsigned DWLineGen( long line_incr, long addr_incr, char *buf )
{
    unsigned opcode, size;
    char *end;
    long addr;

    size = 0;
    if( line_incr < DWLINE_BASE || line_incr > DWLINE_BASE + DWLINE_RANGE - 1 ) {
        buf[0] = 3;
        size = DW_LEB128( buf + 1, line_incr ) - buf;
        line_incr = 0;
    }
    if( addr_incr < 0 ) {
        buf[size] = 2;
        size = DW_LEB128( buf + size + 1, addr_incr ) - buf;
        addr_incr = 0;
    }
    if( addr_incr == 0 && line_incr == 0 ) {
        buf[size] = 1;
        return size + 1;
    }
    line_incr -= DWLINE_BASE;
    opcode = DWLINE_RANGE * (unsigned long)addr_incr;
    if( opcode < (unsigned long)addr_incr ) goto overflow;
    if( opcode + line_incr < opcode ) goto overflow;
    opcode += line_incr;
    if( opcode <= 255 - DWLINE_OPCODE_BASE ) {
        buf[size] = opcode + DWLINE_OPCODE_BASE;
        return size + 1;
    }
    if( addr_incr < 2 * MAX_ADDR_INCR ) {
        buf[size++] = 8;
        buf[size] = opcode - MAX_ADDR_INCR * DWLINE_RANGE + DWLINE_OPCODE_BASE;
        return size + 1;
    }
overflow:
    buf[size] = 2;
    if( line_incr == -DWLINE_BASE ) {
        opcode = 1;
    } else {
        addr = addr_incr % (MAX_ADDR_INCR - 1);
        addr_incr -= addr;
        opcode = line_incr + DWLINE_RANGE * addr + DWLINE_OPCODE_BASE;
    }
    end = DW_LEB128( buf + size + 1, addr_incr );
    *end++ = opcode;
    return end - buf;
}
