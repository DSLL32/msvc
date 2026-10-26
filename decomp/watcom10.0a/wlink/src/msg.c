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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "diagnostic.h"
#include <recon.h>

#ifdef __WATCOMC__
#pragma intrinsic(strlen,memcpy)
#endif

static char *LocFile;          /* original441ed8 */
static char *LocMem;           /* original441edc */
static unsigned LocRec;        /* original441ee0 */
static MSG_ARG_LIST MsgArgInfo = { -1, { { 0 } } }; /* original441ee8 */
unsigned long MaxErrors;       /* original4501a4 */

void LnkMsg( unsigned num, const char *types, ... )
{
    va_list args;
    int which_file;
    unsigned len, class;
    unsigned char waserror;
    char rc_buff[128];
    char buff[256];

    if( !TestBit( MsgFlags, num & 0xff ) ) return;
    LocateFile( (unsigned short)num );
    len = 0;
    waserror = 0;
    class = num & 0x700;
    if( class > 0x100 ) {
        if( class == 0x200 ) Msg_Get( 0xeb, rc_buff );
        else {
            waserror = 1;
            LinkState |= 8;
            Msg_Get( 0xea, rc_buff );
        }
        len = FmtStr( buff, sizeof(buff) - len, rc_buff,
                      (num & 0xff) + (((class - 1) >> 8) * 1000) );
    } else if( class == 0x100 ) waserror = 1;
    which_file = 0;
    if( LocFile != 0 ) which_file += 1;
    if( LocMem != 0 ) which_file += 2;
    if( LocRec != 0 ) which_file += 4;
    if( which_file != 0 ) {
        if( Token.how == SYSTEM ) {
            Msg_Get( 0xec, rc_buff );
            which_file = 1;
        } else if( Token.how == ENVIRONMENT ) {
            Msg_Get( 0xed, rc_buff );
            which_file = 1;
        } else Msg_Get( 0xe2 + which_file, rc_buff );
        FileOrder( rc_buff, which_file );
        len += FmtStr( buff + len, sizeof(buff) - len, rc_buff );
        if( (num & 0x800) && Token.how != SYSTEM && Token.how != ENVIRONMENT ) {
            Msg_Get( 0xee, rc_buff );
            Msg_Do_Put_Args( rc_buff, &MsgArgInfo, "d", Token.line );
            len += FmtStr( buff + len, sizeof(buff) - len, rc_buff );
        }
        LocFile = 0;
        LocMem = 0;
        LocRec = 0;
    }
    va_start( args, types );
    Msg_Get( num & 0xff, rc_buff );
    Msg_Put_Args( rc_buff, &MsgArgInfo, types, &args );
    len += FmtStr( buff + len, sizeof(buff) - len, rc_buff );
    va_end( args );
    if( num & 0x1000 ) {
        if( !(LinkFlags & 0x100) ) {
            WLPrtBanner();
            WriteStdOut( buff, len );
        } else if( class != 0 ) WriteStdOut( buff, len );
    }
    if( (num & 0x2000) && MapFile != -1 ) {
        BufWrite( buff, len );
        WriteMapNL( 1 );
    }
    if( class == 0x400 ) Suicide();
    if( waserror && (LinkFlags & 0x40000UL) ) {
        --MaxErrors;
        if( MaxErrors == 0 ) LnkMsg( 0x3489, 0 );
    }
}

unsigned RecNum;     /* original450110 */

void LocateFile( unsigned num )
{
    unsigned rec;

    if( num & 0x4000 ) {
        rec = (num & 0x8000) ? RecNum : 0;
        if( CurrMod == 0 ) {
            if( CmdFile == 0 ) Locator( 0, 0, 0 );
            else Locator( CmdFile->name, 0, 0 );
        } else Locator( CurrMod->f.source->file->name, CurrMod->name, rec );
    }
}

