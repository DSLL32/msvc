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
/* Quiet file operations. Original QLSeek424e4c..424e8a. */
/* Only the reached target-errno ENOENT diagnostic uses this shared profile. */
#define RECON_WATCOM_STRERROR_ENOENT
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <io.h>
#include <process.h>
#include <conio.h>
#include <string.h>
#include "cmdline.h"
#include <recon.h>

#ifdef __WATCOMC__
#pragma intrinsic(memcpy,strcpy)
#endif

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

static unsigned char CaughtBreak; /* original441f90; TrapBreak sets it. */

void TrapBreak( int value )
{
    value = value;
    CaughtBreak = 1;
}

void CheckBreak( void )
{
    if( CaughtBreak ) {
        CaughtBreak = 0;             /* prevent recursion */
        LnkMsg( 0x340d, 0 );
    }
}

static int OpenFiles; /* original4502bc */

void LnkFilesInit( void )
{
    OpenFiles = 0;
    setmode( fileno( stdin ), O_BINARY );
    setmode( fileno( stdout ), O_BINARY );
}

/* The target empty SetBreak entry shares CheckBreak's final C3 at4250f0.
 * Its trace entry alone is explicitly seeded by this case: a breakpoint there
 * would also see CheckBreak's epilogue. No CheckBreak call is invented. */
void SetBreak( void )
{
}

/* The original is a five-byte tail jump to the public CRT boundary. */
void GetCmdLine( char *buffer )
{
    getcmd( buffer );
}

int QIsADevice( int handle )
{
    return isatty( handle );
}

unsigned char QSysHelp( char **command )
{
    command = command;
    return 0;
}

int QObjOpen( char *name )
{
    return NSOpen( name, O_RDONLY );
}

static int LastResult;              /* original4502c0 */

int NSOpen( char *name, unsigned mode )
{
    int handle;

    handle = DoOpen( name, mode, 0 );
    LastResult = handle;
    if( handle != -1 ) return handle;
    return -1;
}

int DoOpen( char *name, unsigned mode, unsigned char executable )
{
    int handle;

    executable = executable;
    CheckBreak();
    mode |= O_BINARY;
    for( ;; ) {
        if( OpenFiles >= 12 ) CleanCachedHandles();
        handle = open( name, mode, 0600 );
        if( handle != -1 ) {
            ++OpenFiles;
            break;
        }
        if( errno != EMFILE ) break;
        if( !CleanCachedHandles() ) break;
    }
    return handle;
}

int QMakeFileName( char **pos, char *name, char *fname )
{
    char *pathptr;
    unsigned path_len;
    char *file_ptr;

    pathptr = *pos;
    if( pathptr == 0 ) return 0;
    while( *pathptr != '\0' ) {
        if( *pathptr == ';' ) *pos = ++pathptr;
        for( ;; ) {
            if( *pathptr == '\0' || *pathptr == ';' ) break;
            ++pathptr;
        }
        path_len = pathptr - *pos;
        if( path_len != 0 ) {
            memcpy( fname, *pos, path_len );
            file_ptr = fname + path_len;
            switch( file_ptr[-1] ) {
            case ':':
            case '\\':
            case '/':
                break;
            default:
                *file_ptr++ = '\\';
                break;
            }
            strcpy( file_ptr, name );
            *pos = pathptr;
            return 1;
        }
        *pos = pathptr;
    }
    return 0;
}

unsigned QWrite( int file, void *buffer, unsigned length, char *name )
{
    char message[128];
    int written;

    if( length == 0 ) return 0;
    CheckBreak();
    written = write( file, buffer, length );
    if( name != 0 ) {
        if( written == -1 ) {
            LnkMsg( 0x330a, "12", name, strerror(errno) );
        } else if( written != length ) {
            Msg_Get( 0xf9, message );
            LnkMsg( 0x140a, "12", name, message );
        }
    }
    return written;
}

void QWriteNL( int file, char *name )
{
    QWrite( file, "\r\n", 2, name );
}

unsigned char WaitForKey( void )
{
    return getch();
}

unsigned long QFileSize( int file )
{
    unsigned long current, size;
    current = QPos( file );
    size = lseek( file, 0, SEEK_END );
    lseek( file, current, SEEK_SET );
    return size;
}

long QPos( int file )
{
    CheckBreak();
    return lseek( file, 0, SEEK_CUR );
}

unsigned QRead( int file, void *buffer, unsigned length, char *name )
{
    int result;
    CheckBreak();
    result = read( file, buffer, length );
    if( result == -1 ) LnkMsg( 0x330a, "12", name, strerror( errno ) );
    return result;
}

void QClose( int file, char *name )
{
    int result;

    CheckBreak();
    result = close( file );
    --OpenFiles;
    if( result == -1 ) LnkMsg( 0x330a, "12", name, strerror(errno) );
}

int QOpenRW( char *name )
{
    int handle;

    handle = DoOpen( name, O_RDWR | O_CREAT | O_TRUNC, 0 );
    if( handle != -1 ) return handle;
    LnkMsg( 0x3408, "12", name, strerror(errno) );
    return -1;
}

unsigned char QHavePath( char *name )
{
    return name[0] == '\\' || name[0] == '/' || name[1] == ':';
}

long QSeek( int file, long position, char *name )
{
    return QLSeek( file, position, SEEK_SET, name );
}

int ExeCreate( char *name )
{
    int handle;

    handle = DoOpen( name, O_RDWR | O_CREAT | O_TRUNC, 1 );
    if( handle != -1 ) return handle;
    LnkMsg( 0x3408, "12", name, strerror( errno ) );
    return -1;
}

int QOpenR( char *name )
{
    int handle;
    handle = DoOpen( name, O_RDONLY, 0 );
    if( handle != -1 ) return handle;
    LnkMsg( 0x3408, "12", name, strerror(errno) );
    return -1;
}

int TempFileOpen( char *name )
{
    return NSOpen( name, O_RDWR );
}
void QDelete( char *name )
{
    if( remove( name ) == -1 && errno != ENOENT )
        LnkMsg( 0x330a, "12", name, strerror( errno ) );
}

unsigned char QReadStr( int file, char *buffer, unsigned size, char *name )
{
    unsigned char eof;
    char ch;
    eof = 0;
    while( --size > 0 ) {
        if( QRead( file, &ch, 1, name ) == 0 ) {
            eof = 1;
            break;
        }
        if( ch != '\r' ) *buffer++ = ch;
        if( ch == '\n' ) break;
    }
    *buffer = 0;
    return eof;
}

int ExeOpen( char *name )
{
    int handle;

    handle = DoOpen( name, O_RDWR, 1 );
    if( handle != -1 ) return handle;
    LnkMsg( 0x3408, "12", name, strerror( errno ) );
    return -1;
}

void PrintIOError( unsigned error, const char *types, char *name )
{
    LnkMsg( error, types, name, strerror( errno ) );
}
