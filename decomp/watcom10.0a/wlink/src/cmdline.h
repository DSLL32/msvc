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
#ifndef WLINK_CMDLINE_H
#define WLINK_CMDLINE_H
#include "link.h"

#pragma pack(1)
/* Old format union/common record: only encountered fields are identified. */
typedef struct fmt_data {
    unsigned char platform_data[58];
    unsigned short type;
    unsigned short def_seg_flags;
    unsigned char common_unknown[4];
    unsigned long base;
    unsigned char common_tail[5];
    unsigned char flags;          /* bit0 dll, original4500b3 */
    char *osname;              /* old final common pointer, original4500b4 */
} fmt_data;

typedef struct tok {
    char *buff;
    char *this;
    char *next;
    unsigned len;
    unsigned short line;
    unsigned short where : 2;
    unsigned short how : 3;
    unsigned short thumb : 1;
    unsigned short locked : 1;
    unsigned short quoted : 1;
    unsigned short reserved : 8; /* complete the original16-bit flag unit */
} tok;

typedef struct cmdfilelist {
    struct cmdfilelist *prev;
    struct cmdfilelist *next;
    int file;
    char *name;
    tok token;
} cmdfilelist;

typedef struct parse_entry {
    char *keyword;
    unsigned char (*rtn)( void );
    unsigned short format;
} parse_entry;
#pragma pack()
typedef char old_format_is_80[(sizeof(fmt_data) == 80) ? 1 : -1];
typedef char old_token_is_20[(sizeof(tok) == 20) ? 1 : -1];
typedef char old_command_source_is_36[(sizeof(cmdfilelist) == 36) ? 1 : -1];
extern cmdfilelist *CmdFile;
extern fmt_data FmtData;
extern tok Token;
extern char *Name;
extern unsigned short CmdFlags;
extern unsigned char Extension, DBIFlag, MapFlags;
extern char *SymFileName;
extern section *CurrSect;
extern void **CurrFList;
extern path_entry *Path;
extern unsigned long LinkFlags;

extern void ResetCmdFile( void );
extern void GetCmdLine( char * );
extern void NewCommandSource( char *, char *, unsigned );
extern int QIsADevice( int );
extern unsigned char QSysHelp( char ** );
extern void Help( void );
extern void EatWhite( void );
extern void Crash( unsigned char );
extern void DisplayOptions( void );
extern void WriteGenHelp( void );
extern void WriteHelp( unsigned, unsigned, unsigned char );
extern void PressKey( void );
extern unsigned char WaitForKey( void );
extern void WriteMsg( char * );
extern void WLPrtBanner( void );
extern void WriteStdOut( char *, unsigned );
extern void QWriteNL( int, char * );
extern void Msg_Get( unsigned, char * );
extern int WResLoadString( HANDLE_INFO *, unsigned, char __far *, int );
extern int GetString( WResLangInfo *, HANDLE_INFO *, unsigned, char __far *, int );
extern unsigned QRead( int, void *, unsigned, char * );
extern unsigned char ProcOne( parse_entry *, unsigned, unsigned char );
extern unsigned char ProcDosHelp( void );
extern unsigned char ProcOS2Help( void );
extern unsigned char ProcWindowsHelp( void );
extern unsigned char ProcNTHelp( void );
extern unsigned char ProcPharHelp( void );
extern unsigned char ProcNovellHelp( void );
extern unsigned char ProcQNXHelp( void );
extern unsigned char ProcELFHelp( void );
extern int SearchPath( char * );
extern int QObjOpen( char * );
extern int NSOpen( char *, unsigned );
extern int DoOpen( char *, unsigned, unsigned char );
extern unsigned char CleanCachedHandles( void );
extern int QMakeFileName( char **, char *, char * );
extern void SetCommandFile( int, char * );
extern void DoCmdParse( void );
extern void Ignite( void );
extern void Suicide( void );
extern void HintFormat( unsigned short );
extern void CmdNovFini( void );
extern void CmdOvlFini( void );
extern void AddObjLib( char *, unsigned );
extern char *FileName( char *, unsigned, unsigned, int );
extern void CheckTraces( void );
extern void BurnUtils( void );
extern void PruneSystemList( void );
extern void NumberSections( void );
extern void DBIInit( void );
extern void SetQNXFmt( void );
extern void FreeQNXFmt( void );
extern void SetELFFmt( void );
extern void FreeELFFmt( void );
extern void SetOS2Fmt( void );
extern void FreeOS2Fmt( void );
extern void SetPharFmt( void );
extern void FreePharFmt( void );
extern void SetNovFmt( void );
extern void FreeNovFmt( void );
#include <recon.h>
#endif