unsigned FmtStr( char *buff, unsigned len, char *fmt, ... )
{
    va_list args;
    unsigned result;

    va_start( args, fmt );
    result = DoFmtStr( buff, len, fmt, &args );
    va_end( args );
    return result;
}

unsigned DoFmtStr( char *buff, unsigned len, char *src, va_list *args )
/****************************************************************************/
/* quick vsprintf routine                                           */
/* assumptions - format string does not end in '%'                  */
/*             - only use of '%' is as follows                      */
/*                  %s  : string                                    */
/*                  %tn : n character string (%ns)                  */
/*                  %c  : character                                 */
/*                  %x  : 4 digit hex number (%4x)                  */
/*                  %h  : 8 digit hex number (%8x)                  */
/*                  %d  : decimal                                   */
/*                  %l  : long decimal                              */
/*                  %a  : address   ( %x:%x or 32 bit, depends on format) */
/*                  %S  : symbol name                               */
/*                  %f  : an executable format name                 */
/********************************************************************/
{
    char            ch;
    char            *dest;
    char            *str;
    unsigned short     num;
    unsigned long     num2;
    unsigned        size;
    msg_address     addr;
    unsigned int    i;
    static char     hexchar[] = "0123456789abcdef";
    int             temp;

    /* The1994 formatter consumes packed addresses by value and stops at
     * len0, unlike the later pointer/len1 version. */
    dest = buff;
    for(;;) {
        ch = *src++;
        if( ch == '\0' || len == 0 ) break;
        if( ch != '%' ) {
            *dest++ = ch;
            len--;
        } else {
            ch = *src++;
            switch( ch ) {
            case 'S' :
                if( UseArgInfo() ) {
                    str = MsgArgInfo.arg[MsgArgInfo.index].symb->name;
                    IncremIndex();
                } else {
                    str = va_arg( *args, symbol * )->name;
                }
                if( !(LinkFlags & 0x80000UL) ) {
                    size = __demangle_l( str, 0, dest, len );
                    if( size > (len-1) ) size = len - 1;
                } else
                {
                    size = strlen( str );
                    if( size > len ) size = len;
                    memcpy( dest, str, size );
                }
                len -= size;
                dest += size;
                break;
            case 's' :
                if( UseArgInfo() ) {
                    str = MsgArgInfo.arg[MsgArgInfo.index].string;
                    IncremIndex();
                } else {
                    str = va_arg( *args, char * );
                }
                size = strlen( str );
                if( size > len ) size = len;
                memcpy( dest, str, size );
                len -= size;
                dest += size;
                break;
            case 't' :
                str = va_arg( *args, char * );
                num = *src++ - '0';
                num = num * 10 + *src++ - '0';
                if( num > len ) num = len;
                while( (*str != '\0') && (num > 0) ) {
                    *dest++ = *str++;
                    num--;
                }
                while( num > 0 ) {
                    *dest++ = ' ';
                    num--;
                }
                len -= num;
                break;
            case 'c' :
                *dest++ = va_arg( *args, int );
                len--;
                break;
            case 'x' :
                if( UseArgInfo() ) {
                    num = MsgArgInfo.arg[MsgArgInfo.index].int_16;
                    IncremIndex();
                } else {
                    num = va_arg( *args, unsigned );
                }
                if( len < 4 ) return( dest - buff );    //NOTE: premature return
                dest += 4;
                len -= 4;
                str = dest;
                for( i = 4; i > 0; i-- ) {
                    *--str = hexchar[num & 0x0f];
                    num >>= 4;
                }
                break;
            case 'h' :
                num2 = va_arg( *args, unsigned long );
                if( len < 8) return( dest - buff );     //NOTE: premature return
                dest += 8;
                len -= 8;
                str = dest;
                for( i = 8; i > 0; i-- ) {
                    *--str = hexchar[num2 & 0x0f];
                    num2 >>= 4;
                }
                break;
            case 'd' :
                if( len < 5 ) return( dest - buff );    // NOTE: premature return
                if( UseArgInfo() ) {
                    num = MsgArgInfo.arg[MsgArgInfo.index].int_16;
                    IncremIndex();
                } else {
                    num = va_arg( *args, unsigned );
                }
                utoa( num, dest, 10 );
                size = strlen( dest );
                dest += size;
                len -= size;
                break;
            case 'l' :
                if( len < 10 ) return( dest - buff );   //NOTE: premature return
                if( UseArgInfo() ) {
                    num2 = MsgArgInfo.arg[MsgArgInfo.index].int_32;
                    IncremIndex();
                } else {
                    num2 = va_arg( *args, unsigned long );
                }
                ultoa( num2, dest, 10 );
                size = strlen( dest );
                dest += size;
                len -= size;
                break;
            case 'a':
                if( UseArgInfo() ) {
                    addr = MsgArgInfo.arg[MsgArgInfo.index].address;
                    IncremIndex();
                } else {
                    addr = va_arg( *args, msg_address );
                }
                temp = MsgArgInfo.index;
                MsgArgInfo.index = -1;
                if( FmtData.type & 0x9c00 ) {
                    size = FmtStr( dest, len, "%h", addr.off );
                } else if( FmtData.type & 0x4000 ) {
                    size = FmtStr( dest, len, "%h", FindLinearAddr( &addr ) );
                } else if( FmtData.type & 0x100 ) {
                    if( addr.seg == 1 ) size = FmtStr( dest, len, "CODE:%h", addr.off );
                    else size = FmtStr( dest, len, "DATA:%h", addr.off );
                } else if( FmtData.type & 0xff16 ) {
                    size = FmtStr( dest, len, "%x:%h", addr.seg, addr.off );
                } else {
                    size = FmtStr( dest, len, "%x:%x", addr.seg, (unsigned short)addr.off );
                }
                dest += size;
                len -= size;
                MsgArgInfo.index = temp;
                break;
            case 'f':
                num = MakeExeName( dest, len );
                dest += num;
                len -= num;
                break;
            }
        }
    }
    MsgArgInfo.index = -1;
    *dest = '\0';
    return( dest - buff );
}

