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

#ifndef WLINK_DIAGNOSTIC_H
#define WLINK_DIAGNOSTIC_H
#include <stdarg.h>
#include "cmdline.h"

#pragma pack(1)
typedef struct msg_address { unsigned long off; unsigned short seg; } msg_address;
typedef union msg_arg {
    symbol *symb;
    char *string;
    char c;
    unsigned short int_16;
    unsigned long int_32;
    msg_address address;
} MSG_ARG;
typedef struct msg_arg_list {
    int index;
    MSG_ARG arg[3];
} MSG_ARG_LIST;
#pragma pack()
typedef char old_message_argument_is_6[(sizeof(MSG_ARG) == 6) ? 1 : -1];
typedef char old_argument_list_is_22[(sizeof(MSG_ARG_LIST) == 22) ? 1 : -1];
extern unsigned char MsgFlags[150];
extern unsigned char TestBit( void *, unsigned );
extern void LocateFile( unsigned );
extern void Locator( char *, char *, unsigned );
extern void WriteBox( unsigned );
extern void WriteMap( char *, ... );
extern void DoWriteMap( char *, va_list * );
extern unsigned MapPrint( char *, va_list * );
extern void Msg_Write_Map( int, ... );
extern void WriteFormat( int, char *, ... );
extern unsigned FmtStr( char *, unsigned, char *, ... );
extern unsigned DoFmtStr( char *, unsigned, char *, va_list * );
extern int UseArgInfo( void );
extern void IncremIndex( void );
extern int __is_mangled_internal( const char *, unsigned );
extern unsigned __demangle_l( const char *, unsigned, char *, unsigned );
extern unsigned long FindLinearAddr( msg_address * );
extern unsigned MakeExeName( char *, unsigned );
extern void FileOrder( char *, int );
extern void Msg_Do_Put_Args( char *, MSG_ARG_LIST *, char *, ... );
extern void Msg_Put_Args( char *, MSG_ARG_LIST *, const char *, va_list * );
extern void Msg_Add_Args( MSG_ARG *, char, va_list * );
#include <recon.h>
#endif
