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

#include "cmdline.h"
#include "exedos.h"
#include <recon.h>

extern void WriteLoad( void *, unsigned );
extern void WriteDBI( void );
extern unsigned long MemorySize( void );
extern address StartInfo;
unsigned long OvlTabOffset;          /* original450190 */

void FiniDOSLoadFile( void )
{
    unsigned long header_size, root_size, minimum;
    dos_exe_header head;

    if( FmtData.type & 0x40 ) {
        WriteCOMFile();
        return;
    }
    header_size = sizeof(head) + sizeof(unsigned long);
    QSeek( Root->outfile->handle, header_size, Root->outfile->fname );
    root_size = WriteDOSData();
    if( FmtData.type & 0x80 ) PadOvlFiles();
    WriteDBI();
    header_size = (header_size + (unsigned long)Root->relocs * 4 + 15) & ~15UL;
    QSeek( Root->outfile->handle, 0, Root->outfile->fname );
    head.hdr_size = header_size >> 4;
    head.mod_size = root_size & 511;
    head.file_size = (root_size + 511) >> 9;
    head.signature = 0x5a4d;
    head.num_relocs = Root->relocs;
    minimum = (MemorySize() - (root_size - header_size) + 15) >> 4;
    head.min_16 = minimum;
    head.max_16 = 0xffff;
    head.IP = StartInfo.off;
    head.CS_offset = StartInfo.seg;
    head.SS_offset = StackAddr.seg;
    head.SP = StackAddr.off;
    head.chk_sum = 0;
    head.reloc_offset = sizeof(head) + sizeof(unsigned long);
    head.overlay_num = 0;
    WriteLoad( &head, sizeof(head) );
    WriteLoad( &OvlTabOffset, sizeof(OvlTabOffset) );
}

unsigned long WriteDOSData( void )
{
    group_entry *group;
    section *sect;
    outfilelist *file;
    unsigned long header_size, root_size;
    unsigned char repos;

    OrderGroups( CompareDosSegments );
    CurrSect = Root;
    header_size = WriteDOSRootRelocs();
    Root->u.file_loc = header_size;
    if( Root->areas != 0 ) {
        Root->outfile->file_loc = header_size + Root->size;
        ProcAllOvl( AssignFileLocs );
        EmitOvlTable();
    }
    for( file = OutFiles; file != 0; file = file->next ) file->file_loc = 0;
    Root->outfile->file_loc = Root->u.file_loc;
    Root->sect_addr.off = Groups->grp_addr.off;
    Root->sect_addr.seg = Groups->grp_addr.seg;
    group = Groups;
    while( group != 0 ) {
        sect = group->section;
        CurrSect = sect;
        file = sect->outfile;
        if( file->handle == -1 ) OpenOvlFile( file );
        repos = WriteDOSGroup( group );
        group = group->next_group;
        if( group == 0 || sect != group->section ) {
            if( sect == Root ) root_size = file->file_loc;
            else WriteDOSSectRelocs( sect, repos );
        }
        if( repos ) QSeek( file->handle, file->file_loc, file->fname );
    }
    return root_size;
}

struct reloc_info;
extern unsigned char DumpRelocList( struct reloc_info * );
extern unsigned long NullAlign( unsigned long );
unsigned long WriteDOSRootRelocs( void )
{
    DumpRelocList( Root->reloclist );
    NullAlign( 16 );
    return ((unsigned long)Root->relocs * 4 + sizeof(dos_exe_header) + 4 + 15) & ~15UL;
}

void AssignFileLocs( section *sect )
{
    outfilelist *file;

    file = sect->outfile;
    if( LinkFlags & 0x8000 ) file->file_loc = (file->file_loc + 511) & ~511UL;
    sect->u.file_loc = file->file_loc;
    file->file_loc += ((sect->size + 15) & ~15UL) + (((unsigned long)sect->relocs * 4 + 15) & ~15UL);
}

extern void PadLoad( unsigned long );
void WriteDOSSectRelocs( section *sect, unsigned char repos )
{
    unsigned long loc;
    outfilelist *file;

    if( sect->relocs != 0 ) {
        loc = sect->u.file_loc + ((sect->size + 15) & ~15UL);
        file = sect->outfile;
        if( file->file_loc > loc ) QSeek( file->handle, loc, file->fname );
        else {
            if( repos ) QSeek( file->handle, file->file_loc, file->fname );
            if( file->file_loc < loc ) {
                PadLoad( loc - file->file_loc );
                file->file_loc = loc;
            }
        }
        loc += (unsigned long)sect->relocs * 4;
        DumpRelocList( sect->reloclist );
        if( loc > file->file_loc ) file->file_loc = loc;
    }
}

void WriteCOMFile( void )
{
    group_entry *group;
    outfilelist *file;
    long chop;

    if( StartInfo.seg != 0 ) {
        LnkMsg( 0x3356, 0 );
        return;
    }
    if( StackAddr.seg != 0 || StackAddr.off != 0 ) LnkMsg( 0x3257, 0 );
    OrderGroups( CompareDosSegments );
    CurrSect = Root;
    file = Root->outfile;
    Root->u.file_loc = file->file_loc = 0;
    Root->sect_addr.off = Groups->grp_addr.off;
    Root->sect_addr.seg = Groups->grp_addr.seg;
    for( group = Groups; group != 0; group = group->next_group ) {
        chop = group->grp_addr.off - StartInfo.off
            + ((long)(group->grp_addr.seg - StartInfo.seg) << 4);
        /* This version cuts the actual group buffer directly, before the
         * later separate WriteCOMGroup helper existed. */
        if( chop < 0 ) {
            group->size += chop;
            group->data -= chop;
        }
        if( (long)group->size > 0 && WriteDOSGroup( group ) )
            QSeek( file->handle, file->file_loc, file->fname );
        if( chop < 0 ) Root->u.file_loc += chop;
    }
    if( file->file_loc > 0xff9aUL ) LnkMsg( 0x3359, 0 );
    WriteDBI();
}