int UseArgInfo( void )
{
    return MsgArgInfo.index >= 0;
}

void IncremIndex( void )
{
    ++MsgArgInfo.index;
}

unsigned MakeExeName( char *buff, unsigned max )
{
    char rc_buff[128];
    unsigned short format;
    unsigned len, num;
    char *str;

    if( max <= 3 ) return 0;
    len = 1;
    *buff++ = 'a';
    if( FmtData.osname != 0 ) {
        str = FmtData.osname;
    } else {
        format = FmtData.type;
        for( ;; ) {
            num = binary_log( format );
            format &= ~(1U << num);
            if( format == 0 ) break;
        }
        Msg_Get( 0xfc + num, rc_buff );
        str = rc_buff;
    }
    if( tolower( (unsigned char)*str ) == 'a'
        || tolower( (unsigned char)*str ) == 'e'
        || tolower( (unsigned char)*str ) == 'i'
        || tolower( (unsigned char)*str ) == 'o'
        || tolower( (unsigned char)*str ) == 'u' ) {
        *buff++ = 'n';
        ++len;
    }
    *buff++ = ' ';
    num = strlen( str );
    len += num + 2;
    if( len > max ) return len - (num + 2);
    memcpy( buff, str, num );
    buff += num;
    *buff++ = ' ';
    Msg_Get( (FmtData.flags & 1) ? 0x10c : 0x10d, rc_buff );
    str = rc_buff;
    num = strlen( str );
    len += num;
    if( len > max ) return len - num;
    memcpy( buff, str, num + 1 );
    return len;
}

unsigned char SkipSymbol( symbol *sym )
{
    int art;

    if( (sym->info & 0x0100) && !(MapFlags & 2) ) return 1;
    art = __is_mangled_internal( sym->name, 0 );
    return !(MapFlags & 4) && art == 2;
}
